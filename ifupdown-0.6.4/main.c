#line 2827 "ifupdown.nw"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "header.h"
#line 2866 "ifupdown.nw"
#include <unistd.h>
#include <fcntl.h>
#line 2958 "ifupdown.nw"
#include <getopt.h>
#line 3208 "ifupdown.nw"
#include <fnmatch.h>
#line 2985 "ifupdown.nw"
int no_act = 0;
int verbose = 0;
#line 3001 "ifupdown.nw"
static void usage(char *execname);
static void help(char *execname);
static void version(char *execname);
#line 3348 "ifupdown.nw"
static int lookfor_iface(char **ifaces, int n_ifaces, char *iface);
#line 3366 "ifupdown.nw"
static void add_to_state(char ***ifaces, int *n_ifaces, int *max_ifaces, 
                         char *new_iface);
#line 3391 "ifupdown.nw"
static int lock_fd (int fd);
#line 3007 "ifupdown.nw"
static void usage(char *execname) {
	fprintf(stderr, "%s: Use --help for help\n", execname);
	exit(1);
}
#line 3014 "ifupdown.nw"
static void version(char *execname) {
	printf("%s version " IFUPDOWN_VERSION "\n", execname);
	printf("Copyright (c) 1999,2000 Anthony Towns\n\n");
	printf(

"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or (at\n"
"your option) any later version.\n"

	);
	exit(0);
}
#line 3030 "ifupdown.nw"
static void help(char *execname) {
	printf("Usage: %s <options> <ifaces...>\n\n", execname);
	printf("Options:\n");
	printf("\t-h, --help\t\tthis help\n");
	printf("\t-V, --version\t\tcopyright and version information\n");
	printf("\t-a, --all\t\tde/configure all interfaces automatically\n");
	printf("\t-i, --interfaces FILE\tuse FILE for interface definitions\n");
	printf("\t-n, --no-act\t\tprint out what would happen, but don't do it\n");
	printf("\t\t\t\t(note that this option doesn't disable mappings)\n");
	printf("\t-v, --verbose\t\tprint out what would happen before doing it\n");
	printf("\t--no-mappings\t\tdon't run any mappings\n");
	printf("\t--force\t\t\tforce de/configuration\n");
	exit(0);
}
#line 3352 "ifupdown.nw"
static int lookfor_iface(char **ifaces, int n_ifaces, char *iface) {
	int i;
	for (i = 0; i < n_ifaces; i++) {
		if (strncmp(iface, ifaces[i], strlen(iface)) == 0) {
			if (ifaces[i][strlen(iface)] == '=') {
				return i;
			}
		}
	}
	return -1;
}
#line 3371 "ifupdown.nw"
static void add_to_state(char ***ifaces, int *n_ifaces, int *max_ifaces, 
                         char *new_iface)
{
	assert(*max_ifaces >= *n_ifaces);
	if (*max_ifaces == *n_ifaces) {
		*max_ifaces = (*max_ifaces * 2) + 1;
		*ifaces = realloc(*ifaces, sizeof(**ifaces) * *max_ifaces);
		if (*ifaces == NULL) {
			perror("realloc");
			exit(1);
		}
	}

	(*ifaces)[(*n_ifaces)++] = new_iface;
}
#line 3395 "ifupdown.nw"
static int lock_fd (int fd) {
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if  (fcntl(fd, F_SETLKW, &lock) < 0) {
		return -1;
	}

	return 0;
}
#line 2844 "ifupdown.nw"
int main(int argc, char **argv) {
	
#line 2902 "ifupdown.nw"
int (*cmds)(interface_defn *) = NULL;
#line 2962 "ifupdown.nw"
struct option long_opts[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'V'},
	{"verbose",     no_argument,       NULL, 'v'},
	{"all",         no_argument,       NULL, 'a'},
	{"interfaces",  required_argument, NULL, 'i'},
	{"no-act",      no_argument,       NULL, 'n'},
	{"no-mappings", no_argument,       NULL,  1 },
	{"force",       no_argument,       NULL,  2 },
	{0,0,0,0}
};
#line 2990 "ifupdown.nw"
int do_all = 0;
int run_mappings = 1;
int force = 0;
char *interfaces = "/etc/network/interfaces";
char *statefile = "/etc/network/ifstate";
#line 3144 "ifupdown.nw"
interfaces_file *defn;
#line 3283 "ifupdown.nw"
int n_target_ifaces;
char **target_iface;
#line 3339 "ifupdown.nw"
char **state = NULL; /* list of iface=liface */
int n_state = 0;
int max_state = 0;
#line 3413 "ifupdown.nw"
static FILE *state_fp = NULL;

#line 2847 "ifupdown.nw"
	
#line 2871 "ifupdown.nw"
{
	int i;
	for (i = 0; i <= 2; i++) {
		if (fcntl(i, F_GETFD) == -1) {
			if (errno == EBADF && open("/dev/null", 0) == -1) {
				fprintf(stderr,
					"%s: fd %d not available; aborting\n",
					argv[0], i);
				exit(2);
			} else if (errno == EBADF) {
				errno = 0; /* no more problems */
			} else {
				/* some other problem -- eeek */
				perror(argv[0]);
				exit(2);
			}
		}
	}
}

