/* 
   Unix SMB/Netbios implementation.
   Version 1.9.
   Password and authentication handling
   Copyright (C) Andrew Tridgell 1992-1995
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

extern int DEBUGLEVEL;
extern int Protocol;

/* users from session setup */
static pstring session_users="";

/* these are kept here to keep the string_combinations function simple */
static char this_user[100]="";
static char this_salt[100]="";
static char this_crypted[100]="";

#ifdef SMB_PASSWD
/* Data to do lanman1/2 password challenge. */
static unsigned char saved_challenge[8];
static BOOL challenge_sent=False;

/*******************************************************************
Get the next challenge value - no repeats.
********************************************************************/
void generate_next_challenge(char *challenge)
{
  static int counter = 0;
  struct timeval tval;
  int v1,v2;
  GetTimeOfDay(&tval);
  v1 = (counter++) + getpid() + tval.tv_sec;
  v2 = (counter++) * getpid() + tval.tv_usec;
  SIVAL(challenge,0,v1);
  SIVAL(challenge,4,v2);
  E1(challenge,"SAMBA",(char *)saved_challenge);
  memcpy(challenge,saved_challenge,8);
  challenge_sent = True;
}

/*******************************************************************
set the last challenge sent, usually from a password server
********************************************************************/
BOOL set_challenge(char *challenge)
{
  memcpy(saved_challenge,challenge,8);
  challenge_sent = True;
  return(True);
}

/*******************************************************************
get the last challenge sent
********************************************************************/
BOOL last_challenge(char *challenge)
{
  if (!challenge_sent) return(False);
  memcpy(challenge,saved_challenge,8);
  return(True);
}
#endif

/* this holds info on user ids that are already validated for this VC */
static user_struct *validated_users = NULL;
static int num_validated_users = 0;

/****************************************************************************
check if a uid has been validated, and return an index if it has. -1 if not
****************************************************************************/
int valid_uid(int uid)
{
  int i;
  if (uid == -1) return(-1);

  for (i=0;i<num_validated_users;i++)
    if (validated_users[i].uid == uid)
      {
	DEBUG(3,("valid uid %d mapped to vuid %d (user=%s)\n",
		 uid,i,validated_users[i].name));
	return(i);
      }
  return(-1);
}

/****************************************************************************
check if a uid has been validated, and return an pointer to the user_struct
if it has. NULL if not
****************************************************************************/
user_struct *get_valid_user_struct(int uid)
{
  int vuid = valid_uid(uid);
  if(vuid == -1 || validated_users[vuid].guest)
    return NULL;
  return &validated_users[vuid];
}

/****************************************************************************
invalidate a uid
****************************************************************************/
void invalidate_uid(int uid)
{
  int i;
  for (i=0;i<num_validated_users;i++)
    if (validated_users[i].uid == uid)
      {
	user_struct *vuser = &validated_users[i];
	vuser->uid = -1;
	vuser->gid = -1;
	vuser->user_ngroups = 0;
	if(vuser->user_groups && 
	   (vuser->user_groups != (gid_t *)vuser->user_igroups))
	  free(vuser->user_groups);
	vuser->user_groups = NULL;
	if(vuser->user_igroups)
	  free(vuser->user_igroups);
	vuser->user_igroups = NULL;
      }
}


/****************************************************************************
return a validated username
****************************************************************************/
char *validated_username(int vuid)
{
  return(validated_users[vuid].name);
}

/****************************************************************************
register a uid/name pair as being valid and that a valid password
has been given.
****************************************************************************/
void register_uid(int uid,int gid, char *name,BOOL guest)
{
  user_struct *vuser;

  if (valid_uid(uid) >= 0)
    return;
  validated_users = (user_struct *)Realloc(validated_users,
					   sizeof(user_struct)*
					   (num_validated_users+1));

  if (!validated_users)
    {
      DEBUG(0,("Failed to realloc users struct!\n"));
      return;
    }

  vuser = &validated_users[num_validated_users];
  vuser->uid = uid;
  vuser->gid = gid;
  vuser->guest = guest;
  strcpy(vuser->name,name);

  vuser->user_ngroups = 0;
  vuser->user_groups = NULL;
  vuser->user_igroups = NULL;

  /* Find all the groups this uid is in and store them. 
     Used by become_user() */
  setup_groups(name,uid,gid,
	       &vuser->user_ngroups,
	       &vuser->user_igroups,
	       &vuser->user_groups);

  DEBUG(3,("uid %d registered to name %s\n",uid,name));
  
  num_validated_users++;
}


/****************************************************************************
add a name to the session users list
****************************************************************************/
void add_session_user(char *user)
{
  fstring suser;
  StrnCpy(suser,user,sizeof(suser)-1);

  if (!Get_Pwnam(suser,True)) return;

  if (suser && *suser && !in_list(suser,session_users,False))
    {
      if (strlen(suser) + strlen(session_users) + 2 >= sizeof(pstring))
	DEBUG(1,("Too many session users??\n"));
      else
	{
	  strcat(session_users," ");
	  strcat(session_users,suser);
	}
    }
}


