#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "get_num.h"
#include "host_info.h"
#include "my_signal.h"
#include "my_socket.h"
#include "readn.h"
#include "set_timer.h"
#include "set_cpu.h"

int debug = 0;
volatile sig_atomic_t has_alarm = 0;

int usage(void)
{
    fprintf(stderr, "Usage: ./client ip_address [-b bufsize (1024)] [-c cpu_num (none)] [-p port (1234)] [-r rcvbuf (none)]\n");
    
    return 0;
}

void sig_alarm(int signo)
{
    has_alarm = 1;
    return;
}

unsigned long get_seq_num(unsigned char *buf, int len)
{
    unsigned long s;
    s = *(unsigned long *)buf;

    return s;
}

struct arg_to_server {
    int bufsize;
    int sleep_usec;
    int bzsleep_usec;
} arg_to_server;

int main(int argc, char *argv[])
{
    unsigned char read_buf[64*1024];
    int c, n;
    unsigned long total_read_counter = 0;
    int port = 1234;
    unsigned long seq_num;
    char *server_ip_address;
    int so_rcvbuf = -1;
    int cpu_num   = -1;

    arg_to_server.bufsize = 1024;
    arg_to_server.sleep_usec = 0;
    arg_to_server.bzsleep_usec = 0;

    while ( (c = getopt(argc, argv, "b:c:dhp:r:s:S:")) != -1) {
        switch (c) {
            case 'b':
                arg_to_server.bufsize = get_num(optarg);
                break;
            case 'c':
                cpu_num = get_num(optarg);
                break;
            case 'd':
                debug++;
                break;
            case 'h':
                usage();
                exit(0);
            case 'p':
                port = get_num(optarg);
                break;
            case 'r':
                so_rcvbuf = get_num(optarg);
                break;
            case 's':
                arg_to_server.sleep_usec = get_num(optarg);
                break;
            case 'S':
                arg_to_server.bzsleep_usec = get_num(optarg);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;
    if (argc != 1) {
        usage();
        exit(1);
    }

    server_ip_address = argv[0];

    if (cpu_num != -1) {
        if (set_cpu(cpu_num) < 0) {
            errx(1, "set_cpu");
        }
    }

    int sockfd = udp_socket();
    if (sockfd < 0) {
        errx(1, "socket");
    }

    if (connect_udp(sockfd, server_ip_address, port) < 0) {
        errx(1, "connect_udp");
    }
    
    if (so_rcvbuf > 0) {
        if (set_so_rcvbuf(sockfd, so_rcvbuf) < 0) {
            errx(1, "set_so_rcvbuf");
        }
    }

    if (debug) {
        int rcvbuf = get_so_rcvbuf(sockfd);
        fprintf(stderr, "rcvbuf: %d\n", rcvbuf);
    }
    
    //n = write(sockfd, &max_read_counter, sizeof(max_read_counter));
    n = write(sockfd, &arg_to_server, sizeof(arg_to_server));
    if (n < 0) {
        err(1, "write for 1st packet");
    }

    int interval_read_counter = 0;
    int interval_read_bytes   = 0;
    int interval_drop_counter = 0;

    my_signal(SIGALRM, sig_alarm);
    set_timer(1, 0, 1, 0);
    struct timeval start, now, elapsed, prev, interval;

    gettimeofday(&start, NULL);
    prev = start;
    for ( ; ; ) {
        if (has_alarm) {
            has_alarm = 0;

            gettimeofday(&now, NULL);
            timersub(&now, &start, &elapsed);
            timersub(&now, &prev,  &interval);
            prev = now;

            double interval_sec = interval.tv_sec + 0.000001*interval.tv_usec;
            double rate_Gbps = 8.0*(double)interval_read_bytes / interval_sec /1000.0/1000.0/1000.0;

            fprintf(stdout, "%ld.%06ld %ld.%06ld read: %d bytes %.3f Gbps read_count: %d drop_count: %d\n",
                elapsed.tv_sec, elapsed.tv_usec,
                interval.tv_sec, interval.tv_usec,
                interval_read_bytes, 
                rate_Gbps,
                interval_read_counter, interval_drop_counter);
            fflush(stdout);

            interval_read_counter = 0;
            interval_read_bytes   = 0;
            interval_drop_counter = 0;
            
        }

AGAIN:
        n = read(sockfd, read_buf, sizeof(read_buf));
        if (n < 0) {
            if (errno == EINTR) {
                goto AGAIN;
            }
            else {
                err(1, "read");
            }
        }
        interval_read_bytes += n;
        seq_num = get_seq_num(read_buf, n);
        if (total_read_counter != seq_num) {
            if (debug) {
                fprintf(stderr, "seq_num error at seq_num: %ld total_read_counter: %ld\n",
                    seq_num, total_read_counter);
            }
            interval_drop_counter += (seq_num - total_read_counter);
            total_read_counter = seq_num;
        }
        total_read_counter ++;
        interval_read_counter ++;
    }
        
    return 0;
}
