#ifndef _READN_H
#define _READN_H 1

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

extern ssize_t readn(int fd, void *vptr, size_t n);
extern ssize_t writen(int fd, const void *vptr, size_t n);

#endif
