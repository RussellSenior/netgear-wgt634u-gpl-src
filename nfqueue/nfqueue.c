/* http url filter program.
 * 
 * use netfilter packet queuing to queue the packet to userspace.
 * then this program has a verdict of dropping or accepting the packet.
 * 
 * Vincent C. C. Yang , Delta Networks,Inc. Taiwan.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <libipq/libipq.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/sockios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sysexits.h>
#include <syslog.h>

#include "nfqueue.h"

char *gProgramName = PACKAGE;
char *gConfigFile = DEFAULT_CONF_FILE;
unsigned int gGoDaemon = TRUE; /* boolean */	
struct filter_s gFilter; 
unsigned int gLogEnable = TRUE;	/* boolean */

struct ipq_handle *g_qh;
unsigned char gIpqBuf[IPQ_BUF_SIZE];
unsigned char gBlockedUrl[255];

int gRawSock;
struct sockaddr_ll gToSockAddr;
unsigned char gLanHwAddr[6];

/* ---------------------------------------------------------- */

int main(int argc, char **argv)
{
	int optch;
	
	gProgramName = argv[0];
	
	while ((optch = getopt(argc, argv, "c:vdh")) != EOF) {
		switch (optch) {
		case 'v':
			display_version();
			exit(EX_OK);
		case 'c':
			gConfigFile = strdup(optarg);
			if (!gConfigFile) {
				fprintf(stderr, "%s: Could not allocate memory.\n",	gProgramName);
				exit(EX_SOFTWARE);
			}
			break;
        case 'd':
            gGoDaemon = FALSE;
            break;
									
		case 'h':
		default:
			display_usage();
			exit(EX_OK);
		}
	}
	
	checkIfAlreadyRunning();

	if (gGoDaemon == TRUE)
		makedaemon();

	openlog(gProgramName, LOG_PID|LOG_CONS, LOG_NFQUEUE);
	syslog(LOG_NOTICE, "nfqueue started.\n");

	createPidFile( getpid() );
	parse_config();
	setup_signal();
	createRawSocket();
	
	g_qh = ipq_create_handle(0,PF_INET);
	if (!g_qh) {
		ipq_die();
	}
	if (ipq_set_mode(g_qh, IPQ_COPY_PACKET, IPQ_BUF_SIZE) < 0) {
		ipq_die();
	}
	
	while (1)
	{
		ssize_t len;
		int ptype, error;
		ipq_packet_msg_t *packet;

		len = ipq_read(g_qh, gIpqBuf, IPQ_BUF_SIZE, 0);
		if (len < 0) {
			syslog(LOG_ERR, "ipq_read len < 0\n");
			//ipq_perror(NULL);
		}
		else if (len == 0) {
			syslog(LOG_ERR, "ipq_read timeout exceeded\n");
		} 
		else {
			ptype = ipq_message_type(gIpqBuf);
			//DEBUGP("received packet, length=%d, type=%d\n", len, ptype);
			
			switch (ptype) {
				case NLMSG_ERROR:
					error = -ipq_get_msgerr(gIpqBuf);
					syslog(LOG_ERR, "ipq_get_msgerr NLMSG_ERROR: %d\n", error);
					break;

				case IPQM_PACKET:
					packet = ipq_get_packet(gIpqBuf);
					handle_packet(packet);
					break;
			}
		}
	}
	
	exit_program("never here", 4);
}

/* ---------------------------------------------------------- */

void display_version(void)
{
	printf("%s %s \n", PACKAGE, VERSION);
}

void display_usage(void)
{
	printf("Usage: %s [options]\n", PACKAGE);
	printf("\
Options:\n\
  -c FILE	Use an alternate configuration file.\n\
  -h		Display this usage information.\n\
  -v        Display the version number.\n");

	/* Display the modes compiled into nfqueue */
	printf("\nFeatures Compiled In:\n");
#ifdef DEBUG
	printf("    Debugging code\n");
#endif				/* NDEBUG */
}

