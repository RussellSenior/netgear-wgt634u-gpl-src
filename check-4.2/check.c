/*
    check - Compute CRC-32 or Adler-32 checksum on files or within pipe.
    Copyright 2002-2003 Greg Roelofs and Mark Adler

               http://pobox.com/~newt/greg_software.html

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    (versions 3.4 and earlier are in the public domain)
 */

#define VERSION "4.2 of 30 December 2003"

/*
   All check does is take a stream of data from stdin and send it unchanged
   to stdout, calculating the check along the way.  The check and number of
   bytes read are written to stderr.  For example:

       % tar cvf - check.* | check | bzip2 | check > /upload/check.tar.bz2
       check.1
       check.c
       check.h
       check.txt
       CRC-32 = c26ae906, size = 36864 bytes
       CRC-32 = 61abf730, size = 6024 bytes

   It is particularly useful as the final step before writing to tape or
   another backup medium, because then the integrity of the backup copy
   may be verified periodically simply by reading it and piping it into
   check again:

       % tar -cvv -f - / --exclude proc | check -c | dd of=/dev/tape bs=1k
       [...]
       CRC-32 = dc1024bc, size = 34,823,127,040 bytes

       % dd if=/dev/tape bs=1k | check -cn
       CRC-32 = dc1024bc, size = 34,823,127,040 bytes

   (Note that this won't guarantee that tar read the file system correctly
   in the first place!)

   Alternatively, if arguments are given, check assumes that they are files
   for which the check is to be calculated; the file data are not written,
   and the check info is sent to stdout rather than stderr:

       % check check.*
       check.1                          CRC-32 = d0e0fdb3, size = 3202 bytes
       check.c                          CRC-32 = 44e35fbf, size = 17610 bytes
       check.h                          CRC-32 = ee52d619, size = 9575 bytes
       check.txt                        CRC-32 = c3ec3d07, size = 3452 bytes

   As shown in the previous example, data output may be suppressed even in
   filter (pipe) mode via the -n option.

   With the -a option, check calculates the Adler-32 checksum instead of the
   CRC-32.  Adler-32 is twice as fast and only slightly less robust.  (Raw
   CPU speed is actually three times as fast, but typical disk I/O overhead
   will reduce the improvement.)
 */

/*
   Compilation example (Unix/Linux/Cygwin, GNU C, command line):
 
        gcc -O3 -Wall -o check check.c
 
   Compilation example (Windows, MSVC, command line, assuming VCVARS32.BAT or
   whatever has already been run):
 
        cl -nologo -O -W3 -DWIN32 -c check.c
        link -nologo check.obj setargv.obj
 
   "setargv.obj" is included with MSVC and will be found if the batch file has
   been run.  Either Borland or Watcom (both?) may use "wildargs.obj" instead.
   Both object files serve the same purpose:  they expand wildcard arguments
   into a list of files on the local file system, just as Unix shells do by
   default ("globbing").  Note that mingw32 + gcc (Unix-like compilation
   environment for Windows) apparently expands wildcards on its own, so no
   special object files are necessary for it.  emx + gcc for OS/2 (and possibly
   rsxnt + gcc for Win32) has a special _wildcard() function call, which is
   already (conditionally) included in check's source code.
 */

/*
   History:

   ver      date          who           what
   ---  -----------  ------------  -------------------------------------------
   1.0   1 Mar 1993  Greg Roelofs  really simple 32-bit CRC filter, based on
                                    code in Mark's funzip.c, v2.3.
   1.1  15 Apr 1993  Greg Roelofs  added 32MB progress indicator
   2.0   8 May 1994  Greg Roelofs  modified for command-line arguments; made
                                    filter output buffered (37% faster);
                                    nested inner if-blocks (extra 13%); moved
                                    usage info to usage() function
   2.1  15 Mar 1995  Greg Roelofs  added filenames to command-line mode
   3.0  16 Mar 1995  Greg Roelofs  added Adler-32 option
   3.1  17 Mar 1995  Greg Roelofs  added Mark's Adler-32 speed-ups (4x); made
                                    similar improvements to CRC-32 code (2x)
   3.2  18 Mar 1995  Mark Adler    unrolled CRC-32, simplified code some,
                                    changed name to check, added ROLL option
   3.3   9 Dec 2001  Greg Roelofs  added -v option for 32 MB progress meter
                                    (now off by default); added compilation
                                    info, home page info, and EMX wildcard-
                                    expansion
   3.4   9 Mar 2002  Greg Roelofs  added count of 32 MB blocks (cope with
                                    4 GB rollover of "size" variable)
   4.0  15 Dec 2002  Greg Roelofs  replaced 32 MB count with exact "large
                                    integer" calculation of total size; added
                                    -c option and commafy() function to make
                                    large byte sizes pretty; added -n option
                                    to suppress filter output; added argv[0]
                                    test to select CRC/Adler function based
                                    on program name; fixed multiple-option
                                    parsing (loop); switched to GPL
   4.1  23 Feb 2003  Greg Roelofs  improved option-parsing; fixed usage screen
   4.2  30 Dec 2003  Greg Roelofs  improved error-handling; added summary
 */

