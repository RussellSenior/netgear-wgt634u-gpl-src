/* port trigger extension for TCP NAT alteration. */
#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <linux/netfilter_ipv4/ip_nat.h>
#include <linux/netfilter_ipv4/ip_nat_helper.h>
#include <linux/netfilter_ipv4/ip_nat_rule.h>
#include <linux/netfilter_ipv4/ip_conntrack_port_trigger.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>
#include <linux/netfilter_ipv4/ip_conntrack_core.h>
#include <linux/netfilter_ipv4/listhelp.h>

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

#define MAX_PORTS 256
#define MAX_PORTS 512
#define MAX_EXPECTS	MAX_PORTS
#define MAX_ONE_SET_EXPECTS	32
#define MAX_SETTING_SET	64
static int ports[MAX_PORTS];
static int ports_c = 0;
static u_int32_t outgoing_ports[MAX_SETTING_SET][2] = {{0,0}};
static struct ip_nat_helper port_trigger[MAX_PORTS];

#ifdef MODULE_PARM
//MODULE_PARM(ports, "1-" __MODULE_STRING(MAX_PORTS) "i");
MODULE_PARM(outgoing_ports, "0-65536i");
#endif

DECLARE_LOCK_EXTERN(ip_port_triggering_lock);

/* FIXME: Time out? --RR */

static unsigned int
port_trigger_nat_expected(struct sk_buff **pskb,
		 unsigned int hooknum,
		 struct ip_conntrack *ct,
		 struct ip_nat_info *info)
{
	struct ip_nat_multi_range mr;
	u_int32_t newdstip, newsrcip, newip;
	struct ip_ct_port_trigger_expect *exp_port_trigger_info;

	struct ip_conntrack *master = master_ct(ct);
	
	IP_NF_ASSERT(info);
	IP_NF_ASSERT(master);

	IP_NF_ASSERT(!(info->initialized & (1<<HOOK2MANIP(hooknum))));

	DEBUGP("nat_expected: We have a connection!\n");
	exp_port_trigger_info = &ct->master->help.exp_port_trigger_info;

	LOCK_BH(&ip_port_triggering_lock);
/*
	if (exp_port_trigger_info->ftptype == IP_CT_FTP_PORT
	    || exp_port_trigger_info->ftptype == IP_CT_FTP_EPRT) {
*/		/* PORT command: make connection go to the client. */
		newdstip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
		newsrcip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip;
/*		DEBUGP("nat_expected: PORT cmd. %u.%u.%u.%u->%u.%u.%u.%u\n",
		       NIPQUAD(newsrcip), NIPQUAD(newdstip));
	} else {*/
		/* PASV command: make the connection go to the server */
/*		newdstip = master->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip;
		newsrcip = master->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
		DEBUGP("nat_expected: PASV cmd. %u.%u.%u.%u->%u.%u.%u.%u\n",
		       NIPQUAD(newsrcip), NIPQUAD(newdstip));
	}*/
	UNLOCK_BH(&ip_port_triggering_lock);

	if (HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC)
		newip = newsrcip;
	else
		newip = newdstip;

	DEBUGP("nat_expected: IP to %u.%u.%u.%u\n", NIPQUAD(newip));

	mr.rangesize = 1;
	/* We don't want to manip the per-protocol, just the IPs... */
	mr.range[0].flags = IP_NAT_RANGE_MAP_IPS;
	mr.range[0].min_ip = mr.range[0].max_ip = newip;

	/* ... unless we're doing a MANIP_DST, in which case, make
	   sure we map to the correct port */
	if (HOOK2MANIP(hooknum) == IP_NAT_MANIP_DST) {
		mr.range[0].flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
		mr.range[0].min = mr.range[0].max
			= ((union ip_conntrack_manip_proto)
				{ htons(exp_port_trigger_info->port) });
	}
	return ip_nat_setup_info(ct, &mr, hooknum);
}
/*
static int
mangle_rfc959_packet(struct sk_buff **pskb,
		     u_int32_t newip,
		     u_int16_t port,
		     unsigned int matchoff,
		     unsigned int matchlen,
		     struct ip_conntrack *ct,
		     enum ip_conntrack_info ctinfo)
{
	char buffer[sizeof("nnn,nnn,nnn,nnn,nnn,nnn")];

	MUST_BE_LOCKED(&ip_port_triggering_lock);

	sprintf(buffer, "%u,%u,%u,%u,%u,%u",
		NIPQUAD(newip), port>>8, port&0xFF);

	DEBUGP("calling ip_nat_mangle_tcp_packet\n");

	return ip_nat_mangle_tcp_packet(pskb, ct, ctinfo, matchoff, 
					matchlen, buffer, strlen(buffer));
}
*/
/* |1|132.235.1.2|6275| */
/*
static int
mangle_eprt_packet(struct sk_buff **pskb,
		   u_int32_t newip,
		   u_int16_t port,
		   unsigned int matchoff,
		   unsigned int matchlen,
		   struct ip_conntrack *ct,
		   enum ip_conntrack_info ctinfo)
{
	char buffer[sizeof("|1|255.255.255.255|65535|")];

	MUST_BE_LOCKED(&ip_port_triggering_lock);

	sprintf(buffer, "|1|%u.%u.%u.%u|%u|", NIPQUAD(newip), port);

	DEBUGP("calling ip_nat_mangle_tcp_packet\n");

	return ip_nat_mangle_tcp_packet(pskb, ct, ctinfo, matchoff, 
					matchlen, buffer, strlen(buffer));
}
*/
/* |1|132.235.1.2|6275| */
/*
static int
mangle_epsv_packet(struct sk_buff **pskb,
		   u_int32_t newip,
		   u_int16_t port,
		   unsigned int matchoff,
		   unsigned int matchlen,
		   struct ip_conntrack *ct,
		   enum ip_conntrack_info ctinfo)
{
	char buffer[sizeof("|||65535|")];

	MUST_BE_LOCKED(&ip_port_triggering_lock);

	sprintf(buffer, "|||%u|", port);

	DEBUGP("calling ip_nat_mangle_tcp_packet\n");

	return ip_nat_mangle_tcp_packet(pskb, ct, ctinfo, matchoff, 
					matchlen, buffer, strlen(buffer));
}
*/
/*
static int (*mangle[])(struct sk_buff **, u_int32_t, u_int16_t,
		     unsigned int,
		     unsigned int,
		     struct ip_conntrack *,
		     enum ip_conntrack_info)
= { [IP_CT_FTP_PORT] mangle_rfc959_packet,
    [IP_CT_FTP_PASV] mangle_rfc959_packet,
    [IP_CT_FTP_EPRT] mangle_eprt_packet,
    [IP_CT_FTP_EPSV] mangle_epsv_packet
};
*/
static unsigned int help(struct ip_conntrack *ct,
			 struct ip_conntrack_expect *exp,
			 struct ip_nat_info *info,
			 enum ip_conntrack_info ctinfo,
			 unsigned int hooknum,
			 struct sk_buff **pskb)
{
	struct iphdr *iph = (*pskb)->nh.iph;
	struct tcphdr *tcph = (void *)iph + iph->ihl*4;
	unsigned int datalen;
	int dir;
	struct ip_ct_port_trigger_expect *ct_port_trigger_info;