void makedaemon(void)
{
	if (fork() != 0)
		exit(0);

	setsid();
}

/* ---------------------------------------------------------- */

void checkIfAlreadyRunning(void)
{
	int o;

	o = open(PID_FILE, O_RDONLY);
	if ( o == -1 ) return;

	close(o);
	fprintf(stderr,"\
****  %s: already running\n\
****  %s: if not then delete %s file\n",gProgramName, gProgramName, PID_FILE);
	exit(0);
}

void createPidFile( pid_t pid )
{
	FILE *fp;
	char *pidfile;

	pidfile = PID_FILE;

	fp = fopen(pidfile,"w");
	if ( fp == NULL )
	{
		exit_program("createPidFile: fopen failed.\n", 1);
	}
	
	fprintf(fp,"%u\n", pid);
	fclose (fp);
}

void deletePidFile()
{
  unlink(PID_FILE);
}

void touchFile( char * filename )
{
	FILE *fp;
	
	fp = fopen(filename,"w");
	fclose (fp);
}

/* ---------------------------------------------------------- */

void exit_program(char * str, int status)
{	
	deletePidFile();
	syslog(LOG_ERR, "nfqueue exit for the reason : %s\n", str);
	fprintf(stderr, "nfqueue exit for the reason : %s\n", str);
	exit(status);
}

void die(char * str)
{
	ipq_destroy_handle(g_qh);
	exit_program(str, 1);
}

void ipq_die(void)
{
	char * str = ipq_errstr();
	
	ipq_perror(NULL);
	die(str);
}

void dump_metadata(ipq_packet_msg_t *m)
{
	if(!m)
		return;
	fprintf(stderr, "Dumping Metadata:\nID=0x%08lX MARK=0x%08lX TSEC=%ld "
	        "TUSEC=%ld HOOK=%u IN=%s OUT=%s DLEN=%d\n",
	        m->packet_id,
	        m->mark,
	        m->timestamp_sec,
	        m->timestamp_usec,
	        m->hook,
	        m->indev_name[0] ? m->indev_name : "[none]",
	        m->outdev_name[0] ? m->outdev_name : "[none]",
	        m->data_len);
	
	fprintf(stderr, "Dumping Metadata:\nprotocol=0x%04lX hw_type=0x%04lX "
	        "hw_addrlen=%ld hw_addr=%02x %02x %02x %02x %02x %02x\n",
	        m->hw_protocol,
	        m->hw_type,
	        m->hw_addrlen,
	        m->hw_addr[0],
	        m->hw_addr[1],
	        m->hw_addr[2],
	        m->hw_addr[3],
	        m->hw_addr[4],
	        m->hw_addr[5]);
}

void dump_iphdr(unsigned char *buf)
{
	struct iphdr *iph = (struct iphdr *)buf;

	fputs("Dumping IP Header:\n", stderr);
	fprintf(stderr, "VER=%u HLEN=%u TOS=0x%02hX TLEN=%u ID=%u ",
	        iph->version,
	        iph->ihl * 4,
	        iph->tos, 
	        ntohs(iph->tot_len),
	        ntohs(iph->id));
	if (ntohs(iph->frag_off) & IP_DF)
		fputs("DF ", stderr);
	if (ntohs(iph->frag_off) & IP_MF)
		fputs("MF ", stderr);
	if (ntohs(iph->frag_off) & 0x1FFF)
		fprintf(stderr, "FRAG=%u ",
		        (ntohs(iph->frag_off) & 0x1FFF) * 8);
	fprintf(stderr, "TTL=%u PROT=%u CSUM=0x%04X ",
	        iph->ttl,
	        iph->protocol,
	        ntohs(iph->check));
	fprintf(stderr, "SRC=%u.%u.%u.%u DST=%u.%u.%u.%u\n\n",
	        (ntohl(iph->saddr) >> 24) & 0xFF,
	        (ntohl(iph->saddr) >> 16) & 0xFF,
	        (ntohl(iph->saddr) >> 8) & 0xFF,
	        (ntohl(iph->saddr)) & 0xFF,
	        (ntohl(iph->daddr) >> 24) & 0xFF,
	        (ntohl(iph->daddr) >> 16) & 0xFF,
	        (ntohl(iph->daddr) >> 8) & 0xFF,
	        (ntohl(iph->daddr)) & 0xFF);
}

