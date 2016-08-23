/* Shared library add-on to iptables to add stealth support.
 * Copyright (C) 2002 Brad Spengler  <spender@grsecurity.net>
 * This netfilter module is licensed under the GNU GPL.
 */

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <getopt.h>
#include <iptables.h>

/* Function which prints out usage message. */
static void
help(void)
{
	printf("stealth v%s takes no options\n\n", IPTABLES_VERSION);
}

static struct option opts[] = {
	{0}
};

/* Initialize the match. */
static void
init(struct ipt_entry_match *m, unsigned int *nfcache)
{
	*nfcache |= NFC_UNKNOWN;
}

static int
parse(int c, char **argv, int invert, unsigned int *flags,
	const struct ipt_entry *entry,
	unsigned int *nfcache,
	struct ipt_entry_match **match)
{
	return 0;
}

static void
final_check(unsigned int flags)
{
	return;
}

static
struct iptables_match stealth = {
	NULL,
	"stealth",
	IPTABLES_VERSION,
	IPT_ALIGN(0),
	IPT_ALIGN(0),
	&help,
	&init, 
	&parse,
	&final_check,
	NULL,
	NULL,
	opts  
};

void _init(void)
{
	register_match(&stealth);
}
