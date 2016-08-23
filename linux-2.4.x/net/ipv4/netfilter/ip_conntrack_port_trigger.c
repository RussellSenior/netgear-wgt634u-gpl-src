/* port trigger extension for IP connection tracking. */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <linux/ctype.h>
#include <net/checksum.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/netfilter_ipv4/lockhelp.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>
#include <linux/netfilter_ipv4/ip_conntrack_port_trigger.h>
#include <linux/netfilter_ipv4/listhelp.h>

#define ASSERT_READ_LOCK(x) MUST_BE_READ_LOCKED(&ip_conntrack_lock)
#define ASSERT_WRITE_LOCK(x) MUST_BE_WRITE_LOCKED(&ip_conntrack_lock)
#define MAX_PORTS 512
#define MAX_EXPECTS	MAX_PORTS*2
#define MAX_ONE_SET_EXPECTS	32
#define MAX_SETTING_SET	64
#define DEFAULT_EXPECT_TIMEOUT	1200 /* 20 minutes */
#define DEFAULT_UDP_STREAM_TIMEOUT 180
#define UDP_TIMEOUT (60*HZ)
#define UDP_STREAM_TIMEOUT (180*HZ)

extern struct list_head ip_conntrack_expect_list;
//DECLARE_RWLOCK(ip_conntrack_lock);
DECLARE_LOCK(ip_port_triggering_lock);
struct module *ip_conntrack_port_trigger = THIS_MODULE;
static u_int32_t ports[MAX_PORTS];
static u_int32_t protocols[MAX_PORTS];
static u_int32_t ports_c = 0;
static u_int32_t incoming_protocols[MAX_SETTING_SET]={ 0 };
static u_int32_t outgoing_protocols[MAX_SETTING_SET]={ 0 };
static u_int32_t incoming_ports[MAX_SETTING_SET][2] = {{0,0}};
static u_int32_t outgoing_ports[MAX_SETTING_SET][2] = {{0,0}};
static u_int32_t timeout=DEFAULT_EXPECT_TIMEOUT;
struct ip_conntrack_expect expect[MAX_EXPECTS];
struct ip_conntrack *CT;
u_int32_t number_expect_working;
static struct ip_conntrack_helper port_trigger[MAX_PORTS];
#ifdef MODULE_PARM
MODULE_PARM(incoming_protocols, "0-64i"); //1:TCP 2:UDP 3:TCP&UDP
MODULE_PARM(incoming_ports, "0-65536i");
MODULE_PARM(outgoing_protocols, "0-64i"); //1:TCP 2:UDP 
MODULE_PARM(outgoing_ports, "0-65536i");
MODULE_PARM(timeout, "i");
#endif

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

#define DUMP_TUPLE_RAW_MY(x) 						\
	DEBUGP("tuple %p: %u %u.%u.%u.%u:0x%08x -> %u.%u.%u.%u:0x%08x\n",\
	(x), (x)->dst.protonum,						\
	NIPQUAD((x)->src.ip), ntohl((x)->src.u.all), 			\
	NIPQUAD((x)->dst.ip), ntohl((x)->dst.u.all))

static inline int resent_expect(const struct ip_conntrack_expect *i,
			        const struct ip_conntrack_tuple *tuple,
			        const struct ip_conntrack_tuple *mask)
{
	DEBUGP("resent_expect\n");
	DEBUGP("   tuple:   "); DUMP_TUPLE(&i->tuple);
	DEBUGP("ct_tuple:   "); DUMP_TUPLE(&i->ct_tuple);
	DEBUGP("test tuple: "); DUMP_TUPLE(tuple);
	return (((i->ct_tuple.dst.protonum == 0 && ip_ct_tuple_equal(&i->tuple, tuple))
	         || (i->ct_tuple.dst.protonum && ip_ct_tuple_equal(&i->ct_tuple, tuple)))
		&& ip_ct_tuple_equal(&i->mask, mask));
}

void port_trigger_expectfn(struct ip_conntrack *ct)
{
	int i;
	struct ip_conntrack *master;
	DEBUGP("increasing UDP timeouts\n");
	/* increase timeout of UDP data channel conntrack entry */
	if(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) {
		ct->proto.udp.timeout = UDP_TIMEOUT;
		ct->proto.udp.stream_timeout = timeout * HZ;
		ip_ct_refresh(ct, ct->proto.udp.stream_timeout);
	}

	master = master_ct(ct);
	if (!master) {
		DEBUGP(" no master!!!\n");
		return 0;
	}

	LOCK_BH(&ip_port_triggering_lock);
	for(i=0; i<number_expect_working; i++) 
		ip_conntrack_expect_related(CT, &expect[i]);
	UNLOCK_BH(&ip_port_triggering_lock);
	return;
}

