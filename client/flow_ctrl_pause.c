#include "flow_ctrl_pause.h"

/*
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
*/

#include "get_num.h"
#include "host_info.h"
#include "my_signal.h"
#include "my_socket.h"
#include "readn.h"
#include "set_timer.h"

static int get_fill(unsigned char *pkt, char *arg, int pause_time)
{
    int debug = 0;
	int sa[6];
	unsigned char station_addr[6];
	int byte_cnt;
	int offset, i;
	char *cp;

	for (cp = arg; *cp; cp++)
		if (*cp != ':' && !isxdigit(*cp)) {
			(void)fprintf(stderr,
						  "flow-ctrl: patterns must be specified as hex digits.\n");
            return -1;
		}

	byte_cnt = sscanf(arg, "%2x:%2x:%2x:%2x:%2x:%2x",
					  &sa[0], &sa[1], &sa[2], &sa[3], &sa[4], &sa[5]);
	for (i = 0; i < 6; i++)
		station_addr[i] = sa[i];
	if (debug)
		fprintf(stderr, "Command line stations address is "
				"%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x.\n",
				sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);

	if (byte_cnt != 6) {
		(void)fprintf(stderr,
					  "flow-ctrl: The destination address must be specified as "
					  "00:11:22:33:44:55.\n");
        return -1;
	}

	memcpy(pkt, station_addr, 6);
	memcpy(pkt+6, station_addr, 6);
	pkt[12] = 0x88;
	pkt[13] = 0x08;
	/* MAC control opcode 00:01 */
	pkt[14] = 0;
	pkt[15] = 1;
	pkt[16] = pause_time >> 8;
	pkt[17] = pause_time;

	offset = 18;

	memset(pkt+offset, 0xff, 42);
	offset += 42;

	if (debug) {
		fprintf(stderr, "Packet is\n");
		for (i = 0; i < offset; i++) {
			if ((i & 15) == 0)
				fprintf(stderr, "0x%04x: ", i);
			fprintf(stderr, "%02x ", pkt[i]);
			if (((i + 1) & 15) == 0)
				fprintf(stderr, "\n");
		}
		fprintf(stderr, "\n");
	}
	return offset;
}

int flow_ctrl_pause(char *ifname, char *mac_address, int pause_time)
{
    unsigned char buf[1024];
    int sockfd;
    int pkt_size;
    struct ifreq if_hwaddr;
    char *hwaddr;
    int one = 1;
    struct sockaddr whereto;

    sockfd = socket(AF_INET, SOCK_PACKET, SOCK_PACKET);
    if (sockfd < 0) {
        if (errno == EPERM) {
            warn("need root privilege");
        }
        else {
            warn("socket");
        }
        return -1;
    }
    
    pkt_size = get_fill(buf, mac_address, pause_time);

    //setuid(getuid());

    hwaddr = if_hwaddr.ifr_hwaddr.sa_data;
    strcpy(if_hwaddr.ifr_name, ifname);
    if (ioctl(sockfd, SIOCGIFHWADDR, &if_hwaddr) < 0) {
        warn("ioctl");
        return -1;
    }
    memcpy(buf + 6, if_hwaddr.ifr_hwaddr.sa_data, 6);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *) &one, sizeof(one)) < 0) {
        warn("setsockopt");
        return -1;
    }

    whereto.sa_family = 0;
    strcpy(whereto.sa_data, ifname);
    if (sendto(sockfd, buf, pkt_size, 0, &whereto, sizeof(whereto)) < 0) {
        warn("sendto");
        return -1;
    }

    return 0;
}

#ifdef USE_MAIN
int main(int argc, char *argv[])
{
    flow_ctrl_pause("eth0", "01:80:c2:00:00:01", 65535);

    return 0;
}
#endif
