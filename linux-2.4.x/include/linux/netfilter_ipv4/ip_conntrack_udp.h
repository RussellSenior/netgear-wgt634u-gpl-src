#ifndef _CONNTRACK_UDP_H
#define _CONNTRACK_UDP_H

/* UDP tracking. */
struct ip_ct_udp {
	unsigned int stream_timeout;
	unsigned int timeout;
};

#endif /* _CONNTRACK_UDP_H */
