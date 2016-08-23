#ifndef _NFQUEUE_H
#define _NFQUEUE_H

//#define DEBUG
#ifdef DEBUG
#define DEBUGP(format, args...) 	fprintf(stderr, format, ## args)
#else
#define DEBUGP(format, args...)
#endif

#define PACKAGE		"nfqueue"
#define VERSION		"0.9"
#define DEFAULT_CONF_FILE "/etc/nfqueue/nfqueue.conf"
#define PID_FILE	"/var/run/"PACKAGE".pid"
#define LAN_DEV	"br0"
#define LOG_NFQUEUE	LOG_USER
#define BLOCK_HAPPEN_FILE "/var/nfqueue_block_happen"
#define HPPT_RESPONSE_PAGE "/etc/nfqueue/http_response_page"

#define IPQ_BUF_SIZE 2048

#define TRUE	1
#define FALSE	0

#define MAX_ENTRY_NUM	255
#define STRING_LENGTH	128

struct filter_s
{
	struct keyword_s
	{
		unsigned char entryCount;							/* keyword entry count */
		char entry[MAX_ENTRY_NUM][STRING_LENGTH];   /* keyword entry */
	} keyword;

	struct trust_s
	{
	    unsigned char  enable;
		unsigned int  trustIP;
	}trust;
			
};

#define TCP_PHEADER_LEN 	14
typedef struct {
	unsigned int src;
	unsigned int dst;
	unsigned char mbz;
	unsigned char protocol;
	unsigned short length;
	unsigned short checksum;
} __attribute__ ((__packed__)) tcp_PseudoHeader;

/* ---------------------------------------------------------- */

extern char *gProgramName; 
extern char *gConfigFile;
extern unsigned int gGoDaemon; /* boolean */	
extern struct filter_s gFilter; 
extern unsigned int gLogEnable;	/* boolean */

extern struct ipq_handle *g_qh;
extern unsigned char gIpqBuf[IPQ_BUF_SIZE];
extern unsigned char gBlockedUrl[255];

extern int gRawSock;
extern struct sockaddr_ll gToSockAddr;
extern unsigned char gLanHwAddr[6];

extern unsigned char gOutBuf[IPQ_BUF_SIZE];

/* ---------------------------------------------------------- */

void display_version(void);
void display_usage(void);
void makedaemon(void);
void checkIfAlreadyRunning(void);
void createPidFile( pid_t pid );
void deletePidFile();
void touchFile( char * filename );

/* ---------------------------------------------------------- */

void exit_program(char * str, int status);
void die(char * str);
void ipq_die(void);
void dump_metadata(ipq_packet_msg_t *m);
void dump_iphdr(unsigned char *buf);

/* ---------------------------------------------------------- */

void parse_config(void);

/* ---------------------------------------------------------- */

void setup_signal(void);
void sig_handler(int sig);

/* ---------------------------------------------------------- */

void createRawSocket();
int getDevIndex(int sock, char *ifname);
void getDevHwAddr(int sock, char *ifname, unsigned char * hwaddr);

/* ---------------------------------------------------------- */

void handle_packet(ipq_packet_msg_t *packet);
int queue_filter(unsigned char *ip_pkt, size_t buf_len);
int http_filter(unsigned char *dp, unsigned int tcpdata_len);
int is_block (unsigned char *sPtr);

/* ---------------------------------------------------------- */

void send_lan_response(int sock, struct sockaddr_ll *to , unsigned char *src_hw_addr, ipq_packet_msg_t *qpkth);
void send_packet(int sock, struct sockaddr_ll * to, unsigned char *buf, int len);
int build_lan_http_response_tcpdata(unsigned char *dp, int maxlen);
int build_lan_http_response(unsigned char *ip_pkt, int maxlen);
int build_lan_rst(char *ip_pkt);
int modify_packet_rst(char *ip_pkt);
u_int16_t csum_partial(void *buffer, unsigned int len, u_int16_t prevsum);

#endif
