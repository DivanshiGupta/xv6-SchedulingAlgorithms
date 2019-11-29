#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include <stddef.h>


// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      if(myproc())
      {
          int ii =myproc()->q_number;
          myproc()->ticks[ii]+=1;
          myproc()->info.ticks[ii]+=1;
      }
      #ifdef MLFQ
      shift_processes();
      #endif
      wakeup(&ticks);
      release(&tickslock);

      // Update time fields
      if(myproc())
      {
        if(myproc()->state == RUNNING)
         {
            myproc()->rtime++;
            myproc()->info.runtime++;
         }
        else if(myproc()->state == SLEEPING)
          myproc()->iotime++;
      }
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  // #ifdef FCFS
  // #endif
  #ifdef DEFAULT
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();
#endif
  #ifdef FCFS
  //kanish
  #endif
  #ifdef PBS
  if(myproc() && myproc()->state == RUNNING &&  tf->trapno == T_IRQ0+IRQ_TIMER)
  {
   int a = compare_priority(myproc()->priority);
   if(a==1)
   {
    yield();
   }
  }
  #endif

  #ifdef MLFQ
  if(myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
  {
    int y = myproc()->rtime;
    // cprintf("%d\n",y);
    // for(int i=0;i<5;i++)
    // {
    //     cprintf("%d ",myproc()->ticks[i]);
    // }
    // cprintf("\n");
    if(myproc()->q_number == 0 && myproc()->ticks[0]>=1)
    { 
      int f=0;
      for(int i=0;i<c1;i++)
      {
        struct proc* p1 = q1[i];
        if(myproc()->pid == p1->pid)
        {
          f=1;
          break;
        }
      }
        myproc()->q_number =1;
        myproc()->info.current_queue=1;

      if(f==0)
      {
        q1[c1]=myproc();
        // myproc()->q_number =1;
        q1[c1]->q_number =1;
        q1[c1]->info.current_queue =1;

        c1+=1;
        // myproc()->q_number =1;
        int wtime = ticks - myproc()->rtime - myproc()->ctime -50*myproc()->shift; 
        cprintf("%d MOVE: %s with pid %d moved to queue 1 at rtime %d and wait time %d\n",ticks,myproc()->name ,myproc()->pid,y,wtime);
      }
      // myproc()->q_number =1;
      for(int i=0;i<c0;i++)
      {
        struct proc* p1 = q0[i];
        if(myproc()->pid == p1->pid)
        {
          while(i<c0-1)
          {
            q0[i]=q0[i+1];
            i+=1;
          }
        }
      }
      c0-=1;
      // cprintf("%s with pid %d moved to queue 1 at rtime %d\n",myproc()->name,myproc()->pid,y);
      yield();
    }
    else if(myproc()->q_number == 1 && myproc()->ticks[1]>=2)
    {
      int f=0;
      for(int i=0;i<c2;i++)
      {
        struct proc* p1 = q2[i];
        if(myproc()->pid == p1->pid)
        {
          f=1;
          break;
        }
      }
      myproc()->q_number =2;
        myproc()->info.current_queue=2;


      if(f==0)
      {
        q2[c2]=myproc();
        q2[c2]->q_number =2;
        q2[c2]->info.current_queue =2;
        
        c2+=1;
        // myproc()->q_number =2;
        // int wtime = ticks - myproc()->rtime - myproc()->ctime;
        int wtime = ticks - myproc()->rtime - myproc()->ctime -50*myproc()->shift; 


        cprintf("%d MOVE: %s with pid %d moved to queue 2 at rtime %d and wait time %d\n",ticks,myproc()->name,myproc()->pid,y,wtime);
      }

    //   myproc()->q_number =2;
      for(int i=0;i<c1;i++)
      {
        struct proc* p1 = q1[i];
        if(myproc()->pid == p1->pid)
        {
          while(i<c1-1)
          {
            q1[i]=q1[i+1];
            i+=1;
          }
        }
      } 
      c1-=1;
      // cprintf("Process with pid %d moved to queue 2 at rtime %d\n",myproc()->pid,y);

      yield();
    }
    else if(myproc()->q_number == 2 && myproc()->ticks[2]>=4 )
    {
      int f=0;
      for(int i=0;i<c3;i++)
      {
        struct proc* p1 = q3[i];
        if(myproc()->pid == p1->pid)
        {
          f=1;
          break;
        }
      }
        myproc()->q_number =3;
        myproc()->info.current_queue=3;

      if(f==0)
      {
        q3[c3]=myproc();
        q3[c3]->q_number =3;
        q3[c3]->info.current_queue =3;

        
        c3+=1;
        // myproc()->q_number =3;
        // int wtime = ticks - myproc()->rtime - myproc()->ctime;
        int wtime = ticks - myproc()->rtime - myproc()->ctime -50*myproc()->shift; 


        cprintf("%d MOVE: %s with pid %d moved to queue 3 at rtime %d and wait time %d\n",ticks,myproc()->name,myproc()->pid,y,wtime);
      }
      // myproc()->q_number =3;
     
      for(int i=0;i<c2;i++)
      {
        struct proc* p1 = q2[i];
        if(myproc()->pid == p1->pid)
        {
          while(i<c2-1)
          {
            q2[i]=q2[i+1];
            i+=1;
          }
        }
      }
      c2-=1;
      // cprintf("Process with pid %d moved to queue 3 at rtime %d\n",myproc()->pid,y);
      
      yield();

    }
    else if(myproc()->q_number == 3 &&  myproc()->ticks[3]>=8)
    {
      int f=0;
      for(int i=0;i<c4;i++)
      {
        struct proc* p1 = q4[i];
        if(myproc()->pid == p1->pid)
        {
          f=1;
          break;
        }
      }
        myproc()->q_number=4;
        myproc()->info.current_queue=4;
      
      if(f==0)
      {
        q4[c4]=myproc();
        // myproc()->q_number=4;
        q4[c4]->q_number =4;
        q4[c4]->info.current_queue =4;

        c4+=1;
        // int wtime = ticks - myproc()->rtime - myproc()->ctime; 
        int wtime = ticks - myproc()->rtime - myproc()->ctime -50*myproc()->shift; 
      
      cprintf("%d MOVE: %s with pid %d moved to queue 4 at rtime %d and wait time %d\n",ticks,myproc()->name,myproc()->pid,y,wtime);

      }
      // myproc()->q_number =4;
      for(int i=0;i<c3;i++)
      {
        struct proc* p1 = q3[i];
        if(myproc()->pid == p1->pid)
        {
          while(i<c3-1)
          {
            q3[i]=q3[i+1];
            i+=1;
          }
        }
      }
      c3-=1;
      // cprintf("Process with pid %d moved to queue 4 at rtime %d\n",myproc()->pid,y);

      yield();
    }
    else if(myproc()->q_number == 4 && myproc()->ticks[4]%16 == 0 && myproc()->ticks[4]>0)
    {
      q4[c4]=myproc();
        int wtime = ticks - myproc()->rtime - myproc()->ctime -50*myproc()->shift; 

      cprintf("%d MOVE: %s with pid %d moved to queue 4 at rtime %d and wait time %d\n",ticks,myproc()->name,myproc()->pid,y,wtime);

      for(int i=0;i<c4;i++)
      {
        struct proc* p1 = q4[i];
        if(myproc()->pid == p1->pid)
        {
          while(i<c4)
          {
            q4[i]=q4[i+1];
            i+=1;
          }
        }
      }
      yield();
      // cprintf("Process with pid %d moved to queue 4 at rtime %d\n",myproc()->pid,y);
    }
    else
    {
        int f=0;
        if(myproc()->q_number > 0 && c0>0)
        {
            for(int i=0;i<c0;i++)
            {
                if(q0[i]->state!= RUNNABLE)
                {
                    continue;
                }
                if(q0[i]!=NULL)
                {
                    f=1;
                    break;
                }
            }
        }
        if(f==0)
        {
            if(myproc()->q_number > 1 && c1>0)
            {
                for(int i=0;i<c1;i++)
                {
                    if(q1[i]->state!= RUNNABLE)
                    {
                        continue;
                    }
                    if(q1[i]!=NULL)
                    {
                        f=1;
                        break;
                    }
                }
            }
        }
        if(f==0)
        {
            if(myproc()->q_number > 2 && c2>0)
            {
                for(int i=0;i<c2;i++)
                {
                    if(q2[i]->state!= RUNNABLE)
                    {
                        continue;
                    }
                    if(q2[i]!=NULL)
                    {
                        f=1;
                        break;
                    }
                }
            }
        }
        if(f==0)
        {
            if(myproc()->q_number > 3 && c3>0)
            {
                for(int i=0;i<c3;i++)
                {
                    if(q3[i]->state!= RUNNABLE)
                    {
                        continue;
                    }
                    if(q3[i]!=NULL)
                    {
                        f=1;
                        break;
                    }
                }
            }
        }
        if(f==1)
        {
            yield();
        }
    }
  }

  #endif
  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
