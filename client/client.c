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
#include "flow_ctrl_pause.h"

int debug = 0;
volatile sig_atomic_t has_alarm = 0;

int usage(void)
{
    fprintf(stderr, "Usage: ./client ip_address [-c max_read_count] [-p port] [-r rcvbuf] [-f] [-F flow_ctrl_valie] [-I flow_control_interval] [-G]\n");
    
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

int main(int argc, char *argv[])
{
    unsigned char read_buf[64*1024];
    unsigned char write_buf[16];
    int c, n;
    unsigned long max_read_counter = 10000;
    unsigned read_counter = 0;
    int port = 1234;
    unsigned long seq_num;
    char *server_ip_address;
    /* flow_ctrl variables */
    char *if_name              = "eth0";
    int do_flow_ctrl           = 0;
    int flow_ctrl_interval_sec = 2;
    int flow_ctrl_value    = 65535;
    int ignore_seq_num_error = 0;
    int so_rcvbuf = -1;

    while ( (c = getopt(argc, argv, "c:dfhi:p:r:F:GI:")) != -1) {
        switch (c) {
            case 'c':
                max_read_counter = get_num(optarg);
                break;
            case 'd':
                debug++;
                break;
            case 'f':
                do_flow_ctrl = 1;
                break;
            case 'h':
                usage();
                exit(0);
            case 'i':
                if_name = optarg;
                break;
            case 'p':
                port = get_num(optarg);
                break;
            case 'r':
                so_rcvbuf = get_num(optarg);
                break;
            case 'F':
                flow_ctrl_value = get_num(optarg);
                if (flow_ctrl_value < 0 || flow_ctrl_value > 65535) {
                    fprintf(stderr, "Too large flow_ctrl_value (0 - 65535)\n");
                    exit(1);
                }
                break;
            case 'G':
                ignore_seq_num_error = 1;
                break;
            case 'I':
                flow_ctrl_interval_sec = get_num(optarg);
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

    if (do_flow_ctrl) {
        my_signal(SIGALRM, sig_alarm);
        set_timer(flow_ctrl_interval_sec, 0, flow_ctrl_interval_sec, 0);
        if (debug) {
            fprintf(stderr, "flow_ctrl_value:        %d\n", flow_ctrl_value);
            fprintf(stderr, "flow_ctrl_interval_sec: %d\n", flow_ctrl_interval_sec);
        }
    }

    server_ip_address = argv[0];

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
    
    n = write(sockfd, write_buf, sizeof(write_buf));
    if (n < 0) {
        err(1, "write for 1st packet");
    }

    for ( ; ; ) {
        if (read_counter == max_read_counter) {
            exit(0);
        }
        if (debug) {
            if (read_counter % 1000 == 0) {
                int nbytes;
                ioctl(sockfd, FIONREAD, &nbytes);
                fprintf(stderr, "socket buffer: %d bytes\n", nbytes);
            }
        }

        if (do_flow_ctrl && has_alarm) {
            has_alarm = 0;
            fprintf(stderr, "send pause frame\n");
            flow_ctrl_pause(if_name, "01:80:c2:00:00:01", flow_ctrl_value);
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
        seq_num = get_seq_num(read_buf, n);
        if (read_counter != seq_num) {
            fprintf(stderr, "seq_num error at seq_num: %ld read_counter: %d\n",
                seq_num, read_counter);
            if (ignore_seq_num_error) {
                read_counter = seq_num;
            }
            else {
                exit(0);
            }
        }
        read_counter ++;
    }
        
    return 0;
}
