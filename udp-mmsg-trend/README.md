# recvmmsg()を使った実装

```
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
```

データ構造を作るサンプル
[args.c](args.c)

1秒に1回、読んだバイト数とドロップした（と思われる）回数を表示するために
SIGALRMでシグナルを送っているが、最初のSIGALRMが来たあと、recvvmsg()が
errno 512 (unknown error)を返すようだ。

とりあえず回避のためにrecvvmsgした回数をかぞえて1000回ごとに読んだバイト数などを
表示するようにしてある。