#ifdef NO_GETSPNAM
/* a fake shadow password routine which just fills a fake spwd struct
 * with the sp_pwdp field. (sreiz@aie.nl)
 */
static struct spwd *getspnam(char *username) /* fake shadow password routine */
{
       FILE *f;
       char line[1024];
       static char pw[20];
       static struct spwd static_spwd;

       static_spwd.sp_pwdp=0;
       if (!(f=fopen("/etc/master.passwd", "r")))
               return 0;
       while (fgets(line, 1024, f)) {
               if (!strncmp(line, username, strlen(username)) &&
                line[strlen(username)]==':') { /* found entry */
                       char *p, *q;

                       p=line+strlen(username)+1;
                       if ((q=strchr(p, ':'))) {
                               *q=0;
                               if (q-p+1>20)
                                       break;
                               strcpy(pw, p);
                               static_spwd.sp_pwdp=pw;
                       }
                       break;
               }
       }
       fclose(f);
       if (static_spwd.sp_pwdp)
               return &static_spwd;
       return 0;
}
#endif


#ifdef OSF1_ENH_SEC
/****************************************************************************
an enhanced crypt for OSF1
****************************************************************************/
static char *osf1_bigcrypt(char *password,char *salt1)
{
  static char result[AUTH_MAX_PASSWD_LENGTH] = "";
  char *p1;
  char *p2=password;
  char salt[3];
  int i;
  int parts = strlen(password) / AUTH_CLEARTEXT_SEG_CHARS;
  if (strlen(password)%AUTH_CLEARTEXT_SEG_CHARS)
    parts++;

  StrnCpy(salt,salt1,2);
  StrnCpy(result,salt1,2);

  for (i=0; i<parts;i++)
    {
      p1 = crypt(p2,salt);
      strcat(result,p1+2);
      StrnCpy(salt,&result[2+i*AUTH_CIPHERTEXT_SEG_CHARS],2);
      p2 += AUTH_CLEARTEXT_SEG_CHARS;
    }

  return(result);
}
#endif


/****************************************************************************
update the enhanced security database. Only relevant for OSF1 at the moment.
****************************************************************************/
static void update_protected_database( char *user, BOOL result)
{
#ifdef OSF1_ENH_SEC
  struct pr_passwd *mypasswd;
  time_t starttime;

  mypasswd = getprpwnam (user);
  starttime = time (NULL);

  if (result)
    {
      mypasswd->ufld.fd_slogin = starttime;
      mypasswd->ufld.fd_nlogins = 0;
      
      putprpwnam(user,mypasswd);
      
      DEBUG(3,("Update protected database for Account %s after succesful connection\n",user));
    }
  else
    {
      mypasswd->ufld.fd_ulogin = starttime;
      mypasswd->ufld.fd_nlogins = mypasswd->ufld.fd_nlogins + 1;
      if ( mypasswd->ufld.fd_max_tries != 0 && mypasswd->ufld.fd_nlogins > mypasswd->ufld.fd_max_tries )
	{
	  mypasswd->uflg.fg_lock = 0;
	  DEBUG(3,("Account is disabled -- see Account Administrator.\n"));
	}
      putprpwnam ( user , mypasswd );
      DEBUG(3,("Update protected database for Account %s after refusing connection\n",user));
    }
#else
  DEBUG(6,("Updated database with %s %s\n",user,BOOLSTR(result)));
#endif
}


#ifdef AFS_AUTH
/*******************************************************************
check on AFS authentication
********************************************************************/
static BOOL afs_auth(char *this_user,char *password)
{
  long password_expires = 0;
  char *reason;
    
  /* For versions of AFS prior to 3.3, this routine has few arguments, */
  /* but since I can't find the old documentation... :-)               */
  setpag();
  if (ka_UserAuthenticateGeneral(KA_USERAUTH_VERSION+KA_USERAUTH_DOSETPAG,
				 this_user,
				 (char *) 0, /* instance */
				 (char *) 0, /* cell */
				 password,
				 0,          /* lifetime, default */
				 &password_expires, /*days 'til it expires */
				 0,          /* spare 2 */
				 &reason) == 0)
    return(True);
  return(False);
}
#endif


#ifdef DFS_AUTH

sec_login_handle_t my_dce_sec_context;
int dcelogin_atmost_once = 0;