#line 2849 "ifupdown.nw"
	
#line 2908 "ifupdown.nw"
{
	char *command;

	
#line 2919 "ifupdown.nw"
if ((command = strrchr(argv[0],'/'))) {
	command++; /* first char after / */
} else {
	command = argv[0]; /* no /'s in argv[0] */
}
#line 2912 "ifupdown.nw"
	
#line 2927 "ifupdown.nw"
if (strcmp(command, "ifup")==0) {
	cmds = iface_up;
} else if (strcmp(command, "ifdown")==0) {
	cmds = iface_down;
} else {
	fprintf(stderr,"This command should be called as ifup or ifdown\n");
	exit(1);
}
#line 2913 "ifupdown.nw"
}
#line 2850 "ifupdown.nw"
	
#line 3050 "ifupdown.nw"
for(;;) {
	int c;
	c = getopt_long(argc, argv, "s:i:hVvna", long_opts, NULL);
	if (c == EOF) break;

	switch(c) {
		
#line 3068 "ifupdown.nw"
case 'i':
	interfaces = strdup(optarg);
	break;
#line 3073 "ifupdown.nw"
case 'v':
	verbose = 1;
	break;
#line 3078 "ifupdown.nw"
case 'a':
	do_all = 1;
	break;
#line 3083 "ifupdown.nw"
case 'n':
	no_act = 1;
	break;
#line 3088 "ifupdown.nw"
case 1:
	run_mappings = 0;
	break;
#line 3093 "ifupdown.nw"
case 2:
	force = 1;
	break;
#line 3101 "ifupdown.nw"
case 'h':
	help(argv[0]);
	break;
#line 3106 "ifupdown.nw"
case 'V':
	version(argv[0]);
	break;
#line 3115 "ifupdown.nw"
default:
	usage(argv[0]);
	break;
#line 3057 "ifupdown.nw"
	}
}

#line 3127 "ifupdown.nw"
if (argc - optind > 0 && do_all) {
	usage(argv[0]);
}
#line 3133 "ifupdown.nw"
if (argc - optind == 0 && !do_all) {
	usage(argv[0]);
}

#line 2852 "ifupdown.nw"
	
#line 3148 "ifupdown.nw"
defn = read_interfaces(interfaces);
if ( !defn ) {
	fprintf(stderr, "%s: couldn't read interfaces file \"%s\"\n",
		argv[0], interfaces);
	exit(1);
}

#line 2854 "ifupdown.nw"
	