#include "check.h"    /* this contains the CRC data and other ugly stuff */

#define EXACT_STATS   /* do (minimal) large-integer math on byte sizes */
                      /* (undefine to revert to old, approximate version) */

static char *commafy (ulg num_1e9, ulg size_mod_1e9);
static void  usage   (FILE *usagefp);
static void  err     (int, char *, char *);


int main(int argc, char **argv)
{
    register int k;              /* inner loop variable */
    register uch *p;             /* pointer into buf */
    register ulg check;          /* running check */
    register ulg sum2;           /* temporary sum for adler-32 */
    register ulg size;           /* size of data (bytes) */
    register ulg num32M;         /* number of 32MB blocks (for overflow) */
    char *q, buf[BUFSZ];
    char *type = "CRC-32";       /* checksum name */
    int error=0, c;
    int help = FALSE;            /* usage requested? */
    int adler = FALSE;           /* flag (option) */
    int do_commafy = FALSE;      /* flag (option) */
    int suppress_output = FALSE; /* flag (option) */
    int verbose = FALSE;         /* flag (option) */
    int have_filename = FALSE;   /* flag */
    int filter;                  /* flag */
    FILE *in=NULL, *out=NULL;    /* input and output streams */
    FILE *msgs;                  /* where to send usage and check info */


#ifdef __EMX__
    _wildcard(&argc, &argv);    /* Unix-like globbing for OS/2 and DOS */
#endif

    /* check whether (last part of) executable name is "adler" or "adler32" */
    /* XXX - DOS/Windoze bug? (i.e., need to check for '\\', too?) */
    if ((q = strrchr(argv[0], '/')) == NULL)
        q = argv[0];
    else
        ++q;
    if (strncmp(q, "adler", 5) == 0)
        adler = TRUE;

    /* get the command line option(s), if any */
    if (argc == 1) {       /* just command name */
        --argc;
        /* if no file arg(s) and stdin not redirected, give the user help */
        if (isatty(0) || ((c = getc(stdin)) == EOF))
            help = TRUE;   /* stdin == console or /dev/null */
        else
            ungetc(c, stdin);
    } else {
        while (--argc > 0  &&  (*++argv)[0] == '-') {
            while ((c = *++(argv[0]))) { /* argv[0] no longer program name */
                switch (c) {
                    case 'a':            /* do Adler-32 checksum, not CRC-32 */
                        adler = TRUE;
                        break;
                    case 'c':            /* add commas to sizes (readability) */
                        do_commafy = TRUE;
                        break;
                    case 'n':            /* don't write input data to stdout */
                        suppress_output = TRUE;
                        break;
                    case 'v':            /* print progress every 32 MB */
                        verbose = TRUE;
                        break;
                    default:
                        fprintf(stderr,
                          "error:  `%c' is not a valid option\n", c);
                        ++error;
                        break;
                }  /* end switch */
            }  /* end while */
        }  /* end while */
    }  /* end if */

    if (help && !error)
        msgs = (FILE *)stdout;
    else
        msgs = (FILE *)stderr;

    if (help || error)
        usage(msgs);  /* (and exit) */

    if (adler)
        type = "Adler-32";

    if (argc == 0) {
        /* we're a filter (unless suppressed):  prepare to be a binary one */
        filter = suppress_output? FALSE : TRUE;
#ifdef DOS_NT_OS2               /* some buggy C libs require BOTH the */
        setmode(0, O_BINARY);   /*  setmode() call AND the fdopen() in */
        setmode(1, O_BINARY);   /*  binary mode :-( */
#endif
        if ((in = fdopen(0, FOPR)) == NULL)
            err(2, "cannot find stdin", "");
        if (filter && (out = fdopen(1, FOPW)) == NULL)
            err(2, "cannot write to stdout", "");
        msgs = (FILE *)stderr;
    } else {
        filter = FALSE;
        have_filename = TRUE;
        if ((in = fopen(*argv, FOPR)) == NULL)
            err(2, "cannot open ", *argv);
        --argc;
        ++argv;
        msgs = (FILE *)stdout;
    }

    /* main loop over files (first one is already open for reading) */
    for (;;) {
        size = 0L;
        num32M = 0L;
        check = adler ? 1L : 0L;

        /* main loop over file data */
        while ((k = fread(buf, 1, BUFSZ, in)) > 0) {
            size += k;
            if (filter)
                fwrite(buf, 1, k, out);      /* write data immediately */
            if ((size & 0x1ffffffL) == 0) {  /* progr. every 32MB */
                ++num32M;
                if (verbose) {
/*                  fprintf(msgs, "   %lu MB\n", size >> 20);  */
                    fprintf(msgs, "   %lu MB\n", (ulg)32L * num32M);
                    fflush(msgs);
                }
            }
            p = (uch *)buf;
            
            /* do Adler-32 */
            if (adler) {
                sum2 = (check >> 16) & 0xffffL;
                check &= 0xffffL;
#ifndef ROLL
                while (k >= 64) {
                    A6
                    k -= 64;  /* this much unrolling optimal for SGI, NeXT */
                }
#endif /* !ROLL */
                if (k) do {
                    A0
                } while (--k);
                check %= BASE;
                check |= (sum2 % BASE) << 16;

            /* do CRC-32 */
            } else {
                check ^= 0xffffffffL;
#ifndef ROLL
                while (k >= 32) {
                    C5
                    k -= 32;  /* this much unrolling optimal for NeXT */
                }
#endif /* !ROLL */
                if (k) do {
                    C0
                } while (--k);
                check ^= 0xffffffffL;
            }
        }

        /* check for errors reading stream, since fread() doesn't return any
         * real error code of its own (except short buffer, and that only if
         * you "know" the file's length via some other means, like stat()--
         * which may not be accurate if file is growing or shrinking, and
         * which is totally irrelevant anyway for a piped stream) */
        if (ferror(in)) {
            fprintf(msgs, "check error: error reading %s\n", have_filename?
              argv[-1] : "stdin");
            ++error;
            clearerr(in);
        }

        if (filter)
            fflush(out);
        else if (have_filename)
            fprintf(msgs, "%-30s   ", argv[-1]);   /* print filename */
#ifdef EXACT_STATS
        if (num32M < 128L) {  /* less than 4 GB:  size (ulg) didn't overflow */
            if (do_commafy)
                fprintf(msgs, "%s = %08lx, size = %s bytes", type, check,
                  commafy(0L, size));
            else
                fprintf(msgs, "%s = %08lx, size = %lu bytes", type, check,
                  size);
        } else {
            ulg n32M, size_mod_1e9, num_1e9;

            n32M = num32M;
            size_mod_1e9 = 0L;
            num_1e9 = 0L;
            while (n32M > 0L) {
                --n32M;
                size_mod_1e9 += 0x2000000L;         /* 32MB */
                if (size_mod_1e9 > 1000000000L) {   /* 1 billion (10^9) */
                    size_mod_1e9 -= 1000000000L;
                    ++num_1e9;
                }
            }
            size_mod_1e9 += (size % 0x2000000L);    /* anything below 32MB */
            if (size_mod_1e9 > 1000000000L) {       /* 1 billion (10^9) */
                size_mod_1e9 -= 1000000000L;
                ++num_1e9;
            }
            /* if (num_1e9 > 0L) */ /* already guaranteed by 4GB check above */
            if (do_commafy)
                fprintf(msgs, "%s = %08lx, size = %s bytes", type, check,
                  commafy(num_1e9, size_mod_1e9));
            else
                fprintf(msgs, "%s = %08lx, size = %lu%09lu bytes", type, check,
                  num_1e9, size_mod_1e9);
        }
#else /* approximate (old version) */
        fprintf(msgs, "%s = %08lx, size = %lu bytes", type, check, size);
        if (num32M > 0L) {
            fprintf(msgs, " (%lu 32MB blocks, ~", num32M);
            if (num32M > 32L)   /* more than 1 GB */
                fprintf(msgs, "%lu GB)", (ulg)(num32M + 16L)/32);
            else
                fprintf(msgs, "%lu MB)", (ulg)32L * num32M);
        }
#endif
        fprintf(msgs, "\n");
        fflush(msgs);

        if (have_filename)
            fclose(in);  /* no need to do "out" (never opened) */
        if (argc == 0)
            break;
        /* don't want to fail in middle of list, so report error and continue */
        while ((in = fopen(*argv, FOPR)) == NULL) {
            fprintf(msgs,
              "check error: cannot open %s (skipping to next file)\n", *argv);
            ++error;
            --argc;
            ++argv;
            if (argc == 0)
                goto error_exit;   /* print count of errors */
        }
        --argc;
        ++argv;
    }

error_exit:
    if (error) {
        if (error == 1)
            fprintf(msgs, "There was 1 error.\n");
        else
            fprintf(msgs, "There were %d errors.\n", error);
        return 3;
    }

    /* much happiness */
    return 0;

} /* end function main() */




