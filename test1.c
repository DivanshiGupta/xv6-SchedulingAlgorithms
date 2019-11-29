#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "procstat.h"
#include <stddef.h>

int main(int argc, char *argv[]) {
    volatile int id, n = 6, limit = 1e8 * 3;
    volatile int x = 0, z;

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
}