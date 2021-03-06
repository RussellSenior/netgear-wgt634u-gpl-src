/*
 * file - find type of a file or files - main program.
 *
 * Copyright (c) Ian F. Darwin, 1987.
 * Written by Ian F. Darwin.
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or of the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. The author is not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits must appear in the documentation.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits must appear in the documentation.
 *
 * 4. This notice may not be removed or altered.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>	/* for MAXPATHLEN */
#include <sys/stat.h>
#include <fcntl.h>	/* for open() */
#ifdef RESTORE_TIME
# if (__COHERENT__ >= 0x420)
#  include <sys/utime.h>
# else
#  ifdef USE_UTIMES
#   include <sys/time.h>
#  else
#   include <utime.h>
#  endif
# endif
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>	/* for read() */
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <getopt.h>	/* for long options (is this portable?)*/

#include <netinet/in.h>		/* for byte swapping */

#include "file.h"
#include "patchlevel.h"

#ifndef	lint
FILE_RCSID("@(#)$Id: file.c,v 1.59 2001/07/23 00:02:32 christos Exp $")
#endif	/* lint */


#ifdef S_IFLNK
# define USAGE  "Usage: %s [-bciknsvzL] [-f namefile] [-m magicfiles] file...\n"
#else
# define USAGE  "Usage: %s [-bciknsvz] [-f namefile] [-m magicfiles] file...\n"
#endif

#ifndef MAGIC
# define MAGIC "/etc/magic"
#endif

#ifndef MAXPATHLEN
#define	MAXPATHLEN	512
#endif

int 			/* Global command-line options 		*/
	debug = 0, 	/* debugging 				*/
	lflag = 0,	/* follow Symlinks (BSD only) 		*/
	bflag = 0,	/* brief output format	 		*/
	zflag = 0,	/* follow (uncompress) compressed files */
	sflag = 0,	/* read block special files		*/
	iflag = 0,
	nobuffer = 0,   /* Do not buffer stdout */
	kflag = 0;	/* Keep going after the first match	*/

int			/* Misc globals				*/
	nmagic = 0;	/* number of valid magic[]s 		*/

struct  magic *magic;	/* array of magic entries		*/

const char *magicfile = 0;	/* where the magic is		*/
const char *default_magicfile = MAGIC;

char *progname;		/* used throughout 			*/
int lineno;		/* line number in the magic file	*/


static void	unwrap		__P((char *fn));
static void	usage		__P((void));
#if 0
static int	byteconv4	__P((int, int, int));
static short	byteconv2	__P((int, int, int));
#endif

int main __P((int, char *[]));

