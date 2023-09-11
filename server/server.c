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

int usage()
{
    char msg[] = "Usage: server [-d] [-b bufsize] [-c max_write_counter] [-p port] [-s sleep_usec] [-z bzsleep_usec]\n"
                 "default bufsize: 1024 bytes.  Allow k (kilo), m (mega) suffix\n"
                 "default port: 1234\n";
    fprintf(stderr, "%s", msg);

    return 0;
}

int main(int argc, char *argv[])
{
    unsigned char read_buf[64*1024];
    unsigned char *write_buf;
    int write_buf_size = 1024;
    int c, m, n;
    struct sockaddr_in cliaddr;
    struct sockaddr_in servaddr;
    socklen_t len;
    unsigned write_counter = 0;
    unsigned max_write_counter = 10000;
    int usleep_time = 0;
    int use_bzsleep = 0;
    int port = 1234;

    while ( (c = getopt(argc, argv, "db:c:hp:s:z:")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 'b':
                write_buf_size = get_num(optarg);
                break;
            case 'c':
                max_write_counter = get_num(optarg);
                break;
            case 'h':
                usage();
                exit(0);
            case 'p':
                port = get_num(optarg);
                break;
            case 's':
                usleep_time = get_num(optarg);
                break;
            case 'z':
                usleep_time = get_num(optarg);
                use_bzsleep = 1;
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    memset(&cliaddr, 0, sizeof(cliaddr));
    memset(&servaddr, 0, sizeof(servaddr));

    write_buf = malloc(write_buf_size);
    if (write_buf == NULL) {
        err(1, "malloc");
    }

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
        n = recvfrom(sockfd, read_buf, sizeof(read_buf), 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            err(1, "recvfrom");
        }
        char remote_ip[32];
        inet_ntop(AF_INET, (struct sockaddr *)&cliaddr.sin_addr, remote_ip, sizeof(remote_ip));
        fprintfwt(stderr, "access from: %s.  max_write_counter: %d\n", remote_ip, max_write_counter);
        debug_print(stderr, "recvfrom() returns\n");

        for ( ; ; ) {
            int k;
            if (write_counter == max_write_counter) {
                break;
            }

            for (k = 0; k < 4; k ++) {
                int x = htonl(write_counter);
                memcpy(&write_buf[0], &x, sizeof(int));
                m = sendto(sockfd, write_buf, write_buf_size, 0, (struct sockaddr *)&cliaddr, len);
                if (m < 0) {
                    err(1, "sendto");
                }
                write_counter ++;
            }
            if (usleep_time > 0) {
                if (use_bzsleep) {
                    bz_usleep(usleep_time);
                }
                else {
                    usleep(usleep_time);
                }
            }
        }
        fprintfwt(stderr, "write done: max_write_counter: %d\n", max_write_counter);
        
        if (close(sockfd) < 0) {
            err(1, "close");
        }
        write_counter = 0;
    }
        
    return 0;
}
