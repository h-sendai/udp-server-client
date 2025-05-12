#include <sys/prctl.h> /* for PR_SET_TIMERSLACK */
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
#include "bz_usleep.h"
#include "debug_print.h"
#include "logUtil.h"

int debug = 0;
volatile sig_atomic_t has_alarm = 0;

int usage()
{
    char msg[] = "Usage: server [-d] [-l] [-p port] [-s sndbuf]\n"
                 "-l print write count\n"
                 "-p port (default 1234)\n"
                 "-s sndbuf (SO_SNDBUF size)\n"
                 "write() buffer size will be requested by client\n";
    fprintf(stderr, "%s", msg);

    return 0;
}

void sig_alarm(int signo)
{
    has_alarm = 1;
    return;
}

struct arg_to_server {
    int bufsize;
    int sleep_usec;
    int bzsleep_usec;
} arg_to_server;

int main(int argc, char *argv[])
{
    unsigned char *write_buf;
    int write_buf_size; //= 1024;
    int c, m, n;
    struct sockaddr_in cliaddr;
    struct sockaddr_in servaddr;
    socklen_t len;
    int port = 1234;
    int sndbuf_size = 0;
    int print_write_count = 0;

    while ( (c = getopt(argc, argv, "dhlp:s:")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 'h':
                usage();
                exit(0);
            case 'l':
                print_write_count = 1;
                break;
            case 'p':
                port = get_num(optarg);
                break;
            case 's':
                sndbuf_size = get_num(optarg);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    memset(&cliaddr, 0, sizeof(cliaddr));
    memset(&servaddr, 0, sizeof(servaddr));

    prctl(PR_SET_TIMERSLACK, 1);

    /* iterative server.  after sending all udp, wait data from client again */
    for ( ; ; )  {
        int sockfd = udp_socket();
        if (sockfd < 0) {
            errx(1, "socket");
        }

        if (my_bind(sockfd, "0.0.0.0", port) < 0) {
            errx(1, "bind");
        }

        len = sizeof(cliaddr);
        n = recvfrom(sockfd, &arg_to_server, sizeof(arg_to_server), 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            err(1, "recvfrom");
        }
        char remote_ip[32];
        inet_ntop(AF_INET, (struct sockaddr *)&cliaddr.sin_addr, remote_ip, sizeof(remote_ip));
        fprintfwt(stderr, "access from: %s, bufsize: %d bytes\n", remote_ip, arg_to_server.bufsize);
        debug_print(stderr, "recvfrom() returns\n");
        
        if (sndbuf_size > 0) {
            if (set_so_sndbuf(sockfd, sndbuf_size) < 0) {
                errx(1, "set_so_sendbuf");
            }
        }
        int sndbuf = get_so_sndbuf(sockfd);
        debug_print(stderr, "sndbuf: %d bytes\n", sndbuf);
    
        write_buf_size = arg_to_server.bufsize;
        write_buf = malloc(write_buf_size);
        if (write_buf == NULL) {
            err(1, "malloc");
        }

        if (connect(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
            err(1, "connect");
        }

        unsigned long seq_num              = 0;
        unsigned long interval_write_count = 0;
        struct timeval start, elapsed, now;
        gettimeofday(&start, NULL);
        my_signal(SIGALRM, sig_alarm);
        set_timer(1, 0, 1, 0);
        for ( ; ; ) {
            if (has_alarm) {
                has_alarm = 0;
                gettimeofday(&now, NULL);
                timersub(&now, &start, &elapsed);
                if (print_write_count) {
                    printf("%ld.%06ld write_count: %ld\n", elapsed.tv_sec, elapsed.tv_usec, interval_write_count);
                }
                interval_write_count = 0;
            }
            //memcpy(&write_buf[0], &seq_num, sizeof(seq_num));
            unsigned long *long_p = (unsigned long *)&write_buf[0];
            *long_p = seq_num;
            m = writen(sockfd, write_buf, write_buf_size);
            if (m < 0) {
                fprintfwt(stderr, "%s\n", strerror(errno));
                goto END;
            }
            seq_num ++;
            interval_write_count ++;
            if (arg_to_server.sleep_usec > 0) {
                usleep(arg_to_server.sleep_usec);
            }
            if (arg_to_server.bzsleep_usec > 0) {
                bz_usleep(arg_to_server.bzsleep_usec);
            }
        }
        
END:
        if (close(sockfd) < 0) {
            err(1, "close");
        }
        set_timer(0, 0, 0, 0);
    }
        

    return 0;
}
