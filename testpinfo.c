#include "types.h"
#include "stat.h"
#include "user.h"
#include "procstat.h"
int main(int argc, char *argv[])
 {
     int n;
   
        n = atoi(argv[1]);  

    struct proc_stat s1;
    getpinfo(&s1,n);
     printf(1,"pid : %d\n",s1.pid);
     printf(1,"current queue : %d\n",s1.current_queue);
     printf(1,"num_run : %d\n",s1.num_run);
     printf(1,"runtime : %d\n",s1.runtime);
     printf(1,"ticks : %d %d %d %d %d\n",s1.ticks[0],s1.ticks[1],s1.ticks[2],s1.ticks[3],s1.ticks[4]);
        
    
   
}