#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include <stddef.h>

struct proc *q0[10004];
struct proc *q1[10004];
struct proc *q2[10004];
struct proc *q3[10004];
struct proc *q4[10004];
int c0 = 0;
int c1 = 0;
int c2 = 0;
int c3 = 0;
int c4 = 0;

// struct proc_stat
// {
//   int pid;           // PID of each process
//   int runtime;       // Use suitable unit of time
//   int num_run;       // number of time the process is executed
//   int current_queue; // current assigned queue
//   int ticks[5];      // number of ticks each process has received at each of the 5 priority queue
// };

struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int cpuid()
{
  return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
  int apicid, i;

  if (readeflags() & FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i)
  {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;
  // struct proc_stat* w;
  char *sp;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->info.pid = p->pid;
  p->priority = 60; // default priority value
  p->q_number = 0;
   

  // w->pid = p->pid;
  // w->num_run=0;
  // w->current_queue=0;
  // w->runtime=0;
  // w->ticks[0]=0;
  // w->ticks[1]=0;
  // w->ticks[2]=0;
  // w->ticks[3]=0;
  // w->ticks[4]=0;
  p->info.current_queue =0;
  q0[c0] = p;
  c0++;

// #ifdef MLFQ
//   cprintf("MOVE: %s with pid %d moved to queue 0 at rtime %d\n",p->name, p->pid,p->rtime);
// #endif
  release(&ptable.lock);

  // Allocate kernel stack.
  if ((p->kstack = kalloc()) == 0)
  {
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe *)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint *)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context *)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  p->ctime = ticks;
  p->rtime = 0;
  p->etime = 0;
  p->iotime = 0;
  p->ticks[0] = 0;
  p->ticks[1] = 0;
  p->ticks[2] = 0;
  p->ticks[3] = 0;
  p->ticks[4] = 0;
  p->shift =0;
  // p->info->pid = p->pid;
  p->info.runtime = p->rtime;
  p->info.num_run =0;
  p->info.ticks[0]=0;
  p->info.ticks[1]=0;
  p->info.ticks[2]=0;
  p->info.ticks[3]=0;
  p->info.ticks[4]=0;

  // #ifdef MLFQ
  // cprintf("MOVE: %s with pid %d moved to queue 0 at rtime %d\n",p->name, p->pid,p->rtime);
// #endif
  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if ((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0; // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if (n > 0)
  {
    if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  else if (n < 0)
  {
    if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

unsigned short lfsr = 0xACE1u;
unsigned bit;
unsigned rand()
{
  bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
  return lfsr = (lfsr >> 1) | (bit << 15);
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy process state from proc.
  if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
  {
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for (i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;
  cprintf("Child with pid %d created\n",pid);

#ifdef PBS
  np->priority = pid/2;
#endif

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if (curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for (fd = 0; fd < NOFILE; fd++)
  {
    if (curproc->ofile[fd])
    {
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->parent == curproc)
    {
      p->parent = initproc;
      if (p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  curproc->etime =ticks;
  sched();
  panic("zombie exit");
}

 int getpinfo(struct proc_stat* stat1,int pid)
 {
   struct proc* p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->pid == pid)
    {
     stat1->pid = p->info.pid;
     stat1->current_queue =p->info.current_queue;
     stat1->num_run = p->info.num_run;
     stat1->runtime = p->info.runtime;
     stat1->ticks[0] = p->info.ticks[0];
     stat1->ticks[1] = p->info.ticks[1]; 
     stat1->ticks[2] = p->info.ticks[2]; 
     stat1->ticks[3] = p->info.ticks[3]; 
     stat1->ticks[4] = p->info.ticks[4]; 
    //  cprintf("pid : %d\n",stat1->pid);
    //  cprintf("current queue : %d\n",stat1->current_queue);
    //  cprintf("num_run : %d\n",stat1->num_run);
    //  cprintf("runtime : %d\n",stat1->runtime);
    //  cprintf("ticks : %d %d %d %d %d\n",stat1->ticks[0],stat1->ticks[1],stat1->ticks[2],stat1->ticks[3],stat1->ticks[4]);


    }
 }
 return 0;
 }

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc)
        continue;
      havekids = 1;
      if (p->state == ZOMBIE)
      {
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock); //DOC: wait-sleep
  }
}

int waitx(int *wtime, int *rtime)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;)
  {
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc)
        continue;
      havekids = 1;
      if (p->state == ZOMBIE)
      {
        *wtime = p->etime - p->ctime - p->rtime - p->iotime;
        *rtime = p->rtime;

        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }
    if (!havekids || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }
    sleep(curproc, &ptable.lock);
  }
}

int set_priority(int pid, int priority)
{
  struct proc *p = NULL;
  acquire(&ptable.lock);
  int val = -1, f = 0;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      val = p->priority;
      p->priority = priority;
      cprintf("PRiority changed of %d\n",p->pid);
      f = 1;
    }
    if (f == 1)
    {
      break;
    }
  }
  release(&ptable.lock);
  return val;
}