/* ---------------------------------------------------------- */

void parse_config(void)
{
	FILE *fd;
	char buf[255];
	char *s, *t;
	
	fd = fopen(gConfigFile, "r");
	
	if (fd)	{
		
		gFilter.keyword.entryCount = 0;
		gFilter.trust.enable = FALSE;
		gLogEnable = TRUE;
		
		while (fgets(buf, 255, fd)) {
			s = buf;

			/* strip trailing whitespace & comments */
			t = s;
			while (*s && *s != '#') {
				if (!isspace((unsigned char)*(s++)))
					t = s;
			}
			*t = '\0';

			/* skip leading whitespace */
			s = buf;
			while (*s && isspace((unsigned char)*s))
				s++;

			/* skip blank lines and comments */
			if (*s == '\0')
				continue;

			if ( strstr(s, "keyword=") ) {
				s += (sizeof("keyword=") - 1);
				strcpy(gFilter.keyword.entry[gFilter.keyword.entryCount], s);
				gFilter.keyword.entryCount++;
			} 
			else if ( strstr(s, "trustEnable=yes") )
				gFilter.trust.enable = TRUE;
			else if ( strstr(s, "trustIP=") ) {
				s += (sizeof("trustIP=") - 1);
				inet_aton( s, (struct in_addr *) &(gFilter.trust.trustIP) );
			}
			else if ( strstr(s, "logEnable=no") )
				gLogEnable = FALSE;
		}

		if ( ferror(fd) ) {
			perror("fgets");
			exit_program("fgets error", 1);
		}
		fclose(fd);
	}
}

/* ---------------------------------------------------------- */

void setup_signal(void)
{
	int i;
	struct sigaction action;

	sigaction(SIGHUP, NULL, &action);
	action.sa_handler = &sig_handler;
	action.sa_flags = 0;

	for (i=1; i<16; i++) sigaction(i, &action, NULL);
}

void sig_handler(int sig)
{
	switch (sig) {
		case SIGHUP :
			parse_config();
			syslog(LOG_NOTICE, "Re-reading config file.\n");
			break;
			
		case SIGTERM :
			die("Stopped by SIGTERM.\n");
			break;
	}

	return;
}

/* ---------------------------------------------------------- */

void createRawSocket()
{
	struct ifreq ifr;

	//if ((gRawSock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
	if ((gRawSock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
		exit_program("Can't create raw socket.", 1);
	DEBUGP("gRawSock=%d\n", gRawSock);

	gToSockAddr.sll_ifindex = getDevIndex(gRawSock, LAN_DEV);
	DEBUGP("gToSockAddr.sll_ifindex=%d\n", gToSockAddr.sll_ifindex);

	getDevHwAddr(gRawSock, LAN_DEV, gLanHwAddr);
	DEBUGP("gLanHwAddr=%02x %02x %02x %02x %02x %02x\n", 
			gLanHwAddr[0], gLanHwAddr[1], gLanHwAddr[2],
			gLanHwAddr[3], gLanHwAddr[4], gLanHwAddr[5] );
}

int getDevIndex(int sock, char *ifname)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if(ioctl(sock, SIOCGIFINDEX, &ifr) == -1) 
		exit_program("fail in SIOCGIFINDEX", 1);
	return ifr.ifr_ifindex;
}

void getDevHwAddr(int sock, char *ifname, unsigned char * hwaddr)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1)
		exit_program("fail in SIOCGIFHWADDR", 1);
	memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
}

/* ---------------------------------------------------------- */