/*******************************************************************
check on a DCE/DFS authentication
********************************************************************/
static BOOL dfs_auth(char *this_user,char *password)
{
  error_status_t err;
  int err2;
  int prterr;
  boolean32 password_reset;
  sec_passwd_rec_t my_dce_password;
  sec_login_auth_src_t auth_src = sec_login_auth_src_network;
  unsigned char dce_errstr[dce_c_error_string_len];

  /*
   * We only go for a DCE login context if the given password
   * matches that stored in the local password file.. 
   * Assumes local passwd file is kept in sync w/ DCE RGY!
   */

  if (!strcmp((char *)crypt(password,this_salt),this_crypted) ||
      dcelogin_atmost_once)
    return(False);

  if (sec_login_setup_identity(
			       (unsigned char *)this_user,
			       sec_login_no_flags,
			       &my_dce_sec_context,
			       &err) == 0)
    {
      dce_error_inq_text(err, dce_errstr, &err2);
      DEBUG(0,("DCE Setup Identity for %s failed: %s\n",
	       this_user,dce_errstr));
      return(False);
    }

  my_dce_password.version_number = sec_passwd_c_version_none;
  my_dce_password.pepper = NULL; 
  my_dce_password.key.key_type = sec_passwd_plain;
  my_dce_password.key.tagged_union.plain  = (idl_char *)password;
  
  if (sec_login_valid_and_cert_ident(my_dce_sec_context,
				     &my_dce_password,
				     &password_reset,
				     &auth_src,
				     &err) == 0 )
    { 
      dce_error_inq_text(err, dce_errstr, &err2);
      DEBUG(0,("DCE Identity Validation failed for principal %s: %s\n",
	       this_user,dce_errstr));
	  
      return(False);
    }

  sec_login_set_context(my_dce_sec_context, &err);
  if (err != error_status_ok )
    {  
      dce_error_inq_text(err, dce_errstr, &err2);
      DEBUG(0,("DCE login failed for principal %s, cant set context: %s\n",
	       this_user,dce_errstr));
      sec_login_purge_context(my_dce_sec_context, &err);
      return(False);
    }
  else
    {
      DEBUG(0,("DCE login succeeded for principal %s on pid %d\n",
	       this_user, getpid()));
    }

  dcelogin_atmost_once = 1;
  return (True);
}

void dfs_unlogin(void)
{
  error_status_t err;
  int err2;
  unsigned char dce_errstr[dce_c_error_string_len];

  sec_login_purge_context(my_dce_sec_context, &err);
  if (err != error_status_ok )
    {  
      dce_error_inq_text(err, dce_errstr, &err2);
      DEBUG(0,("DCE purge login context failed for server instance %d: %s\n",
	       getpid(), dce_errstr));
    }
}

#endif


#ifdef LINUX_BIGCRYPT
/****************************************************************************
an enhanced crypt for Linux to handle password longer than 8 characters
****************************************************************************/
static int linux_bigcrypt(char *password,char *salt1, char *crypted)
{
#define LINUX_PASSWORD_SEG_CHARS 8
  char salt[3];
  int i;
  
  StrnCpy(salt,salt1,2);
  crypted +=2;
  
  for ( i=strlen(password); i > 0; i -= LINUX_PASSWORD_SEG_CHARS) {
    char * p = crypt(password,salt) + 2;
    if(strncmp(p, crypted, LINUX_PASSWORD_SEG_CHARS) != 0)
      return(0);
    password += LINUX_PASSWORD_SEG_CHARS;
    crypted  += strlen(p);
  }
  
  return(1);
}
#endif


/****************************************************************************
apply a function to upper/lower case combinations
of a string and return true if one of them returns true.
try all combinations with N uppercase letters.
offset is the first char to try and change (start with 0)
it assumes the string starts lowercased
****************************************************************************/
static BOOL string_combinations2(char *s,int offset,BOOL (*fn)(),int N)
{
  int len = strlen(s);
  int i;

#ifdef PASSWORD_LENGTH
  len = MIN(len,PASSWORD_LENGTH);
#endif

  if (N <= 0 || offset >= len)
    return(fn(s));

  for (i=offset;i<(len-(N-1));i++)
    {      
      char c = s[i];
      if (!islower(c)) continue;
      s[i] = toupper(c);
      if (string_combinations2(s,i+1,fn,N-1))
	return(True);
      s[i] = c;
    }
  return(False);
}

/****************************************************************************
apply a function to upper/lower case combinations
of a string and return true if one of them returns true.
try all combinations with up to N uppercase letters.
offset is the first char to try and change (start with 0)
it assumes the string starts lowercased
****************************************************************************/
static BOOL string_combinations(char *s,BOOL (*fn)(),int N)
{
  int n;
  for (n=1;n<=N;n++)
    if (string_combinations2(s,0,fn,n)) return(True);
  return(False);
}



/****************************************************************************
core of password checking routine
****************************************************************************/
BOOL password_check(char *password)
{
#ifdef AFS_AUTH
  if (afs_auth(this_user,password)) return(True);
#endif

#ifdef DFS_AUTH
  if (dfs_auth(this_user,password)) return(True);
#endif 

#ifdef PWDAUTH
  if (pwdauth(this_user,password) == 0)
    return(True);
#endif

#ifdef OSF1_ENH_SEC
  return(strcmp(osf1_bigcrypt(password,this_salt),this_crypted) == 0);
#endif

#ifdef ULTRIX_AUTH
  return (strcmp((char *)crypt16(password, this_salt ),this_crypted) == 0);
#endif

#ifdef LINUX_BIGCRYPT
  return(linux_bigcrypt(password,this_salt,this_crypted));
#endif

#ifdef NO_CRYPT
  DEBUG(1,("Warning - no crypt available\n"));
  return(False);
#else
 {
   this_salt[3]='\0';
   this_salt[4]='\0';
   return(strcmp((char *)crypt(password,this_salt),this_crypted) == 0);
 }
#endif
}

