#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "procstat.h"
#include <stddef.h>

int main(int argc, char *argv[]) {
#if defined DEFAULT || defined FCFS || defined MLFQ
    volatile int id, n = 7, limit = 1e8;
    volatile int x = 0;
    volatile int z;
    x = 0;
    id = 0;
    for (int k = 0; k < n; k++) {
        id = fork();
        if (id < 0) {
            printf(1, "%d failed in fork!\n", getpid());
        }
        if (id == 0) {  // child
            for (z = 0; z < limit; z += 1)
                x = x + 3;  // useless calculations to consume CPU time
            exit();
        }
    }
    for (int k = 0; k < n; k++) {
        wait();
    }

    exit();
#endif

#ifdef PBS
    volatile int id, n = 2, limit = 1e8;
    volatile int x = 0;
    volatile int z;
    x = 0;
    id = 0;

    for (int k = 0; k < n; k++) {
        id = fork();
        if (id < 0) {
            printf(1, "%d failed in fork!\n", getpid());
        }
        if (id == 0) {  // child
            for (z = 0; z < limit / 2; z += 1) {
                if (z % (limit / 10) == 0) {
                    printf(1, "PID %d: completed stage %d of 10\n", getpid(),
                           z / (limit / 10));
                }
                x = x + 3;  // useless calculations to consume CPU time
            }

            int pid = getpid();
            set_priority(pid, 100 - pid / 2);
            for (z = limit / 2; z < limit; z += 1) {
                if (z % (limit / 10) == 0) {
                    printf(1, "PID %d: completed stage %d of 10\n", getpid(),
                           z / (limit / 10));
                }
                x = x + 3;  // useless calculations to consume CPU time
            }
            exit();
        }
    }
    for (int k = 0; k < n; k++) {
        wait();
    }

    exit();
#endif
}