#line 3417 "ifupdown.nw"
{
	state_fp = fopen(statefile, no_act ? "r" : "a+");
	if (state_fp == NULL && !no_act) {
		fprintf(stderr, 
			"%s: failed to open statefile %s: %s\n",
			argv[0], statefile, strerror(errno));
		exit (1);
	}

	if (state_fp != NULL) {
		char buf[80];
		char *p;

		if (!no_act) {
			int flags;

			if ((flags = fcntl(fileno(state_fp), F_GETFD)) < 0
			    || fcntl(fileno(state_fp), F_SETFD, flags | FD_CLOEXEC) < 0) {
				fprintf(stderr, 
					"%s: failed to set FD_CLOEXEC on statefile %s: %s\n",
					argv[0], statefile, strerror(errno));
				exit(1);
			}

			if (lock_fd (fileno(state_fp)) < 0) {
				fprintf(stderr, 
					"%s: failed to lock statefile %s: %s\n",
					argv[0], statefile, strerror(errno));
				exit(1);
			}

		}

		rewind (state_fp);
		while((p = fgets(buf, sizeof buf, state_fp)) != NULL) {
			char *pch;

			pch = buf + strlen(buf) - 1;
			while(pch > buf && isspace(*pch)) pch--;
			*(pch+1) = '\0';

			pch = buf;
			while(isspace(*pch)) pch++;

			add_to_state(&state, &n_state, &max_state, strdup(pch));
		}
	}
}
#line 3294 "ifupdown.nw"
if (do_all) {
	if (
#line 2942 "ifupdown.nw"
(cmds == iface_up)
#line 3295 "ifupdown.nw"
                                     ) {
		target_iface = defn->autointerfaces;
		n_target_ifaces = defn->n_autointerfaces;
	} else if (
#line 2946 "ifupdown.nw"
(cmds == iface_down)
#line 3298 "ifupdown.nw"
                                            ) {
		target_iface = state;
		n_target_ifaces = n_state;
	} else {
		assert(0);
	}	
} else {
	target_iface = argv + optind;
	n_target_ifaces = argc - optind;
}
#line 3163 "ifupdown.nw"
{
	int i;
	for (
#line 3288 "ifupdown.nw"
i = 0; i < n_target_ifaces; i++
#line 3165 "ifupdown.nw"
                                      ) {
		char iface[80], liface[80];

		
#line 3311 "ifupdown.nw"
strncpy(iface, target_iface[i], sizeof(iface));
iface[sizeof(iface)-1] = '\0';

{
	char *pch;
	if ((pch = strchr(iface, '='))) {
		*pch = '\0';
		strncpy(liface, pch+1, sizeof(liface));
		liface[sizeof(liface)-1] = '\0';
	} else {
		strncpy(liface, iface, sizeof(liface));
		liface[sizeof(liface)-1] = '\0';
	}
}
#line 3169 "ifupdown.nw"
		if (!force) {
			
#line 3500 "ifupdown.nw"
{
	int already_up = lookfor_iface(state, n_state, iface);;

	if (
#line 2942 "ifupdown.nw"
(cmds == iface_up)
#line 3503 "ifupdown.nw"
                                     ) {
		if (already_up != -1) {
			fprintf(stderr, 
				"%s: interface %s already configured\n",
				argv[0], iface);
			continue;
		}
	} else if (
#line 2946 "ifupdown.nw"
(cmds == iface_down)
#line 3510 "ifupdown.nw"
                                            ) {
		if (already_up == -1) {
			fprintf(stderr, "%s: interface %s not configured\n",
				argv[0], iface);
			continue;
		}
		strncpy(liface, strchr(state[already_up], '=') + 1, 80);
		liface[79] = 0;
	} else {
		assert(0);
	}
}
#line 3171 "ifupdown.nw"
		}

		if (
#line 2942 "ifupdown.nw"
(cmds == iface_up)
#line 3173 "ifupdown.nw"
                                       && run_mappings) {
			
#line 3191 "ifupdown.nw"
{
	mapping_defn *currmap;
	for (currmap = defn->mappings; currmap; currmap = currmap->next) {
		int i;
		for (i = 0; i < currmap->n_matches; i++) {
			
#line 3212 "ifupdown.nw"
if (fnmatch(currmap->match[i], liface, 0) != 0)
	continue;
#line 3197 "ifupdown.nw"
			
#line 3220 "ifupdown.nw"
if (verbose) {
	fprintf(stderr, "Running mapping script %s on %s\n",
		currmap->script, liface);
}
run_mapping(iface, liface, sizeof(liface), currmap);
#line 3198 "ifupdown.nw"
			break;
		}
	}
}
#line 3175 "ifupdown.nw"
		}

		
#line 3230 "ifupdown.nw"
{
	interface_defn *currif;
	int okay = 0;
	for (currif = defn->ifaces; currif; currif = currif->next) {
		if (strcmp(liface, currif->iface) == 0) {
			okay = 1;
			
#line 3250 "ifupdown.nw"
{
	char *oldiface = currif->iface;
	currif->iface = iface;

	if (verbose) {
		fprintf(stderr, "Configuring interface %s=%s (%s)\n", 
			iface, liface, currif->address_family->name);
	}

	switch(cmds(currif)) {
	    case -1:
		printf("Don't seem to be have all the variables for %s/%s.\n", 
			liface, currif->address_family->name);
		break;
	    case 0:
		/* this wasn't entirely successful, should it be added to
		 *      the state file?
		 */
	    case 1:
		/* successful */
	}
	currif->iface = oldiface;
}
#line 3237 "ifupdown.nw"
		}
	}

	if (!okay && !force) {
		fprintf(stderr, "Ignoring unknown interface %s=%s.\n", 
			iface, liface);
	} else {
		
#line 3528 "ifupdown.nw"
{
	int already_up = lookfor_iface(state, n_state, iface);

	if (
#line 2942 "ifupdown.nw"
(cmds == iface_up)
#line 3531 "ifupdown.nw"
                                     ) {
		char *newiface = 
			malloc(strlen(iface) + 1 + strlen(liface) + 1);
		sprintf(newiface, "%s=%s", iface, liface);

		if (already_up == -1) {
			add_to_state(&state, &n_state, &max_state, newiface);
		} else {
			free(state[already_up]);
			state[already_up] = newiface;
		}
	} else if (
#line 2946 "ifupdown.nw"
(cmds == iface_down)
#line 3542 "ifupdown.nw"
                                            ) {
		if (already_up != -1) {
			state[already_up] = state[--n_state];
		}
	} else {
		assert(0);
	}
}
#line 3245 "ifupdown.nw"
	}
}
#line 3178 "ifupdown.nw"
		
#line 3476 "ifupdown.nw"
if (state_fp != NULL && !no_act) {
	int i;

	if (ftruncate(fileno(state_fp), 0) < 0)
	{
		fprintf(stderr, 
			"%s: failed to truncate statefile %s: %s\n",
			argv[0], statefile, strerror(errno));
		exit(1);
	}

	rewind(state_fp);
	for (i = 0; i < n_state; i++) {
		fprintf(state_fp, "%s\n", state[i]);
	}
	fflush(state_fp);
}
#line 3179 "ifupdown.nw"
	}
}
#line 3468 "ifupdown.nw"
if (state_fp != NULL) {
	fclose(state_fp);
	state_fp = NULL;
}

#line 2856 "ifupdown.nw"
	return 0;
}
