/*
 * ProFTPD - FTP server daemon
 * Copyright (c) 1997, 1998 Public Flood Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * As a special exemption, Public Flood Software/MacGyver aka Habeeb J. Dihu
 * and other respective copyright holders give permission to link this program
 * with OpenSSL, and distribute the resulting executable, without including
 * the source code for OpenSSL in the source distribution.
 */

/* Simply utility to create the proftpd shutdown message file, allowing
 * an admin to configure the shutdown, deny, disc and messages.
 *
 * Usage: ftpshut [ -l min ] [ -d min ] time [ warning-message ... ]
 */

#include "conf.h"

static void show_usage(char *progname)
{
  printf("usage: %s [ -l min ] [ -d min ] time [ warning-message ... ]\n",
         progname);

  exit(1);
}

static int isnumeric(char *str)
{
  while(str && isspace(*str))
    str++;
  
  if(!str || !*str)
    return 0;
  
  for(; str && *str; str++) {
    if(!isdigit(*str))
      return 0;
  }
  
  return 1;
}

int main(int argc, char *argv[])
{
  int deny = 10,disc = 5,c;
  FILE *outf;
  char *shut,*msg,*progname = argv[0];
  time_t now;
  struct tm *tm;
  int mn = 0,hr = 0;

  opterr = 0;

  while((c = getopt(argc,argv,"l:d:")) != -1) {
    switch(c) {
    case 'l':
    case 'd':
      if(!optarg) {
        fprintf(stderr,"%s: -%c requires an argument.\n", progname, c);
        show_usage(progname);
      }

      if(!isnumeric(optarg)) {
	fprintf(stderr, "%s: -%c requires a numeric argument.\n",
		progname, c);
	show_usage(progname);
      }
      
      if(c == 'd')
	disc = atoi(optarg);
      else if(c == 'l')
	deny = atoi(optarg);
      
      break;
      
    case '?':
      fprintf(stderr,"%s: unknown option '%c'.\n",progname,(char)optopt);
      
    case 'h':
    default:
      show_usage(progname);
    }
  }

  /* everything left on the command line is the message */
  if(optind >= argc)
    show_usage(progname);

  shut = argv[optind++];

  if(optind < argc)
    msg = argv[optind];
  else
    msg = "going down at %s";

  time(&now);
  tm = localtime(&now);

  /* shut must be either 'now', '+number' or 'HHMM' */
  if(strcasecmp(shut,"now") != 0) {
    if(*shut == '+') {
      shut++;
      while(shut && *shut && isspace((UCHAR)*shut)) shut++;

      if(!isnumeric(shut)) {
	fprintf(stderr, "%s: Invalid time interval specified.\n", progname);
	show_usage(progname);
      }
      
      now += (60 * atoi(shut));
      tm = localtime(&now);
    } else {
      if((strlen(shut) != 4 && strlen(shut) != 2) || !isnumeric(shut)) {
	fprintf(stderr, "%s: Invalid time interval specified.\n", progname);
	show_usage(progname);
      }

      if(strlen(shut) > 2) {
        mn = atoi((shut + strlen(shut) - 2));

	if(mn > 59) {
	  fprintf(stderr, "%s: Invalid time interval specified.\n",
		  progname);
	  show_usage(progname);
	}

        *(shut + strlen(shut) - 2) = '\0';
      }

      hr = atoi(shut);

      if(hr > 23) {
	fprintf(stderr, "%s: Invalid time interval specified.\n",
		progname);
	show_usage(progname);
      }
      
      if(hr < tm->tm_hour || (hr == tm->tm_hour && mn <= tm->tm_min)) {
        now += 86400;		/* one day forward */
        tm = localtime(&now);
      }
      tm->tm_hour = hr;
      tm->tm_min = mn;
    }
  }

  umask(022);
  if((outf = fopen(SHUTMSG_PATH,"w")) == NULL) {
    fprintf(stderr,"%s: %s: %s\n",progname,
            SHUTMSG_PATH,strerror(errno));
    exit(1);
  }

  fprintf(outf,"%d %d %d %d %d %d",
          tm->tm_year+1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
          tm->tm_min, tm->tm_sec);
  fprintf(outf," %02d%02d %02d%02d\n",
          (deny / 60),(deny % 60),
          (disc / 60),(disc % 60));
  fprintf(outf,"%s\n",msg);
  fclose(outf);
  return 0;
}