#ifdef SMB_PASSWD
/****************************************************************************
core of smb password checking routine.
****************************************************************************/
BOOL smb_password_check(char *password, unsigned char *part_passwd, unsigned char *c8)
{
  /* Finish the encryption of part_passwd. */
  unsigned char p21[21];
  unsigned char p24[24];

  if(part_passwd == NULL)
    DEBUG(10,("No password set - allowing access\n"));
  /* No password set - always true ! */
  if(part_passwd == NULL)
    return 1;

  memset(p21,'\0',21);
  memcpy(p21,part_passwd,16);
  E_P24(p21, c8, p24);
#if DEBUG_PASSWORD
  {
    int i;
    DEBUG(100,("Part password (P16) was |"));
    for(i = 0; i < 16; i++)
      DEBUG(100,("%X ", (unsigned char)part_passwd[i]));
    DEBUG(100,("|\n"));
    DEBUG(100,("Password from client was |"));
    for(i = 0; i < 24; i++)
      DEBUG(100,("%X ", (unsigned char)password[i]));
    DEBUG(100,("|\n"));
    DEBUG(100,("Given challenge was |"));
    for(i = 0; i < 8; i++)
      DEBUG(100,("%X ", (unsigned char)c8[i]));
    DEBUG(100,("|\n"));
    DEBUG(100,("Value from encryption was |"));
    for(i = 0; i < 24; i++)
      DEBUG(100,("%X ", (unsigned char)p24[i]));
    DEBUG(100,("|\n"));
  }
#endif
  return (memcmp(p24, password, 24) == 0);
}
#endif

