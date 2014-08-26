#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "my_socket.h"
#include "my_signal.h"
#include "host_info.h"
#include "get_num.h"
#include "readn.h"

host_info *host_list = NULL;
int debug = 0;
int display_stat = 0;
struct timeval start_time;

int usage(void)
{
	char *message =
"./sample ip_address:port [-b read_size | -a] [-r so_rcvbuf] [-d] [-L low_watermark] [ip_address:port ...]\n";

	fprintf(stderr, message);
	return 0;
}

int get_sock_recv_buf(int sockfd)
{
    int bufsize;
    socklen_t len = sizeof(bufsize);
    if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, &len) < 0) {
        perror("getsockopt");
        return -1;
    }
    return bufsize;
}

void sig_int(int signo)
{
    int i;
    host_info *p;
    struct timeval stop_time, diff;

    gettimeofday(&stop_time, NULL);
    timersub(&stop_time, &start_time, &diff);
    double time = diff.tv_sec + 0.000001*diff.tv_usec;

	for (p = host_list; p != NULL; p = p->next) {
        int r = get_so_rcvbuf(p->sockfd);
        printf("%s %lld bytes %.6f sec %.3f MB/s read_count: %lld init_rcvbuf: %d final_rcvbuf: %d\n",
            p->ip_address,
            p->read_bytes,
            time,
            (double)p->read_bytes/1024.0/1024.0/time,
            p->read_count,
            p->init_so_rcvbuf,
            r);
    }

    if (display_stat) {
        for (p = host_list; p != NULL; p = p->next) {
            for (i = 0; i < 33; i++) {
                printf("1460*%d <= n < 1460*%d %lld\n", i, i+1, p->read_bytes_histo[i]);
            }
            printf("n >= 1460*%d %lld\n", 33, p->read_bytes_histo[33]);
        }
    }

    exit(0);
}

int main(int argc, char *argv[])
{
	int i, n, ch;
	host_info *p;
	int timeout = 2;
	int n_server;
    int best_effort = 0;
    int recv_low_watermark = 0;
    unsigned write_buf[16];

	int epfd;
	int nfds;
	struct epoll_event ev, *ev_ret;
    int read_size = 1460;
    int so_rcvbuf = 0;
    int enable_quick_ack = 0;

	while ( (ch = getopt(argc, argv, "ab:dqr:SL:")) != -1) {
		switch (ch) {
            case 'a':
                best_effort = 1;
                read_size = DEFAULT_BUFSIZE; /* 2MB */
                break;
            case 'b':
                read_size = get_num(optarg);
                break;
			case 'd':
				debug = 1;
				break;
			case 'q':
				enable_quick_ack = 1;
				break;
            case 'r':
                so_rcvbuf = get_num(optarg);
                break;
            case 'S':
                display_stat = 1;
                break;
            case 'L':
                recv_low_watermark = get_num(optarg);
                break;
			default:
				break;
		}
	}

	argc -= optind;
	argv += optind;
			
	if (argc == 0) {
		usage();
		exit(1);
	}

	n_server = argc;

    my_signal(SIGINT,  sig_int);
    my_signal(SIGTERM, sig_int);

	/* Prepare host_list */
	for (i = 0; i < argc; i++) {
		if (debug) {
			printf("%s\n", argv[i]);
		}
		host_list = addend(host_list, new_host(argv[i]));
	}

	/* get sockfd and connect to server */
	//for (p = host_list; p != NULL; p = p->next) {
	//	if (debug) {
	//		printf("try to connect: %s Port: %d\n", p->ip_address, p->port);
	//	}
	//	connect_to_server(p, timeout);
	//}

	for (p = host_list; p != NULL; p = p->next) {
		if ( (p->sockfd = udp_socket()) < 0) {
			errx(1, "socket create fail");
		}
	}

	for (p = host_list; p != NULL; p = p->next) {
        if (so_rcvbuf > 0) {
            if (set_so_rcvbuf(p->sockfd, so_rcvbuf) < 0) {
                errx(1, "set_so_rcvbuf");
            }
        }
        p->init_so_rcvbuf = get_so_rcvbuf(p->sockfd);
        if (debug) {
            printf("SO_RCVBUF: %d\n", p->init_so_rcvbuf);
        }
    }

	for (p = host_list; p != NULL; p = p->next) {
		if (connect_udp(p->sockfd, p->ip_address, p->port) < 0) {
			errx(1, "connect to %s fail", p->ip_address);
		}
	}

	/* EPOLL Data structure */
	if ( (ev_ret = malloc(sizeof(struct epoll_event) * n_server)) == NULL) {
		err(1, "malloc for epoll_event data structure");
	}
	if ( (epfd = epoll_create(n_server)) < 0) {
		err(1, "epoll_create");
	}
	for (p = host_list; p != NULL; p = p->next) {
        if (debug) {
		    fprintf(stderr, "%s port %d\n", p->ip_address, p->port);
        }
		memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN;
		ev.data.ptr = p;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, p->sockfd, &ev) < 0) {
			err(1, "XXX epoll_ctl at %s port %d", p->ip_address, p->port);
		}
	}

    gettimeofday(&start_time, NULL);

	for (p = host_list; p != NULL; p = p->next) {
        if (write(p->sockfd, write_buf, sizeof(write_buf)) < 0) {
            err(1, "write to server %s port %d fail", p->ip_address, p->port);
        }
    }

	for ( ; ; ) {
		nfds = epoll_wait(epfd, ev_ret, n_server, timeout * 1000);
		if (nfds < 0) {
			if (errno == EINTR) {
				continue;
			}
			else {
				err(1, "epoll_wait");
			}
		}
		else if (nfds == 0) {
			fprintf(stderr, "epoll_wait timed out: %d sec\n", timeout);
			continue;
		}

		for (i = 0; i < nfds; i++) {
			p = ev_ret[i].data.ptr;
            if (best_effort) {
                if (enable_quick_ack) {
                    set_so_quickack(p->sockfd);
                }
			    n = read(p->sockfd, p->buf, read_size);
            }
            else {
                if (enable_quick_ack) {
                    set_so_quickack(p->sockfd);
                }
			    n = readn(p->sockfd, p->buf, read_size);
            }
			if (n < 0) {
				err(1, "read error");
			}
			else if (n == 0) {
				epoll_ctl(epfd, EPOLL_CTL_DEL, p->sockfd, NULL);
				if (close(p->sockfd) < 0) {
					err(1, "close on %s", p->ip_address);
				}
				n_server --;
				if (n_server == 0) {
					exit(0);
				}
			}
			else {
				p->read_bytes += n;
				p->read_count ++;
                int m = n/1460;
                if (m > 32) {
                    p->read_bytes_histo[33] ++;
                }
                else {
                    p->read_bytes_histo[m] ++;
                }
			}
		}
	}
    return 0;
}
