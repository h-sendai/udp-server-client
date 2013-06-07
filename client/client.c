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
    fprintf(stderr, "Usage: ./client ip_address [-p port] [-f] [-F flow_ctrl_valie] [-I flow_control_interval] [-G]\n");
    
    return 0;
}

void sig_alarm(int signo)
{
    has_alarm = 1;
    return;
}

int get_seq_num(unsigned char *buf, int len)
{
    unsigned int s;
    s = *(unsigned int *)buf;
    s = ntohl(s);

    return s;
}

int main(int argc, char *argv[])
{
    unsigned char read_buf[64*1024];
    unsigned char write_buf[16];
    int c, n;
    int max_read_counter = 10000;
    int read_counter = 0;
    int port = 1234;
    int seq_num;
    char *server_ip_address;
    /* flow_ctrl variables */
    char *if_name              = "eth0";
    int do_flow_ctrl           = 0;
    int flow_ctrl_interval_sec = 2;
    int flow_ctrl_value    = 65535;
    int ignore_seq_num_error = 0;

    while ( (c = getopt(argc, argv, "c:dfi:p:F:GI:")) != -1) {
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
            case 'i':
                if_name = optarg;
                break;
            case 'p':
                port = get_num(optarg);
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
    
    n = write(sockfd, write_buf, sizeof(write_buf));
    if (n < 0) {
        err(1, "write for 1st packet");
    }

    for ( ; ; ) {
        if (read_counter == max_read_counter) {
            exit(0);
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
            fprintf(stderr, "seq_num error at seq_num: %d read_counter: %d\n",
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