/* FIXME: This should be in userspace.  Later. */
static int conntrack_port_trigger_help(const struct iphdr *iph, size_t len,
		struct ip_conntrack *ct,
		enum ip_conntrack_info ctinfo)
{
	const char *data;
    unsigned int datalen;
	struct udphdr *udph;
	unsigned int udplen;
	struct tcphdr *tcph;
    unsigned int tcplen;
	u_int32_t old_seq_aft_nl;
	int old_seq_aft_nl_set;
	int dir = CTINFO2DIR(ctinfo);
//	struct ip_ct_port_trigger_master *ct_port_trigger_info = &ct->help.ct_port_trigger_info;
	struct ip_conntrack_expect *exp = NULL;
	struct ip_ct_port_trigger_expect *exp_port_trigger_info = NULL;
	struct ip_conntrack_expect *old;
	unsigned int i=0, j=0;
	int portDst=0, arrayRowWorkOn=0;
	CT = ct;
	/* Until there's been traffic both ways, don't look in packets. */
	if(iph->protocol == IPPROTO_TCP && ctinfo != IP_CT_ESTABLISHED
	    && ctinfo != IP_CT_ESTABLISHED+IP_CT_IS_REPLY) {
		DEBUGP("port trigger: Conntrackinfo = %u\n", ctinfo);
		return NF_ACCEPT;
	}
	if(dir == IP_CT_DIR_REPLY)	return NF_ACCEPT;
	if(incoming_ports[0][0] == 0 || incoming_ports[0][0]>incoming_ports[0][1]) return NF_ACCEPT;

	if(iph->protocol == IPPROTO_UDP) {
		udph = (void *)iph + iph->ihl * 4;
	//udplen not negative guaranteed by ip_conntrack_proto_udp.c 
		udplen = len - iph->ihl * 4;
		data = (const char *)udph + 8;
		datalen = udplen - 8;
	}
	if(iph->protocol == IPPROTO_TCP) {
		tcph = (void *)iph + iph->ihl * 4;
	//tcplen not negative guaranteed by ip_conntrack_tcp.c 
    	tcplen = len - iph->ihl * 4;
    	data = (const char *)tcph + tcph->doff * 4;
    	datalen = tcplen - tcph->doff * 4;
	}