/****************************************************************************
check if a username/password is OK
****************************************************************************/
BOOL password_ok(char *user,char *password, int pwlen, struct passwd *pwd, BOOL is_nt_password)
{
  pstring pass2;
  int level = lp_passwordlevel();
  struct passwd *pass;
#ifdef SMB_PASSWD
  char challenge[8];
  struct smb_passwd *smb_pass;
  BOOL challenge_done = False;
#endif

  if (password) password[pwlen] = 0;

#ifdef SMB_PASSWD
  if (pwlen == 24)
    challenge_done = last_challenge(challenge);
#endif

#if DEBUG_PASSWORD
#ifdef SMB_PASSWD
  if (challenge_done)
    {
      int i;      
      DEBUG(100,("checking user=[%s] pass=[",user));
      for( i = 0; i < 24; i++)
	DEBUG(100,("%0x ", (unsigned char)password[i]));
      DEBUG(100,("]\n"));
    }
  else
#endif
    DEBUG(100,("checking user=[%s] pass=[%s]\n",user,password));
#endif

  if (!password)
    return(False);

  if (((!*password) || (!pwlen)) && !lp_null_passwords())
    return(False);

  if (pwd && !user) 
    {
      pass = (struct passwd *) pwd;
      user = pass->pw_name;
    } 
  else 
    pass = Get_Pwnam(user,True);

#ifdef SMB_PASSWD

  DEBUG(4,("SMB Password - pwlen = %d, challenge_done = %d\n", pwlen, challenge_done));

  if((pwlen == 24) && challenge_done)
    {
      DEBUG(4,("Checking SMB password for user %s (l=24)\n",user));

      if (!pass) 
	{
	  DEBUG(3,("Couldn't find user %s\n",user));
	  return(False);
	}

      smb_pass = get_smbpwnam(user);
      if(!smb_pass)
	{
	  DEBUG(3,("Couldn't find user %s in smb_passwd file.\n", user));
	  return(False);
	}

      /* Ensure the uid's match */
      if(smb_pass->smb_userid != pass->pw_uid)
	{
	  DEBUG(3,("Error : UNIX and SMB uids in password files do not match !\n"));
	  return(False);
	}

	if(Protocol >= PROTOCOL_NT1 && is_nt_password)
	{
		/* We have the NT MD4 hash challenge available - see if we can
		   use it (ie. does it exist in the smbpasswd file).
		*/
		if(smb_pass->smb_nt_passwd != NULL)
		{
		  DEBUG(4,("Checking NT MD4 password\n"));
		  if(smb_password_check(password, 
					smb_pass->smb_nt_passwd, 
					(char *)challenge))
   		  {
	      	update_protected_database(user,True);
	        return(True);
    	  }
		  DEBUG(4,("NT MD4 password check failed\n"));
		  return (False);
		}
	}

	/* Try against the lanman password */

      if (smb_password_check(password, 
			     smb_pass->smb_passwd,
			     (char *)challenge)) {
	update_protected_database(user,True);
	return(True);
      }

	DEBUG(3,("Error smb_password_check failed\n"));
    }
#endif 

  DEBUG(4,("Checking password for user %s (l=%d)\n",user,pwlen));

  if (!pass) 
    {
      DEBUG(3,("Couldn't find user %s\n",user));
      return(False);
    }

#ifdef SHADOW_PWD
  {
    struct spwd *spass;

    /* many shadow systems require you to be root to get the password,
       in most cases this should already be the case when this
       function is called, except perhaps for IPC password changing
       requests */

    spass = getspnam(pass->pw_name);
    if (spass && spass->sp_pwdp)
      pass->pw_passwd = spass->sp_pwdp;
  }
#endif

#ifdef SecureWare
  {
    struct pr_passwd *pr_pw = getprpwnam(pass->pw_name);
    if (pr_pw && pr_pw->ufld.fd_encrypt)
      pass->pw_passwd = pr_pw->ufld.fd_encrypt;
  }
#endif

#ifdef HPUX_10_TRUSTED
  {
    struct pr_passwd *pr_pw = getprpwnam(pass->pw_name);
    if (pr_pw && pr_pw->ufld.fd_encrypt)
      pass->pw_passwd = pr_pw->ufld.fd_encrypt;
  }
#endif

#ifdef OSF1_ENH_SEC
  {
    struct pr_passwd *mypasswd;
    DEBUG(5,("Checking password for user %s in OSF1_ENH_SEC\n",user));
    mypasswd = getprpwnam (user);
    if ( mypasswd )
      { 
  	strcpy(pass->pw_name,mypasswd->ufld.fd_name);
  	strcpy(pass->pw_passwd,mypasswd->ufld.fd_encrypt);
      }
    else
      {
	DEBUG(5,("No entry for user %s in protected database !\n",user));
	return(False);
      }
  }
#endif

#ifdef ULTRIX_AUTH
  {
    AUTHORIZATION *ap = getauthuid( pass->pw_uid );
    if (ap)
      {
	strcpy( pass->pw_passwd, ap->a_password );
	endauthent();
      }
  }
#endif

  /* extract relevant info */
  strcpy(this_user,pass->pw_name);  
  strcpy(this_salt,pass->pw_passwd);
  strcpy(this_crypted,pass->pw_passwd);
 
  if (!*this_crypted) {
    if (!lp_null_passwords()) {
      DEBUG(2,("Disallowing access to %s due to null password\n",this_user));
      return(False);
    }
#ifndef PWDAUTH
    if (!*password) {
      DEBUG(3,("Allowing access to %s with null password\n",this_user));
      return(True);
    }
#endif    
  }

  /* try it as it came to us */
  if (password_check(password))
    {
      update_protected_database(user,True);
      return(True);
    }

  /* if the password was given to us with mixed case then we don't
     need to proceed as we know it hasn't been case modified by the
     client */
  if (strhasupper(password) && strhaslower(password))
    return(False);

  /* make a copy of it */
  StrnCpy(pass2,password,sizeof(pstring)-1);
  
  /* try all lowercase */
  strlower(password);
  if (password_check(password))
    {
      update_protected_database(user,True);
      return(True);
    }

  /* give up? */
  if(level < 1)
    {
      update_protected_database(user,False);

      /* restore it */
      strcpy(password,pass2);

      return(False);
    }

  /* last chance - all combinations of up to level chars upper! */
  strlower(password);

  if (string_combinations(password,password_check,level))
    {
      update_protected_database(user,True);
      return(True);
    }

  update_protected_database(user,False);
  
  /* restore it */
  strcpy(password,pass2);
  
  return(False);
}



/****************************************************************************
check if a username is valid
****************************************************************************/
BOOL user_ok(char *user,int snum)
{
  pstring valid, invalid;
  BOOL ret;

  StrnCpy(valid, lp_valid_users(snum), sizeof(pstring));
  StrnCpy(invalid, lp_invalid_users(snum), sizeof(pstring));

  string_sub(valid,"%S",lp_servicename(snum));
  string_sub(invalid,"%S",lp_servicename(snum));

  ret = !user_in_list(user,invalid);

  if (ret && valid && *valid)
    ret = user_in_list(user,valid);

  if (ret && lp_onlyuser(snum)) {
    char *user_list = lp_username(snum);
    string_sub(user_list,"%S",lp_servicename(snum));
    ret = user_in_list(user,user_list);
  }

  return(ret);
}




