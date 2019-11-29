struct proc_stat
{
  int pid;           // PID of each process
  int runtime;       // Use suitable unit of time
  int num_run;       // number of time the process is executed
  int current_queue; // current assigned queue
  int ticks[5];      // number of ticks each process has received at each of the 5 priority queue
};