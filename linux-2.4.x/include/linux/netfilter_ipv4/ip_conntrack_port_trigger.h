#ifndef _IP_CONNTRACK_PORT_TRIGGER_H
#define _IP_CONNTRACK_PORT_TRIGGER_H
/* port trigger tracking. */

#ifdef __KERNEL__

#include <linux/netfilter_ipv4/lockhelp.h>

/* Protects port trigger part of conntracks */
DECLARE_LOCK_EXTERN(ip_port_trigger_lock);

#endif /* __KERNEL__ */

/* This structure is per expected connection */
struct ip_ct_port_trigger_expect
{
	/* We record seq number and length of port trigger ip/port text here: all in
	 * host order. */

 	/* sequence number of IP address in packet is in ip_conntrack_expect */
	u_int32_t len;			/* length of IP address */
	u_int16_t port; 		/* TCP port that was to be used */
};

/* This structure exists only once per master */
struct ip_ct_port_trigger_master {
	/* Next valid seq position for cmd matching after newline */
	u_int32_t seq_aft_nl[IP_CT_DIR_MAX];
	/* 0 means seq_match_aft_nl not set */
	int seq_aft_nl_set[IP_CT_DIR_MAX];
};

#endif /* _IP_CONNTRACK_PORT_TRIGGER_H */