/****************************************************************************
validate a group username entry. Return the username or NULL
****************************************************************************/
static char *validate_group(char *group,char *password,int pwlen,int snum)
{
#ifdef NETGROUP
  {
    char *host, *user, *domain;
    setnetgrent(group);
    while (getnetgrent(&host, &user, &domain)) {
      if (user) {
	if (user_ok(user, snum) && 
	    password_ok(user,password,pwlen,NULL,False)) {
	  endnetgrent();
	  return(user);
	}
      }
    }
    endnetgrent();
  }
#endif
  
#if HAVE_GETGRNAM 
  {
    struct group *gptr = (struct group *)getgrnam(group);
    char **member;
    if (gptr)
      {
	member = gptr->gr_mem;
	while (member && *member)
	  {
	    static fstring name;
	    strcpy(name,*member);
	    if (user_ok(name,snum) &&
		password_ok(name,password,pwlen,NULL,False))
	      return(&name[0]);
	    member++;
	  }
#ifdef GROUP_CHECK_PWENT
	{
	  struct passwd *pwd;
	  static fstring tm;
	  
	  setpwent ();
	  while (pwd = getpwent ()) {
	    if (*(pwd->pw_passwd) && pwd->pw_gid == gptr->gr_gid) {
	      /* This Entry have PASSWORD and same GID then check pwd */
	      if (password_ok(NULL, password, pwlen, pwd,False)) {
		strcpy(tm, pwd->pw_name);
		endpwent ();
		return tm;
	      }
	    }
	  }
	  endpwent ();
	}
#endif /* GROUP_CHECK_PWENT */
      }
  }      
#endif
  return(NULL);
}



/****************************************************************************
check for authority to login to a service with a given username/password
****************************************************************************/
BOOL authorise_login(int snum,char *user,char *password, int pwlen, 
		     BOOL *guest,BOOL *force,int vuid)
{
  BOOL ok = False;
  
  *guest = False;
  
#if DEBUG_PASSWORD
  DEBUG(100,("checking authorisation on user=%s pass=%s\n",user,password));
#endif

  /* there are several possabilities:
     1) login as the given user with given password
     2) login as a previously registered username with the given password
     3) login as a session list username with the given password
     4) login as a previously validated user/password pair
     5) login as the "user =" user with given password
     6) login as the "user =" user with no password (guest connection)
     7) login as guest user with no password

     if the service is guest_only then steps 1 to 5 are skipped
  */

  if (GUEST_ONLY(snum)) *force = True;

  if (!(GUEST_ONLY(snum) && GUEST_OK(snum)))
    {

      /* check the given username and password */
      if (!ok && (*user) && user_ok(user,snum)) {
	ok = password_ok(user,password, pwlen, NULL, False);
	if (ok) DEBUG(3,("ACCEPTED: given username password ok\n"));
      }

      /* check for a previously registered guest username */
      if (!ok && (vuid >= 0) && validated_users[vuid].guest) {	  
	if (user_ok(validated_users[vuid].name,snum) &&
	    password_ok(validated_users[vuid].name, password, pwlen, NULL, False)) {
	  strcpy(user, validated_users[vuid].name);
	  validated_users[vuid].guest = False;
	  DEBUG(3,("ACCEPTED: given password with registered user %s\n", user));
	  ok = True;
	}
      }


      /* now check the list of session users */
      if (!ok)
	{
	  char *auser;
	  char *user_list = strdup(session_users);
	  if (!user_list) return(False);

	  for (auser=strtok(user_list,LIST_SEP); 
	       !ok && auser; 
	       auser = strtok(NULL,LIST_SEP))
	    {
	      fstring user2;
	      strcpy(user2,auser);
	      if (!user_ok(user2,snum)) continue;
		  
	      if (password_ok(user2,password, pwlen, NULL, False)) {
		ok = True;
		strcpy(user,user2);
		DEBUG(3,("ACCEPTED: session list username and given password ok\n"));
	      }
	    }
	  free(user_list);
	}

      /* check for a previously validated username/password pair */
      if (!ok && !lp_revalidate(snum) &&
	  (vuid >= 0) && !validated_users[vuid].guest &&
	  user_ok(validated_users[vuid].name,snum)) {
	strcpy(user,validated_users[vuid].name);
	*guest = False;
	DEBUG(3,("ACCEPTED: validated uid ok as non-guest\n"));
	ok = True;
      }

      /* check for a rhosts entry */
      if (!ok && user_ok(user,snum) && check_hosts_equiv(user)) {
	ok = True;
	DEBUG(3,("ACCEPTED: hosts equiv or rhosts entry\n"));
      }

      /* check the user= fields and the given password */
      if (!ok && lp_username(snum)) {
	char *auser;
	pstring user_list;
	StrnCpy(user_list,lp_username(snum),sizeof(pstring));

	string_sub(user_list,"%S",lp_servicename(snum));
	  
	for (auser=strtok(user_list,LIST_SEP);
	     auser && !ok;
	     auser = strtok(NULL,LIST_SEP))
	  {
	    if (*auser == '@')
	      {
		auser = validate_group(auser+1,password,pwlen,snum);
		if (auser)
		  {
		    ok = True;
		    strcpy(user,auser);
		    DEBUG(3,("ACCEPTED: group username and given password ok\n"));
		  }
	      }
	    else
	      {
		fstring user2;
		strcpy(user2,auser);
		if (user_ok(user2,snum) && 
		    password_ok(user2,password,pwlen,NULL, False))
		  {
		    ok = True;
		    strcpy(user,user2);
		    DEBUG(3,("ACCEPTED: user list username and given password ok\n"));
		  }
	      }
	  }
      }      
    } /* not guest only */

  /* check for a normal guest connection */
  if (!ok && GUEST_OK(snum))
    {
      fstring guestname;
      StrnCpy(guestname,lp_guestaccount(snum),sizeof(guestname)-1);
      if (Get_Pwnam(guestname,True))
	{
	  strcpy(user,guestname);
	  ok = True;
	  DEBUG(3,("ACCEPTED: guest account and guest ok\n"));
	}
      else
	DEBUG(0,("Invalid guest account %s??\n",guestname));
      *guest = True;
      *force = True;
    }

  if (ok && !user_ok(user,snum))
    {
      DEBUG(0,("rejected invalid user %s\n",user));
      ok = False;
    }

  return(ok);
}