void help()
{
	puts(
"Usage: file [OPTION]... [FILE]...
Determine file type of FILEs.

  -m, --magic-file LIST      use LIST as a colon-separated list of magic
                               number files
  -z, --uncompress           try to look inside compressed files
  -b, --brief                do not prepend filenames to output lines
  -c, --checking-printout    print the parsed form of the magic file, use in
                               conjunction with -m to debug a new magic file
                               before installing it
  -f, --files-from FILE      read the filenames to be examined from FILE
  -i, --mime                 output mime type strings
  -k, --keep-going           don't stop at the first match
  -L, --dereference          causes symlinks to be followed
  -n, --no-buffer            do not buffer output
  -s, --special-files        treat special (block/char devices) files as
                             ordinary ones
      --help                 display this help and exit
      --version              output version information and exit");
	exit(0);
}

/*
 * main - parse arguments and handle options
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	int c, longindex;
	int action = 0, didsomefiles = 0, errflg = 0, ret = 0, app = 0;
	char *mime, *home, *usermagic;
	struct stat sb;
	static struct option long_options[] =
	{
		{"version", 0, 0, 'v'},
		{"help", 0, 0, 0},
		{"brief", 0, 0, 'b'},
		{"checking-printout", 0, 0, 'c'},
		{"debug", 0, 0, 'd'},
		{"files-from", 1, 0, 'f'},
		{"mime", 0, 0, 'i'},
		{"keep-going", 0, 0, 'k'},
#ifdef S_IFLNK
		{"dereference", 0, 0, 'L'},
#endif
		{"magic-file", 1, 0, 'm'},
		{"uncompress", 0, 0, 'z'},
		{"no-buffer", 0, 0, 'n'},
		{"special-files", 0, 0, 's'},
		{"compile", 0, 0, 'C'},
		{0, 0, 0, 0},
	};

#ifdef LC_CTYPE
	setlocale(LC_CTYPE, ""); /* makes islower etc work for other langs */
#endif

	if ((progname = strrchr(argv[0], '/')) != NULL)
		progname++;
	else
		progname = argv[0];

	magicfile = default_magicfile;
	if ((usermagic = getenv("MAGIC")) != NULL)
		magicfile = usermagic;
	else
		if ((home = getenv("HOME")) != NULL) {
			if ((usermagic = malloc(strlen(home) + 8)) != NULL) {
				(void)strcpy(usermagic, home);
				(void)strcat(usermagic, "/.magic");
				if (stat(usermagic, &sb)<0) 
					free(usermagic);
				else
					magicfile = usermagic;
			}
		}

	while ((c = getopt_long(argc, argv, "bcdf:ikm:nsvzCL", long_options, &longindex)) != EOF)
		switch (c) {
		case 0 :
			if (longindex==1)
				help();
			break;
		case 'b':
			++bflag;
			break;
		case 'c':
			action = CHECK;
			break;
		case 'C':
			action = COMPILE;
			break;
		case 'd':
			++debug;
			break;
		case 'f':
			if (!app) {
				ret = apprentice(magicfile, action);
				if (action)
					exit(ret);
				app = 1;
			}
			unwrap(optarg);
			++didsomefiles;
			break;
		case 'i':
			iflag++;
			if ((mime = malloc(strlen(magicfile) + 6)) != NULL) {
				(void)strcpy(mime, magicfile);
				(void)strcat(mime, ".mime");
				magicfile = mime;
			}
			break;
		case 'k':
			kflag = 1;
			break;
		case 'm':
			magicfile = optarg;
			break;
		case 'n':
			++nobuffer;
			break;
		case 's':
			sflag++;
			break;
		case 'v':
  			(void) fprintf(stdout, "%s-"VERSION"\n", progname);
			(void) fprintf(stdout, "magic data from %s\n",
				       magicfile);
			return 1;
		case 'z':
			zflag++;
			break;
#ifdef S_IFLNK
		case 'L':
			++lflag;
			break;
#endif
		case '?':
		default:
			errflg++;
			break;
		}

	if (errflg) {
		usage();
		fputs("Try `file --help' for more information.\n",stderr);
	}

	if (!app) {
		ret = apprentice(magicfile, action);
		if (action)
			exit(ret);
		app = 1;
	}

	if (optind == argc) {
		if (!didsomefiles) {
			usage();
		}
	}
	else {
		int i, wid, nw;
		for (wid = 0, i = optind; i < argc; i++) {
			nw = strlen(argv[i]);
			if (nw > wid)
				wid = nw;
		}
		for (; optind < argc; optind++)
			process(argv[optind], wid);
	}

	return 0;
}


/*
 * unwrap -- read a file of filenames, do each one.
 */
static void
unwrap(fn)
	char *fn;
{
	char buf[MAXPATHLEN];
	FILE *f;
	int wid = 0, cwid;

	if (strcmp("-", fn) == 0) {
		f = stdin;
		wid = 1;
	} else {
		if ((f = fopen(fn, "r")) == NULL) {
			error("Cannot open `%s' (%m).\n", fn);
			/*NOTREACHED*/
		}

		while (fgets(buf, MAXPATHLEN, f) != NULL) {
			cwid = strlen(buf) - 1;
			if (cwid > wid)
				wid = cwid;
		}

		rewind(f);
	}

	while (fgets(buf, MAXPATHLEN, f) != NULL) {
		buf[strlen(buf)-1] = '\0';
		process(buf, wid);
		if(nobuffer)
			(void) fflush(stdout);
	}

	(void) fclose(f);
}


#if 0
/*
 * byteconv4
 * Input:
 *	from		4 byte quantity to convert
 *	same		whether to perform byte swapping
 *	big_endian	whether we are a big endian host
 */
static int
byteconv4(from, same, big_endian)
	int from;
	int same;
	int big_endian;
{
	if (same)
		return from;
	else if (big_endian) {		/* lsb -> msb conversion on msb */
		union {
			int i;
			char c[4];
		} retval, tmpval;

		tmpval.i = from;
		retval.c[0] = tmpval.c[3];
		retval.c[1] = tmpval.c[2];
		retval.c[2] = tmpval.c[1];
		retval.c[3] = tmpval.c[0];

		return retval.i;
	}
	else
		return ntohl(from);	/* msb -> lsb conversion on lsb */
}

