#include <time.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static long long gettime_ll(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
        warn("clock_gettime() in gettime_ll");
        return -1;
    }
    
    return ts.tv_sec*1000000000 + ts.tv_nsec;
}

int bz_nanosleep(long nsec)
{
    int loop = 0;
    int debug = 0;

    long long target = gettime_ll();
    target += nsec;

    while (gettime_ll() < target) {
        if (debug) {
            loop ++;
        }
    }

    if (debug) {
        fprintf(stderr, "loop: %d\n", loop);
    }

    return 0;
}

#ifdef USE_MAIN
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "bz_nanosleep.h"
#include "timespecop.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: ./sample nsec n_loop\n");
        exit(1);
    }

    long nsec = strtol(argv[1], NULL, 0);
    long n_loop = strtol(argv[2], NULL, 0);

    prctl(PR_SET_TIMERSLACK, 1);

    struct timespec ts0, ts1, diff;
    for (int i = 0; i < n_loop; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &ts0);
        bz_nanosleep(nsec);
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        timespecsub(&ts1, &ts0, &diff);
        long delta = diff.tv_sec*1000000000 + diff.tv_nsec;
        printf("%ld ns\n", delta);
    }

    return 0;
}
#endif
