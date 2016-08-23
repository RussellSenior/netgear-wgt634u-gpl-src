/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Authorization "module" (c) 1998,99 Martin Hinner <martin@tdp.cz>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#ifdef EMBED
#include <unistd.h>
#else
#include <crypt.h>
#endif
//#include <inl_misc.h>
#include "boa.h"
#include <shadow.h>
#include <crypt.h>
#ifdef USE_AUTH

static char auth_userpass[0x80];
//static char admin_passwd[0x80]="admin:password";
static char admin_name[0x80]="root";

void auth_read_admin_password()
{
//    struct passwd *pw;
    
//    pw = getpwnam("root");
//    strncpy(admin_passwd, pw->pw_passwd, sizeof(admin_passwd));
}



/*
 * Name: base64decode
 *
 * Description: Decodes BASE-64 encoded string
 */
int base64decode(void *dst,char *src,int maxlen)
{
 int bitval,bits;
 int val;
 int len,x,y;

 len = strlen(src);
 bitval=0;
 bits=0;
 y=0;

 for(x=0;x<len;x++)
  {
   if ((src[x]>='A')&&(src[x]<='Z')) val=src[x]-'A'; else
   if ((src[x]>='a')&&(src[x]<='z')) val=src[x]-'a'+26; else
   if ((src[x]>='0')&&(src[x]<='9')) val=src[x]-'0'+52; else
   if (src[x]=='+') val=62; else
   if (src[x]=='-') val=63; else
    val=-1;
   if (val>=0)
    {
     bitval=bitval<<6;
     bitval+=val;
     bits+=6;
     while (bits>=8)
      {
       if (y<maxlen)
        ((char *)dst)[y++]=(bitval>>(bits-8))&0xFF;
       bits-=8;
       bitval &= (1<<bits)-1;
      }
    }
  }
 if (y<maxlen)
   ((char *)dst)[y++]=0;
 return y;
}



#if 0

static char base64chars[64] = "abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

/*
 * Name: base64encode()
 *
 * Description: Encodes a buffer using BASE64.
 */
void base64encode(unsigned char *from, char *to, int len)
{
  while (len) {
    unsigned long k;
    int c;
    c = (len < 3) ? len : 3;
    k = 0;
    len -= c;
    while (c--)
      k = (k << 8) | *from++;
    *to++ = base64chars[ (k >> 18) & 0x3f ];
    *to++ = base64chars[ (k >> 12) & 0x3f ];
    *to++ = base64chars[ (k >> 6) & 0x3f ];
    *to++ = base64chars[ k & 0x3f ];
  }
  *to++ = 0;
}


/*
 * Name: auth_check_adminpass
 *
 * Description: if user = admin & pass= the passowrd of root, then return 0
 * , else returns nonzero; As one-way function is used RSA's MD5 w/
 *  BASE64 encoding.
 */
int auth_check_adminpass(char *user,char *pass)
{
	struct MD5Context mc;
 	unsigned char final[16];
	char encoded_passwd[0x40];
  /* Encode password ('pass') using one-way function and then use base64 encoding. */
	MD5Init(&mc);
	MD5Update(&mc, pass, strlen(pass));
	MD5Final(final, &mc);
	strcpy(encoded_passwd,"$1$");
	base64encode(final, encoded_passwd+3, 16);

//    printf("auth_check_userpass(%s,%s,...);\n",user,pass);

	if (!strcmp(user,"admin"))
	{
		if ( ! strcmp(encoded_passwd, admin_passwd) )
			return 0;
	}

	return 1;
}
#endif

int auth_authorize(request * req)
{
        char *usr, *pwd;
	struct spwd *spwd;
	struct passwd *pw;
	/* request timeout */	
	if ( timeoutflag )
	{
		timeoutflag=0;
		onceloginflag=0;
		send_r_unauthorizedtimeout(req, auth_realm);
		return 0;
	}
	/* request one user login */	
	if( onceloginflag )
	{
		if( strcmp(remoteIPaddr,req->remote_ip_addr) != 0 ){       
		send_r_unauthorizedlogin(req);
		return 0;
		}
	
	}
	
	if (! req->authorization){
        /* The request contains no authentication, reject it */
        send_r_unauthorized(req, auth_realm);
	return 0;
	}
    	else{
        if (strncasecmp(req->authorization,"Basic ",6))
        {
            /* We only accept Basic Authentication */
            send_r_bad_request(req);
            return 0;
        }

        base64decode(auth_userpass,req->authorization+6,0x100);
			
		usr = auth_userpass;
        if ( (pwd = strchr(auth_userpass, ':')) == 0 )
        {
            send_r_bad_request(req);
            return 0;
        }
        *pwd++=0;
        
		// swap admin with root, otherwise reject
		if (strcmp(usr,"admin")==0)
			usr=admin_name;
		else 
		{
			send_r_unauthorized(req, auth_realm);
		    	return 0;
		}
		
		// get uid of usr , check if == root's uid
		if ( (pw=getpwnam(usr)) == NULL )
		{	
			send_r_unauthorized(req, auth_realm);
			return 0;	
		}
		if ( pw->pw_uid != 0)
		{
			send_r_unauthorized(req, auth_realm);
	        	return 0;
		}
		// get passwd of usr, check if == root's passwd
		spwd=getspnam(usr);
		if( strcmp(crypt(pwd,spwd->sp_pwdp),spwd->sp_pwdp) == 0 ){
			strcpy(remoteIPaddr,req->remote_ip_addr);
			onceloginflag=1;
			/* request pathname logout.html */
			if ( !strcmp(req->pathname,"/var/www/logout.html")){
				onceloginflag=0;
			}
			return 1;
		}
		else{ 
			send_r_unauthorized(req, auth_realm);
            		return 0;
       		}
   
        }
}

#endif
