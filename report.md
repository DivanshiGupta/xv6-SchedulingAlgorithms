
ASSIGNMENT 5
DIVANSHI GUPTA 
2018101050

#SCHEDULING ALGORITHMS IMPLEMENTATION

1. FCFS IMPLEMENTATION:
In proc.c I have scheduled processes based on their creation time(ctime). The processes run in order of increasing ctime.
In trap.c I have removed the part where yield() function is called. Since in FCFS there should be no contest switching and a new process is started only when the current process ends.

2. PRIORITY IMPLEMENTATION (PBS)
In allocproc function I have initialised the default priority of process as 60. In scheduler funtion the process with the highest priority(least in value) is selected. If there are multiple processes with least value priority then they are scheduled using round robin scheduling algorithm. In trap.c a compare_priority() function is called which compares if there exists another process which has priority value less than the current running priority. 
If there exists any such process then yield() is called.

3. MLFQ IMPLEMENTATION
    In scheduler function I have checked through the queues and schedued the process that is found first. In trap.c I have moved the process to next queue if the process runs for the alloted time of the queue. Also i have checked again if there exists any process in the queue lower than the queue of current process, if yes then yield() is called. Also i have added shift_process() function which checks if the waittime of any function exceeds 50 then it is shifted to the queue above it.


#GETPINFO SYSTEM CALL 

I have created a system call getpinfo that outputs all the values related to the process as stored in proc_stat structure which is declared in procstat.h

#WAITX SYSTEM CALL

I have creted waitx system call that has same return values as wait system call. Also a file name time.c is included in which this system call is called and a file test1.c is included which can be run to compare various schedulers based on the values returned by their waitx calls.

# Q : Explain in the report how could this be exploited by a process ?

Ans : It will give up IO just before its time slice is over so that it can retain the same priority queue over and over again. Thus it's priority will never decrement.

# Benchmark Program (Tester1)

TIME
Round Robin :  
FCFS :
PBS :
MLFQ :