/****************************************************************************
read the a hosts.equiv or .rhosts file and check if it
allows this user from this machine
****************************************************************************/
static BOOL check_user_equiv(char *user, char *remote, char *equiv_file)
{
  pstring buf;
  int plus_allowed = 1;
  char *file_host;
  char *file_user;
  FILE *fp = fopen(equiv_file, "r");
  DEBUG(5, ("check_user_equiv %s %s %s\n", user, remote, equiv_file));
  if (! fp) return False;
  while(fgets(buf, sizeof(buf), fp)) 
  {
    trim_string(buf," "," ");

    if (buf[0] != '#' && buf[0] != '\n') 
    {
      BOOL is_group = False;
      int plus = 1;
      char *bp = buf;
      if (strcmp(buf, "NO_PLUS\n") == 0)
      {
	DEBUG(6, ("check_user_equiv NO_PLUS\n"));
	plus_allowed = 0;
      }
      else {
	if (buf[0] == '+') 
	{
	  bp++;
	  if (*bp == '\n' && plus_allowed) 
	  {
	    /* a bare plus means everbody allowed */
	    DEBUG(6, ("check_user_equiv everybody allowed\n"));
	    fclose(fp);
	    return True;
	  }
	}
	else if (buf[0] == '-')
	{
	  bp++;
	  plus = 0;
	}
	if (*bp == '@') 
	{
	  is_group = True;
	  bp++;
	}
	file_host = strtok(bp, " \t\n");
	file_user = strtok(NULL, " \t\n");
	DEBUG(7, ("check_user_equiv %s %s\n", file_host, file_user));
	if (file_host && *file_host) 
	{
	  BOOL host_ok = False;

#ifdef NETGROUP	  
	  if (is_group)
	    {
	      static char *mydomain = NULL;
	      if (!mydomain)
		yp_get_default_domain(&mydomain);
	      if (mydomain && innetgr(file_host,remote,user,mydomain))
		host_ok = True;
	    }
#else
	  if (is_group)
	    {
	      DEBUG(1,("Netgroups not configured - add -DNETGROUP and recompile\n"));
	      continue;
	    }
#endif

	  /* is it this host */
	  /* the fact that remote has come from a call of gethostbyaddr
	   * means that it may have the fully qualified domain name
	   * so we could look up the file version to get it into
	   * a canonical form, but I would rather just type it
	   * in full in the equiv file
	   */
	  if (!host_ok && !is_group && strequal(remote, file_host))
	    host_ok = True;

	  if (!host_ok)
	    continue;

	  /* is it this user */
	  if (file_user == 0 || strequal(user, file_user)) 
	    {
	      fclose(fp);
	      DEBUG(5, ("check_user_equiv matched %s%s %s\n",
			(plus ? "+" : "-"), file_host,
			(file_user ? file_user : "")));
	      return (plus ? True : False);
	    }
	}
      }
    }
  }
  fclose(fp);
  return False;
}


/****************************************************************************
check for a possible hosts equiv or rhosts entry for the user
****************************************************************************/
BOOL check_hosts_equiv(char *user)
{
  char *fname = NULL;
  pstring rhostsfile;
  struct passwd *pass = Get_Pwnam(user,True);

  extern struct from_host Client_info;
  extern int Client;

  if (!pass) 
    return(False);

  fromhost(Client,&Client_info);

  fname = lp_hosts_equiv();

  /* note: don't allow hosts.equiv on root */
  if (fname && *fname && (pass->pw_uid != 0))
    {
      if (check_user_equiv(user,Client_info.name,fname))
	return(True);
    }
  
  if (lp_use_rhosts())
    {
      char *home = get_home_dir(user);
      if (home)
	{
	  sprintf(rhostsfile, "%s/.rhosts", home);
	  if (check_user_equiv(user,Client_info.name,rhostsfile))
	    return(True);
	}
    }

  return(False);
}


int password_client = -1;
static fstring pserver;

