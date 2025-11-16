#define _GNU_SOURCE
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
//              int flags, struct timespec *timeout);
//
// struct mmsghdr {
//     struct msghdr msg_hdr;  /* Message header */
//     unsigned int  msg_len;  /* Number of received bytes for header */
// };
//
// struct msghdr {
//     void         *msg_name;       /* Optional address */
//     socklen_t     msg_namelen;    /* Size of address */
//     struct iovec *msg_iov;        /* Scatter/gather array */
//     size_t        msg_iovlen;     /* # elements in msg_iov */
//     void         *msg_control;    /* Ancillary data, see below */
//     size_t        msg_controllen; /* Ancillary data buffer len */
//     int           msg_flags;      /* Flags on received message */
// } ;
//
// #include <sys/uio.h>
//
// struct iovec {
//     void   *iov_base;  /* Starting address */
//     size_t  iov_len;   /* Size of the memory pointed to by iov_base. */
// };

int main(int argc, char *argv[])
{
    unsigned int vlen = 16;
    int bufsize = 4*1024; // 4 kB buf

    struct mmsghdr *msgvec = (struct mmsghdr *)malloc(sizeof(struct mmsghdr)*vlen);
    memset(msgvec, 0, sizeof(struct mmsghdr)*vlen);

    struct iovec **iov;
    iov = malloc(sizeof(struct iovec *) * vlen);
    for (int i = 0; i < vlen; ++i) {
        iov[i] = malloc(sizeof(struct iovec));
        iov[i]->iov_base = malloc(bufsize);
        memset(iov[i]->iov_base, 0, bufsize);
        iov[i]->iov_len = bufsize;
        msgvec[i].msg_hdr.msg_iov = iov[i];
        msgvec[i].msg_hdr.msg_iovlen = 1;
        msgvec[i].msg_len = 0;
    }

    int sockfd = 0;
    int n = recvmmsg(sockfd, msgvec, vlen, 0, NULL);
    if (n < 0) {
        err(1, "recvmmsg");
    }
    fprintf(stderr, "n: %d\n", n);

#if 0
    struct iovec *iov;
    iov = malloc(sizeof(struct iovec *) * vlen);
    for (int i = 0; i < vlen; ++i) {
        iov[i].iov_base =  malloc(bufsize);
        memset(iov[i].iov_base, 0, bufsize);
        iov[i].iov_len  = bufsize;
        msgvec[i].msg_hdr.msg_iov    = &iov[i];
        msgvec[i].msg_hdr.msg_iovlen = 1;
        msgvec[i].msg_len = 0;
    }

    int sockfd = 0;
    int n = recvmmsg(sockfd, msgvec, vlen, 0, NULL);
    if (n < 0) {
        err(1, "recvmmsg");
    }
    fprintf(stderr, "n: %d\n", n);
    /* or use array */
#endif
#if 0
    struct iovec iov[vlen];
    for (int i = 0; i < vlen; ++i) {
        iov[i].iov_base = malloc(bufsize);
        iov[i].iov_len  = bufsize;
        msgvec[i].msg_hdr.msg_iov = &iov[i];
        msgvec[i].msg_hdr.msg_iovlen = 1;
        msgvec[i].msg_len = 0;
    }
#endif

    return 0;
}