//function useful for priority scheduling (yield is called if the current priority is not lowesst)
int compare_priority(int pri)
{
  struct proc *p = NULL;
  acquire(&ptable.lock);
  // int f1=0;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state != RUNNABLE)
    {
      continue;
    }
    if (p->priority <= pri)
    {
      release(&ptable.lock);
      return 1;
      // f1=1;
      // break;
    }
  }
  release(&ptable.lock);

  return 0;
}

void shift_processes()
{
  for(int i=0;i<c1;i++)
  {
    struct proc *p1 = q1[i];
        // *wtime = p->ctime - p->state - p->rtime - p->iotime;
    int wtime = ticks - p1->ctime -p1->rtime - 50 *p1->shift ;
    if(p1->state != RUNNABLE)
    {
      continue;
    }
    if(p1!=NULL && wtime >=50) 
    {
      int f=0;
      for(int j=0;j<c0;j++)
      {
        struct proc *p2 = q0[j];
        if(p2 ->pid == p1->pid)
        {
          f=1;
          break;
        }
      }
      if(f==0)
      {
        cprintf("%d SHIFT: %s with pid %d shifted to queue 0 having wait time %d",ticks,p1->name,p1->pid,wtime);
        // p1->wtime=0;
        p1->q_number=0;
        p1->info.current_queue=0;

        p1->shift +=1;
        q0[c0]=p1;
        c0++;
        p1->ticks[0]=0;
        p1->ticks[1]=0;

      }
      for(int j=i;j<c1-1;j++)
      {
        q1[j]=q1[j+1];
      }  
      c1-=1;
    }
  }


  for(int i=0;i<c2;i++)
  {
    struct proc *p1 = q2[i];
        // *wtime = p->ctime - p->state - p->rtime - p->iotime;
    int wtime = ticks - p1->ctime -p1->rtime - 50*p1->shift;
    
    if(p1->state != RUNNABLE)
    {
      continue;
    }
    if(p1!=NULL && wtime >=50) 
    {
      int f=0;
      for(int j=0;j<c1;j++)
      {
        struct proc *p2 = q1[j];
        if(p2 ->pid == p1->pid)
        {
          f=1;
          break;
        }
      }
      if(f==0)
      {
        cprintf("%d SHIFT: %s with pid %d shifted to queue 1 haing wait time %d\n",ticks,p1->name,p1->pid,wtime);
        // p1->wtime=0;
        p1->q_number=1;
        p1->info.current_queue=1;

        p1->shift+=1;
        q1[c1]=p1;
        c1++;
        p1->ticks[2]=0;
        p1->ticks[1]=0;
      }
      for(int j=i;j<c2-1;j++)
      {
        q2[j]=q2[j+1];
      }  
      c2-=1;
    }
  }

  

  for(int i=0;i<c3;i++)
  {
    struct proc *p1 = q3[i];
    // p1->wtime = ticks - p1->ctime -p1->rtime -p1->iotime;
    // wtime%=50;
     int wtime = ticks - p1->ctime -p1->rtime - 50*p1->shift;
    

    if(p1->state != RUNNABLE)
    {
      continue;
    }
    if(p1!=NULL && wtime >=50) 
    {
      int f=0;
      for(int j=0;j<c2;j++)
      {
        struct proc *p2 = q2[j];
        if(p2 ->pid == p1->pid)
        {
          f=1;
          break;
        }
      }
      if(f==0)
      {
        cprintf("%d SHIFT: %s with pid %d  shifted to queue 2 having wait time %d\n",ticks,p1->name,p1->pid,wtime);
        // p1->wtime=0;
        p1->q_number=2;
        p1->info.current_queue=2;
        p1->shift+=1;
        q2[c2]=p1;
        c2++;
        p1->ticks[2]=0;
        p1->ticks[3]=0;
      }
      for(int j=i;j<c3-1;j++)
      {
        q3[j]=q3[j+1];
      }  
      c3-=1;
    }
  }

  for(int i=0;i<c4;i++)
  {
    struct proc *p1 = q4[i];
    int wtime = ticks - p1->ctime -p1->rtime -50*p1->shift;


    if(p1->state != RUNNABLE)
    {
      continue;
    }
    if(p1!=NULL && wtime >=50) 
    {
      int f=0;
      for(int j=0;j<c3;j++)
      {
        struct proc *p2 = q3[j];
        if(p2 ->pid == p1->pid)
        {
          f=1;
          break;
        }
      }
      if(f==0)
      {
        cprintf("%d SHIFT: %s with pid %d shifted to queue 3 having wait time %d \n",ticks,p1->name,p1->pid,wtime);
        p1->q_number=3;
        p1->info.current_queue=3;

        p1->shift+=1;
        q3[c3]=p1;
        c3++;
        p1->ticks[4]=0;
        p1->ticks[3]=0;
      }
      for(int j=i;j<c4-1;j++)
      {
        q4[j]=q4[j+1];
      }  
      c4-=1;
    }
  }
}
//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void)
{
  // struct proc *p = NULL;
  struct cpu *c = mycpu();
  c->proc = 0;

  for (;;)
  {

    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
#ifdef DEFAULT
    struct proc *p = NULL;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      cprintf("DEFAULT: Process %s with pid %d scheduled to run\n", p->name, p->pid);
      swtch(&(c->scheduler), p->context);
      p->info.num_run +=1;

      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
#endif

#ifdef FCFS

    struct proc *p = NULL;
    struct proc *min_process = NULL;
    int min_t = 0, f1 = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {

      if (p->state != RUNNABLE)
      {
        continue;
      }
      if (f1 == 0)
      {
        min_process = p;
        min_t = p->ctime;
        f1 = 1;
      }
      else
      {
        if (p->ctime < min_t)
        {
          min_process = p;
          min_t = p->ctime;
        }
      }
    }
    if (min_process != NULL)
    {
      // p= min_process;
      c->proc = min_process;
      switchuvm(min_process);
      min_process->state = RUNNING;
      cprintf("FCFS: Process %s with pid %d  and ctime %d scheduled to run\n",min_process->name, min_process->pid, min_process->ctime);
      swtch(&(c->scheduler), min_process->context);
      p->info.num_run +=1;
      switchkvm();
      c->proc = 0;
    }
#endif

#ifdef PBS
    struct proc *p = NULL;

    int max_p = 200;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
      {
        continue;
      }
      if (max_p > p->priority)
      {
        max_p = p->priority;
      }
    }

  
      for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if(p->state!= RUNNABLE)
        {
          continue;
        }
      if(p->priority == max_p )
       {
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        cprintf("PRIORITY: Process %s with pid %d scheduled to run with priority %d\n", p->name, p->pid, p->priority);
        swtch(&(c->scheduler), p->context);
        p->info.num_run +=1;      
        switchkvm();
        int mx = 101;
        for(struct proc *p = ptable.proc; p< &ptable.proc[NPROC];p++){
          if(p->state != RUNNABLE){
            if(mx > p->priority){
              mx = p->priority;
            }
          }
        }
        if(mx < max_p){
          break;
        }
        c->proc = 0;
       }
    
  }

