/*
 * Copyright 1999-2001 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
*/

#include <alloca.h>
#include <ctype.h>
#include <fcntl.h>
#include <popt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pump.h"

static void parseError(int line, char * message) {
    fprintf(stderr, "error parsing config file at line %d: %s\n", line, 
	    message);
}

static int readStanza(char ** cfg, struct pumpOverrideInfo * overrideList, 
	              struct pumpOverrideInfo * override, int * lineNum) {
    struct pumpOverrideInfo * nextO = override + 1;
    char * start, * end, * next, * rest, * chptr;
    int line;
    int argc;
    const char ** argv;
    int num;

    if (!lineNum) {
	line = 1;
	lineNum = &line;
    }

    next = *cfg;
    while (next && *next) {
	start = next;

	while (isspace(*start) && *start != '\n') start++;
	if (*start == '\n') {
	    next = start + 1;
	    (*lineNum)++;
	    continue;
	} else if (*start == '#') {
	    next = strchr(start, '\n') + 1;
	    (*lineNum)++;
	    continue;
	}

	end = strchr(start, '\n');
	*end = '\0';
	next = end + 1;

	chptr = start;
	while (!isspace(*chptr) && *chptr) chptr++;
	rest = chptr;
	while (*rest && isspace(*rest)) rest++;
	*chptr = '\0';

	if (!strcmp(start, "device")) {
	    if (overrideList != override) {
		parseError(*lineNum, "device directive may not occur inside "
			   "of device specification");
		return 1;
	    }

	    if (!*rest || *rest == '{') {
		parseError(*lineNum, "device directive must be followed by a "
			   "device name");
		return 1;
	    }

	    end = rest + 1;
	    while (!isspace(*end) && *end != '{' && *end) end++;
	    chptr = end;
	    while (isspace(*end) && *end) end++;

	    if (*end != '{') {
		parseError(*lineNum, "device directive must end with a {");
		return 1;
	    }

	    end++;
	    *chptr = '\0';

	    while (*end && isspace(*end)) end++;
	    if (*end) {
		parseError(*lineNum, "unexpected information after {");
		return 1;
 	    }
	
	    *nextO = *override;
	    strcpy(nextO->intf.device, rest);
	    nextO->script = override->script ? strdup(override->script) : NULL;

	    (*lineNum)++;
	    if (readStanza(&next, overrideList, nextO, lineNum)) return 1;
	    (*lineNum)--;	/* it'll get incremented again at the end
				   of this while loop */
	    nextO++;
	} else if (!strcmp(start, "}")) {
	    if (overrideList == override) {
		parseError(*lineNum,"} may only occur inside of a "
			   "device directive");
		return 1;
	    } else if (*rest) {
		parseError(*lineNum, "unexpected information after }");
	    }

	    *cfg = next;
	    (*lineNum)++;

	    return 0;
	} else if (!strcmp(start, "timeout")) {
	    poptParseArgvString(rest, &argc, &argv);

	    if (argc != 1) {
		parseError(*lineNum, "timeout directive expects a "
			   "single argument");
		return 1;
	    }

	    num = strtol(argv[0], &chptr, 0);
	    if (*chptr) {
		parseError(*lineNum, "timeout requires a numeric argument");
		return 1;
	    }

	    override->timeout = num;
	} else if (!strcmp(start, "retries")) {
	    poptParseArgvString(rest, &argc, &argv);

	    if (argc != 1) {
		parseError(*lineNum, "retries directive expects a "
			   "single argument");
		return 1;
	    }

	    num = strtol(argv[0], &chptr, 0);
	    if (*chptr) {
		parseError(*lineNum, "retries requires a numeric argument");
		return 1;
	    }

	    override->numRetries = num;
	} else if (!strcmp(start, "domainsearch")) {
	    if (overrideList != override) {
		parseError(*lineNum, "domainsearch directive may not occur "
			   "inside of device specification");
		return 1;
	    }

	    poptParseArgvString(rest, &argc, &argv);

	    if (argc != 1) {
		parseError(*lineNum, "domainsearch directive expects a "
			   "single argument");
		return 1;
	    }

	    /*
		We don't free this as other configurations may have inherited 
		it. This could be the wrong decision, but leak would be tiny
		so why worry?
	    */
	    override->searchPath = strdup(argv[0]);
	    free(argv);
	} else if (!strcmp(start, "nodns")) {
	    if (*rest) {
		parseError(*lineNum, "unexpected argument to nodns directive");
		return 1;
	    }
	    override->flags |= OVERRIDE_FLAG_NODNS;
	} else if (!strcmp(start, "nobootp")) {
	    if (*rest) {
		parseError(*lineNum, "unexpected argument to nobootp directive");
		return 1;
	    }
	    override->flags |= OVERRIDE_FLAG_NOBOOTP;
	} else if (!strcmp(start, "nogateway")) {
	    if (*rest) {
		parseError(*lineNum, "unexpected argument to nogateway directive");
		return 1;
	    }
	    override->flags |= OVERRIDE_FLAG_NOGATEWAY;
	} else if (!strcmp(start, "nonisdomain")) {
	    if (*rest) {
		parseError(*lineNum, "unexpected argument to nonisdomain directive");
		return 1;
	    }
	    override->flags |= OVERRIDE_FLAG_NONISDOMAIN;
	} else if (!strcmp(start, "script")) {
	    if (overrideList != override) {
		parseError(*lineNum, "script directive may not occur "
			   "inside of device specification");
		return 1;
	    }

	    poptParseArgvString(rest, &argc, &argv);

	    if (argc != 1) {
		parseError(*lineNum, "script directive expects a "
			   "single argument");
		return 1;
	    }
	    override->script = strdup(argv[0]);
	    free(argv);
        } else {
	    char * error;

	    error = malloc(strlen(start) + 50);
	    sprintf(error, "unrecognized directive %s", start);
	    parseError(*lineNum, error);
	    free(error);

	    return 1;
	}

	(*lineNum)++;
    }

    *cfg = next;
   
    return 0; 
}

int readPumpConfig(char * configFile, struct pumpOverrideInfo ** overrides) {
    int fd;
    char * buf, * chptr;
    struct stat sb;
    int count;
    void * orig = *overrides;

    if ((fd = open(configFile, O_RDONLY)) < 0) {
	*overrides = calloc(sizeof(**overrides), 2);
	pumpInitOverride(*overrides);
	close(fd);
	return 0;
    }

    fstat(fd, &sb);
    buf = alloca(sb.st_size + 2);
    if (read(fd, buf, sb.st_size) != sb.st_size) {
	close(fd);
	return 1;
    }
    close(fd);
    buf[sb.st_size] = '\n';
    buf[sb.st_size + 1] = '\0';

    /* this will give us an upper limit anyway */
    count = 1;
    chptr = buf;
    while ((chptr = strchr(chptr, '{'))) {
	chptr++;
	count++;
    }

    *overrides = calloc(sizeof(**overrides), count + 1);
    pumpInitOverride(*overrides);

    count = readStanza(&buf, *overrides, *overrides, NULL);
    if (count) {
	free(*overrides);
	*overrides = orig;
    }

    return count; 
}
