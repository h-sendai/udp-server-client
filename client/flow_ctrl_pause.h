#ifndef _FLOW_CTRL_PAUSE
#define _FLOW_CTRL_PAUSE

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <linux/if.h>

#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int flow_ctrl_pause(char *ifname, char *mac_address, int pause_time);

#endif