/****************************************************************************
attempted support for server level security 
****************************************************************************/
BOOL server_cryptkey(char *buf)
{
  pstring inbuf,outbuf;
  fstring pass_protocol;
  extern fstring remote_machine;
  char *p;
  int len;
  fstring desthost;
  struct in_addr dest_ip;
  int port = SMB_PORT;
  BOOL ret;

  if (password_client >= 0)
    close(password_client);
  password_client = -1;

  if (Protocol < PROTOCOL_NT1) {
    strcpy(pass_protocol,"LM1.2X002");
  } else {
    strcpy(pass_protocol,"NT LM 0.12");
  }

  bzero(inbuf,sizeof(inbuf));
  bzero(outbuf,sizeof(outbuf));

  for (p=strtok(lp_passwordserver(),LIST_SEP); p ; p = strtok(NULL,LIST_SEP)) {
    strcpy(desthost,p);
    standard_sub_basic(desthost);
    strupper(desthost);

    dest_ip = *interpret_addr2(desthost);
    if (zero_ip(dest_ip)) {
      DEBUG(1,("Can't resolve address for %s\n",p));
      continue;
    }

    if (ismyip(dest_ip)) {
      DEBUG(1,("Password server loop - disabling password server %s\n",p));
      continue;
    }

    password_client = open_socket_out(SOCK_STREAM, &dest_ip, port, SHORT_CONNECT_TIMEOUT);
    if (password_client >= 0) {
      DEBUG(3,("connected to password server %s\n",p));
      StrnCpy(pserver,p,sizeof(pserver)-1);
      break;
    }
  }

  if (password_client < 0) {
    DEBUG(1,("password server not available\n"));
    return(False);
  }


  /* send a session request (RFC 8002) */

  /* put in the destination name */
  len = 4;
  p = outbuf+len;
  name_mangle(desthost,p,' ');
  len += name_len(p);

  /* and my name */
  p = outbuf+len;
  name_mangle(remote_machine,p,' ');
  len += name_len(p);

  _smb_setlen(outbuf,len);
  CVAL(outbuf,0) = 0x81;

  send_smb(password_client,outbuf);
  
 
  if (!receive_smb(password_client,inbuf,5000) ||
      CVAL(inbuf,0) != 0x82) {
    DEBUG(1,("%s rejected the session\n",pserver));
    close(password_client); password_client = -1;
    return(False);
  }

  DEBUG(3,("got session\n"));

  bzero(outbuf,smb_size);

  /* setup the protocol string */
  set_message(outbuf,0,strlen(pass_protocol)+2,True);
  p = smb_buf(outbuf);
  *p++ = 2;
  strcpy(p,pass_protocol);

  CVAL(outbuf,smb_com) = SMBnegprot;
  CVAL(outbuf,smb_flg) = 0x8;
  SSVAL(outbuf,smb_flg2,0x1);

  send_smb(password_client,outbuf);
  ret = receive_smb(password_client,inbuf,5000);

  if (!ret || CVAL(inbuf,smb_rcls) || SVAL(inbuf,smb_vwv0)) {
    DEBUG(1,("%s rejected the protocol\n",pserver));
    close(password_client); password_client= -1;
    return(False);
  }

  if (!(CVAL(inbuf,smb_vwv1) & 1)) {
    DEBUG(1,("%s isn't in user level security mode\n",pserver));
    close(password_client); password_client= -1;
    return(False);
  }

  memcpy(buf,inbuf,smb_len(inbuf)+4);

  DEBUG(3,("password server OK\n"));

  return(True);
}

/****************************************************************************
attempted support for server level security 
****************************************************************************/
BOOL server_validate(char *buf)
{
  pstring inbuf,outbuf;  
  BOOL ret;

  if (password_client < 0) {
    DEBUG(1,("%s not connected\n",pserver));
    return(False);
  }  

  bzero(inbuf,sizeof(inbuf));
  memcpy(outbuf,buf,sizeof(outbuf));

  /* send a session setup command */
  CVAL(outbuf,smb_flg) = 0x8;
  SSVAL(outbuf,smb_flg2,0x1);
  CVAL(outbuf,smb_vwv0) = 0xFF;

  set_message(outbuf,smb_numwords(outbuf),smb_buflen(outbuf),False);

  SCVAL(inbuf,smb_rcls,1);

  send_smb(password_client,outbuf);
  ret = receive_smb(password_client,inbuf,5000);

  if (!ret || CVAL(inbuf,smb_rcls) != 0) {
    DEBUG(1,("password server %s rejected the password\n",pserver));
    return(False);
  }

  /* if logged in as guest then reject */
  if ((SVAL(inbuf,smb_vwv2) & 1) != 0) {
    DEBUG(1,("password server %s gave us guest only\n",pserver));
    return(False);
  }

  DEBUG(3,("password server %s accepted the password\n",pserver));

#if !KEEP_PASSWORD_SERVER_OPEN
  close(password_client); password_client= -1;
#endif

  return(True);
}