void handle_packet(ipq_packet_msg_t *packet)
{
	ipq_packet_msg_t *qpkth;
	int verdict, ret;

	qpkth = packet;
	//dump_metadata(qpkth);

	if (qpkth->hook == NF_IP_PRE_ROUTING)
		verdict = queue_filter(qpkth->payload, qpkth->data_len);
	else 
		verdict = NF_ACCEPT;

	/* in this program we let verdict == NF_QUEUE means "been blocked". */
	if ( verdict == NF_QUEUE ) {
		/* send LAN side response & close it (RST flag) */
		send_lan_response(gRawSock, &gToSockAddr, gLanHwAddr, qpkth);

		/* close WAN side connections (RST flag) */
		qpkth->data_len = modify_packet_rst(qpkth->payload);
		ret = ipq_set_verdict(g_qh, qpkth->packet_id, NF_ACCEPT, qpkth->data_len, qpkth->payload);
	}
	else /* verdict == NF_ACCEPT, NF_DROP */
		ret = ipq_set_verdict(g_qh, qpkth->packet_id, verdict, 0, NULL);
		
	if (ret < 0) {
		ipq_die();
	}
}

int queue_filter(unsigned char *ip_pkt, size_t buf_len)
{
    unsigned int ip_len, iphdr_len, tcphdr_len, tcpdata_len;
    struct iphdr * ip = (struct iphdr *)(ip_pkt);
    struct tcphdr *tp;
	unsigned short dport;
    unsigned char *dp;
	unsigned int sourceIP;
	int verdict;

	ip_len = ntohs(ip->tot_len);
	if (ip_len != (unsigned int) buf_len)
		return (NF_DROP);
	
    iphdr_len = (ip->ihl) << 2;								/* quadword to byte format */
    tp = (struct tcphdr *)((unsigned char *)ip + iphdr_len);	/* tcp frame pointer */
    tcphdr_len = (tp->doff) << 2;							/* quadword to byte format */
    dp = (unsigned char *)tp + tcphdr_len;					/* point to data port */
    tcpdata_len = ip_len - iphdr_len - tcphdr_len;

	sourceIP = ip->saddr;
	if ( gFilter.trust.enable && (sourceIP == gFilter.trust.trustIP) )
        return ( NF_ACCEPT );

	dport = ntohs(tp->dest);
	switch (dport)
	{
		case 80 :
			verdict = http_filter(dp, tcpdata_len);
			break;
		default :
			return (NF_ACCEPT);
	}
	
	if ( (gLogEnable == TRUE) && (verdict == NF_QUEUE) ) {
		syslog( LOG_INFO, "[Blocked] \"%s\" want to access the blocked URL \"%s\"\n",
				inet_ntoa(*((struct in_addr*)&sourceIP)), gBlockedUrl);
		touchFile(BLOCK_HAPPEN_FILE);
	}

	return verdict;
}

