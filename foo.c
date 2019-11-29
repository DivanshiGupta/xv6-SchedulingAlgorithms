#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include"procstat.h"

int main(int argc, char *argv[]) {
    int k, n, id;
    double x = 0, z;
    // set_priority(4, 70);
    if (argc < 2)
        n = 1;  // default value
    else
        n = atoi(argv[1]);  // from command line

    if (n < 0 || n > 20)
        n = 2;

    x = 0;
    id = 0;
    for (k = 0; k < n; k++) {
        id = fork();

        // getpinfo();
        if (id < 0) {
            printf(1, "%d failed in fork!\n", getpid());
        } else if (id > 0) {  // parent
            // printf(1, "Parent %d creating child %d\n", getpid(), id);
            // wait();
        } else {  // child
            // printf(1, "Child %d created\n", getpid());
            for (z = 0; z < 3000000.0; z += 0.1)
                x = x +
                    3.14 * 89.64;  // useless calculations to consume CPU time
            exit();
        }
        // set_priority(6, 80);
    }
    // getpinfo(struct proc_stat s1,3);
    // cout << s1->pid;
    // cout << 
    for (k = 0; k < n; k++) {
        // int a, b;
        wait();
        // struct proc_stat s1 ; 
        // getpinfo(&s1,4);
    //      printf(1,"pid : %d\n",s1.pid);
    //  printf(1,"current queue : %d\n",s1.current_queue);
    //  printf(1,"num_run : %d\n",s1.num_run);
    //  printf(1,"runtime : %d\n",s1.runtime);
    //  printf(1,"ticks : %d %d %d %d %d\n",s1.ticks[0],s1.ticks[1],s1.ticks[2],s1.ticks[3],s1.ticks[4]);
    }
    exit();
}