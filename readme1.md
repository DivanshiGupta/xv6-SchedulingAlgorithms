Comparison Between different scheduling algorithm

A benchmark program was run which runs  for at least  25 - 30 seconds and following values of waitime(in ticks) of the program was noted using different scheduling algorithm.

DEFAULT — 2244
FCFS —- 5774
PBS —-  2203
MLFQ —- 2167


 FCFS is waiting for the largest time as there is no preemption and if a process with larger time comes first all the rest processes have to starve for cpu execution.

PBS is DEFAULT are almost same as we have given same priority to all processes in PBS , so it would also act as default round robin and as round robin has preemption of processes after every particular time slice so not process would starve for CPU. Thus, its time is less than FCFS.

MLFQ has the least time as there are many queues with different priorities and a process which takes more CPU time is shifted to less priority queue and also prevents starvation by aging i.e shifting to upper queues after some fixed time.Thus , it takes the least time. 

Possible Exploitation of MLFQ Policy by a Process
If a process voluntarily relinquishes control of the CPU, it leaves the queuing network, and when the process becomes ready again after the I/O, it is inserted at the tail of the same queue, from which it is relinquished earlier. This can be exploited by a process, as just when the time-slice is about to expire, the process can voluntarily relinquish control of the CPU, and get inserted in the same queue again. If it ran as normal, then due to time-slice getting expired, it would have been preempted to a lower priority queue. The process, after exploitation, will remain in the higher priority queue, so that it can run again sooner that it should have.