int http_filter(unsigned char *dp, unsigned int tcpdata_len)
{
    unsigned char *sPtr, *ePtr, *eop;
    int need_parse = FALSE, endOfHeader = FALSE;
	
    switch (*dp)
    {
        case 'O':
        	if (memcmp(dp, "OPTIONS", strlen("OPTIONS")) == 0)
                need_parse=TRUE;
            break;
        case 'G':
        	if (memcmp(dp, "GET", strlen("GET")) == 0)
                need_parse=TRUE;
            break;
        case 'H':
            if (memcmp(dp, "HEAD", strlen("HEAD")) == 0)
                need_parse=TRUE;
            break;
        case 'P':
            if (memcmp(dp, "POST", strlen("POST")) == 0)
                need_parse=TRUE;
            else if (memcmp(dp, "PUT", strlen("PUT")) == 0)
                need_parse=TRUE;
            break;
        case 'D':
            if (memcmp(dp, "DELETE", strlen("DELETE")) == 0)
                need_parse=TRUE;
            break;
        case 'T':
            if (memcmp(dp, "TRACE", strlen("TRACE")) == 0)
                need_parse=TRUE;
            break;
    }
    
    if (need_parse == FALSE)
        return ( NF_ACCEPT );
	
    sPtr = ePtr = dp;	
    eop = dp + tcpdata_len;

    while ( ! endOfHeader )
    {
        if ( (*ePtr == 0x0D) && (*(ePtr+1) == 0x0A) )
        {
            if ( (*(ePtr+2) == 0x0D) && (*(ePtr+3) == 0x0A) )
                endOfHeader = TRUE;           //end of header found
			
			if ( (*sPtr == 'H') && (memcmp(sPtr, "Host:", strlen("Host:")) == 0) )
            {
                sPtr += 6;           //skip header command "Host: "
                *ePtr = 0x00;        //temporary make NULL terminate
				if ( is_block(sPtr) ) return (NF_QUEUE);
                *ePtr = 0x0D;        //restore to origin packet
            }
            else if ( (*sPtr == 'G') && (memcmp(sPtr, "GET ", strlen("GET ")) == 0) )
            {
                sPtr += 5;           //skip header command "GET /"
                *ePtr = 0x00;        //temporary make NULL terminate
				if ( is_block(sPtr) ) return (NF_QUEUE);
                *ePtr = 0x0D;        //restore to origin packet
            }
            sPtr = ePtr = ePtr + 2;       //skip CRLF
        }
		
        if (++ePtr >= eop)
            endOfHeader = TRUE;                   //end of packet found
    }
    
    return ( NF_ACCEPT );
}

int is_block (unsigned char *sPtr)
{
	int i;
		
	for ( i = 0; i < gFilter.keyword.entryCount; i++ ) 
	{
		if ( ( strlen(gFilter.keyword.entry[i]) != 0) &&
		     ( strstr(sPtr, gFilter.keyword.entry[i]) != NULL) )
		{
			if (gLogEnable == TRUE) {
				memccpy( gBlockedUrl, sPtr, 0, sizeof(gBlockedUrl)-1 );
			}
			return TRUE;
		}
	}

	return FALSE;
}

/* ---------------------------------------------------------- */

unsigned char gOutBuf[IPQ_BUF_SIZE];
void send_lan_response(int sock, struct sockaddr_ll *to , unsigned char *src_hw_addr, ipq_packet_msg_t *qpkth)
{
	unsigned char *out_payload = ( (unsigned char *) (gOutBuf + ETH_ALEN*2 + 2) );
	unsigned char *ip_pkt = qpkth->payload;
	unsigned char *des_hw_addr = qpkth->hw_addr;
	unsigned int ip_pkt_len = qpkth->data_len;
	unsigned int et_pkt_len;
	
	memcpy(gOutBuf, des_hw_addr, ETH_ALEN);
	memcpy(gOutBuf+ETH_ALEN, src_hw_addr, ETH_ALEN);
	*((unsigned short *)(gOutBuf+ETH_ALEN*2)) = htons(ETH_P_IP);

	memcpy(out_payload, ip_pkt, ip_pkt_len);

	/* relocation http response */
	ip_pkt_len = build_lan_http_response( out_payload, (IPQ_BUF_SIZE - ETH_ALEN*2 - 2) );
	DEBUGP("ip_pkt_len=%d\n", ip_pkt_len);
	et_pkt_len = ip_pkt_len + ETH_ALEN*2 + 2;
	send_packet(sock, to, gOutBuf, et_pkt_len);

	ip_pkt_len = build_lan_rst(out_payload);
	DEBUGP("ip_pkt_len=%d\n", ip_pkt_len);
	et_pkt_len = ip_pkt_len + ETH_ALEN*2 + 2;
	send_packet(sock, to, gOutBuf, et_pkt_len);
}

void send_packet(int sock, struct sockaddr_ll *to, unsigned char *buf, int len)
{
	if ( sendto( sock, buf, len, 0, (struct sockaddr *)to, sizeof(*to) ) < 0 ) {
		die("fail in sendto");
	}
}

