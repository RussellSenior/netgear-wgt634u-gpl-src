#ifndef _IPT_QUOTA_H
#define _IPT_QUOTA_H

/* print debug info in both kernel/netfilter module & iptable library */
//#define DEBUG_IPT_QUOTA

struct ipt_quota_info {
        u_int64_t quota;
};

#endif /*_IPT_QUOTA_H*/