	if (!exp)
		DEBUGP("ip_nat_port_trigger: no exp!!");

	ct_port_trigger_info = &exp->help.exp_port_trigger_info;

	/* Only mangle things once: original direction in POST_ROUTING
	   and reply direction on PRE_ROUTING. */
	/*
	dir = CTINFO2DIR(ctinfo);
	if (!((hooknum == NF_IP_POST_ROUTING && dir == IP_CT_DIR_ORIGINAL)
	      || (hooknum == NF_IP_PRE_ROUTING && dir == IP_CT_DIR_REPLY))) {
		DEBUGP("nat_ftp: Not touching dir %s at hook %s\n",
		       dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
		       hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
		       : hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
		       : hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "???");
		return NF_ACCEPT;
	}
	
if(dir == IP_CT_DIR_REPLY) return NF_ACCEPT;
	datalen = (*pskb)->len - iph->ihl * 4 - tcph->doff * 4;
	LOCK_BH(&ip_port_triggering_lock);
	if (between(exp->seq + ct_port_trigger_info->len,
		    ntohl(tcph->seq),
		    ntohl(tcph->seq) + datalen)) {
		if (!ftp_data_fixup(ct_port_trigger_info, ct, pskb, ctinfo, exp)) {
			UNLOCK_BH(&ip_port_triggering_lock);
			return NF_DROP;
		}
	} else {
		if (net_ratelimit()) {
			printk("FTP_NAT: partial packet %u/%u in %u/%u\n",
			       exp->seq, ct_port_trigger_info->len,
			       ntohl(tcph->seq),
			       ntohl(tcph->seq) + datalen);
		}
		UNLOCK_BH(&ip_port_triggering_lock);
		return NF_DROP;
	}
	UNLOCK_BH(&ip_port_triggering_lock);
*/
	return NF_ACCEPT;
}

/* Not __exit: called from init() */
static void fini(void)
{
	int i;

	for (i = 0; i < ports_c; i++) {
		DEBUGP("ip_nat_port_trigger: unregistering port %d\n", ports[i]);
		ip_nat_helper_unregister(&port_trigger[i]);
	}
}

static int __init init(void)
{
	int i=0, ret=0, j=0, k=0;
	char *tmpname;

	if(outgoing_ports[0][0] == 0 || outgoing_ports[0][0]>outgoing_ports[0][1])
		ports[0] = 0;
	else {
		j=0;
		for(i=0; (i<MAX_SETTING_SET) && (j<MAX_PORTS) && outgoing_ports[i][0]; i++) {
			for(k=0;k<=(outgoing_ports[i][1]-outgoing_ports[i][0]) && j<MAX_PORTS;k++)
				ports[j++] = outgoing_ports[i][0]+k;
		}
		ports[j] = 0;
	}

	for (i = 0; (i < MAX_PORTS) && ports[i]; i++) {

		memset(&port_trigger[i], 0, sizeof(struct ip_nat_helper));

		port_trigger[i].tuple.dst.protonum = IPPROTO_TCP;
		port_trigger[i].tuple.src.u.tcp.port = htons(ports[i]);
		port_trigger[i].mask.dst.protonum = 0xFFFF;
		port_trigger[i].mask.src.u.tcp.port = 0xFFFF;
		port_trigger[i].help = help;
		port_trigger[i].me = THIS_MODULE;
		port_trigger[i].flags = 0;
		port_trigger[i].expect = port_trigger_nat_expected;
/*
		tmpname = &ftp_names[i][0];
		if (ports[i] == FTP_PORT)
			sprintf(tmpname, "ftp");
		else
			sprintf(tmpname, "ftp-%d", i);
		port_trigger[i].name = tmpname;

		DEBUGP("ip_nat_ftp: Trying to register for port %d\n",
				ports[i]);*/
		ret = ip_nat_helper_register(&port_trigger[i]);

		if (ret) {
			printk("ip_nat_port_trigger: error registering "
			       "helper for port %d\n", ports[i]);
			fini();
			return ret;
		}
		ports_c++;
	}

	return ret;
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LIGHT C.C. LU");