int build_lan_http_response_tcpdata(unsigned char *dp, int maxlen)
{
	char * default_http_response_page =
					"HTTP/1.0 200 OK\n"
					"Content-type: text/html\n"
					"\n"
					"Blocked by DNI router.\n";
    FILE* r_file;
    char c;
    int len;

	r_file = fopen(HPPT_RESPONSE_PAGE, "r");
	if( r_file != NULL ) {
		len = 0;
		while ( ( (c = fgetc(r_file)) != EOF ) && ( len < maxlen ) ) 
		{
			*(dp+len) = c;
			len++;
		}
		fclose(r_file);
	}
	else {
		len = strlen(default_http_response_page);
		memcpy(dp, default_http_response_page, len);
	}

    return len;
}

int build_lan_http_response(unsigned char *ip_pkt, int maxlen)
{
    unsigned int ip_len, iphdr_len, tcphdr_len, tcpdata_len, new_tcpdata_len;
    struct iphdr * ip = (struct iphdr *)(ip_pkt);
    struct tcphdr *tp;
	unsigned short sport, dport;
    unsigned char *dp;
	unsigned int sourceIP, destIP;
	unsigned int seq, ack;
	tcp_PseudoHeader ph;
    unsigned int max_tcpdata_len;

	ip_len = ntohs(ip->tot_len);
    iphdr_len = (ip->ihl) << 2;								/* quadword to byte format */
    tp = (struct tcphdr *)((unsigned char *)ip + iphdr_len);	/* tcp frame pointer */
    tcphdr_len = (tp->doff) << 2;							/* quadword to byte format */
    dp = (unsigned char *)tp + tcphdr_len;					/* point to data port */
    tcpdata_len = ip_len - iphdr_len - tcphdr_len;
	max_tcpdata_len = maxlen - iphdr_len - tcphdr_len;

	/* checnge ip header source/dest ip */
	sourceIP = ip->saddr;
	destIP = ip->daddr;
	ip->saddr = destIP;
	ip->daddr = sourceIP;
	
	/* change tcp header source/dest port */
    sport = tp->source;
	dport = tp->dest;
	tp->source = dport;
	tp->dest = sport;

	/* adjust seq and ack */
	seq = tp->seq;
	ack = tp->ack_seq;
	tp->seq = ack;
	tp->ack_seq = htonl((ntohl(seq) + tcpdata_len));

	/* http response */
	new_tcpdata_len = build_lan_http_response_tcpdata(dp, max_tcpdata_len);
	DEBUGP("new_tcpdata_len=%d\n", new_tcpdata_len);
	ip->tot_len = htons(iphdr_len + tcphdr_len + new_tcpdata_len);

	/* ip checksum */
	ip->check = 0;
	ip->check = ~csum_partial(ip_pkt, sizeof(struct iphdr), 0);

	/* tcp checksum */
	ph.src = ip->saddr;
	ph.dst = ip->daddr;
	ph.mbz = 0;
	ph.protocol = 0x06;		/* TCP */
	ph.length = htons(tcphdr_len + new_tcpdata_len);
	
	tp->check = 0;
	ph.checksum = csum_partial((char *)tp, (tcphdr_len + new_tcpdata_len), 0);
    tp->check = ~csum_partial((char *)&ph, TCP_PHEADER_LEN, 0);	
	
	return ntohs(ip->tot_len);
}	
	