#endif

#ifdef MLFQ
    struct proc *p = NULL;

    int f1 = 0;
    // cprintf("%d %d %d %d %d \n",c0,c1,c2,c3,c4);
    if (c0 != 0)
    {
      for (int i = 0; i < c0; i++)
      {
        if (q0[i] == NULL)
        {
          continue;
        }
        if (q0[i]->state != RUNNABLE)
        {
          continue;
        }
        p = q0[i];
        f1 = 1;
        break;
      }
    }
    if (f1 == 0)
    {
      if (c1 != 0)
      {
        for (int i = 0; i < c1; i++)
        {
          if (q1[i] == NULL)
          {
            continue;
          }
          if (q1[i]->state != RUNNABLE)
          {
            continue;
          }
          p = q1[i];
          f1 = 1;
          break;
        }
      }
    }

    if (f1 == 0)
    {
      if (c2 != 0)
      {
        for (int i = 0; i < c2; i++)
        {
          if (q2[i] == NULL)
          {
            continue;
          }
          if (q2[i]->state != RUNNABLE)
          {
            continue;
          }
          f1 = 1;
          p = q2[i];
          break;
        }
      }
    }
    if (f1 == 0)
    {
      if (c3 != 0)
      {
        for (int i = 0; i < c3; i++)
        {
          if (q3[i] == NULL)
          {
            continue;
          }
          if (q3[i]->state != RUNNABLE)
          {
            continue;
          }
          f1 = 1;
          p = q3[i];
          break;
        }
      }
    }
    if (f1 == 0)
    {
      if (c4 != 0)
      {
        for (int i = 0; i < c4; i++)
        {
          if (q4[i] == NULL)
          {
            continue;
          }
          if (q4[i]->state != RUNNABLE)
          {
            continue;
          }
          p = q4[i];
          f1 = 1;
          break;
        }
      }
    }
    if (f1 == 1)
    {
      // cprintf("%d %d %d %d %d\n",c0,c1,c2,c3,c4);
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      
      cprintf("%d MLFQ: %s with pid %d scheduled to run in queue %d\n",ticks,p->name, p->pid, p->q_number);
      p->info.num_run +=1;
      swtch(&(c->scheduler), p->context);
      switchkvm();
      c->proc = 0;
    }
