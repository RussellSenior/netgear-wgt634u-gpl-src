#ifdef SMB_PASSWD

/*
 * Unix SMB/Netbios implementation. Version 1.9. smbpasswd module. Copyright
 * (C) Jeremy Allison 1995.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"
#include "des.h"

/* Static buffers we will return. */
static struct smb_passwd pw_buf;
static pstring  user_name;
static unsigned char smbpwd[16];
static unsigned char smbntpwd[16];

static int gethexpwd(char *p, char *pwd)
{
	int i;
	unsigned char   lonybble, hinybble;
	char           *hexchars = "0123456789ABCDEF";
	char           *p1, *p2;
	for (i = 0; i < 32; i += 2) {
		hinybble = toupper(p[i]);
		lonybble = toupper(p[i + 1]);

		p1 = strchr(hexchars, hinybble);
		p2 = strchr(hexchars, lonybble);
		if (!p1 || !p2)
			return (False);

		hinybble = PTR_DIFF(p1, hexchars);
		lonybble = PTR_DIFF(p2, hexchars);

		pwd[i / 2] = (hinybble << 4) | lonybble;
	}
	return (True);
}

struct smb_passwd *
_my_get_smbpwnam(FILE * fp, char *name, BOOL * valid_old_pwd, 
		BOOL *got_valid_nt_entry, long *pwd_seekpos)
{
	char            linebuf[256];
	unsigned char   c;
	unsigned char  *p;
	long            uidval;
	long            linebuf_len;

	/*
	 * Scan the file, a line at a time and check if the name matches.
	 */
	while (!feof(fp)) {
		linebuf[0] = '\0';
		*pwd_seekpos = ftell(fp);

		fgets(linebuf, 256, fp);
		if (ferror(fp))
			return NULL;

		/*
		 * Check if the string is terminated with a newline - if not
		 * then we must keep reading and discard until we get one.
		 */
		linebuf_len = strlen(linebuf);
		if (linebuf[linebuf_len - 1] != '\n') {
			c = '\0';
			while (!ferror(fp) && !feof(fp)) {
				c = fgetc(fp);
				if (c == '\n')
					break;
			}
		} else
			linebuf[linebuf_len - 1] = '\0';

		if ((linebuf[0] == 0) && feof(fp))
			break;
		/*
		 * The line we have should be of the form :-
		 * 
		 * username:uid:[32hex bytes]:....other flags presently
		 * ignored....
		 * 
		 * or,
		 * 
		 * username:uid:[32hex bytes]:[32hex bytes]:....ignored....
		 * 
		 * if Windows NT compatible passwords are also present.
		 */

		if (linebuf[0] == '#' || linebuf[0] == '\0')
			continue;
		p = (unsigned char *) strchr(linebuf, ':');
		if (p == NULL)
			continue;
		/*
		 * As 256 is shorter than a pstring we don't need to check
		 * length here - if this ever changes....
		 */
		strncpy(user_name, linebuf, PTR_DIFF(p, linebuf));
		user_name[PTR_DIFF(p, linebuf)] = '\0';
		if (!strequal(user_name, name))
			continue;

		/* User name matches - get uid and password */
		p++;		/* Go past ':' */
		if (!isdigit(*p))
			return (False);

		uidval = atoi((char *) p);
		while (*p && isdigit(*p))
			p++;

		if (*p != ':')
			return (False);

		/*
		 * Now get the password value - this should be 32 hex digits
		 * which are the ascii representations of a 16 byte string.
		 * Get two at a time and put them into the password.
		 */
		p++;
		*pwd_seekpos += PTR_DIFF(p, linebuf);	/* Save exact position
							 * of passwd in file -
							 * this is used by
							 * smbpasswd.c */
		if (*p == '*' || *p == 'X') {
			/* Password deliberately invalid - end here. */
			*valid_old_pwd = False;
			*got_valid_nt_entry = False;
			pw_buf.smb_nt_passwd = NULL;	/* No NT password (yet)*/

			/* Now check if the NT compatible password is
			   available. */
			p += 33; /* Move to the first character of the line after 
						the lanman password. */
			if ((linebuf_len >= (PTR_DIFF(p, linebuf) + 33)) && (p[32] == ':')) {
				/* NT Entry was valid - even if 'X' or '*', can be overwritten */
				*got_valid_nt_entry = True;
				if (*p != '*' && *p != 'X') {
				  if (gethexpwd((char *)p,(char *)smbntpwd))
				    pw_buf.smb_nt_passwd = smbntpwd;
				}
			}
			pw_buf.smb_name = user_name;
			pw_buf.smb_userid = uidval;
			pw_buf.smb_passwd = NULL;	/* No password */
			return (&pw_buf);
		}
		if (linebuf_len < (PTR_DIFF(p, linebuf) + 33))
			return (False);

		if (p[32] != ':')
			return (False);

		if (!strncasecmp((char *)p, "NO PASSWORD", 11)) {
		  pw_buf.smb_passwd = NULL;	/* No password */
		} else {
		  if(!gethexpwd((char *)p,(char *)smbpwd))
		    return False;
		  pw_buf.smb_passwd = smbpwd;
		}

		pw_buf.smb_name = user_name;
		pw_buf.smb_userid = uidval;
		pw_buf.smb_nt_passwd = NULL;
		*got_valid_nt_entry = False;
		*valid_old_pwd = True;

		/* Now check if the NT compatible password is
		   available. */
		p += 33; /* Move to the first character of the line after 
					the lanman password. */
		if ((linebuf_len >= (PTR_DIFF(p, linebuf) + 33)) && (p[32] == ':')) {
			/* NT Entry was valid - even if 'X' or '*', can be overwritten */
			*got_valid_nt_entry = True;
			if (*p != '*' && *p != 'X') {
			  if (gethexpwd((char *)p,(char *)smbntpwd))
			    pw_buf.smb_nt_passwd = smbntpwd;
			}
		}
		return &pw_buf;
	}
	return NULL;
}

