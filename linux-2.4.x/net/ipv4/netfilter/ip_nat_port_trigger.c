/* port trigger extension for TCP & UDP NAT alteration. */
#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/udp.h>
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
#define MAX_EXPECTS	MAX_PORTS*2
#define MAX_ONE_SET_EXPECTS	32
#define MAX_SETTING_SET	64
static u_int32_t ports[MAX_PORTS];
static u_int32_t protocols[MAX_PORTS];
static u_int32_t ports_c = 0;
static u_int32_t outgoing_protocols[MAX_SETTING_SET]={ 0 };
static u_int32_t outgoing_ports[MAX_SETTING_SET][2] = {{0,0}};
static struct ip_nat_helper port_trigger[MAX_PORTS];

#ifdef MODULE_PARM
MODULE_PARM(outgoing_protocols, "0-64i"); //1:TCP 2:UDP
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
	newdstip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
	newsrcip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip;
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
static unsigned int help(struct ip_conntrack *ct,
			 struct ip_conntrack_expect *exp,
			 struct ip_nat_info *info,
			 enum ip_conntrack_info ctinfo,
			 unsigned int hooknum,
			 struct sk_buff **pskb)
{
	int dir;
	struct ip_ct_port_trigger_expect *ct_port_trigger_info;

	if (!exp)
		DEBUGP("ip_nat_port_trigger: no exp!!");

	ct_port_trigger_info = &exp->help.exp_port_trigger_info;

	return NF_ACCEPT;
}

/* Not __exit: called from init() */
static void fini(void)
{
	int i;

	for (i = 0; i < ports_c; i++) {
		DEBUGP("ip_nat_port_trigger: unregistering helper for port %d\n", ports[i]);
		ip_nat_helper_unregister(&port_trigger[i]);
	}
}

static int __init init(void)
{
	int i=0, ret=0, j=0, k=0;

	if(outgoing_ports[0][0] == 0 || outgoing_ports[0][0]>outgoing_ports[0][1]) {
		ports[0] = 0;
		protocols[0] = 0;
	}
	else {
		j=0;
		for(i=0; (i<MAX_SETTING_SET) && (j<MAX_PORTS) && outgoing_ports[i][0]; i++) {
			for(k=0;k<=(outgoing_ports[i][1]-outgoing_ports[i][0]) && j<MAX_PORTS;k++) {
				ports[j] = outgoing_ports[i][0]+k;
				protocols[j++] = (outgoing_protocols[i]==1)?IPPROTO_TCP:IPPROTO_UDP;
			}
		}
		ports[j] = 0;
		protocols[j] = 0;
	}

	for (i = 0; (i < MAX_PORTS) && ports[i]; i++) {
		memset(&port_trigger[i], 0, sizeof(struct ip_nat_helper));
		if(protocols[i] == IPPROTO_TCP) 
			port_trigger[i].tuple.src.u.tcp.port = htons(ports[i]);
		else
			port_trigger[i].tuple.src.u.udp.port = htons(ports[i]);
		port_trigger[i].tuple.dst.protonum = protocols[i];
		port_trigger[i].mask.dst.protonum = 0xFFFF;
		port_trigger[i].mask.src.u.udp.port = 0xFFFF;
		port_trigger[i].mask.src.u.tcp.port = 0xFFFF;
		port_trigger[i].help = help;
		port_trigger[i].me = THIS_MODULE;
		port_trigger[i].flags = 0;
		port_trigger[i].expect = port_trigger_nat_expected;
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
MODULE_AUTHOR("LIGHT C. C. LU");
