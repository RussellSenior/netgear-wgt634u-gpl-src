/*
    check.h -- Copyright 2002-2003 Greg Roelofs and Mark Adler

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
 */


#if defined(__GO32__) && defined(unix)   /* DOS extender */
#  undef unix
#endif

#if defined(unix) || defined(__convexc__) || defined(M_XENIX)
#  ifndef UNIX
#    define UNIX
#  endif /* !UNIX */
#endif /* unix || __convexc__ || M_XENIX */

/* define MSDOS for Turbo C (unless OS/2) and Power C as well as Microsoft C */
#ifdef __POWERC
#  define __TURBOC__
#  define MSDOS
#endif /* __POWERC */
#if defined(__MSDOS__) && (!defined(MSDOS))   /* just to make sure */
#  define MSDOS
#endif

#if defined(__OS2__) && (!defined(OS2))
#  define OS2
#endif

#if defined(MSDOS) || defined(NT) || defined(OS2)
#  define DOS_NT_OS2
#endif

#if defined(linux) && (!defined(LINUX))
#  define LINUX
#endif

/* use prototypes and ANSI libraries if __STDC__, or Microsoft or Borland C, or
 * Silicon Graphics, or Convex, or IBM C Set/2, or GNU gcc/emx, or Watcom C,
 * or Macintosh, or Windows NT, or Sequent, or Atari.
 */
#if __STDC__ || defined(MSDOS) || defined(sgi) || defined(__convexc__)
#  ifndef PROTO
#    define PROTO
#  endif
#  define MODERN
#endif
#if defined(__IBMC__) || defined(__EMX__) || defined(__WATCOMC__)
#  ifndef PROTO
#    define PROTO
#  endif
#  define MODERN
#endif
#if defined(THINK_C) || defined(MPW) || defined(WIN32) || defined(_SEQUENT_)
#  ifndef PROTO
#    define PROTO
#  endif
#  define MODERN
#endif
#if defined(ATARI_ST)
#  ifndef PROTO
#    define PROTO
#  endif
#  define MODERN
#endif

/* turn off prototypes if requested */
#if defined(NOPROTO) && defined(PROTO)
#  undef PROTO
#endif

/* used to remove arguments in function prototypes for non-ANSI C */
#ifdef PROTO
#  define OF(a) a
#else /* !PROTO */
#  define OF(a) ()
#endif /* ?PROTO */

/* bad or (occasionally?) missing stddef.h: */
#if defined(M_XENIX) || defined(DNIX) || (defined(__GNUC__) && defined(sun))
#  define NO_STDDEF_H
#endif

/* cannot depend on MODERN for presence of stdlib.h */
#if defined(__GNUC__) || defined(apollo)   /* both define __STDC__ */
#  if (!defined(__EMX__) && !defined(__386BSD__) && !defined(LINUX))
#    define NO_STDLIB_H
#  endif
#endif /* __GNUC__ || apollo */

#if defined(__SYSTEM_FIVE) || defined(M_SYSV) || defined(M_SYS5)
#  ifndef SYSV
#    define SYSV
#  endif /* !SYSV */
#endif /* __SYSTEM_FIVE || M_SYSV || M_SYS5 */

#if defined(SYSV) || defined(CRAY) || defined(LINUX)
#  ifndef TERMIO
#    define TERMIO
#  endif /* !TERMIO */
#endif /* SYSV || CRAY || LINUX */

#if defined(ultrix) || defined(bsd4_2) || defined(sun) || defined(pyr)
#  if (!defined(BSD)) && (!defined(SYSV))
#    define BSD
#  endif /* !BSD && !SYSV */
#endif /* ultrix || bsd4_2 || sun || pyr */
#if defined(__convexc__) || defined(TOPS20) || defined(__386BSD__)
#  if (!defined(BSD)) && (!defined(SYSV))
#    define BSD
#  endif /* !BSD && !SYSV */
#endif /* __convexc__ || TOPS20 || __386BSD__ */



#ifndef MINIX            /* Minix needs it after all the other includes (?) */
#  include <stdio.h>
#endif
#ifdef USE_STRINGS_H
#  include <strings.h>   /* strcpy, strcmp, memcpy, index/rindex, etc. */
#else
#  include <string.h>    /* strcpy, strcmp, memcpy, strchr/strrchr, etc. */
#endif
#ifdef VMS
#  include <types.h>
#else /* !VMS */
#  ifdef AMIGA
#    ifdef AZTEC_C
#      include <clib/dos_protos.h>
#      include <pragmas/dos_lib.h>
#      define MODERN
#      define O_BINARY 0
#    else /* !AZTEC_C */
#      include <sys/types.h>
#      include <proto/dos.h>
#      define O_BINARY   O_RAW
#    endif /* ?AZTEC_C */
#    include <time.h>
#    define dup
#  else /* !AMIGA */
#    if (!defined(THINK_C)) && (!defined(MPW)) && (!defined(ATARI_ST))
#      include <sys/types.h>         /* off_t, time_t, dev_t, ... */
#    endif /* !THINK_C && !MPW */
#  endif /* ?AMIGA */
#endif /* ?VMS */

#ifdef MODERN
#  ifndef NO_STDDEF_H
#    include <stddef.h>
#  endif
#  ifndef NO_STDLIB_H
#    include <stdlib.h>    /* standard library prototypes, malloc(), etc. */
#  endif
#  if defined(__386BSD__) || defined(LINUX) || defined(SYSV)
#    include <unistd.h>
#  endif
   typedef size_t extent;
   typedef void voidp;
#else /* !MODERN */
   char *malloc();
   typedef unsigned int extent;
#  define void int
   typedef char voidp;
#endif /* ?MODERN */


typedef unsigned char   uch;
typedef unsigned short  ush;
typedef unsigned long   ulg;


/* enforce binary i/o if recognized */
#if defined(__STDC__) || defined(DOS_NT_OS2)
#  define BINIO
#endif

#ifdef BINIO
#  define FOPR "rb"
#  define FOPW "wb"
#else
#  define FOPR "r"
#  define FOPW "w"
#endif


#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif


/* macros for Adler-32 checksum */
#define BASE 65521  /* largest prime smaller than 65536 */
#define NMAX 5552   /* largest n s.t.  255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define A0 check += *p++; sum2 += check;
#ifndef ROLL
#  define A1 A0 A0
#  define A2 A1 A1
#  define A3 A2 A2
#  define A4 A3 A3
#  define A5 A4 A4
#  define A6 A5 A5
#endif /* !ROLL */

#define BUFSZ 4096  /* must be less than NMAX for Adler-32; must be a power
                       of 2 for progress indicator */


/* macros for CRC-32 */

#define C0 check = crc_32_tab[((int)check ^ (*p++)) & 0xff] ^ (check >> 8);
#ifndef ROLL
#  define C1 C0 C0
#  define C2 C1 C1
#  define C3 C2 C2
#  define C4 C3 C3
#  define C5 C4 C4
#endif /* !ROLL */

/* table of CRC-32's of all single-byte values (made by makecrc.c) */
ulg crc_32_tab[] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};