int build_lan_rst(char *ip_pkt)
{
    unsigned int ip_len, iphdr_len, tcphdr_len, tcpdata_len, new_tcpdata_len;
    struct iphdr * ip = (struct iphdr *)(ip_pkt);
    struct tcphdr *tp;
	unsigned short sport, dport;
    unsigned char *dp;
	unsigned int sourceIP, destIP;
	unsigned int seq, ack;
	tcp_PseudoHeader ph;

	ip_len = ntohs(ip->tot_len);
    iphdr_len = (ip->ihl) << 2;								/* quadword to byte format */
    tp = (struct tcphdr *)((unsigned char *)ip + iphdr_len);	/* tcp frame pointer */
    tcphdr_len = (tp->doff) << 2;							/* quadword to byte format */
    dp = (unsigned char *)tp + tcphdr_len;					/* point to data port */
    tcpdata_len = ip_len - iphdr_len - tcphdr_len;

	/* already changed by previous build (build_lan_http_response) */
#if 0	
	/* checnge ip header source/dest ip */
	sourceIP = ip->saddr;
	destIP = ip->daddr;
	ip->saddr = destIP;
	ip->daddr = sourceIP;
	
	/* change tcp header source/dest port */
    sport = tp->source;
	dport = tp->dest;
	tp->source = dport;
	tp->dest = sport;
#endif

	/* send tcp FIN and ACK to br0 interface through raw socket */
	tp->fin = 1;
	tp->ack = 1;

	/* adjust seq and ack */
	seq = tp->seq;
	tp->seq = htonl((ntohl(seq) + tcpdata_len));

	/* tcpdata len always zero */
	new_tcpdata_len = 0;
	ip->tot_len = htons(iphdr_len + tcphdr_len + new_tcpdata_len);
	
	/* ipheader checksum */
	ip->check = 0;
	ip->check = ~csum_partial(ip_pkt, sizeof(struct iphdr), 0);

	/* tcp checksum */
	ph.src = ip->saddr;
	ph.dst = ip->daddr;
	ph.mbz = 0;
	ph.protocol = 0x06;		/* TCP */
	ph.length = htons(tcphdr_len + new_tcpdata_len);
	
	tp->check = 0;
	ph.checksum = csum_partial((char *)tp, (tcphdr_len + new_tcpdata_len), 0);
	tp->check = ~csum_partial((char *)&ph, TCP_PHEADER_LEN, 0); 
	
	return ntohs(ip->tot_len);
}	

int modify_packet_rst(char *ip_pkt)
{
    unsigned int ip_len, iphdr_len, tcphdr_len, tcpdata_len;
    struct iphdr * ip = (struct iphdr *)(ip_pkt);
    struct tcphdr *tp;
	tcp_PseudoHeader ph;

	ip_len = ntohs(ip->tot_len);
    iphdr_len = (ip->ihl) << 2;								/* quadword to byte format */
    tp = (struct tcphdr *)((unsigned char *)ip + iphdr_len);	/* tcp frame pointer */
    tcphdr_len = (tp->doff) << 2;							/* quadword to byte format */
    tcpdata_len = ip_len - iphdr_len - tcphdr_len;

	/* force send tcp RST to original connection */
	tp->rst = 1;
	tp->ack = 0;

	
	/* tcpdata_len always zero */
	tcpdata_len = 0;
	ip->tot_len = htons(iphdr_len + tcphdr_len + tcpdata_len);

	/* ip checksum */
	ip->check = 0;
	ip->check = ~csum_partial(ip_pkt, sizeof(struct iphdr), 0);

	/* tcp checksum */
	ph.src = ip->saddr;
	ph.dst = ip->daddr;
	ph.mbz = 0;
	ph.protocol = 0x06;		/* TCP */
	ph.length = htons(tcphdr_len + tcpdata_len);
	
	tp->check = 0;
	ph.checksum = csum_partial((char *)tp, (tcphdr_len + tcpdata_len), 0);
	tp->check = ~csum_partial((char *)&ph, TCP_PHEADER_LEN, 0); 

	return ntohs(ip->tot_len);
}	

u_int16_t csum_partial(void *buffer, unsigned int len, u_int16_t prevsum)
{
	u_int32_t sum = 0;
	u_int16_t *ptr = buffer;
	
	while (len > 1)  {
		sum += *ptr++;
		len -= 2;
	}
	if (len) {
		union {
			u_int8_t byte;
			u_int16_t wyde;
		} odd;
	
		odd.wyde = 0;
		odd.byte = *((u_int8_t *)ptr);
		sum += odd.wyde;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += prevsum;
	return (sum + (sum >> 16));
}

/* ---------------------------------------------------------- */