#endif
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&ptable.lock))
    panic("sched ptable.lock");
  if (mycpu()->ncli != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (readeflags() & FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  acquire(&ptable.lock); //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first)
  {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if (p == 0)
    panic("sleep");

  if (lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if (lk != &ptable.lock)
  {                        //DOC: sleeplock0
    acquire(&ptable.lock); //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if (lk != &ptable.lock)
  { //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      p->killed = 1;
      // Wake process from sleep if necessary.
      if (p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [EMBRYO] "embryo",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if (p->state == SLEEPING)
    {
      getcallerpcs((uint *)p->context->ebp + 2, pc);
      for (i = 0; i < 10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("qu no %d", p->q_number);
    cprintf("\n");
  }

  cprintf("q0 %d\n", c0);
  for (int j = 0; j < c0; j++)
  {
    cprintf("q0 : %d ", q0[j]->pid);
  }
  cprintf("\n");

  cprintf("q1 %d\n", c1);
  for (int j = 0; j < c1; j++)
  {
    cprintf("q1 : %d ", q1[j]->pid);
  }
  cprintf("\n");

  cprintf("q2 %d\n", c2);
  for (int j = 0; j < c2; j++)
  {
    cprintf("q2 : %d ", q2[j]->pid);
  }
  cprintf("\n");

  cprintf("q3 %d\n", c3);
  for (int j = 0; j < c3; j++)
  {
    cprintf("q3 : %d ", q3[j]->pid);
  }
  cprintf("\n");
  cprintf("q4 %d\n", c4);
  for (int j = 0; j < c4; j++)
  {
    cprintf("q4 : %d ", q4[j]->pid);
  }
  cprintf("\n");
}
