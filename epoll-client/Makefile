PROG = epoll-client
CFLAGS = -std=gnu99 -g -O2
# -Wall -D_BSD_SOURCE=1 -D_POSIX_SOURCE

all: ${PROG}

OBJS += $(PROG).o
OBJS += my_socket.o
OBJS += host_info.o
OBJS += get_num.o
OBJS += readn.o
OBJS += my_signal.o

${PROG}: $(OBJS)

clean:
	rm -f *.o ${PROG}