/* convert a (possibly large) integer to a string with commas (e.g., 10,000) */

static char *commafy(ulg num_1e9, ulg size_mod_1e9)
{
    static char *p, *q, commabuf[64];
    int i, len, numcommas;

    if (num_1e9 > 0L)
        snprintf(commabuf, 64, "%lu%09lu", num_1e9, size_mod_1e9);
    else
        snprintf(commabuf, 64, "%lu", size_mod_1e9);
    len = strlen(commabuf);
    numcommas = (len - 1)/3;

    p = commabuf + len - 1;           /* old end of string (final digit) */
    q = commabuf + len + numcommas;   /* new end of string... */
    *q-- = '\0';                      /* ...now terminated (point at digit) */

    /* working backwards, copy un-comma'd digits to end of comma buffer (in
     * place!), and insert commas as go */
    for (i = 0;  i < len;  ++i) {
        if (i && i % 3 == 0)
            *q-- = ',';   /* could localize for Europe (dots) trivially... */
        *q-- = *p--;
    }

    if (++q != commabuf || ++p != commabuf) {
        fprintf(stderr, "check:  internal logic error in commafy()\n");
        snprintf(commabuf, 64, "%lu,%09lu", num_1e9, size_mod_1e9);
    }

    return commabuf;
}




static void usage(FILE *usagefp)   /* print usage and exit */
{
    fprintf(usagefp, "check, version %s, by Greg Roelofs and Mark Adler.\n",
      VERSION);
    fprintf(usagefp,
     "   (This software is licensed under the GNU General Public License and\n"
     "   comes with ABSOLUTELY NO WARRANTY; see the accompanying file COPYING\n"
     "   or http://www.fsf.org/copyleft/gpl.html for details.)\n\n");

    fprintf(usagefp, "usage: ... | check [-acv] | ...\n");
    fprintf(usagefp, "   or: ... | check [-acv] -n\n");
    fprintf(usagefp, "   or: check [-acv] file [file ...]\n\n");

    fprintf(usagefp,
      "Writes stdin to stdout unchanged and computes 32-bit CRC on stderr,\n");
    fprintf(usagefp,
      "or computes multiple 32-bit CRCs on stdout for given list of files.\n");
    fprintf(usagefp,
    "With -n (null) option in filter mode, disables writing data to stdout.\n");
    fprintf(usagefp,
    "With -a option, computes (faster) Adler-32 checksum instead of CRC-32.\n");
    fprintf(usagefp,
   "With -c option, prints sizes with commas (e.g., 1,000 instead of 1000).\n");
    fprintf(usagefp,
      "With -v (verbose) option, prints progress info every 32 MB.\n");

#ifdef TEST_COMMAFY
    fprintf(usagefp,
      "GRR DEBUG:  commafy(10000000L, 384398468L) = %s.\n",
      commafy(10000000L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(1000000L, 384398468L) = %s.\n",
      commafy(1000000L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(100000L, 384398468L) = %s.\n",
      commafy(100000L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(10000L, 384398468L) = %s.\n",
      commafy(10000L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(1000L, 384398468L) = %s.\n",
      commafy(1000L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(100L, 384398468L) = %s.\n",
      commafy(100L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(10L, 384398468L) = %s.\n",
      commafy(10L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(1L, 384398468L) = %s.\n",
      commafy(1L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 384398468L) = %s.\n",
      commafy(0L, 384398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 84398468L) = %s.\n",
      commafy(0L, 84398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 4398468L) = %s.\n",
      commafy(0L, 4398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 398468L) = %s.\n",
      commafy(0L, 398468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 98468L) = %s.\n",
      commafy(0L, 98468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 8468L) = %s.\n",
      commafy(0L, 8468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 468L) = %s.\n",
      commafy(0L, 468L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 68L) = %s.\n",
      commafy(0L, 68L));
    fprintf(usagefp,
      "GRR DEBUG:  commafy(0L, 8L) = %s.\n",
      commafy(0L, 8L));
#endif /* TEST_COMMAFY */

    exit(3);
}




/* exit on error with a message and a code */

static void err(int n, char *m, char *f)
{
    fprintf(stderr, "check error: %s%s\n", m, f);
    exit(n);
}
