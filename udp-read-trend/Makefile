PROG = udp-read-trend
CPPFLAGS += -I.
CFLAGS = -g -O2 -Wall

all: $(PROG)

OBJS += get_num.o
OBJS += host_info.o
OBJS += my_signal.o
OBJS += my_socket.o
OBJS += readn.o
OBJS += set_timer.o
OBJS += set_cpu.o
OBJS += logUtil.o

OBJS += $(PROG).o

$(PROG): $(OBJS)

clean:
	rm -f *.o $(PROG)
