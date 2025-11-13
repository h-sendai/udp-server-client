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
#include "logUtil.h"

int debug = 0;
volatile sig_atomic_t has_alarm  = 0;
unsigned long total_read_bytes   = 0;
unsigned long total_drop_counter = 0;
unsigned long total_read_counter = 0;
struct timeval start;

int usage(void)
{
    char msg[] = 
    "Usage: ./udp-read-trend ip_address [options]\n"
    "Options:\n"
    "    -b bufsize (1 k)\n"
    "    -c cpu_num\n"
    "    -i interval_sec\n (1.0 sec)\n"
    "    -p port (1234)\n"
    "    -r rcvbuf\n"
    "    -s sleep_usec\n"
    "    -S bz_sleep_usec\n"
    "bufsize, sleep_usec, bz_sleep_usec will be send to server\n"
    ;

    fprintf(stderr, "%s", msg);
    
    return 0;
}

void sig_alarm(int signo)
{
    has_alarm = 1;
    return;
}

void sig_int(int signo)
{
    struct timeval now, running_time;
    gettimeofday(&now, NULL);
    timersub(&now, &start, &running_time);
    double running_time_sec = running_time.tv_sec + 0.000001*running_time.tv_usec;
    double read_rate_MB = (double) total_read_bytes / running_time_sec / 1024.0 / 1024.0;
    double read_rate_Gb = (double) total_read_bytes * 8 / running_time_sec / 1000.0 / 1000.0 / 1000.0;
    
    fprintf(stderr, "# running_time: %.6f sec total_read_bytes: %ld bytes total_read_counter: %ld rate: %.3f MB/s %.3f Gbps total_drop_count: %ld\n",
        running_time_sec, total_read_bytes, total_read_counter, read_rate_MB, read_rate_Gb, total_drop_counter);
    exit(0);
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
    int port = 1234;
    unsigned long seq_num;
    char *server_ip_address;
    int so_rcvbuf = -1;
    int cpu_num   = -1;
    struct timeval tv_interval = { 1, 0 };
    char *tv_interval_s = NULL;

    arg_to_server.bufsize = 1024;
    arg_to_server.sleep_usec = 0;
    arg_to_server.bzsleep_usec = 0;

    while ( (c = getopt(argc, argv, "b:c:dhi:p:r:s:S:")) != -1) {
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
            case 'i':
                tv_interval_s = optarg;
                break;
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

    unsigned long interval_read_counter = 0;
    unsigned long interval_read_bytes   = 0;
    unsigned long interval_drop_counter = 0;

    my_signal(SIGINT,  sig_int);
    my_signal(SIGALRM, sig_alarm);
    if (tv_interval_s != NULL) {
        tv_interval = str2timeval(tv_interval_s);
    }
    if (debug) {
        fprintf(stderr, "tv_interval.tv_sec:  %ld\n", tv_interval.tv_sec);
        fprintf(stderr, "tv_interval.tv_usec: %ld\n", tv_interval.tv_usec);
    }

    set_timer(tv_interval.tv_sec, tv_interval.tv_usec, tv_interval.tv_sec, tv_interval.tv_usec);
    struct timeval now, elapsed, prev, interval;

    int rcvbuf = get_so_rcvbuf(sockfd);
    fprintfwt(stdout, "program start (rcvbuf: %d bytes)\n", rcvbuf);
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

            fprintf(stdout, "%ld.%06ld %ld.%06ld read: %ld bytes %.3f Gbps read_count: %ld drop_count: %ld\n",
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

        n = readn(sockfd, read_buf, arg_to_server.bufsize);
        if (n < 0) {
            /* Process read() error only */
            /* EINTR will be treated in readn() function */
            err(1, "read");
        }
        interval_read_bytes += n;
        total_read_bytes    += n;
        seq_num = get_seq_num(read_buf, n);
        if (total_read_counter != seq_num) {
            if (debug) {
                fprintf(stderr, "seq_num error at seq_num: %ld total_read_counter: %ld\n",
                    seq_num, total_read_counter);
            }
            interval_drop_counter += (seq_num - total_read_counter);
            total_drop_counter    += (seq_num - total_read_counter);
            total_read_counter = seq_num;
        }
        total_read_counter ++;
        interval_read_counter ++;
    }
        
    return 0;
}