/*
 * byteconv2
 * Same as byteconv4, but for shorts
 */
static short
byteconv2(from, same, big_endian)
	int from;
	int same;
	int big_endian;
{
	if (same)
		return from;
	else if (big_endian) {		/* lsb -> msb conversion on msb */
		union {
			short s;
			char c[2];
		} retval, tmpval;

		tmpval.s = (short) from;
		retval.c[0] = tmpval.c[1];
		retval.c[1] = tmpval.c[0];

		return retval.s;
	}
	else
		return ntohs(from);	/* msb -> lsb conversion on lsb */
}
#endif

/*
 * process - process input file
 */
void
process(inname, wid)
	const char	*inname;
	int wid;
{
	int	fd = 0;
	static  const char stdname[] = "standard input";
	unsigned char	buf[HOWMANY+1];	/* one extra for terminating '\0' */
	struct stat	sb;
	int nbytes = 0;	/* number of bytes read from a datafile */
	char match = '\0';

	if (strcmp("-", inname) == 0) {
		if (fstat(0, &sb)<0) {
			error("cannot fstat `%s' (%m).\n", stdname);
			/*NOTREACHED*/
		}
		inname = stdname;
	}

	if (wid > 0 && !bflag)
	     (void) printf("%s:%*s ", inname, 
			   (int) (wid - strlen(inname)), "");

	if (inname != stdname) {
		/*
		 * first try judging the file based on its filesystem status
		 */
		if (fsmagic(inname, &sb) != 0) {
			putchar('\n');
			return;
		}

		if ((fd = open(inname, O_RDONLY)) < 0) {
			/* We can't open it, but we were able to stat it. */
			if (sb.st_mode & 0002) ckfputs("writeable, ", stdout);
			if (sb.st_mode & 0111) ckfputs("executable, ", stdout);
			ckfprintf(stdout, "can't read `%s' (%s).\n",
			    inname, strerror(errno));
			return;
		}
	}


	/*
	 * try looking at the first HOWMANY bytes
	 */
	if ((nbytes = read(fd, (char *)buf, HOWMANY)) == -1) {
		error("read failed (%m).\n");
		/*NOTREACHED*/
	}

	if (nbytes == 0)
		ckfputs(iflag ? "application/x-empty" : "empty", stdout);
	else {
		buf[nbytes++] = '\0';	/* null-terminate it */
		match = tryit(buf, nbytes, zflag);
	}

#ifdef BUILTIN_ELF
	if (match == 's' && nbytes > 5) {
		/*
		 * We matched something in the file, so this *might*
		 * be an ELF file, and the file is at least 5 bytes long,
		 * so if it's an ELF file it has at least one byte
		 * past the ELF magic number - try extracting information
		 * from the ELF headers that can't easily be extracted
		 * with rules in the magic file.
		 */
		tryelf(fd, buf, nbytes);
	}
#endif

	if (inname != stdname) {
#ifdef RESTORE_TIME
		/*
		 * Try to restore access, modification times if read it.
		 * This is really *bad* because it will modify the status
		 * time of the file... And of course this will affect
		 * backup programs
		 */
# ifdef USE_UTIMES
		struct timeval  utsbuf[2];
		utsbuf[0].tv_sec = sb.st_atime;
		utsbuf[1].tv_sec = sb.st_mtime;

		(void) utimes(inname, utsbuf); /* don't care if loses */
# else
		struct utimbuf  utbuf;

		utbuf.actime = sb.st_atime;
		utbuf.modtime = sb.st_mtime;
		(void) utime(inname, &utbuf); /* don't care if loses */
# endif
#endif
		(void) close(fd);
	}
	(void) putchar('\n');
}


int
tryit(buf, nb, zfl)
	unsigned char *buf;
	int nb, zfl;
{
	/* try compression stuff */
	if (zfl && zmagic(buf, nb))
		return 'z';

	/* try tests in /etc/magic (or surrogate magic file) */
	if (softmagic(buf, nb))
		return 's';

	/* try known keywords, check whether it is ASCII */
	if (ascmagic(buf, nb))
		return 'a';

	/* abandon hope, all ye who remain here */
	ckfputs("data", stdout);
		return '\0';
}

static void
usage()
{
	(void)fprintf(stderr, USAGE, progname);
	(void)fprintf(stderr, "Usage: %s -C [-m magic]\n", progname);
	exit(1);
}