	/* Not whole TCP/UDP header? */
	if(iph->protocol == IPPROTO_UDP) {
		if (udplen < sizeof(struct udphdr) || udplen < 8) {
			DEBUGP("port trigger: udplen = %u\n", (unsigned)udplen);
			return NF_ACCEPT;
		}
	}
	if(iph->protocol == IPPROTO_TCP) {
		if (tcplen < sizeof(struct tcphdr) || tcplen < tcph->doff*4) {
	        DEBUGP("port trigger: tcplen = %u\n", (unsigned)tcplen);
			return NF_ACCEPT;
		}
	}
	/* Checksum invalid?  Ignore. */
	/* FIXME: Source route IP option packets --RR */
	if(iph->protocol == IPPROTO_TCP) {
		if (tcp_v4_check(tcph, tcplen, iph->saddr, iph->daddr,
			 csum_partial((char *)tcph, tcplen, 0))) {
			DEBUGP("port_trigger_help: bad csum: %p %u %u.%u.%u.%u %u.%u.%u.%u\n",
		       tcph, tcplen, NIPQUAD(iph->saddr),
		       NIPQUAD(iph->daddr));
			return NF_ACCEPT;
		}
	}
	if(iph->protocol == IPPROTO_UDP) 
		portDst = ntohs(udph->dest);
	else
		portDst = ntohs(tcph->dest);
	arrayRowWorkOn=-1;
	for(i=0; i<MAX_SETTING_SET; i++) {
		if(portDst>=outgoing_ports[i][0] && portDst<=outgoing_ports[i][1]) {
			arrayRowWorkOn=i;
			break;
		}
	}
	if(arrayRowWorkOn == -1) {
		DEBUGP("arrayRowWorkOn == -1 portDst=%u\n", portDst);
		return NF_ACCEPT;
	}
	if(incoming_protocols[arrayRowWorkOn] == 3) {
		for(j=0; j<(1+incoming_ports[arrayRowWorkOn][1]-incoming_ports[arrayRowWorkOn][0])*2 ; j++)  {
			exp = &expect[j];
			exp_port_trigger_info = &exp->help.exp_port_trigger_info;
			memset(exp, 0, sizeof(struct ip_conntrack_expect));
			exp_port_trigger_info->port = incoming_ports[arrayRowWorkOn][0]+j/2;
			exp->tuple = ((struct ip_conntrack_tuple)
				{ { ct->tuplehash[!dir].tuple.src.ip,
				  { 0 } },
//				{ ct->tuplehash[!dir].tuple.dst.ip,
				{ 0,
				{ htons(incoming_ports[arrayRowWorkOn][0]+j/2) },
				(j%2==0)?IPPROTO_TCP:IPPROTO_UDP }});
			exp->mask = ((struct ip_conntrack_tuple)
				{ { 0xFFFFFFFF, { 0 } },
				  { 0x0, { 0xFFFF }, 0xFFFF }});
			exp->expectfn = port_trigger_expectfn;
			LOCK_BH(&ip_port_triggering_lock);
			old = LIST_FIND(&ip_conntrack_expect_list, resent_expect,
	        	struct ip_conntrack_expect *, &exp->tuple, &exp->mask);
			if (!(old && old->expectant->tuplehash[0].tuple.src.ip != ct->tuplehash[0].tuple.src.ip)) 
				ip_conntrack_expect_related(ct, exp);
			UNLOCK_BH(&ip_port_triggering_lock);
		}
	}
	else {
		for(j=0; j<=(incoming_ports[arrayRowWorkOn][1]-incoming_ports[arrayRowWorkOn][0]) ; j++)  {
			exp = &expect[j];
			exp_port_trigger_info = &exp->help.exp_port_trigger_info;
			memset(exp, 0, sizeof(struct ip_conntrack_expect));
			exp_port_trigger_info->port = incoming_ports[arrayRowWorkOn][0]+j;
			exp->tuple = ((struct ip_conntrack_tuple)
				{ { ct->tuplehash[!dir].tuple.src.ip,
				  { 0 } },
//				{ ct->tuplehash[!dir].tuple.dst.ip,
				{ 0,
				{ htons(incoming_ports[arrayRowWorkOn][0]+j) },
				(incoming_protocols[arrayRowWorkOn]==1)?IPPROTO_TCP:IPPROTO_UDP }});
			exp->mask = ((struct ip_conntrack_tuple)
				{ { 0xFFFFFFFF, { 0 } },
				  { 0x0, { 0xFFFF }, 0xFFFF }});
			exp->expectfn = port_trigger_expectfn;
			LOCK_BH(&ip_port_triggering_lock);
			old = LIST_FIND(&ip_conntrack_expect_list, resent_expect,
	        	struct ip_conntrack_expect *, &exp->tuple, &exp->mask);
			if (!(old && old->expectant->tuplehash[0].tuple.src.ip != ct->tuplehash[0].tuple.src.ip)) 
				ip_conntrack_expect_related(ct, exp);
			UNLOCK_BH(&ip_port_triggering_lock);
		}
	}
	number_expect_working = j;

	return NF_ACCEPT;
}

/* Not __exit: called from init() */
static void fini(void)
{
	int i;
	for (i = 0; i < ports_c; i++) {
		DEBUGP("ip_ct_port_trigger: unregistering helper for port %d\n",
				ports[i]);
		ip_conntrack_helper_unregister(&port_trigger[i]);
	}
}

static int __init init(void)
{
	int i, ret, j, k;

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

	ports_c=0;
	for (i = 0; (i < MAX_PORTS) && ports[i]; i++) {
		memset(&port_trigger[i], 0, sizeof(struct ip_conntrack_helper));
		port_trigger[i].flags = IP_CT_HELPER_F_REUSE_EXPECT;
		port_trigger[i].me = ip_conntrack_port_trigger;
		port_trigger[i].max_expected = MAX_ONE_SET_EXPECTS;
		port_trigger[i].timeout = timeout;
		if(protocols[i] == IPPROTO_TCP) 
			port_trigger[i].tuple.src.u.tcp.port = htons(ports[i]);
		else
			port_trigger[i].tuple.src.u.udp.port = htons(ports[i]);
		port_trigger[i].tuple.dst.protonum = protocols[i];
		port_trigger[i].mask.src.u.udp.port = 0xFFFF;
		port_trigger[i].mask.src.u.tcp.port = 0xFFFF;
		port_trigger[i].mask.dst.protonum = 0xFFFF;
		port_trigger[i].help = conntrack_port_trigger_help;
		ret = ip_conntrack_helper_register(&port_trigger[i]);
		if (ret) {
			fini();
			printk(KERN_ERR "Unable to register conntrack application "
				"helper for port trigger: %d\n", ret);
			return -EIO;
		}
		ports_c++;
	}
	return 0;
}

#ifdef CONFIG_IP_NF_NAT_NEEDED
EXPORT_SYMBOL(ip_port_triggering_lock);
#endif

module_init(init);
module_exit(fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LIGHT C. C. LU");
