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

int main(int argc, char *argv[])
{
    unsigned char read_buf[64*1024];
    unsigned char *write_buf;
    int write_buf_size = 1024;
    int c, m, n;
    struct sockaddr_in cliaddr;
    struct sockaddr_in servaddr;
    socklen_t len;
    int write_counter = 0;
    int max_write_counter = 10000;
    int usleep_time = 0;
    int use_bzsleep = 0;
    int port = 1234;

    while ( (c = getopt(argc, argv, "b:c:p:s:z:")) != -1) {
        switch (c) {
            case 'b':
                write_buf_size = get_num(optarg);
                break;
            case 'c':
                max_write_counter = get_num(optarg);
                break;
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

    //int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int sockfd = udp_socket();
    if (sockfd < 0) {
        errx(1, "socket");
    }

    if (my_bind(sockfd, "0.0.0.0", port) < 0) {
        errx(1, "bind");
    }
    
    write_buf = malloc(write_buf_size);
    if (write_buf == NULL) {
        err(1, "malloc");
    }

    len = sizeof(cliaddr);
    n = recvfrom(sockfd, read_buf, sizeof(read_buf), 0, (struct sockaddr *)&cliaddr, &len);
    if (n < 0) {
        err(1, "recvfrom");
    }
    
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
        
    return 0;
}
