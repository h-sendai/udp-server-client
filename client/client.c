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

int usage(void)
{
    fprintf(stderr, "Usage: ./client ip_address [-p port]\n");
    
    return 0;
}

int main(int argc, char *argv[])
{
    unsigned char read_buf[64*1024];
    unsigned char write_buf[16];
    int c, n;
    int max_read_counter = 10000;
    int read_counter = 0;
    int port = 1234;
    char *server_ip_address;

    while ( (c = getopt(argc, argv, "c:dp:")) != -1) {
        switch (c) {
            case 'c':
                max_read_counter = get_num(optarg);
                break;
            case 'p':
                port = get_num(optarg);
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
            break;
        }
        n = read(sockfd, read_buf, sizeof(read_buf));
        if (n < 0) {
            err(1, "read");
        }
        read_counter ++;
    }
        
    return 0;
}