/*
 * Print command usage on stderr and die.
 */
static void usage(char *name)
{
	fprintf(stderr, "Usage is : %s [username] <password>\n", name);
	exit(1);
}

 int main(int argc, char **argv)
{
  int             real_uid;
  struct passwd  *pwd;
  fstring         old_passwd;
  uchar           old_p16[16];
  uchar           old_nt_p16[16];
  fstring         new_passwd;
  uchar           new_p16[16];
  uchar           new_nt_p16[16];
  char           *p;
  struct smb_passwd *smb_pwent;
  FILE           *fp;
  BOOL            valid_old_pwd = False;
  BOOL 			got_valid_nt_entry = False;
  long            seekpos;
  int             pwfd;
  char            ascii_p16[66];
  char            c;
  int             ret, i, err, writelen;
  int             lockfd = -1;
  char           *pfile = SMB_PASSWD_FILE;
  char            readbuf[16 * 1024];
  FILE * passwd_fd;
  FILE * smbpasswd_fd;
  BOOL user_found = False;
  int smbpasswd_hd;
  
  TimeInit();

  setup_logging(argv[0],True);
  
  charset_initialise();
  
#ifndef DEBUG_PASSWORD
  /* Check the effective uid */
  if (geteuid() != 0) {
    fprintf(stderr, "%s: Must be setuid root.\n", argv[0]);
    exit(1);
  }
#endif
  
  /* Get the real uid */
  real_uid = getuid();
  
  /* Deal with usage problems */
  if (real_uid == 0) {
    /* As root we can change anothers password. */
    if (argc != 2 && argc != 3)
      usage(argv[0]);
  } else if (argc != 2)
    usage(argv[0]);
  
  
  if (real_uid == 0 && argc == 3) {
    /* If we are root we can change anothers password. */
    strncpy(user_name, argv[1], sizeof(user_name) - 1);
    user_name[sizeof(user_name) - 1] = '\0';
    pwd = getpwnam(user_name);
  } else {
    pwd = getpwuid(real_uid);
  }

  if (pwd == 0) {
    fprintf(stderr, "%s: Unable to get UNIX password entry for user.\n", argv[0]);
    exit(1);
  }
  
  smbpasswd_hd = open(pfile, O_RDONLY|O_CREAT);
  close(smbpasswd_hd);

  if ((passwd_fd = fopen("/etc/passwd", "r")) == NULL) {
    err = errno;
    fprintf(stderr, "%s: Failed to open password file %s.\n",
	    argv[0], "/etc/passwd");
    errno = err;
    perror(argv[0]);
    exit(err);
  }

  if ((smbpasswd_fd = fopen(pfile, "r+")) == NULL) {
    err = errno;
    fprintf(stderr, "%s: Failed to open password file %s.\n",
	    argv[0], pfile);
    errno = err;
    perror(argv[0]);
    exit(err);
  }

  /* try to find the user in smbpasswd file */
  while (1) {
    char tmpbuf[50];
    char * tmpptr;

    tmpptr = fgets(readbuf, 255, smbpasswd_fd);
    if (tmpptr != NULL) {
      strcpy(tmpbuf, user_name);
      tmpptr = strstr(readbuf, strcat(tmpbuf, ":"));
      if (tmpptr == NULL) 
	continue;
      else { 
	if (strncmp(readbuf, tmpbuf, strlen(tmpbuf)))
	  continue;
	else {
	  printf("Found user:%s\n", user_name);
	  user_found = True;
	  fclose(smbpasswd_fd);
	  break;
	}
      }
    }
    else {
      printf("Did not find entry of user: %s in %s\n", user_name, pfile);
      break;
    }
  }

  while (user_found == False) {
    char * tmpptr;
    char * val;
    char tmpbuf[50];
    char uid_buf[10];
    char user_desc[50];

    tmpptr = fgets(readbuf, 255, passwd_fd);
    if (tmpptr == NULL) {
      printf("Could not find the entry of user: %s\n", user_name);
      fclose(passwd_fd);
      break;
    }

    strcpy(tmpbuf, user_name);
    tmpptr = strstr(readbuf, strcat(tmpbuf, ":"));
    if (tmpptr == NULL) 
      continue;
    else { /* found the user in etc/passwd */
      if (strncmp(readbuf, tmpbuf, strlen(tmpbuf)))
	  continue;
      val = tmpptr;
      while ((*val) != ':') 
	val++;
      val++;
      while ((*val) != ':')
	val++;
      val++;
      tmpptr = val;
      while ((*val) != ':')
	val++;
      strncpy(uid_buf, tmpptr, (val - tmpptr));
      uid_buf[val - tmpptr] = '\0';
      printf("%s\n", uid_buf);
      val++;
      while ((*val) != ':') 
	val++;
      val++;
      tmpptr = val;
      while ((*val) != ':') 
	val++;
      strncpy(tmpbuf, tmpptr, (val - tmpptr));
      tmpbuf[val - tmpptr] = '\0';
      strcpy(user_desc, tmpbuf);
      printf("%s\n", user_desc);
      fseek(smbpasswd_fd, 0, SEEK_END);
      fprintf(smbpasswd_fd,"%s:%s:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:[U          ]:LCT-00000000:%s\n", user_name, uid_buf, user_desc );
      fclose(smbpasswd_fd);
      fclose(passwd_fd);
      break;
    }
  }

  /* If we are root we don't ask for the old password. */
  old_passwd[0] = '\0';
  if (real_uid != 0) {
    p = getpass("Old SMB password:");
    strncpy(old_passwd, p, sizeof(fstring));
    old_passwd[sizeof(fstring)-1] = '\0'; 
  }
  new_passwd[0] = '\0';
  //  p = getpass("New SMB password:");

  if (argc == 3)
    p = argv[2];
  else
    p = argv[1];

  strncpy(new_passwd, p, sizeof(fstring));
  new_passwd[sizeof(fstring)-1] = '\0';
/*  p = getpass("Retype new SMB password:");
  if (strcmp(p, new_passwd)) {
    fprintf(stderr, "%s: Mismatch - password unchanged.\n", argv[0]);
    exit(1);
  }*/
  
  if (new_passwd[0] == '\0') {
    printf("Password not set\n");
    exit(0);
  }
  
  /* Calculate the MD4 hash (NT compatible) of the old and new passwords */
  memset(old_nt_p16, '\0', 16);
  E_md4hash((uchar *)old_passwd, old_nt_p16);
  
  memset(new_nt_p16, '\0', 16);
  E_md4hash((uchar *) new_passwd, new_nt_p16);
  
  /* Mangle the passwords into Lanman format */
  old_passwd[14] = '\0';
  strupper(old_passwd);
  new_passwd[14] = '\0';
  strupper(new_passwd);
  
  /*
   * Calculate the SMB (lanman) hash functions of both old and new passwords.
   */
  
  memset(old_p16, '\0', 16);
  E_P16((uchar *) old_passwd, old_p16);
  
  memset(new_p16, '\0', 16);
  E_P16((uchar *) new_passwd, new_p16);
  
  /*
   * Open the smbpaswd file XXXX - we need to parse smb.conf to get the
   * filename
   */
  if ((fp = fopen(pfile, "r+")) == NULL) {
    err = errno;
    fprintf(stderr, "%s: Failed to open password file %s.\n",
	    argv[0], pfile);
    errno = err;
    perror(argv[0]);
    exit(err);
  }
  /* Set read buffer to 16k for effiecient reads */
  setvbuf(fp, readbuf, _IOFBF, sizeof(readbuf));
  
  /* make sure it is only rw by the owner */
  chmod(pfile, 0600);
  
  /* Lock the smbpasswd file for write. */
  if ((lockfd = pw_file_lock(pfile, F_WRLCK, 5)) < 0) {
    err = errno;
    fprintf(stderr, "%s: Failed to lock password file %s.\n",
	    argv[0], pfile);
    fclose(fp);
    errno = err;
    perror(argv[0]);
    exit(err);
  }
  /* Get the smb passwd entry for this user */
  smb_pwent = _my_get_smbpwnam(fp, pwd->pw_name, &valid_old_pwd, 
			       &got_valid_nt_entry, &seekpos);
  if (smb_pwent == NULL) {
    fprintf(stderr, "%s: Failed to find entry for user %s in file %s.\n",
	    argv[0], pwd->pw_name, pfile);

    fclose(fp);
    pw_file_unlock(lockfd);
    exit(1);
  }
  /* If we are root we don't need to check the old password. */
  if (real_uid != 0) {
    if ((valid_old_pwd == False) || (smb_pwent->smb_passwd == NULL)) {
      fprintf(stderr, "%s: User %s is disabled, plase contact your administrator to enable it.\n", argv[0], pwd->pw_name);
      fclose(fp);
      pw_file_unlock(lockfd);
      exit(1);
    }
    /* Check the old Lanman password */
    if (memcmp(old_p16, smb_pwent->smb_passwd, 16)) {
      fprintf(stderr, "%s: Couldn't change password.\n", argv[0]);
      fclose(fp);
      pw_file_unlock(lockfd);
      exit(1);
    }
    /* Check the NT password if it exists */
    if (smb_pwent->smb_nt_passwd != NULL) {
      if (memcmp(old_nt_p16, smb_pwent->smb_nt_passwd, 16)) {
	fprintf(stderr, "%s: Couldn't change password.\n", argv[0]);
	fclose(fp);
	pw_file_unlock(lockfd);
	exit(1);
      }
    }
  }
  /*
   * If we get here either we were root or the old password checked out
   * ok.
   */
  /* Create the 32 byte representation of the new p16 */
  for (i = 0; i < 16; i++) {
    sprintf(&ascii_p16[i * 2], "%02X", (uchar) new_p16[i]);
  }
  if(got_valid_nt_entry) {
    /* Add on the NT md4 hash */
    ascii_p16[32] = ':';
    for (i = 0; i < 16; i++) {
      sprintf(&ascii_p16[(i * 2)+33], "%02X", (uchar) new_nt_p16[i]);
    }
  }
  /*
   * Do an atomic write into the file at the position defined by
   * seekpos.
   */
  pwfd = fileno(fp);
  ret = lseek(pwfd, seekpos - 1, SEEK_SET);
  if (ret != seekpos - 1) {
    err = errno;
    fprintf(stderr, "%s: seek fail on file %s.\n",
	    argv[0], pfile);
    fclose(fp);
    errno = err;
    perror(argv[0]);
    pw_file_unlock(lockfd);
    exit(1);
  }
  /* Sanity check - ensure the character is a ':' */
  if (read(pwfd, &c, 1) != 1) {
    err = errno;
    fprintf(stderr, "%s: read fail on file %s.\n",
	    argv[0], pfile);
    fclose(fp);
    errno = err;
    perror(argv[0]);
    pw_file_unlock(lockfd);
    exit(1);
  }
  if (c != ':') {
    fprintf(stderr, "%s: sanity check on passwd file %s failed.\n",
	    argv[0], pfile);
    fclose(fp);
    pw_file_unlock(lockfd);
    exit(1);
  }
  writelen = (got_valid_nt_entry) ? 65 : 32;
  if (write(pwfd, ascii_p16, writelen) != writelen) {
    err = errno;
    fprintf(stderr, "%s: write fail in file %s.\n",
	    argv[0], pfile);
    fclose(fp);
    errno = err;
    perror(argv[0]);
    pw_file_unlock(lockfd);
    exit(err);
  }
  fclose(fp);
  pw_file_unlock(lockfd);
  printf("Password changed\n");
  return 0;
}

#else

#include "includes.h"

int 
main(int argc, char **argv)
{
  printf("smb password encryption not selected in Makefile\n");
  return 0;
}
#endif
