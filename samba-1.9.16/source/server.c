/* 
   Unix SMB/Netbios implementation.
   Version 1.9.
   Main SMB server routines
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
#include "trans2.h"

pstring servicesf = CONFIGFILE;
extern pstring debugf;
extern pstring sesssetup_user;

char *InBuffer = NULL;
char *OutBuffer = NULL;
char *last_inbuf = NULL;

BOOL share_mode_pending = False;

/* the last message the was processed */
int last_message = -1;

/* a useful macro to debug the last message processed */
#define LAST_MESSAGE() smb_fn_name(last_message)

extern pstring scope;
extern int DEBUGLEVEL;
extern int case_default;
extern BOOL case_sensitive;
extern BOOL case_preserve;
extern BOOL use_mangled_map;
extern BOOL short_case_preserve;
extern BOOL case_mangle;
extern time_t smb_last_time;
extern struct in_addr myipaddr;

extern int smb_read_error;

extern pstring user_socket_options;

connection_struct Connections[MAX_CONNECTIONS];
files_struct Files[MAX_OPEN_FILES];

extern int Protocol;

int maxxmit = BUFFER_SIZE;

/* a fnum to use when chaining */
int chain_fnum = -1;

/* number of open connections */
static int num_connections_open = 0;

extern fstring remote_machine;


/* these can be set by some functions to override the error codes */
int unix_ERR_class=SUCCESS;
int unix_ERR_code=0;


extern int extra_time_offset;

extern pstring myhostname;

static int find_free_connection(int hash);

/* for readability... */
#define IS_DOS_READONLY(test_mode) (((test_mode) & aRONLY) != 0)
#define IS_DOS_DIR(test_mode) (((test_mode) & aDIR) != 0)
#define IS_DOS_ARCHIVE(test_mode) (((test_mode) & aARCH) != 0)
#define IS_DOS_SYSTEM(test_mode) (((test_mode) & aSYSTEM) != 0)
#define IS_DOS_HIDDEN(test_mode) (((test_mode) & aHIDDEN) != 0)



/****************************************************************************
  change a dos mode to a unix mode
    base permission for files:
         everybody gets read bit set
         dos readonly is represented in unix by removing everyone's write bit
         dos archive is represented in unix by the user's execute bit
         dos system is represented in unix by the group's execute bit
         dos hidden is represented in unix by the other's execute bit
    base permission for directories:
         dos directory is represented in unix by unix's dir bit and the exec bit
****************************************************************************/
mode_t unix_mode(int cnum,int dosmode)
{
  mode_t result = (S_IRUSR | S_IRGRP | S_IROTH);

  if ( !IS_DOS_READONLY(dosmode) )
    result |= (S_IWUSR | S_IWGRP | S_IWOTH);
 
  if (IS_DOS_DIR(dosmode))
    result |= (S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH | S_IWUSR);
 
  if (MAP_ARCHIVE(cnum) && IS_DOS_ARCHIVE(dosmode))
    result |= S_IXUSR;

  if (MAP_SYSTEM(cnum) && IS_DOS_SYSTEM(dosmode))
    result |= S_IXGRP;
 
  if (MAP_HIDDEN(cnum) && IS_DOS_HIDDEN(dosmode))
    result |= S_IXOTH;  
 
  result &= CREATE_MODE(cnum);
  return(result);
}


/****************************************************************************
  change a unix mode to a dos mode
****************************************************************************/
int dos_mode(int cnum,char *path,struct stat *sbuf)
{
  int result = 0;
  extern struct current_user current_user;

  if (CAN_WRITE(cnum) && !lp_alternate_permissions(SNUM(cnum))) {
    if (!((sbuf->st_mode & S_IWOTH) ||
	  Connections[cnum].admin_user ||
	  ((sbuf->st_mode & S_IWUSR) && current_user.uid==sbuf->st_uid) ||
	  ((sbuf->st_mode & S_IWGRP) && 
	   in_group(sbuf->st_gid,current_user.gid,
		    current_user.ngroups,current_user.igroups))))
      result |= aRONLY;
  } else {
    if ((sbuf->st_mode & S_IWUSR) == 0)
      result |= aRONLY;
  }

  if ((sbuf->st_mode & S_IXUSR) != 0)
    result |= aARCH;

  if (MAP_SYSTEM(cnum) && ((sbuf->st_mode & S_IXGRP) != 0))
    result |= aSYSTEM;

  if (MAP_HIDDEN(cnum) && ((sbuf->st_mode & S_IXOTH) != 0))
    result |= aHIDDEN;   
  
  if (S_ISDIR(sbuf->st_mode))
    result = aDIR | (result & aRONLY);

#if LINKS_READ_ONLY
  if (S_ISLNK(sbuf->st_mode) && S_ISDIR(sbuf->st_mode))
    result |= aRONLY;
#endif

  /* hide files with a name starting with a . */
  if (lp_hide_dot_files(SNUM(cnum)))
    {
      char *p = strrchr(path,'/');
      if (p)
	p++;
      else
	p = path;
      
      if (p[0] == '.' && p[1] != '.' && p[1] != 0)
	result |= aHIDDEN;
    }

  return(result);
}


/*******************************************************************
chmod a file - but preserve some bits
********************************************************************/
int dos_chmod(int cnum,char *fname,int dosmode,struct stat *st)
{
  struct stat st1;
  int mask=0;
  int tmp;
  int unixmode;

  if (!st) {
    st = &st1;
    if (sys_stat(fname,st)) return(-1);
  }

  if (S_ISDIR(st->st_mode)) dosmode |= aDIR;

  if (dos_mode(cnum,fname,st) == dosmode) return(0);

  unixmode = unix_mode(cnum,dosmode);

  /* preserve the s bits */
  mask |= (S_ISUID | S_ISGID);

  /* preserve the t bit */
#ifdef S_ISVTX
  mask |= S_ISVTX;
#endif

  /* possibly preserve the x bits */
  if (!MAP_ARCHIVE(cnum)) mask |= S_IXUSR;
  if (!MAP_SYSTEM(cnum)) mask |= S_IXGRP;
  if (!MAP_HIDDEN(cnum)) mask |= S_IXOTH;

  unixmode |= (st->st_mode & mask);

  /* if we previously had any r bits set then leave them alone */
  if ((tmp = st->st_mode & (S_IRUSR|S_IRGRP|S_IROTH))) {
    unixmode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
    unixmode |= tmp;
  }

  /* if we previously had any w bits set then leave them alone 
   if the new mode is not rdonly */
  if (!IS_DOS_READONLY(dosmode) &&
      (tmp = st->st_mode & (S_IWUSR|S_IWGRP|S_IWOTH))) {
    unixmode &= ~(S_IWUSR|S_IWGRP|S_IWOTH);
    unixmode |= tmp;
  }

  return(sys_chmod(fname,unixmode));
}


/****************************************************************************
check if two filenames are equal

this needs to be careful about whether we are case sensitive
****************************************************************************/
static BOOL fname_equal(char *name1, char *name2)
{
  int l1 = strlen(name1);
  int l2 = strlen(name2);

  /* handle filenames ending in a single dot */
  if (l1-l2 == 1 && name1[l1-1] == '.' && lp_strip_dot())
    {
      BOOL ret;
      name1[l1-1] = 0;
      ret = fname_equal(name1,name2);
      name1[l1-1] = '.';
      return(ret);
    }

  if (l2-l1 == 1 && name2[l2-1] == '.' && lp_strip_dot())
    {
      BOOL ret;
      name2[l2-1] = 0;
      ret = fname_equal(name1,name2);
      name2[l2-1] = '.';
      return(ret);
    }

  /* now normal filename handling */
  if (case_sensitive)
    return(strcmp(name1,name2) == 0);

  return(strequal(name1,name2));
}


/****************************************************************************
mangle the 2nd name and check if it is then equal to the first name
****************************************************************************/
static BOOL mangled_equal(char *name1, char *name2)
{
  pstring tmpname;

  if (is_8_3(name2))
    return(False);

  strcpy(tmpname,name2);
  mangle_name_83(tmpname);

  return(strequal(name1,tmpname));
}


/****************************************************************************
scan a directory to find a filename, matching without case sensitivity

If the name looks like a mangled name then try via the mangling functions
****************************************************************************/
static BOOL scan_directory(char *path, char *name,int snum,BOOL docache)
{
  void *cur_dir;
  char *dname;
  BOOL mangled;
  pstring name2;

  mangled = is_mangled(name);

  /* handle null paths */
  if (*path == 0)
    path = ".";

  if (docache && (dname = DirCacheCheck(path,name,snum))) {
    strcpy(name, dname);	
    return(True);
  }      

  if (mangled)
    check_mangled_stack(name);

  /* open the directory */
  if (!(cur_dir = OpenDir(path))) 
    {
      DEBUG(3,("scan dir didn't open dir [%s]\n",path));
      return(False);
    }

  /* now scan for matching names */
  while ((dname = ReadDirName(cur_dir))) 
    {
      if (*dname == '.' &&
	  (strequal(dname,".") || strequal(dname,"..")))
	continue;

      strcpy(name2,dname);
      if (!name_map_mangle(name2,False,snum)) continue;

      if ((mangled && mangled_equal(name,name2))
	  || fname_equal(name, name2))
	{
	  /* we've found the file, change it's name and return */
	  if (docache) DirCacheAdd(path,name,dname,snum);
	  strcpy(name, dname);
	  CloseDir(cur_dir);
	  return(True);
	}
    }

  CloseDir(cur_dir);
  return(False);
}

/****************************************************************************
This routine is called to convert names from the dos namespace to unix
namespace. It needs to handle any case conversions, mangling, format
changes etc.

We assume that we have already done a chdir() to the right "root" directory
for this service.

The function will return False if some part of the name except for the last
part cannot be resolved
****************************************************************************/
BOOL unix_convert(char *name,int cnum)
{
  struct stat st;
  char *start, *end;
  pstring dirpath;

  *dirpath = 0;

  /* convert to basic unix format - removing \ chars and cleaning it up */
  unix_format(name);
  unix_clean_name(name);

  DEBUG(3, ("Before case 1:%s\n", name));
  if (!case_sensitive && 
      (!case_preserve || (is_8_3(name) && !short_case_preserve)))
    strnorm(name);
  DEBUG(3, ("After  case 1:%s\n", name));
  /* names must be relative to the root of the service - trim any leading /.
   also trim trailing /'s */
  trim_string(name,"/","/");

  /* check if it's a printer file */
  if (Connections[cnum].printer)
    {
      if ((! *name) || strchr(name,'/') || !is_8_3(name))
	{
	  char *s;
	  fstring name2;
	  sprintf(name2,"%.6s.XXXXXX",remote_machine);
	  /* sanitise the name */
	  for (s=name2 ; *s ; s++)
	    if (!issafe(*s)) *s = '_';
	  strcpy(name,(char *)mktemp(name2));	  
	}      
      return(True);
    }

  /* stat the name - if it exists then we are all done! */
  if (sys_stat(name,&st) == 0)
    return(True);

  DEBUG(5,("unix_convert(%s,%d)\n",name,cnum));

  /* a special case - if we don't have any mangling chars and are case
     sensitive then searching won't help */
  if (case_sensitive && !is_mangled(name) && 
      !lp_strip_dot() && !use_mangled_map)
    return(False);

  /* now we need to recursively match the name against the real 
     directory structure */

  start = name;
  while (strncmp(start,"./",2) == 0)
    start += 2;

  /* now match each part of the path name separately, trying the names
     as is first, then trying to scan the directory for matching names */
  for (;start;start = (end?end+1:(char *)NULL)) 
    {
      /* pinpoint the end of this section of the filename */
      end = strchr(start, '/');

      /* chop the name at this point */
      if (end) *end = 0;

      /* check if the name exists up to this point */
      if (sys_stat(name, &st) == 0) 
	{
	  /* it exists. it must either be a directory or this must be
	     the last part of the path for it to be OK */
	  if (end && !(st.st_mode & S_IFDIR)) 
	    {
	      /* an intermediate part of the name isn't a directory */
	      DEBUG(5,("Not a dir %s\n",start));
	      *end = '/';
	      return(False);
	    }
	}
      else 
	{
	  pstring rest;

	  *rest = 0;

	  /* remember the rest of the pathname so it can be restored
	     later */
	  if (end) strcpy(rest,end+1);


	  /* try to find this part of the path in the directory */
	  if (strchr(start,'?') || strchr(start,'*') ||
	      !scan_directory(dirpath, start, SNUM(cnum), end?True:False))
	    {
	      if (end) 
		{
		  /* an intermediate part of the name can't be found */
		  DEBUG(5,("Intermediate not found %s\n",start));
		  *end = '/';
		  return(False);
		}
	      
	      /* just the last part of the name doesn't exist */
	      /* we may need to strupper() or strlower() it in case
		 this conversion is being used for file creation 
		 purposes */
	      /* if the filename is of mixed case then don't normalise it */
	      if (!case_preserve && 
		  (!strhasupper(start) || !strhaslower(start)))		
		strnorm(start);
	      /* check on the mangled stack to see if we can recover the 
		 base of the filename */
	      if (is_mangled(start))
		check_mangled_stack(start);

	      DEBUG(5,("New file %s\n",start));
	      return(True); 
	    }

	  /* restore the rest of the string */
	  if (end) 
	    {
	      strcpy(start+strlen(start)+1,rest);
	      end = start + strlen(start);
	    }
	}

      /* add to the dirpath that we have resolved so far */
      if (*dirpath) strcat(dirpath,"/");
      strcat(dirpath,start);

      /* restore the / that we wiped out earlier */
      if (end) *end = '/';
    }
  
  /* the name has been resolved */
  DEBUG(5,("conversion finished %s\n",name));
  return(True);
}


/****************************************************************************
normalise for DOS usage 
****************************************************************************/
static void disk_norm(int *bsize,int *dfree,int *dsize)
{
  /* check if the disk is beyond the max disk size */
  int maxdisksize = lp_maxdisksize();
  if (maxdisksize) {
    /* convert to blocks - and don't overflow */
    maxdisksize = ((maxdisksize*1024)/(*bsize))*1024;
    if (*dsize > maxdisksize) *dsize = maxdisksize;
    if (*dfree > maxdisksize) *dfree = maxdisksize-1; /* the -1 should stop 
							 applications getting 
							 div by 0 errors */
  }  

  while (*dfree > WORDMAX || *dsize > WORDMAX || *bsize < 512) 
    {
      *dfree /= 2;
      *dsize /= 2;
      *bsize *= 2;
      if (*bsize > WORDMAX )
	{
	  *bsize = WORDMAX;
	  if (*dsize > WORDMAX)
	    *dsize = WORDMAX;
	  if (*dfree >  WORDMAX)
	    *dfree = WORDMAX;
	  break;
	}
    }
}

/****************************************************************************
  return number of 1K blocks available on a path and total number 
****************************************************************************/
int disk_free(char *path,int *bsize,int *dfree,int *dsize)
{
  char *df_command = lp_dfree_command();
  char truepath[50];
  char * tmpptr;
  char * tmpptr2;
  BOOL forfree = False;
#ifndef NO_STATFS
#ifdef USE_STATVFS
  struct statvfs fs;
#else
#ifdef ULTRIX
  struct fs_data fs;
#else
  struct statfs fs;
#endif
#endif
#endif

  forfree = False;

  if (*bsize && ( *dsize == *bsize -1 ) && (*dfree == *bsize +1)) {
    DEBUG(2, ("dirpath=%s,connectpath=%s,origpath=%s\n", Connections[*bsize].dirpath, Connections[*bsize].connectpath, Connections[*bsize].origpath));
    tmpptr = Connections[*bsize].dirpath;
    tmpptr2 = tmpptr;
    if (*tmpptr == '.') tmpptr ++;
    strcpy(truepath, Connections[*bsize].connectpath);
    strcat(truepath, tmpptr);
    DEBUG(2, (" path=%s\n", truepath));
    path = truepath;
    forfree = True;
  }

#ifdef QUOTAS
  if (disk_quotas(path, bsize, dfree, dsize))
    {
      disk_norm(bsize,dfree,dsize);
      return(((*bsize)/1024)*(*dfree));
    }
#endif

  /* possibly use system() to get the result */
  if (df_command && *df_command)
    {
      int ret;
      pstring syscmd;
      pstring outfile;
	  
      sprintf(outfile,"/tmp/dfree.smb.%d",(int)getpid());
      sprintf(syscmd,"%s %s",df_command,path);
      standard_sub_basic(syscmd);

      ret = smbrun(syscmd,outfile);
      DEBUG(3,("Running the command `%s' gave %d\n",syscmd,ret));
	  
      {
	FILE *f = fopen(outfile,"r");	
	*dsize = 0;
	*dfree = 0;
	*bsize = 1024;
	if (f)
	  {
	    fscanf(f,"%d %d %d",dsize,dfree,bsize);
	    fclose(f);
	  }
	else
	  DEBUG(0,("Can't open %s\n",outfile));
      }
	  
      unlink(outfile);
      disk_norm(bsize,dfree,dsize);
      return(((*bsize)/1024)*(*dfree));
    }


#ifdef NO_STATFS
  DEBUG(1,("Warning - no statfs function\n"));
  return(1);
#else
#ifdef STATFS4
  if (statfs(path,&fs,sizeof(fs),0) != 0)
#else
#ifdef USE_STATVFS
    if (statvfs(path, &fs))
#else
#ifdef STATFS3
      if (statfs(path,&fs,sizeof(fs)) == -1)	 
#else
	if (statfs(path,&fs) == -1)
#endif /* STATFS3 */
#endif /* USE_STATVFS */
#endif /* STATFS4 */
	  {
	    DEBUG(3,("dfree call failed code errno=%d\n",errno));
	    *bsize = 1024;
	    *dfree = 1;
	    *dsize = 1;
	    return(((*bsize)/1024)*(*dfree));
	  }

#ifdef ULTRIX
  *bsize = 1024;
  *dfree = fs.fd_req.bfree;
  *dsize = fs.fd_req.btot;
#else
#ifdef USE_STATVFS
  *bsize = fs.f_frsize;
#else
#ifdef USE_F_FSIZE
  /* eg: osf1 has f_fsize = fundamental filesystem block size, 
     f_bsize = optimal transfer block size (MX: 94-04-19) */
  *bsize = fs.f_fsize;
#else
  *bsize = fs.f_bsize;
#endif /* STATFS3 */
#endif /* USE_STATVFS */

#ifdef STATFS4
  *dfree = fs.f_bfree;
#else
  *dfree = fs.f_bavail;
#endif /* STATFS4 */
  *dsize = fs.f_blocks;
#endif /* ULTRIX */

#if defined(SCO) || defined(ISC) || defined(MIPS)
  *bsize = 512;
#endif

/* handle rediculous bsize values - some OSes are broken */
  if ((*bsize) < 512 || (*bsize)>0xFFFF) *bsize = 1024;

  if (forfree != True)  disk_norm(bsize,dfree,dsize);
  if (*bsize < 256)    *bsize = 512;
  if ((*dsize)<1)
    {
      DEBUG(0,("dfree seems to be broken on your system\n"));
      *dsize = 20*1024*1024/(*bsize);
      *dfree = MAX(1,*dfree);
    }

  if ((forfree == True) && (strcmp(tmpptr2, ".") == 0)) {
      DEBUG(2, ("Will not return some free space information\n"));
      *dfree = 0;
    }

  DEBUG(2, ("disk_free:bsize=%d,dfree=%d,dsize=%d\n",*bsize, *dfree, *dsize));
  return(((*bsize)/1024)*(*dfree));
#endif
}


/****************************************************************************
wrap it to get filenames right
****************************************************************************/
int sys_disk_free(char *path,int *bsize,int *dfree,int *dsize)
{
  return(disk_free(dos_to_unix(path,False),bsize,dfree,dsize));
}



/****************************************************************************
check a filename - possibly caling reducename

This is called by every routine before it allows an operation on a filename.
It does any final confirmation necessary to ensure that the filename is
a valid one for the user to access.
****************************************************************************/
BOOL check_name(char *name,int cnum)
{
  BOOL ret;

  errno = 0;

  ret = reduce_name(name,Connections[cnum].connectpath,lp_widelinks(SNUM(cnum)));
  if (!ret)
    DEBUG(5,("check_name on %s failed\n",name));

  return(ret);
}

/****************************************************************************
check a filename - possibly caling reducename
****************************************************************************/
static void check_for_pipe(char *fname)
{
  /* special case of pipe opens */
  char s[10];
  StrnCpy(s,fname,9);
  strlower(s);
  if (strstr(s,"pipe/"))
    {
      DEBUG(3,("Rejecting named pipe open for %s\n",fname));
      unix_ERR_class = ERRSRV;
      unix_ERR_code = ERRaccess;
    }
}


/****************************************************************************
open a file
****************************************************************************/
void open_file(int fnum,int cnum,char *fname1,int flags,int mode)
{
  pstring fname;

  Files[fnum].open = False;
  Files[fnum].fd = -1;
  errno = EPERM;

  strcpy(fname,fname1);

  /* check permissions */
  if ((flags != O_RDONLY) && !CAN_WRITE(cnum) && !Connections[cnum].printer)
    {
      DEBUG(3,("Permission denied opening %s\n",fname));
      check_for_pipe(fname);
      return;
    }

  /* this handles a bug in Win95 - it doesn't say to create the file when it 
     should */
  if (Connections[cnum].printer)
    flags |= O_CREAT;

/*
  if (flags == O_WRONLY)
    DEBUG(3,("Bug in client? Set O_WRONLY without O_CREAT\n"));
*/

#if UTIME_WORKAROUND
  /* XXXX - is this OK?? */
  /* this works around a utime bug but can cause other problems */
  if ((flags & (O_WRONLY|O_RDWR)) && (flags & O_CREAT) && !(flags & O_APPEND))
    sys_unlink(fname);
#endif


  Files[fnum].fd = sys_open(fname,flags,mode);

  if ((Files[fnum].fd>=0) && 
      Connections[cnum].printer && lp_minprintspace(SNUM(cnum))) {
    pstring dname;
    int dum1,dum2,dum3;
    char *p;
    strcpy(dname,fname);
    p = strrchr(dname,'/');
    if (p) *p = 0;
    if (sys_disk_free(dname,&dum1,&dum2,&dum3) < 
	lp_minprintspace(SNUM(cnum))) {
      close(Files[fnum].fd);
      Files[fnum].fd = -1;
      sys_unlink(fname);
      errno = ENOSPC;
      return;
    }
  }
    

  /* Fix for files ending in '.' */
  if((Files[fnum].fd == -1) && (errno == ENOENT) && 
     (strchr(fname,'.')==NULL))
    {
      strcat(fname,".");
      Files[fnum].fd = sys_open(fname,flags,mode);
    }

#if (defined(ENAMETOOLONG) && defined(HAVE_PATHCONF))
  if ((Files[fnum].fd == -1) && (errno == ENAMETOOLONG))
    {
      int max_len;
      char *p = strrchr(fname, '/');

      if (p == fname)	/* name is "/xxx" */
	{
	  max_len = pathconf("/", _PC_NAME_MAX);
	  p++;
	}
      else if ((p == NULL) || (p == fname))
	{
	  p = fname;
	  max_len = pathconf(".", _PC_NAME_MAX);
	}
      else
	{
	  *p = '\0';
	  max_len = pathconf(fname, _PC_NAME_MAX);
	  *p = '/';
	  p++;
	}
      if (strlen(p) > max_len)
	{
	  char tmp = p[max_len];

	  p[max_len] = '\0';
	  if ((Files[fnum].fd = sys_open(fname,flags,mode)) == -1)
	    p[max_len] = tmp;
	}
    }
#endif

  if (Files[fnum].fd < 0)
    {
      DEBUG(3,("Error opening file %s (%s) (flags=%d)\n",
	       fname,strerror(errno),flags));
      check_for_pipe(fname);
      return;
    }

  if (Files[fnum].fd >= 0)
    {
      struct stat st;
      Connections[cnum].num_files_open++;
      fstat(Files[fnum].fd,&st);
      Files[fnum].mode = st.st_mode;
      Files[fnum].open_time = time(NULL);
      Files[fnum].size = 0;
      Files[fnum].pos = -1;
      Files[fnum].open = True;
      Files[fnum].mmap_ptr = NULL;
      Files[fnum].mmap_size = 0;
      Files[fnum].can_lock = True;
      Files[fnum].can_read = ((flags & O_WRONLY)==0);
      Files[fnum].can_write = ((flags & (O_WRONLY|O_RDWR))!=0);
      Files[fnum].share_mode = 0;
      Files[fnum].share_pending = False;
      Files[fnum].print_file = Connections[cnum].printer;
      Files[fnum].modified = False;
      Files[fnum].cnum = cnum;
      string_set(&Files[fnum].name,fname);
      Files[fnum].wbmpx_ptr = NULL;      

      /*
       * If the printer is marked as postscript output a leading
       * file identifier to ensure the file is treated as a raw
       * postscript file.
       * This has a similar effect as CtrlD=0 in WIN.INI file.
       * tim@fsg.com 09/06/94
       */
      if (Files[fnum].print_file && POSTSCRIPT(cnum) && 
	  Files[fnum].can_write) 
	{
	  DEBUG(3,("Writing postscript line\n"));
	  write_file(fnum,"%!\n",3);
	}
      
      DEBUG(2,("%s %s opened file %s read=%s write=%s (numopen=%d fnum=%d)\n",
	       timestring(),Connections[cnum].user,fname,
	       BOOLSTR(Files[fnum].can_read),BOOLSTR(Files[fnum].can_write),
	       Connections[cnum].num_files_open,fnum));

    }

#if USE_MMAP
  /* mmap it if read-only */
  if (!Files[fnum].can_write)
    {
      Files[fnum].mmap_size = file_size(fname);
      Files[fnum].mmap_ptr = (char *)mmap(NULL,Files[fnum].mmap_size,
					  PROT_READ,MAP_SHARED,Files[fnum].fd,0);

      if (Files[fnum].mmap_ptr == (char *)-1 || !Files[fnum].mmap_ptr)
	{
	  DEBUG(3,("Failed to mmap() %s - %s\n",fname,strerror(errno)));
	  Files[fnum].mmap_ptr = NULL;
	}
    }
#endif
}

/*******************************************************************
sync a file
********************************************************************/
void sync_file(int fnum)
{
#ifndef NO_FSYNC
  fsync(Files[fnum].fd);
#endif
}

/****************************************************************************
run a file if it is a magic script
****************************************************************************/
static void check_magic(int fnum,int cnum)
{
  if (!*lp_magicscript(SNUM(cnum)))
    return;

  DEBUG(5,("checking magic for %s\n",Files[fnum].name));

  {
    char *p;
    if (!(p = strrchr(Files[fnum].name,'/')))
      p = Files[fnum].name;
    else
      p++;

    if (!strequal(lp_magicscript(SNUM(cnum)),p))
      return;
  }

  {
    int ret;
    pstring magic_output;
    pstring fname;
    strcpy(fname,Files[fnum].name);

    if (*lp_magicoutput(SNUM(cnum)))
      strcpy(magic_output,lp_magicoutput(SNUM(cnum)));
    else
      sprintf(magic_output,"%s.out",fname);

    chmod(fname,0755);
    ret = smbrun(fname,magic_output);
    DEBUG(3,("Invoking magic command %s gave %d\n",fname,ret));
    unlink(fname);
  }
}


/****************************************************************************
close a file - possibly invalidating the read prediction
****************************************************************************/
void close_file(int fnum)
{
  int cnum = Files[fnum].cnum;
  invalidate_read_prediction(Files[fnum].fd);
  Files[fnum].open = False;
  Connections[cnum].num_files_open--;
  if(Files[fnum].wbmpx_ptr) 
    {
      free((char *)Files[fnum].wbmpx_ptr);
      Files[fnum].wbmpx_ptr = NULL;
    }

#if USE_MMAP
  if(Files[fnum].mmap_ptr) 
    {
      munmap(Files[fnum].mmap_ptr,Files[fnum].mmap_size);
      Files[fnum].mmap_ptr = NULL;
    }
#endif

  if (lp_share_modes(SNUM(cnum)))
    del_share_mode(fnum);

  close(Files[fnum].fd);

  /* NT uses smbclose to start a print - weird */
  if (Files[fnum].print_file)
    print_file(fnum);

  /* check for magic scripts */
  check_magic(fnum,cnum);

  DEBUG(2,("%s %s closed file %s (numopen=%d)\n",
	   timestring(),Connections[cnum].user,Files[fnum].name,
	   Connections[cnum].num_files_open));
}

enum {AFAIL,AREAD,AWRITE,AALL};

/*******************************************************************
reproduce the share mode access table
********************************************************************/
static int access_table(int new_deny,int old_deny,int old_mode,
			int share_pid,char *fname)
{
  if (new_deny == DENY_ALL || old_deny == DENY_ALL) return(AFAIL);

  if (new_deny == DENY_DOS || old_deny == DENY_DOS) {
    if (old_deny == new_deny && share_pid == getpid()) 
	return(AALL);    

    if (old_mode == 0) return(AREAD);

    /* the new smbpub.zip spec says that if the file extension is
       .com, .dll, .exe or .sym then allow the open. I will force
       it to read-only as this seems sensible although the spec is
       a little unclear on this. */
    if ((fname = strrchr(fname,'.'))) {
      if (strequal(fname,".com") ||
	  strequal(fname,".dll") ||
	  strequal(fname,".exe") ||
	  strequal(fname,".sym"))
	return(AREAD);
    }

    return(AFAIL);
  }

  switch (new_deny) 
    {
    case DENY_WRITE:
      if (old_deny==DENY_WRITE && old_mode==0) return(AREAD);
      if (old_deny==DENY_READ && old_mode==0) return(AWRITE);
      if (old_deny==DENY_NONE && old_mode==0) return(AALL);
      return(AFAIL);
    case DENY_READ:
      if (old_deny==DENY_WRITE && old_mode==1) return(AREAD);
      if (old_deny==DENY_READ && old_mode==1) return(AWRITE);
      if (old_deny==DENY_NONE && old_mode==1) return(AALL);
      return(AFAIL);
    case DENY_NONE:
      if (old_deny==DENY_WRITE) return(AREAD);
      if (old_deny==DENY_READ) return(AWRITE);
      if (old_deny==DENY_NONE) return(AALL);
      return(AFAIL);      
    }
  return(AFAIL);      
}

/*******************************************************************
check if the share mode on a file allows it to be deleted or unlinked
return True if sharing doesn't prevent the operation
********************************************************************/
BOOL check_file_sharing(int cnum,char *fname)
{
  int pid=0;
  int share_mode = get_share_mode_byname(cnum,fname,&pid);

  if (!pid || !share_mode) return(True);
 
  if (share_mode == DENY_DOS)
    return(pid == getpid());

  /* XXXX exactly what share mode combinations should be allowed for
     deleting/renaming? */
  return(False);
}

/****************************************************************************
  C. Hoch 11/22/95
  Helper for open_file_shared. 
  Truncate a file after checking locking; close file if locked.
  **************************************************************************/
static void truncate_unless_locked(int fnum, int cnum)
{
  if (Files[fnum].can_write){
    if (is_locked(fnum,cnum,0x3FFFFFFF,0)){
      close_file(fnum);   
      errno = EACCES;
      unix_ERR_class = ERRDOS;
      unix_ERR_code = ERRlock;
    }
    else
      ftruncate(Files[fnum].fd,0); 
  }
}


/****************************************************************************
open a file with a share mode
****************************************************************************/
void open_file_shared(int fnum,int cnum,char *fname,int share_mode,int ofun,
		      int mode,int *Access,int *action)
{
  int flags=0;
  int flags2=0;
  int deny_mode = (share_mode>>4)&7;
  struct stat sbuf;
  BOOL file_existed = file_exist(fname,&sbuf);
  BOOL fcbopen = False;
  int share_pid=0;

  Files[fnum].open = False;
  Files[fnum].fd = -1;

  /* this is for OS/2 EAs - try and say we don't support them */
  if (strstr(fname,".+,;=[].")) {
    unix_ERR_class = ERRDOS;
    unix_ERR_code = ERROR_EAS_NOT_SUPPORTED;
    return;
  }

  if ((ofun & 0x3) == 0 && file_existed) {
    errno = EEXIST;
    return;
  }
      
  if (ofun & 0x10)
    flags2 |= O_CREAT;
  if ((ofun & 0x3) == 2)
    flags2 |= O_TRUNC;

  /* note that we ignore the append flag as 
     append does not mean the same thing under dos and unix */

  switch (share_mode&0xF)
    {
    case 1: 
      flags = O_WRONLY; 
      break;
    case 0xF: 
      fcbopen = True;
      flags = O_RDWR; 
      break;
    case 2: 
      flags = O_RDWR; 
      break;
    default:
      flags = O_RDONLY;
      break;
    }
  
  if (flags != O_RDONLY && file_existed && 
      (!CAN_WRITE(cnum) || IS_DOS_READONLY(dos_mode(cnum,fname,&sbuf)))) {
    if (!fcbopen) {
      errno = EACCES;
      return;
    }
    flags = O_RDONLY;
  }

  if (deny_mode > DENY_NONE && deny_mode!=DENY_FCB) {
    DEBUG(2,("Invalid deny mode %d on file %s\n",deny_mode,fname));
    errno = EINVAL;
    return;
  }

  if (deny_mode == DENY_FCB) deny_mode = DENY_DOS;

  if (lp_share_modes(SNUM(cnum))) {
    int old_share=0;

    if (file_existed)
      old_share = get_share_mode(cnum,&sbuf,&share_pid);

    if (share_pid) {
      /* someone else has a share lock on it, check to see 
	 if we can too */
      int old_open_mode = old_share&0xF;
      int old_deny_mode = (old_share>>4)&7;

      if (deny_mode > 4 || old_deny_mode > 4 || old_open_mode > 2) {
	DEBUG(2,("Invalid share mode (%d,%d,%d) on file %s\n",
		 deny_mode,old_deny_mode,old_open_mode,fname));
	errno = EACCES;
	unix_ERR_class = ERRDOS;
	unix_ERR_code = ERRbadshare;
	return;
      }

      {
	int access_allowed = access_table(deny_mode,old_deny_mode,old_open_mode,
					  share_pid,fname);

	if ((access_allowed == AFAIL) ||
	    (access_allowed == AREAD && flags == O_WRONLY) ||
	    (access_allowed == AWRITE && flags == O_RDONLY)) {
	  DEBUG(2,("Share violation on file (%d,%d,%d,%d,%s) = %d\n",
		   deny_mode,old_deny_mode,old_open_mode,
		   share_pid,fname,
		   access_allowed));
	  errno = EACCES;
	  unix_ERR_class = ERRDOS;
	  unix_ERR_code = ERRbadshare;
	  return;
	}
	
	if (access_allowed == AREAD)
	  flags = O_RDONLY;
	
	if (access_allowed == AWRITE)
	  flags = O_WRONLY;
      }
    }
  }

  DEBUG(4,("calling open_file with flags=0x%X flags2=0x%X mode=0%o\n",
	   flags,flags2,mode));

  open_file(fnum,cnum,fname,flags|(flags2&~(O_TRUNC)),mode);
  if (!Files[fnum].open && flags==O_RDWR && errno!=ENOENT && fcbopen) {
    flags = O_RDONLY;
    open_file(fnum,cnum,fname,flags,mode);
  }

  if (Files[fnum].open) {
    int open_mode=0;
    switch (flags) {
    case O_RDONLY:
      open_mode = 0;
      break;
    case O_RDWR:
      open_mode = 2;
      break;
    case O_WRONLY:
      open_mode = 1;
      break;
    }

    Files[fnum].share_mode = (deny_mode<<4) | open_mode;
    Files[fnum].share_pending = True;

    if (Access) {
      (*Access) = open_mode;
    }
    
    if (action) {
      if (file_existed && !(flags2 & O_TRUNC)) *action = 1;
      if (!file_existed) *action = 2;
      if (file_existed && (flags2 & O_TRUNC)) *action = 3;
    }

    if (!share_pid)
      share_mode_pending = True;

    if ((flags2&O_TRUNC) && file_existed)
      truncate_unless_locked(fnum,cnum);
  }
}



/*******************************************************************
check for files that we should now set our share modes on
********************************************************************/
static void check_share_modes(void)
{
  int i;
  for (i=0;i<MAX_OPEN_FILES;i++)
    if(Files[i].open && Files[i].share_pending) {
      if (lp_share_modes(SNUM(Files[i].cnum))) {
	int pid=0;
	get_share_mode_by_fnum(Files[i].cnum,i,&pid);
	if (!pid) {
	  set_share_mode(i,Files[i].share_mode);
	  Files[i].share_pending = False;
	}
      } else {
	Files[i].share_pending = False;	
      }
    }
}


/****************************************************************************
seek a file. Try to avoid the seek if possible
****************************************************************************/
int seek_file(int fnum,int pos)
{
  int offset = 0;
  if (Files[fnum].print_file && POSTSCRIPT(Files[fnum].cnum))
    offset = 3;

  Files[fnum].pos = lseek(Files[fnum].fd,pos+offset,SEEK_SET) - offset;
  return(Files[fnum].pos);
}

/****************************************************************************
read from a file
****************************************************************************/
int read_file(int fnum,char *data,int pos,int n)
{
  int ret=0,readret;

  if (!Files[fnum].can_write)
    {
      ret = read_predict(Files[fnum].fd,pos,data,NULL,n);

      data += ret;
      n -= ret;
      pos += ret;
    }

#if USE_MMAP
  if (Files[fnum].mmap_ptr)
    {
      int num = MIN(n,Files[fnum].mmap_size-pos);
      if (num > 0)
	{
	  memcpy(data,Files[fnum].mmap_ptr+pos,num);
	  data += num;
	  pos += num;
	  n -= num;
	  ret += num;
	}
    }
#endif

  if (n <= 0)
    return(ret);

  if (seek_file(fnum,pos) != pos)
    {
      DEBUG(3,("Failed to seek to %d\n",pos));
      return(ret);
    }
  
  if (n > 0) {
    readret = read(Files[fnum].fd,data,n);
    if (readret > 0) ret += readret;
  }

  return(ret);
}


/****************************************************************************
write to a file
****************************************************************************/
int write_file(int fnum,char *data,int n)
{
  if (!Files[fnum].can_write) {
    errno = EPERM;
    return(0);
  }

  if (!Files[fnum].modified) {
    struct stat st;
    Files[fnum].modified = True;
    if (fstat(Files[fnum].fd,&st) == 0) {
      int dosmode = dos_mode(Files[fnum].cnum,Files[fnum].name,&st);
      if (MAP_ARCHIVE(Files[fnum].cnum) && !IS_DOS_ARCHIVE(dosmode)) {	
	dos_chmod(Files[fnum].cnum,Files[fnum].name,dosmode | aARCH,&st);
      }
    }  
  }

  return(write_data(Files[fnum].fd,data,n));
}


/****************************************************************************
load parameters specific to a connection/service
****************************************************************************/
BOOL become_service(int cnum,BOOL do_chdir)
{
  extern char magic_char;
  static int last_cnum = -1;
  int snum;

  if (!OPEN_CNUM(cnum))
    {
      last_cnum = -1;
      return(False);
    }

  Connections[cnum].lastused = smb_last_time;

  snum = SNUM(cnum);
  
  if (do_chdir &&
      ChDir(Connections[cnum].connectpath) != 0 &&
      ChDir(Connections[cnum].origpath) != 0)
    {
      DEBUG(0,("%s chdir (%s) failed cnum=%d\n",timestring(),
	    Connections[cnum].connectpath,cnum));     
      return(False);
    }

  if (cnum == last_cnum)
    return(True);

  last_cnum = cnum;

  case_default = lp_defaultcase(snum);
  case_preserve = lp_preservecase(snum);
  short_case_preserve = lp_shortpreservecase(snum);
  case_mangle = lp_casemangle(snum);
  case_sensitive = lp_casesensitive(snum);
  magic_char = lp_magicchar(snum);
  use_mangled_map = (*lp_mangled_map(snum) ? True:False);
  return(True);
}


/****************************************************************************
  find a service entry
****************************************************************************/
int find_service(char *service)
{
   int iService;

   string_sub(service,"\\","/");

   iService = lp_servicenumber(service);

   /* now handle the special case of a home directory */
   if (iService < 0)
   {
      char *phome_dir = get_home_dir(service);
      DEBUG(3,("checking for home directory %s gave %s\n",service,
	    phome_dir?phome_dir:"(NULL)"));
      if (phome_dir)
      {   
	 int iHomeService;
	 if ((iHomeService = lp_servicenumber(HOMES_NAME)) >= 0)
	 {
	    lp_add_home(service,iHomeService,phome_dir);
	    iService = lp_servicenumber(service);
	 }
      }
   }

   /* If we still don't have a service, attempt to add it as a printer. */
   if (iService < 0)
   {
      int iPrinterService;

      if ((iPrinterService = lp_servicenumber(PRINTERS_NAME)) >= 0)
      {
         char *pszTemp;

         DEBUG(3,("checking whether %s is a valid printer name...\n", service));
         pszTemp = PRINTCAP;
         if ((pszTemp != NULL) && pcap_printername_ok(service, pszTemp))
         {
            DEBUG(3,("%s is a valid printer name\n", service));
            DEBUG(3,("adding %s as a printer service\n", service));
            lp_add_printer(service,iPrinterService);
            iService = lp_servicenumber(service);
            if (iService < 0)
               DEBUG(0,("failed to add %s as a printer service!\n", service));
         }
         else
            DEBUG(3,("%s is not a valid printer name\n", service));
      }
   }

   /* just possibly it's a default service? */
   if (iService < 0) 
     {
       char *defservice = lp_defaultservice();
       if (defservice && *defservice && !strequal(defservice,service)) {
	 iService = find_service(defservice);
	 if (iService >= 0) {
	   string_sub(service,"_","/");
	   iService = lp_add_service(service,iService);
	 }
       }
     }

   if (iService >= 0)
      if (!VALID_SNUM(iService))
      {
         DEBUG(0,("Invalid snum %d for %s\n",iService,service));
	 iService = -1;
      }

   if (iService < 0)
      DEBUG(3,("find_service() failed to find service %s\n", service));

   return (iService);
}


/****************************************************************************
  create an error packet from a cached error.
****************************************************************************/
int cached_error_packet(char *inbuf,char *outbuf,int fnum,int line)
{
  write_bmpx_struct *wbmpx = Files[fnum].wbmpx_ptr;

  int32 eclass = wbmpx->wr_errclass;
  int32 err = wbmpx->wr_error;

  /* We can now delete the auxiliary struct */
  free((char *)wbmpx);
  Files[fnum].wbmpx_ptr = NULL;
  return error_packet(inbuf,outbuf,eclass,err,line);
}


struct
{
  int unixerror;
  int smbclass;
  int smbcode;
} unix_smb_errmap[] =
{
  {EPERM,ERRDOS,ERRnoaccess},
  {EACCES,ERRDOS,ERRnoaccess},
  {ENOENT,ERRDOS,ERRbadfile},
  {EIO,ERRHRD,ERRgeneral},
  {EBADF,ERRSRV,ERRsrverror},
  {EINVAL,ERRSRV,ERRsrverror},
  {EEXIST,ERRDOS,ERRfilexists},
  {ENFILE,ERRDOS,ERRnofids},
  {EMFILE,ERRDOS,ERRnofids},
  {ENOSPC,ERRHRD,ERRdiskfull},
#ifdef EDQUOT
  {EDQUOT,ERRHRD,ERRdiskfull},
#endif
#ifdef ENOTEMPTY
  {ENOTEMPTY,ERRDOS,ERRnoaccess},
#endif
#ifdef EXDEV
  {EXDEV,ERRDOS,ERRdiffdevice},
#endif
  {EROFS,ERRHRD,ERRnowrite},
  {0,0,0}
};


/****************************************************************************
  create an error packet from errno
****************************************************************************/
int unix_error_packet(char *inbuf,char *outbuf,int def_class,uint32 def_code,int line)
{
  int eclass=def_class;
  int ecode=def_code;
  int i=0;

  if (unix_ERR_class != SUCCESS)
    {
      eclass = unix_ERR_class;
      ecode = unix_ERR_code;
      unix_ERR_class = SUCCESS;
      unix_ERR_code = 0;
    }
  else
    {
      while (unix_smb_errmap[i].smbclass != 0)
	{
	  if (unix_smb_errmap[i].unixerror == errno)
	    {
	      eclass = unix_smb_errmap[i].smbclass;
	      ecode = unix_smb_errmap[i].smbcode;
	      break;
	    }
	  i++;
	}
    }

  return(error_packet(inbuf,outbuf,eclass,ecode,line));
}


/****************************************************************************
  create an error packet. Normally called using the ERROR() macro
****************************************************************************/
int error_packet(char *inbuf,char *outbuf,int error_class,uint32 error_code,int line)
{
  int outsize = set_message(outbuf,0,0,True);
  int cmd;
  cmd = CVAL(inbuf,smb_com);
  
  CVAL(outbuf,smb_rcls) = error_class;
  SSVAL(outbuf,smb_err,error_code);  
  
  DEBUG(3,("%s error packet at line %d cmd=%d (%s) eclass=%d ecode=%d\n",
	   timestring(),
	   line,
	   (int)CVAL(inbuf,smb_com),
	   smb_fn_name(CVAL(inbuf,smb_com)),
	   error_class,
	   error_code));

  if (errno != 0)
    DEBUG(3,("error string = %s\n",strerror(errno)));
  
  return(outsize);
}


#ifndef SIGCLD_IGNORE
/****************************************************************************
this prevents zombie child processes
****************************************************************************/
static int sig_cld()
{
  static int depth = 0;
  if (depth != 0)
    {
      DEBUG(0,("ERROR: Recursion in sig_cld? Perhaps you need `#define USE_WAITPID'?\n"));
      depth=0;
      return(0);
    }
  depth++;

  BlockSignals(True,SIGCLD);
  DEBUG(5,("got SIGCLD\n"));

#ifdef USE_WAITPID
  while (waitpid((pid_t)-1,(int *)NULL, WNOHANG) > 0);
#endif

  /* Stop zombies */
  /* Stevens, Adv. Unix Prog. says that on system V you must call
     wait before reinstalling the signal handler, because the kernel
     calls the handler from within the signal-call when there is a
     child that has exited. This would lead to an infinite recursion
     if done vice versa. */
        
#ifndef DONT_REINSTALL_SIG
#ifdef SIGCLD_IGNORE
  signal(SIGCLD, SIG_IGN);  
#else
  signal(SIGCLD, SIGNAL_CAST sig_cld);
#endif
#endif

#ifndef USE_WAITPID
  while (wait3(WAIT3_CAST1 NULL, WNOHANG, WAIT3_CAST2 NULL) > 0);
#endif
  depth--;
  BlockSignals(False,SIGCLD);
  return 0;
}
#endif

/****************************************************************************
  this is called when the client exits abruptly
  **************************************************************************/
static int sig_pipe()
{
  extern int password_client;
  BlockSignals(True,SIGPIPE);

  if (password_client != -1) {
    DEBUG(3,("lost connection to password server\n"));
    close(password_client);
    password_client = -1;
#ifndef DONT_REINSTALL_SIG
    signal(SIGPIPE, SIGNAL_CAST sig_pipe);
#endif
    BlockSignals(False,SIGPIPE);
    return 0;
  }

  exit_server("Got sigpipe\n");
  return(0);
}

/****************************************************************************
  open the socket communication
****************************************************************************/
static BOOL open_sockets(BOOL is_daemon,int port)
{
  extern int Client;

  if (is_daemon)
    {
      int s;
      struct sockaddr addr;
      int in_addrlen = sizeof(addr);
       
      /* Stop zombies */
#ifdef SIGCLD_IGNORE
      signal(SIGCLD, SIG_IGN);
#else
      signal(SIGCLD, SIGNAL_CAST sig_cld);
#endif

      /* open an incoming socket */
      s = open_socket_in(SOCK_STREAM, port, 0,interpret_addr(lp_socket_address()));
      if (s == -1)
	return(False);

      /* ready to listen */
      if (listen(s, 5) == -1) 
	{
	  DEBUG(0,("listen: %s",strerror(errno)));
	  close(s);
	  return False;
	}
      
      /* now accept incoming connections - forking a new process
	 for each incoming connection */
      DEBUG(2,("waiting for a connection\n"));
      while (1)
	{
	  Client = accept(s,&addr,&in_addrlen);

	  if (Client == -1 && errno == EINTR)
	    continue;

	  if (Client == -1)
	    {
	      DEBUG(0,("accept: %s",strerror(errno)));
	      continue;
	    }

#ifdef NO_FORK_DEBUG
#ifndef NO_SIGNAL_TEST
          signal(SIGPIPE, SIGNAL_CAST sig_pipe);
          signal(SIGCLD, SIGNAL_CAST SIG_DFL);
#endif
	  return True;
#else
	  if (Client != -1 && fork()==0)
	    {
#ifndef NO_SIGNAL_TEST
	      signal(SIGPIPE, SIGNAL_CAST sig_pipe);
	      signal(SIGCLD, SIGNAL_CAST SIG_DFL);
#endif
	      /* close the listening socket */
	      close(s);

	      /* close our standard file descriptors */
	      close_low_fds();
  
	      set_socket_options(Client,"SO_KEEPALIVE");
	      set_socket_options(Client,user_socket_options);

	      return True; 
	    }
          close(Client); /* The parent doesn't need this socket */
#endif
	}
    }
  else
    {
      /* We will abort gracefully when the client or remote system 
	 goes away */
#ifndef NO_SIGNAL_TEST
      signal(SIGPIPE, SIGNAL_CAST sig_pipe);
#endif
      Client = dup(0);

      /* close our standard file descriptors */
      close_low_fds();

      set_socket_options(Client,"SO_KEEPALIVE");
      set_socket_options(Client,user_socket_options);
    }

  return True;
}


/****************************************************************************
check if a snum is in use
****************************************************************************/
BOOL snum_used(int snum)
{
  int i;
  for (i=0;i<MAX_CONNECTIONS;i++)
    if (OPEN_CNUM(i) && (SNUM(i) == snum))
      return(True);
  return(False);
}

/****************************************************************************
  reload the services file
  **************************************************************************/
BOOL reload_services(BOOL test)
{
  BOOL ret;

  if (lp_loaded())
    {
      pstring fname;
      strcpy(fname,lp_configfile());
      if (file_exist(fname,NULL) && !strcsequal(fname,servicesf))
	{
	  strcpy(servicesf,fname);
	  test = False;
	}
    }

  reopen_logs();

  if (test && !lp_file_list_changed())
    return(True);

  lp_killunused(snum_used);

  ret = lp_load(servicesf,False);

  /* perhaps the config filename is now set */
  if (!test)
    reload_services(True);

  reopen_logs();

  load_interfaces();

  {
    extern int Client;
    if (Client != -1) {      
      set_socket_options(Client,"SO_KEEPALIVE");
      set_socket_options(Client,user_socket_options);
    }
  }

  create_mangled_stack(lp_mangledstack());

  /* this forces service parameters to be flushed */
  become_service(-1,True);

  return(ret);
}



/****************************************************************************
this prevents zombie child processes
****************************************************************************/
static int sig_hup()
{
  BlockSignals(True,SIGHUP);
  DEBUG(0,("Got SIGHUP\n"));
  reload_services(False);
#ifndef DONT_REINSTALL_SIG
  signal(SIGHUP,SIGNAL_CAST sig_hup);
#endif
  BlockSignals(False,SIGHUP);
  return(0);
}

/****************************************************************************
Setup the groups a user belongs to.
****************************************************************************/
int setup_groups(char *user, int uid, int gid, int *p_ngroups, 
		 int **p_igroups, gid_t **p_groups)
{
  if (-1 == initgroups(user,gid))
    {
      if (getuid() == 0)
	{
	  DEBUG(0,("Unable to initgroups!\n"));
	  if (gid < 0 || gid > 16000 || uid < 0 || uid > 16000)
	    DEBUG(0,("This is probably a problem with the account %s\n",user));
	}
    }
  else
    {
      int i,ngroups;
      int *igroups;
      gid_t grp = 0;
      ngroups = getgroups(0,&grp);
      if (ngroups <= 0)
        ngroups = 32;
      igroups = (int *)malloc(sizeof(int)*ngroups);
      for (i=0;i<ngroups;i++)
        igroups[i] = 0x42424242;
      ngroups = getgroups(ngroups,(gid_t *)igroups);

      if (igroups[0] == 0x42424242)
        ngroups = 0;

      *p_ngroups = ngroups;

      /* The following bit of code is very strange. It is due to the
         fact that some OSes use int* and some use gid_t* for
         getgroups, and some (like SunOS) use both, one in prototypes,
         and one in man pages and the actual code. Thus we detect it
         dynamically using some very ugly code */
      if (ngroups > 0)
        {
	  /* does getgroups return ints or gid_t ?? */
	  static BOOL groups_use_ints = True;

	  if (groups_use_ints && 
	      ngroups == 1 && 
	      SVAL(igroups,2) == 0x4242)
	    groups_use_ints = False;
	  
          for (i=0;groups_use_ints && i<ngroups;i++)
            if (igroups[i] == 0x42424242)
    	      groups_use_ints = False;
	      
          if (groups_use_ints)
            {
    	      *p_igroups = igroups;
    	      *p_groups = (gid_t *)igroups;	  
            }
          else
            {
	      gid_t *groups = (gid_t *)igroups;
	      igroups = (int *)malloc(sizeof(int)*ngroups);
	      for (i=0;i<ngroups;i++)
	        igroups[i] = groups[i];
	      *p_igroups = igroups;
	      *p_groups = (gid_t *)groups;
	    }
	}
      DEBUG(3,("%s is in %d groups\n",user,ngroups));
      for (i=0;i<ngroups;i++)
        DEBUG(3,("%d ",igroups[i]));
      DEBUG(3,("\n"));
    }
  return 0;
}

/****************************************************************************
  make a connection to a service
****************************************************************************/
int make_connection(char *service,char *user,char *password, int pwlen, char *dev,int vuid)
{
  int cnum;
  int snum;
  struct passwd *pass = NULL;
  connection_struct *pcon;
  BOOL guest = False;
  BOOL force = False;
  static BOOL first_connection = True;

  strlower(service);

  snum = find_service(service);
  if (snum < 0)
    {
      if (strequal(service,"IPC$"))
	{	  
	  DEBUG(3,("%s refusing IPC connection\n",timestring()));
	  return(-3);
	}

      DEBUG(0,("%s couldn't find service %s\n",timestring(),service));      
      return(-2);
    }

  if (strequal(service,HOMES_NAME))
    {
      if (*user && Get_Pwnam(user,True))
	return(make_connection(user,user,password,pwlen,dev,vuid));

      if (validated_username(vuid))
	{
	  strcpy(user,validated_username(vuid));
	  return(make_connection(user,user,password,pwlen,dev,vuid));
	}
    }

  if (!lp_snum_ok(snum) || !check_access(snum)) {    
    return(-4);
  }

  /* you can only connect to the IPC$ service as an ipc device */
  if (strequal(service,"IPC$"))
    strcpy(dev,"IPC");

  if (*dev == '?' || !*dev)
    {
      if (lp_print_ok(snum))
	strcpy(dev,"LPT1:");
      else
	strcpy(dev,"A:");
    }

  /* if the request is as a printer and you can't print then refuse */
  strupper(dev);
  if (!lp_print_ok(snum) && (strncmp(dev,"LPT",3) == 0)) {
    DEBUG(1,("Attempt to connect to non-printer as a printer\n"));
    return(-6);
  }

  /* lowercase the user name */
  strlower(user);

  /* add it as a possible user name */
  add_session_user(service);

  /* shall we let them in? */
  if (!authorise_login(snum,user,password,pwlen,&guest,&force,vuid))
    {
      DEBUG(2,("%s invalid username/password for %s\n",timestring(),service));
      return(-1);
    }
  
  cnum = find_free_connection(str_checksum(service) + str_checksum(user));
  if (cnum < 0)
    {
      DEBUG(0,("%s couldn't find free connection\n",timestring()));      
      return(-1);
    }

  pcon = &Connections[cnum];
  bzero((char *)pcon,sizeof(*pcon));

  /* find out some info about the user */
  pass = Get_Pwnam(user,True);

  if (pass == NULL)
    {
      DEBUG(0,("%s couldn't find account %s\n",timestring(),user)); 
      return(-7);
    }

  pcon->read_only = lp_readonly(snum);

  {
    pstring list;
    StrnCpy(list,lp_readlist(snum),sizeof(pstring)-1);
    string_sub(list,"%S",service);

    if (user_in_list(user,list))
      pcon->read_only = True;

    StrnCpy(list,lp_writelist(snum),sizeof(pstring)-1);
    string_sub(list,"%S",service);

    if (user_in_list(user,list))
      pcon->read_only = False;    
  }

  /* admin user check */
  if (user_in_list(user,lp_admin_users(snum)) &&
      !pcon->read_only)
    {
      pcon->admin_user = True;
      DEBUG(0,("%s logged in as admin user (root privileges)\n",user));
    }
  else
    pcon->admin_user = False;
    
  pcon->force_user = force;
  pcon->uid = pass->pw_uid;
  pcon->gid = pass->pw_gid;
  pcon->num_files_open = 0;
  pcon->lastused = time(NULL);
  pcon->service = snum;
  pcon->used = True;
  pcon->printer = (strncmp(dev,"LPT",3) == 0);
  pcon->ipc = (strncmp(dev,"IPC",3) == 0);
  pcon->dirptr = NULL;
  string_set(&pcon->dirpath,"");
  string_set(&pcon->user,user);

#if HAVE_GETGRNAM 
  if (*lp_force_group(snum))
    {
      struct group *gptr = (struct group *)getgrnam(lp_force_group(snum));
      if (gptr)
	{
	  pcon->gid = gptr->gr_gid;
	  DEBUG(3,("Forced group %s\n",lp_force_group(snum)));
	}
      else
	DEBUG(1,("Couldn't find group %s\n",lp_force_group(snum)));
    }
#endif

  if (*lp_force_user(snum))
    {
      struct passwd *pass2;
      fstring fuser;
      strcpy(fuser,lp_force_user(snum));
      pass2 = (struct passwd *)Get_Pwnam(fuser,True);
      if (pass2)
	{
	  pcon->uid = pass2->pw_uid;
	  string_set(&pcon->user,fuser);
	  strcpy(user,fuser);
	  pcon->force_user = True;
	  DEBUG(3,("Forced user %s\n",fuser));	  
	}
      else
	DEBUG(1,("Couldn't find user %s\n",fuser));
    }

  {
    pstring s;
    strcpy(s,lp_pathname(snum));
    standard_sub(cnum,s);
    string_set(&pcon->connectpath,s);
    DEBUG(3,("Connect path is %s\n",s));
  }

  /* groups stuff added by ih */
  pcon->ngroups = 0;
  pcon->groups = NULL;

  if (!IS_IPC(cnum))
    {
      /* Find all the groups this uid is in and store them. Used by become_user() */
      setup_groups(pcon->user,pcon->uid,pcon->gid,&pcon->ngroups,&pcon->igroups,&pcon->groups);
      
      /* check number of connections */
      if (!claim_connection(cnum,
			    lp_servicename(SNUM(cnum)),
			    lp_max_connections(SNUM(cnum)),False))
	{
	  DEBUG(1,("too many connections - rejected\n"));
	  return(-8);
	}  

      if (lp_status(SNUM(cnum)))
	claim_connection(cnum,"STATUS.",MAXSTATUS,first_connection);

      first_connection = False;
    } /* IS_IPC */

  pcon->open = True;

  /* execute any "root preexec = " line */
  if (*lp_rootpreexec(SNUM(cnum)))
    {
      pstring cmd;
      strcpy(cmd,lp_rootpreexec(SNUM(cnum)));
      standard_sub(cnum,cmd);
      DEBUG(5,("cmd=%s\n",cmd));
      smbrun(cmd,NULL);
    }

  if (!become_user(cnum,pcon->uid))
    {
      DEBUG(0,("Can't become connected user!\n"));
      pcon->open = False;
      if (!IS_IPC(cnum)) {
	yield_connection(cnum,
			 lp_servicename(SNUM(cnum)),
			 lp_max_connections(SNUM(cnum)));
	if (lp_status(SNUM(cnum))) yield_connection(cnum,"STATUS.",MAXSTATUS);
      }
      return(-1);
    }

  if (ChDir(pcon->connectpath) != 0)
    {
      DEBUG(0,("Can't change directory to %s (%s)\n",
	       pcon->connectpath,strerror(errno)));
      pcon->open = False;
      unbecome_user();
      if (!IS_IPC(cnum)) {
	yield_connection(cnum,
			 lp_servicename(SNUM(cnum)),
			 lp_max_connections(SNUM(cnum)));
	if (lp_status(SNUM(cnum))) yield_connection(cnum,"STATUS.",MAXSTATUS);
      }
      return(-5);      
    }

  string_set(&pcon->origpath,pcon->connectpath);

#if SOFTLINK_OPTIMISATION
  /* resolve any soft links early */
  {
    pstring s;
    strcpy(s,pcon->connectpath);
    GetWd(s);
    string_set(&pcon->connectpath,s);
    ChDir(pcon->connectpath);
  }
#endif

  num_connections_open++;
  add_session_user(user);
  
  /* execute any "preexec = " line */
  if (*lp_preexec(SNUM(cnum)))
    {
      pstring cmd;
      strcpy(cmd,lp_preexec(SNUM(cnum)));
      standard_sub(cnum,cmd);
      smbrun(cmd,NULL);
    }
  
  /* we've finished with the sensitive stuff */
  unbecome_user();

  {
    extern struct from_host Client_info;
    DEBUG(IS_IPC(cnum)?3:1,("%s %s (%s) connect to service %s as user %s (uid=%d,gid=%d) (pid %d)\n",
			    timestring(),
			    Client_info.name,Client_info.addr,
			    lp_servicename(SNUM(cnum)),user,
			    pcon->uid,
			    pcon->gid,
			    (int)getpid()));
  }

  return(cnum);
}


/****************************************************************************
  find first available file slot
****************************************************************************/
int find_free_file(void )
{
  int i;
  /* we start at 1 here for an obscure reason I can't now remember,
     but I think is important :-) */
  for (i=1;i<MAX_OPEN_FILES;i++)
    if (!Files[i].open)
      return(i);
  DEBUG(1,("ERROR! Out of file structures - perhaps increase MAX_OPEN_FILES?\n"));
  return(-1);
}

/****************************************************************************
  find first available connection slot, starting from a random position.
The randomisation stops problems with the server dieing and clients
thinking the server is still available.
****************************************************************************/
static int find_free_connection(int hash )
{
  int i;
  BOOL used=False;
  hash = (hash % (MAX_CONNECTIONS-2))+1;

 again:

  for (i=hash+1;i!=hash;)
    {
      if (!Connections[i].open && Connections[i].used == used) 
	{
	  DEBUG(3,("found free connection number %d\n",i));
	  return(i);
	}
      i++;
      if (i == MAX_CONNECTIONS)
	i = 1;
    }

  if (!used)
    {
      used = !used;
      goto again;
    }

  DEBUG(1,("ERROR! Out of connection structures\n"));
  return(-1);
}


/****************************************************************************
reply for the core protocol
****************************************************************************/
int reply_corep(char *outbuf)
{
  int outsize = set_message(outbuf,1,0,True);

  Protocol = PROTOCOL_CORE;

  return outsize;
}


/****************************************************************************
reply for the coreplus protocol
****************************************************************************/
int reply_coreplus(char *outbuf)
{
  int raw = (lp_readraw()?1:0) | (lp_writeraw()?2:0);
  int outsize = set_message(outbuf,13,0,True);
  SSVAL(outbuf,smb_vwv5,raw); /* tell redirector we support
				 readbraw and writebraw (possibly) */
  CVAL(outbuf,smb_flg) = 0x81; /* Reply, SMBlockread, SMBwritelock supported */
  SSVAL(outbuf,smb_vwv1,0x1); /* user level security, don't encrypt */	

  Protocol = PROTOCOL_COREPLUS;

  return outsize;
}


/****************************************************************************
reply for the lanman 1.0 protocol
****************************************************************************/
int reply_lanman1(char *outbuf)
{
  int raw = (lp_readraw()?1:0) | (lp_writeraw()?2:0);
  int secword=0;
  BOOL doencrypt = SMBENCRYPT();
  time_t t = time(NULL);

  if (lp_security()>=SEC_USER) secword |= 1;
  if (doencrypt) secword |= 2;

  set_message(outbuf,13,doencrypt?8:0,True);
  SSVAL(outbuf,smb_vwv1,secword); 
#ifdef SMB_PASSWD
  /* Create a token value and add it to the outgoing packet. */
  if (doencrypt) 
    generate_next_challenge(smb_buf(outbuf));
#endif

  Protocol = PROTOCOL_LANMAN1;

  if (lp_security() == SEC_SERVER && server_cryptkey(outbuf)) {
    DEBUG(3,("using password server validation\n"));
#ifdef SMB_PASSWD
  if (doencrypt) set_challenge(smb_buf(outbuf));    
#endif
  }

  CVAL(outbuf,smb_flg) = 0x81; /* Reply, SMBlockread, SMBwritelock supported */
  SSVAL(outbuf,smb_vwv2,maxxmit);
  SSVAL(outbuf,smb_vwv3,lp_maxmux()); /* maxmux */
  SSVAL(outbuf,smb_vwv4,1);
  SSVAL(outbuf,smb_vwv5,raw); /* tell redirector we support
				 readbraw writebraw (possibly) */
  SIVAL(outbuf,smb_vwv6,getpid());
  SSVAL(outbuf,smb_vwv10, TimeDiff(t)/60);

  put_dos_date(outbuf,smb_vwv8,t);

  return (smb_len(outbuf)+4);
}


/****************************************************************************
reply for the lanman 2.0 protocol
****************************************************************************/
int reply_lanman2(char *outbuf)
{
  int raw = (lp_readraw()?1:0) | (lp_writeraw()?2:0);
  int secword=0;
  BOOL doencrypt = SMBENCRYPT();
  time_t t = time(NULL);

  if (lp_security()>=SEC_USER) secword |= 1;
  if (doencrypt) secword |= 2;

  set_message(outbuf,13,doencrypt?8:0,True);
  SSVAL(outbuf,smb_vwv1,secword); 
#ifdef SMB_PASSWD
  /* Create a token value and add it to the outgoing packet. */
  if (doencrypt) 
    generate_next_challenge(smb_buf(outbuf));
#endif

  SIVAL(outbuf,smb_vwv6,getpid());

  Protocol = PROTOCOL_LANMAN2;

  if (lp_security() == SEC_SERVER && server_cryptkey(outbuf)) {
    DEBUG(3,("using password server validation\n"));
#ifdef SMB_PASSWD
    if (doencrypt) set_challenge(smb_buf(outbuf));    
#endif
  }

  CVAL(outbuf,smb_flg) = 0x81; /* Reply, SMBlockread, SMBwritelock supported */
  SSVAL(outbuf,smb_vwv2,maxxmit);
  SSVAL(outbuf,smb_vwv3,lp_maxmux()); 
  SSVAL(outbuf,smb_vwv4,1);
  SSVAL(outbuf,smb_vwv5,raw); /* readbraw and/or writebraw */
  SSVAL(outbuf,smb_vwv10, TimeDiff(t)/60);
  put_dos_date(outbuf,smb_vwv8,t);

  return (smb_len(outbuf)+4);
}

/****************************************************************************
reply for the nt protocol
****************************************************************************/
int reply_nt1(char *outbuf)
{
  int capabilities=0x300; /* has dual names + lock_and_read */
  int secword=0;
  BOOL doencrypt = SMBENCRYPT();
  time_t t = time(NULL);

  if (lp_security()>=SEC_USER) secword |= 1;
  if (doencrypt) secword |= 2;

  set_message(outbuf,17,doencrypt?8:0,True);
  CVAL(outbuf,smb_vwv1) = secword;
#ifdef SMB_PASSWD
  /* Create a token value and add it to the outgoing packet. */
  if (doencrypt) {
    generate_next_challenge(smb_buf(outbuf));
    /* Tell the nt machine how long the challenge is. */
    SSVALS(outbuf,smb_vwv16+1,8);
  }
#endif

  SIVAL(outbuf,smb_vwv7+1,getpid()); /* session key */

  Protocol = PROTOCOL_NT1;

  if (lp_security() == SEC_SERVER && server_cryptkey(outbuf)) {
    DEBUG(3,("using password server validation\n"));
#ifdef SMB_PASSWD
    if (doencrypt) set_challenge(smb_buf(outbuf));    
#endif
  }

  if (lp_readraw() && lp_writeraw())
    capabilities |= 1;

  SSVAL(outbuf,smb_vwv1+1,lp_maxmux()); /* maxmpx */
  SSVAL(outbuf,smb_vwv2+1,1); /* num vcs */
  SIVAL(outbuf,smb_vwv3+1,0xFFFF); /* max buffer */
  SIVAL(outbuf,smb_vwv5+1,0xFFFF); /* raw size */
  SIVAL(outbuf,smb_vwv9+1,capabilities); /* capabilities */
  put_long_date(outbuf+smb_vwv11+1,t);
  SSVALS(outbuf,smb_vwv15+1,TimeDiff(t)/60);

  return (smb_len(outbuf)+4);
}


/* these are the protocol lists used for auto architecture detection:

WinNT 3.51:
protocol [PC NETWORK PROGRAM 1.0]
protocol [XENIX CORE]
protocol [MICROSOFT NETWORKS 1.03]
protocol [LANMAN1.0]
protocol [Windows for Workgroups 3.1a]
protocol [LM1.2X002]
protocol [LANMAN2.1]
protocol [NT LM 0.12]

Win95:
protocol [PC NETWORK PROGRAM 1.0]
protocol [XENIX CORE]
protocol [MICROSOFT NETWORKS 1.03]
protocol [LANMAN1.0]
protocol [Windows for Workgroups 3.1a]
protocol [LM1.2X002]
protocol [LANMAN2.1]
protocol [NT LM 0.12]

OS/2:
protocol [PC NETWORK PROGRAM 1.0]
protocol [XENIX CORE]
protocol [LANMAN1.0]
protocol [LM1.2X002]
protocol [LANMAN2.1]
*/

/*
  * Modified to recognize the architecture of the remote machine better.
  *
  * This appears to be the matrix of which protocol is used by which
  * MS product.
       Protocol                       WfWg    Win95   WinNT  OS/2
       PC NETWORK PROGRAM 1.0          1       1       1      1
       XENIX CORE                                      2      2
       MICROSOFT NETWORKS 3.0          2       2       
       DOS LM1.2X002                   3       3       
       MICROSOFT NETWORKS 1.03                         3
       DOS LANMAN2.1                   4       4       
       LANMAN1.0                                       4      3
       Windows for Workgroups 3.1a     5       5       5
       LM1.2X002                                       6      4
       LANMAN2.1                                       7      5
       NT LM 0.12                              6       8
  *
  *  tim@fsg.com 09/29/95
  */
  
#define ARCH_WFWG     0x3      /* This is a fudge because WfWg is like Win95 */
#define ARCH_WIN95    0x2
#define	ARCH_OS2      0xC      /* Again OS/2 is like NT */
#define ARCH_WINNT    0x8
#define ARCH_SAMBA    0x10
 
#define ARCH_ALL      0x1F
 
/* List of supported protocols, most desired first */
struct {
  char *proto_name;
  char *short_name;
  int (*proto_reply_fn)(char *);
  int protocol_level;
} supported_protocols[] = {
  {"NT LANMAN 1.0",           "NT1",      reply_nt1,      PROTOCOL_NT1},
  {"NT LM 0.12",              "NT1",      reply_nt1,      PROTOCOL_NT1},
  {"LM1.2X002",               "LANMAN2",  reply_lanman2,  PROTOCOL_LANMAN2},
  {"Samba",                   "LANMAN2",  reply_lanman2,  PROTOCOL_LANMAN2},
  {"DOS LM1.2X002",           "LANMAN2",  reply_lanman2,  PROTOCOL_LANMAN2},
  {"LANMAN1.0",               "LANMAN1",  reply_lanman1,  PROTOCOL_LANMAN1},
  {"MICROSOFT NETWORKS 3.0",  "LANMAN1",  reply_lanman1,  PROTOCOL_LANMAN1},
  {"MICROSOFT NETWORKS 1.03", "COREPLUS", reply_coreplus, PROTOCOL_COREPLUS},
  {"PC NETWORK PROGRAM 1.0",  "CORE",     reply_corep,    PROTOCOL_CORE}, 
  {NULL,NULL},
};


/****************************************************************************
  reply to a negprot
****************************************************************************/
static int reply_negprot(char *inbuf,char *outbuf)
{
  extern fstring remote_arch;
  int outsize = set_message(outbuf,1,0,True);
  int Index=0;
  int choice= -1;
  int protocol;
  char *p;
  int bcc = SVAL(smb_buf(inbuf),-2);
  int arch = ARCH_ALL;

  p = smb_buf(inbuf)+1;
  while (p < (smb_buf(inbuf) + bcc))
    { 
      Index++;
      DEBUG(3,("Requested protocol [%s]\n",p));
      if (strcsequal(p,"Windows for Workgroups 3.1a"))
	arch &= ( ARCH_WFWG | ARCH_WIN95 | ARCH_WINNT );
      else if (strcsequal(p,"DOS LM1.2X002"))
	arch &= ( ARCH_WFWG | ARCH_WIN95 );
      else if (strcsequal(p,"DOS LANMAN2.1"))
	arch &= ( ARCH_WFWG | ARCH_WIN95 );
      else if (strcsequal(p,"NT LM 0.12"))
	arch &= ( ARCH_WIN95 | ARCH_WINNT );
      else if (strcsequal(p,"LANMAN2.1"))
	arch &= ( ARCH_WINNT | ARCH_OS2 );
      else if (strcsequal(p,"LM1.2X002"))
	arch &= ( ARCH_WINNT | ARCH_OS2 );
      else if (strcsequal(p,"MICROSOFT NETWORKS 1.03"))
	arch &= ARCH_WINNT;
      else if (strcsequal(p,"XENIX CORE"))
	arch &= ( ARCH_WINNT | ARCH_OS2 );
      else if (strcsequal(p,"Samba")) {
	arch = ARCH_SAMBA;
	break;
      }
 
      p += strlen(p) + 2;
    }
    
  switch ( arch ) {
  case ARCH_SAMBA:
    strcpy(remote_arch,"Samba");
    break;
  case ARCH_WFWG:
    strcpy(remote_arch,"WfWg");
    break;
  case ARCH_WIN95:
    strcpy(remote_arch,"Win95");
    break;
  case ARCH_WINNT:
    strcpy(remote_arch,"WinNT");
    break;
  case ARCH_OS2:
    strcpy(remote_arch,"OS2");
    break;
  default:
    strcpy(remote_arch,"UNKNOWN");
    break;
  }
 
  /* possibly reload - change of architecture */
  reload_services(True);      
    
  /* a special case to stop password server loops */
  if (Index == 1 && strequal(remote_machine,myhostname) && 
      lp_security()==SEC_SERVER)
    exit_server("Password server loop!");
  
  /* Check for protocols, most desirable first */
  for (protocol = 0; supported_protocols[protocol].proto_name; protocol++)
    {
      p = smb_buf(inbuf)+1;
      Index = 0;
      if (lp_maxprotocol() >= supported_protocols[protocol].protocol_level)
	while (p < (smb_buf(inbuf) + bcc))
	  { 
	    if (strequal(p,supported_protocols[protocol].proto_name))
	      choice = Index;
	    Index++;
	    p += strlen(p) + 2;
	  }
      if(choice != -1)
	break;
    }
  
  SSVAL(outbuf,smb_vwv0,choice);
  if(choice != -1) {
    extern fstring remote_proto;
    strcpy(remote_proto,supported_protocols[protocol].short_name);
    reload_services(True);          
    outsize = supported_protocols[protocol].proto_reply_fn(outbuf);
    DEBUG(3,("Selected protocol %s\n",supported_protocols[protocol].proto_name));
  }
  else {
    DEBUG(0,("No protocol supported !\n"));
  }
  SSVAL(outbuf,smb_vwv0,choice);
  
  DEBUG(5,("%s negprot index=%d\n",timestring(),choice));

  return(outsize);
}


/****************************************************************************
close all open files for a connection
****************************************************************************/
static void close_open_files(int cnum)
{
  int i;
  for (i=0;i<MAX_OPEN_FILES;i++)
    if( Files[i].cnum == cnum && Files[i].open) {
      close_file(i);
    }
}



/****************************************************************************
close a cnum
****************************************************************************/
void close_cnum(int cnum, int uid)
{
  extern struct from_host Client_info;

  DirCacheFlush(SNUM(cnum));

  unbecome_user();

  if (!OPEN_CNUM(cnum))
    {
      DEBUG(0,("Can't close cnum %d\n",cnum));
      return;
    }

  DEBUG(IS_IPC(cnum)?3:1,("%s %s (%s) closed connection to service %s\n",
			  timestring(),
			  Client_info.name,Client_info.addr,
			  lp_servicename(SNUM(cnum))));

  yield_connection(cnum,
		   lp_servicename(SNUM(cnum)),
		   lp_max_connections(SNUM(cnum)));

  if (lp_status(SNUM(cnum)))
    yield_connection(cnum,"STATUS.",MAXSTATUS);

  close_open_files(cnum);
  dptr_closecnum(cnum);

  /* execute any "postexec = " line */
  if (*lp_postexec(SNUM(cnum)) && become_user(cnum,uid))
    {
      pstring cmd;
      strcpy(cmd,lp_postexec(SNUM(cnum)));
      standard_sub(cnum,cmd);
      smbrun(cmd,NULL);
      unbecome_user();
    }

  unbecome_user();
  /* execute any "root postexec = " line */
  if (*lp_rootpostexec(SNUM(cnum)))
    {
      pstring cmd;
      strcpy(cmd,lp_rootpostexec(SNUM(cnum)));
      standard_sub(cnum,cmd);
      smbrun(cmd,NULL);
    }

  Connections[cnum].open = False;
  num_connections_open--;
  if (Connections[cnum].ngroups && Connections[cnum].groups)
    {
      if (Connections[cnum].igroups != (int *)Connections[cnum].groups)
	free(Connections[cnum].groups);
      free(Connections[cnum].igroups);
      Connections[cnum].groups = NULL;
      Connections[cnum].igroups = NULL;
      Connections[cnum].ngroups = 0;
    }

  string_set(&Connections[cnum].user,"");
  string_set(&Connections[cnum].dirpath,"");
  string_set(&Connections[cnum].connectpath,"");
}


/****************************************************************************
simple routines to do connection counting
****************************************************************************/
BOOL yield_connection(int cnum,char *name,int max_connections)
{
  struct connect_record crec;
  pstring fname;
  FILE *f;
  int mypid = getpid();
  int i;

  DEBUG(3,("Yielding connection to %d %s\n",cnum,name));

  if (max_connections <= 0)
    return(True);

  bzero(&crec,sizeof(crec));

  strcpy(fname,lp_lockdir());
  standard_sub(cnum,fname);
  trim_string(fname,"","/");

  strcat(fname,"/");
  strcat(fname,name);
  strcat(fname,".LCK");

  f = fopen(fname,"r+");
  if (!f)
    {
      DEBUG(2,("Coudn't open lock file %s (%s)\n",fname,strerror(errno)));
      return(False);
    }

  fseek(f,0,SEEK_SET);

  /* find a free spot */
  for (i=0;i<max_connections;i++)
    {
      if (fread(&crec,sizeof(crec),1,f) != 1)
	{
	  DEBUG(2,("Entry not found in lock file %s\n",fname));
	  fclose(f);
	  return(False);
	}
      if (crec.pid == mypid && crec.cnum == cnum)
	break;
    }

  if (crec.pid != mypid || crec.cnum != cnum)
    {
      fclose(f);
      DEBUG(2,("Entry not found in lock file %s\n",fname));
      return(False);
    }

  bzero((void *)&crec,sizeof(crec));
  
  /* remove our mark */
  if (fseek(f,i*sizeof(crec),SEEK_SET) != 0 ||
      fwrite(&crec,sizeof(crec),1,f) != 1)
    {
      DEBUG(2,("Couldn't update lock file %s (%s)\n",fname,strerror(errno)));
      fclose(f);
      return(False);
    }

  DEBUG(3,("Yield successful\n"));

  fclose(f);
  return(True);
}


/****************************************************************************
simple routines to do connection counting
****************************************************************************/
BOOL claim_connection(int cnum,char *name,int max_connections,BOOL Clear)
{
  struct connect_record crec;
  pstring fname;
  FILE *f;
  int snum = SNUM(cnum);
  int i,foundi= -1;
  int total_recs;

  if (max_connections <= 0)
    return(True);

  DEBUG(5,("trying claim %s %s %d\n",lp_lockdir(),name,max_connections));

  strcpy(fname,lp_lockdir());
  standard_sub(cnum,fname);
  trim_string(fname,"","/");

  if (!directory_exist(fname,NULL))
    mkdir(fname,0755);

  strcat(fname,"/");
  strcat(fname,name);
  strcat(fname,".LCK");

  if (!file_exist(fname,NULL))
    {
      f = fopen(fname,"w");
      if (f) fclose(f);
    }

  total_recs = file_size(fname) / sizeof(crec);

  f = fopen(fname,"r+");

  if (!f)
    {
      DEBUG(1,("couldn't open lock file %s\n",fname));
      return(False);
    }

  /* find a free spot */
  for (i=0;i<max_connections;i++)
    {

      if (i>=total_recs || 
	  fseek(f,i*sizeof(crec),SEEK_SET) != 0 ||
	  fread(&crec,sizeof(crec),1,f) != 1)
	{
	  if (foundi < 0) foundi = i;
	  break;
	}

      if (Clear && crec.pid && !process_exists(crec.pid))
	{
	  fseek(f,i*sizeof(crec),SEEK_SET);
	  bzero((void *)&crec,sizeof(crec));
	  fwrite(&crec,sizeof(crec),1,f);
	  if (foundi < 0) foundi = i;
	  continue;
	}
      if (foundi < 0 && (!crec.pid || !process_exists(crec.pid)))
	{
	  foundi=i;
	  if (!Clear) break;
	}
    }  

  if (foundi < 0)
    {
      DEBUG(3,("no free locks in %s\n",fname));
      fclose(f);
      return(False);
    }      

  /* fill in the crec */
  bzero((void *)&crec,sizeof(crec));
  crec.magic = 0x280267;
  crec.pid = getpid();
  crec.cnum = cnum;
  crec.uid = Connections[cnum].uid;
  crec.gid = Connections[cnum].gid;
  StrnCpy(crec.name,lp_servicename(snum),sizeof(crec.name)-1);
  crec.start = time(NULL);

  {
    extern struct from_host Client_info;
    StrnCpy(crec.machine,Client_info.name,sizeof(crec.machine)-1);
    StrnCpy(crec.addr,Client_info.addr,sizeof(crec.addr)-1);
  }
  
  /* make our mark */
  if (fseek(f,foundi*sizeof(crec),SEEK_SET) != 0 ||
      fwrite(&crec,sizeof(crec),1,f) != 1)
    {
      fclose(f);
      return(False);
    }

  fclose(f);
  return(True);
}

#if DUMP_CORE
/*******************************************************************
prepare to dump a core file - carefully!
********************************************************************/
static BOOL dump_core(void)
{
  char *p;
  pstring dname;
  strcpy(dname,debugf);
  if ((p=strrchr(dname,'/'))) *p=0;
  strcat(dname,"/corefiles");
  mkdir(dname,0700);
  sys_chown(dname,getuid(),getgid());
  chmod(dname,0700);
  if (chdir(dname)) return(False);
  umask(~(0700));

#ifndef NO_GETRLIMIT
#ifdef RLIMIT_CORE
  {
    struct rlimit rlp;
    getrlimit(RLIMIT_CORE, &rlp);
    rlp.rlim_cur = MAX(4*1024*1024,rlp.rlim_cur);
    setrlimit(RLIMIT_CORE, &rlp);
    getrlimit(RLIMIT_CORE, &rlp);
    DEBUG(3,("Core limits now %d %d\n",rlp.rlim_cur,rlp.rlim_max));
  }
#endif
#endif


  DEBUG(0,("Dumping core in %s\n",dname));
  return(True);
}
#endif

/****************************************************************************
exit the server
****************************************************************************/
void exit_server(char *reason)
{
  static int firsttime=1;
  int i;

  if (!firsttime) exit(0);
  firsttime = 0;

  unbecome_user();
  DEBUG(2,("Closing connections\n"));
  for (i=0;i<MAX_CONNECTIONS;i++)
    if (Connections[i].open)
      close_cnum(i,-1);
#ifdef DFS_AUTH
  if (dcelogin_atmost_once)
    dfs_unlogin();
#endif
  if (!reason) {   
    int oldlevel = DEBUGLEVEL;
    DEBUGLEVEL = 10;
    DEBUG(0,("Last message was %s\n",smb_fn_name(last_message)));
    if (last_inbuf)
      show_msg(last_inbuf);
    DEBUGLEVEL = oldlevel;
    DEBUG(0,("===============================================================\n"));
#if DUMP_CORE
    if (dump_core()) return;
#endif
  }    
  DEBUG(3,("%s Server exit  (%s)\n",timestring(),reason?reason:""));
  exit(0);
}

/****************************************************************************
do some standard substitutions in a string
****************************************************************************/
void standard_sub(int cnum,char *s)
{
  if (!strchr(s,'%')) return;

  if (VALID_CNUM(cnum))
    {
      string_sub(s,"%S",lp_servicename(Connections[cnum].service));
      string_sub(s,"%P",Connections[cnum].connectpath);
      string_sub(s,"%u",Connections[cnum].user);
      if (strstr(s,"%H")) {
	char *home = get_home_dir(Connections[cnum].user);
	if (home) string_sub(s,"%H",home);
      }
      string_sub(s,"%g",gidtoname(Connections[cnum].gid));
    }
  standard_sub_basic(s);
}

/*
These flags determine some of the permissions required to do an operation 

Note that I don't set NEED_WRITE on some write operations because they
are used by some brain-dead clients when printing, and I don't want to
force write permissions on print services.
*/
#define AS_USER (1<<0)
#define NEED_WRITE (1<<1)
#define TIME_INIT (1<<2)
#define CAN_IPC (1<<3)
#define AS_GUEST (1<<5)


/* 
   define a list of possible SMB messages and their corresponding
   functions. Any message that has a NULL function is unimplemented -
   please feel free to contribute implementations!
*/
struct smb_message_struct
{
  int code;
  char *name;
  int (*fn)();
  int flags;
#if PROFILING
  unsigned long time;
#endif
}
 smb_messages[] = {

    /* CORE PROTOCOL */

   {SMBnegprot,"SMBnegprot",reply_negprot,0},
   {SMBtcon,"SMBtcon",reply_tcon,0},
   {SMBtdis,"SMBtdis",reply_tdis,0},
   {SMBexit,"SMBexit",reply_exit,0},
   {SMBioctl,"SMBioctl",reply_ioctl,0},
   {SMBecho,"SMBecho",reply_echo,0},
   {SMBsesssetupX,"SMBsesssetupX",reply_sesssetup_and_X,0},
   {SMBtconX,"SMBtconX",reply_tcon_and_X,0},
   {SMBulogoffX, "SMBulogoffX", reply_ulogoffX, 0}, 
   {SMBgetatr,"SMBgetatr",reply_getatr,AS_USER},
   {SMBsetatr,"SMBsetatr",reply_setatr,AS_USER | NEED_WRITE},
   {SMBchkpth,"SMBchkpth",reply_chkpth,AS_USER},
   {SMBsearch,"SMBsearch",reply_search,AS_USER},
   {SMBopen,"SMBopen",reply_open,AS_USER},

   /* note that SMBmknew and SMBcreate are deliberately overloaded */   
   {SMBcreate,"SMBcreate",reply_mknew,AS_USER},
   {SMBmknew,"SMBmknew",reply_mknew,AS_USER}, 

   {SMBunlink,"SMBunlink",reply_unlink,AS_USER | NEED_WRITE},
   {SMBread,"SMBread",reply_read,AS_USER},
   {SMBwrite,"SMBwrite",reply_write,AS_USER},
   {SMBclose,"SMBclose",reply_close,AS_USER | CAN_IPC},
   {SMBmkdir,"SMBmkdir",reply_mkdir,AS_USER | NEED_WRITE},
   {SMBrmdir,"SMBrmdir",reply_rmdir,AS_USER | NEED_WRITE},
   {SMBdskattr,"SMBdskattr",reply_dskattr,AS_USER},
   {SMBmv,"SMBmv",reply_mv,AS_USER | NEED_WRITE},

   /* this is a Pathworks specific call, allowing the 
      changing of the root path */
   {pSETDIR,"pSETDIR",reply_setdir,AS_USER}, 

   {SMBlseek,"SMBlseek",reply_lseek,AS_USER},
   {SMBflush,"SMBflush",reply_flush,AS_USER},
   {SMBctemp,"SMBctemp",reply_ctemp,AS_USER},
   {SMBsplopen,"SMBsplopen",reply_printopen,AS_USER},
   {SMBsplclose,"SMBsplclose",reply_printclose,AS_USER},
   {SMBsplretq,"SMBsplretq",reply_printqueue,AS_USER|AS_GUEST},
   {SMBsplwr,"SMBsplwr",reply_printwrite,AS_USER},
   {SMBlock,"SMBlock",reply_lock,AS_USER},
   {SMBunlock,"SMBunlock",reply_unlock,AS_USER},
   
   /* CORE+ PROTOCOL FOLLOWS */
   
   {SMBreadbraw,"SMBreadbraw",reply_readbraw,AS_USER},
   {SMBwritebraw,"SMBwritebraw",reply_writebraw,AS_USER},
   {SMBwriteclose,"SMBwriteclose",reply_writeclose,AS_USER},
   {SMBlockread,"SMBlockread",reply_lockread,AS_USER},
   {SMBwriteunlock,"SMBwriteunlock",reply_writeunlock,AS_USER},
   
   /* LANMAN1.0 PROTOCOL FOLLOWS */
   
   {SMBreadBmpx,"SMBreadBmpx",reply_readbmpx,AS_USER},
   {SMBreadBs,"SMBreadBs",NULL,AS_USER},
   {SMBwriteBmpx,"SMBwriteBmpx",reply_writebmpx,AS_USER},
   {SMBwriteBs,"SMBwriteBs",reply_writebs,AS_USER},
   {SMBwritec,"SMBwritec",NULL,AS_USER},
   {SMBsetattrE,"SMBsetattrE",reply_setattrE,AS_USER | NEED_WRITE},
   {SMBgetattrE,"SMBgetattrE",reply_getattrE,AS_USER},
   {SMBtrans,"SMBtrans",reply_trans,AS_USER | CAN_IPC},
   {SMBtranss,"SMBtranss",NULL,AS_USER | CAN_IPC},
   {SMBioctls,"SMBioctls",NULL,AS_USER},
   {SMBcopy,"SMBcopy",reply_copy,AS_USER | NEED_WRITE},
   {SMBmove,"SMBmove",NULL,AS_USER | NEED_WRITE},
   
   {SMBopenX,"SMBopenX",reply_open_and_X,AS_USER | CAN_IPC},
   {SMBreadX,"SMBreadX",reply_read_and_X,AS_USER},
   {SMBwriteX,"SMBwriteX",reply_write_and_X,AS_USER},
   {SMBlockingX,"SMBlockingX",reply_lockingX,AS_USER},
   
   {SMBffirst,"SMBffirst",reply_search,AS_USER},
   {SMBfunique,"SMBfunique",reply_search,AS_USER},
   {SMBfclose,"SMBfclose",reply_fclose,AS_USER},

   /* LANMAN2.0 PROTOCOL FOLLOWS */
   {SMBfindnclose, "SMBfindnclose", reply_findnclose, AS_USER},
   {SMBfindclose, "SMBfindclose", reply_findclose,AS_USER},
   {SMBtrans2, "SMBtrans2", reply_trans2, AS_USER},
   {SMBtranss2, "SMBtranss2", reply_transs2, AS_USER},

   /* messaging routines */
   {SMBsends,"SMBsends",reply_sends,AS_GUEST},
   {SMBsendstrt,"SMBsendstrt",reply_sendstrt,AS_GUEST},
   {SMBsendend,"SMBsendend",reply_sendend,AS_GUEST},
   {SMBsendtxt,"SMBsendtxt",reply_sendtxt,AS_GUEST},

   /* NON-IMPLEMENTED PARTS OF THE CORE PROTOCOL */
   
   {SMBsendb,"SMBsendb",NULL,AS_GUEST},
   {SMBfwdname,"SMBfwdname",NULL,AS_GUEST},
   {SMBcancelf,"SMBcancelf",NULL,AS_GUEST},
   {SMBgetmac,"SMBgetmac",NULL,AS_GUEST}
 };

/****************************************************************************
return a string containing the function name of a SMB command
****************************************************************************/
char *smb_fn_name(int type)
{
  static char *unknown_name = "SMBunknown";
  static int num_smb_messages = 
    sizeof(smb_messages) / sizeof(struct smb_message_struct);
  int match;

  for (match=0;match<num_smb_messages;match++)
    if (smb_messages[match].code == type)
      break;

  if (match == num_smb_messages)
    return(unknown_name);

  return(smb_messages[match].name);
}


/****************************************************************************
do a switch on the message type, and return the response size
****************************************************************************/
static int switch_message(int type,char *inbuf,char *outbuf,int size,int bufsize)
{
  static int pid= -1;
  int outsize = 0;
  static int num_smb_messages = 
    sizeof(smb_messages) / sizeof(struct smb_message_struct);
  int match;

#if PROFILING
  struct timeval msg_start_time;
  struct timeval msg_end_time;
  static unsigned long total_time = 0;

  GetTimeOfDay(&msg_start_time);
#endif

  if (pid == -1)
    pid = getpid();

  errno = 0;
  last_message = type;

  /* make sure this is an SMB packet */
  if (strncmp(smb_base(inbuf),"\377SMB",4) != 0)
    {
      DEBUG(2,("Non-SMB packet of length %d\n",smb_len(inbuf)));
      return(-1);
    }

  for (match=0;match<num_smb_messages;match++)
    if (smb_messages[match].code == type)
      break;

  if (match == num_smb_messages)
    {
      DEBUG(0,("Unknown message type %d!\n",type));
      outsize = reply_unknown(inbuf,outbuf);
    }
  else
    {
      DEBUG(3,("switch message %s (pid %d)\n",smb_messages[match].name,pid));
      if (smb_messages[match].fn)
	{
	  int cnum = SVAL(inbuf,smb_tid);
	  int flags = smb_messages[match].flags;
	  int uid = SVAL(inbuf,smb_uid);

	  /* does this protocol need to be run as root? */
	  if (!(flags & AS_USER))
	    unbecome_user();

	  /* does this protocol need to be run as the connected user? */
	  if ((flags & AS_USER) && !become_user(cnum,uid)) {
	    if (flags & AS_GUEST) 
	      flags &= ~AS_USER;
	    else
	      return(ERROR(ERRSRV,ERRinvnid));
	  }
	  /* this code is to work around a bug is MS client 3 without
	     introducing a security hole - it needs to be able to do
	     print queue checks as guest if it isn't logged in properly */
	  if (flags & AS_USER)
	    flags &= ~AS_GUEST;

	  /* does it need write permission? */
	  if ((flags & NEED_WRITE) && !CAN_WRITE(cnum))
	    return(ERROR(ERRSRV,ERRaccess));

	  /* ipc services are limited */
	  if (IS_IPC(cnum) && (flags & AS_USER) && !(flags & CAN_IPC))
	    return(ERROR(ERRSRV,ERRaccess));	    

	  /* load service specific parameters */
	  if (OPEN_CNUM(cnum) && !become_service(cnum,(flags & AS_USER)?True:False))
	    return(ERROR(ERRSRV,ERRaccess));

	  /* does this protocol need to be run as guest? */
	  if ((flags & AS_GUEST) && (!become_guest() || !check_access(-1)))
	    return(ERROR(ERRSRV,ERRaccess));

	  last_inbuf = inbuf;

	  outsize = smb_messages[match].fn(inbuf,outbuf,size,bufsize);
	}
      else
	{
	  outsize = reply_unknown(inbuf,outbuf);
	}
    }

#if PROFILING
  GetTimeOfDay(&msg_end_time);
  if (!(smb_messages[match].flags & TIME_INIT))
    {
      smb_messages[match].time = 0;
      smb_messages[match].flags |= TIME_INIT;
    }
  {
    unsigned long this_time =     
      (msg_end_time.tv_sec - msg_start_time.tv_sec)*1e6 +
	(msg_end_time.tv_usec - msg_start_time.tv_usec);
    smb_messages[match].time += this_time;
    total_time += this_time;
  }
  DEBUG(2,("TIME %s  %d usecs   %g pct\n",
	   smb_fn_name(type),smb_messages[match].time,
	(100.0*smb_messages[match].time) / total_time));
#endif

  return(outsize);
}


/****************************************************************************
  construct a chained reply and add it to the already made reply
  **************************************************************************/
int chain_reply(char *inbuf,char *outbuf,int size,int bufsize)
{
  static char *orig_inbuf;
  static char *orig_outbuf;
  int smb_com1, smb_com2 = CVAL(inbuf,smb_vwv0);
  unsigned smb_off2 = SVAL(inbuf,smb_vwv1);
  char *inbuf2, *outbuf2;
  int outsize2;
  char inbuf_saved[smb_wct];
  char outbuf_saved[smb_wct];
  extern int chain_size;
  int wct = CVAL(outbuf,smb_wct);
  int outsize = smb_size + 2*wct + SVAL(outbuf,smb_vwv0+2*wct);

  /* maybe its not chained */
  if (smb_com2 == 0xFF) {
    CVAL(outbuf,smb_vwv0) = 0xFF;
    return outsize;
  }

  if (chain_size == 0) {
    /* this is the first part of the chain */
    orig_inbuf = inbuf;
    orig_outbuf = outbuf;
  }

  /* we need to tell the client where the next part of the reply will be */
  SSVAL(outbuf,smb_vwv1,smb_offset(outbuf+outsize,outbuf));
  CVAL(outbuf,smb_vwv0) = smb_com2;

  /* remember how much the caller added to the chain, only counting stuff
     after the parameter words */
  chain_size += outsize - smb_wct;

  /* work out pointers into the original packets. The
     headers on these need to be filled in */
  inbuf2 = orig_inbuf + smb_off2 + 4 - smb_wct;
  outbuf2 = orig_outbuf + SVAL(outbuf,smb_vwv1) + 4 - smb_wct;

  /* remember the original command type */
  smb_com1 = CVAL(orig_inbuf,smb_com);

  /* save the data which will be overwritten by the new headers */
  memcpy(inbuf_saved,inbuf2,smb_wct);
  memcpy(outbuf_saved,outbuf2,smb_wct);

  /* give the new packet the same header as the last part of the SMB */
  memmove(inbuf2,inbuf,smb_wct);

  /* create the in buffer */
  CVAL(inbuf2,smb_com) = smb_com2;

  /* create the out buffer */
  bzero(outbuf2,smb_size);
  set_message(outbuf2,0,0,True);
  CVAL(outbuf2,smb_com) = CVAL(inbuf2,smb_com);
  
  memcpy(outbuf2+4,inbuf2+4,4);
  CVAL(outbuf2,smb_rcls) = SUCCESS;
  CVAL(outbuf2,smb_reh) = 0;
  CVAL(outbuf2,smb_flg) = 0x80 | (CVAL(inbuf2,smb_flg) & 0x8); /* bit 7 set 
								  means a reply */
  SSVAL(outbuf2,smb_flg2,1); /* say we support long filenames */
  SSVAL(outbuf2,smb_err,SUCCESS);
  SSVAL(outbuf2,smb_tid,SVAL(inbuf2,smb_tid));
  SSVAL(outbuf2,smb_pid,SVAL(inbuf2,smb_pid));
  SSVAL(outbuf2,smb_uid,SVAL(inbuf2,smb_uid));
  SSVAL(outbuf2,smb_mid,SVAL(inbuf2,smb_mid));

  DEBUG(3,("Chained message\n"));
  show_msg(inbuf2);

  /* process the request */
  outsize2 = switch_message(smb_com2,inbuf2,outbuf2,size-chain_size,
			    bufsize-chain_size);

  /* copy the new reply and request headers over the old ones, but
     preserve the smb_com field */
  memmove(orig_outbuf,outbuf2,smb_wct);
  CVAL(orig_outbuf,smb_com) = smb_com1;

  /* restore the saved data, being careful not to overwrite any
   data from the reply header */
  memcpy(inbuf2,inbuf_saved,smb_wct);
  {
    int ofs = smb_wct - PTR_DIFF(outbuf2,orig_outbuf);
    if (ofs < 0) ofs = 0;
    memmove(outbuf2+ofs,outbuf_saved+ofs,smb_wct-ofs);
  }

  return outsize2;
}



/****************************************************************************
  construct a reply to the incoming packet
****************************************************************************/
int construct_reply(char *inbuf,char *outbuf,int size,int bufsize)
{
  int type = CVAL(inbuf,smb_com);
  int outsize = 0;
  int msg_type = CVAL(inbuf,0);
  extern int chain_size;

  smb_last_time = time(NULL);

  chain_size = 0;
  chain_fnum = -1;

  bzero(outbuf,smb_size);

  if (msg_type != 0)
    return(reply_special(inbuf,outbuf));  

  CVAL(outbuf,smb_com) = CVAL(inbuf,smb_com);
  set_message(outbuf,0,0,True);
  
  memcpy(outbuf+4,inbuf+4,4);
  CVAL(outbuf,smb_rcls) = SUCCESS;
  CVAL(outbuf,smb_reh) = 0;
  CVAL(outbuf,smb_flg) = 0x80 | (CVAL(inbuf,smb_flg) & 0x8); /* bit 7 set 
							     means a reply */
  SSVAL(outbuf,smb_flg2,1); /* say we support long filenames */
  SSVAL(outbuf,smb_err,SUCCESS);
  SSVAL(outbuf,smb_tid,SVAL(inbuf,smb_tid));
  SSVAL(outbuf,smb_pid,SVAL(inbuf,smb_pid));
  SSVAL(outbuf,smb_uid,SVAL(inbuf,smb_uid));
  SSVAL(outbuf,smb_mid,SVAL(inbuf,smb_mid));

  outsize = switch_message(type,inbuf,outbuf,size,bufsize);

  outsize += chain_size;

  if(outsize > 4)
    smb_setlen(outbuf,outsize - 4);
  return(outsize);
}


/****************************************************************************
  process commands from the client
****************************************************************************/
static void process(void)
{
  static int trans_num = 0;
  int nread;
  extern struct from_host Client_info;
  extern int Client;

  fromhost(Client,&Client_info);
  
  InBuffer = (char *)malloc(BUFFER_SIZE + SAFETY_MARGIN);
  OutBuffer = (char *)malloc(BUFFER_SIZE + SAFETY_MARGIN);
  if ((InBuffer == NULL) || (OutBuffer == NULL)) 
    return;

  InBuffer += SMB_ALIGNMENT;
  OutBuffer += SMB_ALIGNMENT;

#if PRIME_NMBD
  DEBUG(3,("priming nmbd\n"));
  {
    struct in_addr ip;
    ip = *interpret_addr2("localhost");
    if (zero_ip(ip)) ip = *interpret_addr2("127.0.0.1");
    *OutBuffer = 0;
    send_one_packet(OutBuffer,1,ip,NMB_PORT,SOCK_DGRAM);
  }
#endif    

  while (True)
    {
      int32 len;      
      int msg_type;
      int msg_flags;
      int type;
      int deadtime = lp_deadtime()*60;
      int counter;
      int last_keepalive=0;

      if (deadtime <= 0)
	deadtime = DEFAULT_SMBD_TIMEOUT;

      if (lp_readprediction())
	do_read_prediction();

      {
	extern pstring share_del_pending;
	if (*share_del_pending) {
	  unbecome_user();
	  if (!unlink(share_del_pending))
	    DEBUG(3,("Share file deleted %s\n",share_del_pending));
	  else
	    DEBUG(2,("Share del failed of %s\n",share_del_pending));
	  share_del_pending[0] = 0;
	}
      }

      if (share_mode_pending) {
	unbecome_user();
	check_share_modes();
	share_mode_pending=False;
      }

      errno = 0;      

      for (counter=SMBD_SELECT_LOOP; 
	   !receive_smb(Client,InBuffer,SMBD_SELECT_LOOP*1000); 
	   counter += SMBD_SELECT_LOOP)
	{
	  int i;
	  time_t t;
	  BOOL allidle = True;
	  extern int keepalive;

	  if (smb_read_error == READ_EOF) {
	    DEBUG(3,("end of file from client\n"));
	    return;
	  }

	  if (smb_read_error == READ_ERROR) {
	    DEBUG(3,("receive_smb error (%s) exiting\n",
		     strerror(errno)));
	    return;
	  }

	  t = time(NULL);

	  /* become root again if waiting */
	  unbecome_user();

	  /* check for smb.conf reload */
	  if (!(counter%SMBD_RELOAD_CHECK))
	    reload_services(True);

	  /* check the share modes every 10 secs */
	  if (!(counter%SHARE_MODES_CHECK))
	    check_share_modes();

	  /* clean the share modes every 5 minutes */
	  if (!(counter%SHARE_MODES_CLEAN))
	    clean_share_modes();

	  /* automatic timeout if all connections are closed */      
	  if (num_connections_open==0 && counter >= IDLE_CLOSED_TIMEOUT) {
	    DEBUG(2,("%s Closing idle connection\n",timestring()));
	    return;
	  }

	  if (keepalive && (counter-last_keepalive)>keepalive) {
	    extern int password_client;
	    if (!send_keepalive(Client)) {
	      DEBUG(2,("%s Keepalive failed - exiting\n",timestring()));
	      return;
	    }	    
	    /* also send a keepalive to the password server if its still
	       connected */
	    if (password_client != -1)
	      send_keepalive(password_client);
	    last_keepalive = counter;
	  }

	  /* check for connection timeouts */
	  for (i=0;i<MAX_CONNECTIONS;i++)
	    if (Connections[i].open)
	      {
		/* close dirptrs on connections that are idle */
		if ((t-Connections[i].lastused)>DPTR_IDLE_TIMEOUT)
		  dptr_idlecnum(i);

		if (Connections[i].num_files_open > 0 ||
		    (t-Connections[i].lastused)<deadtime)
		  allidle = False;
	      }

	  if (allidle && num_connections_open>0) {
	    DEBUG(2,("%s Closing idle connection 2\n",timestring()));
	    return;
	  }
	}

      msg_type = CVAL(InBuffer,0);
      msg_flags = CVAL(InBuffer,1);
      type = CVAL(InBuffer,smb_com);

      len = smb_len(InBuffer);

      DEBUG(6,("got message type 0x%x of len 0x%x\n",msg_type,len));

      nread = len + 4;
      
      DEBUG(3,("%s Transaction %d of length %d\n",timestring(),trans_num,nread));

#ifdef WITH_VTP
      if(trans_num == 1 && VT_Check(InBuffer)) {
        VT_Process();
        return;
      }
#endif


      if (msg_type == 0)
	show_msg(InBuffer);

      nread = construct_reply(InBuffer,OutBuffer,nread,maxxmit);
      
      if(nread > 0) {
        if (CVAL(OutBuffer,0) == 0)
	  show_msg(OutBuffer);
	
        if (nread != smb_len(OutBuffer) + 4) 
	  {
	    DEBUG(0,("ERROR: Invalid message response size! %d %d\n",
		     nread,
		     smb_len(OutBuffer)));
	  }
	else
	  send_smb(Client,OutBuffer);
      }
      trans_num++;
    }
}


/****************************************************************************
  initialise connect, service and file structs
****************************************************************************/
static void init_structs(void )
{
  int i;
  get_myname(myhostname,NULL);

  for (i=0;i<MAX_CONNECTIONS;i++)
    {
      Connections[i].open = False;
      Connections[i].num_files_open=0;
      Connections[i].lastused=0;
      Connections[i].used=False;
      string_init(&Connections[i].user,"");
      string_init(&Connections[i].dirpath,"");
      string_init(&Connections[i].connectpath,"");
      string_init(&Connections[i].origpath,"");
    }

  for (i=0;i<MAX_OPEN_FILES;i++)
    {
      Files[i].open = False;
      string_init(&Files[i].name,"");
    }

  init_dptrs();
}

/****************************************************************************
usage on the program
****************************************************************************/
static void usage(char *pname)
{
  DEBUG(0,("Incorrect program usage - are you sure the command line is correct?\n"));

  printf("Usage: %s [-D] [-p port] [-d debuglevel] [-l log basename] [-s services file]\n",pname);
  printf("Version %s\n",VERSION);
  printf("\t-D                    become a daemon\n");
  printf("\t-p port               listen on the specified port\n");
  printf("\t-d debuglevel         set the debuglevel\n");
  printf("\t-l log basename.      Basename for log/debug files\n");
  printf("\t-s services file.     Filename of services file\n");
  printf("\t-P                    passive only\n");
  printf("\t-a                    overwrite log file, don't append\n");
  printf("\n");
}


/****************************************************************************
  main program
****************************************************************************/
 int main(int argc,char *argv[])
{
  extern BOOL append_log;
  /* shall I run as a daemon */
  BOOL is_daemon = False;
  int port = SMB_PORT;
  int opt;
  extern char *optarg;

#ifdef NEED_AUTH_PARAMETERS
  set_auth_parameters(argc,argv);
#endif

#ifdef SecureWare
  setluid(0);
#endif

  /* get br0's ipaddress and store it to global struct in_addr myipaddr */
  {
      int fd;
      struct ifreq ifr;
      struct sockaddr_in* sin;

      if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
        ifr.ifr_addr.sa_family = AF_INET;
        strcpy(ifr.ifr_name, "br0");
        if (ioctl(fd, SIOCGIFADDR, &ifr) != 0)
          DEBUG(0,("Cannot get br0's ip addr\n"));
        else {
          myipaddr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;
          DEBUG(0,("br0's IP address: %s\n",inet_ntoa(myipaddr)));
        }
        close(fd);
      }
  }

  append_log = True;

  TimeInit();

  strcpy(debugf,SMBLOGFILE);  

  setup_logging(argv[0],False);

  charset_initialise();

  /* make absolutely sure we run as root - to handle cases whre people
     are crazy enough to have it setuid */
#ifdef USE_SETRES
  setresuid(0,0,0);
#else
  setuid(0);
  seteuid(0);
  setuid(0);
  seteuid(0);
#endif

  fault_setup(exit_server);

  umask(0777 & ~DEF_CREATE_MASK);

  init_uid();

  /* this is for people who can't start the program correctly */
  while (argc > 1 && (*argv[1] != '-'))
    {
      argv++;
      argc--;
    }

  while ((opt = getopt(argc, argv, "O:i:l:s:d:Dp:hPa")) != EOF)
    switch (opt)
      {
      case 'O':
	strcpy(user_socket_options,optarg);
	break;
      case 'i':
	strcpy(scope,optarg);
	break;
      case 'P':
	{
	  extern BOOL passive;
	  passive = True;
	}
	break;	
      case 's':
	strcpy(servicesf,optarg);
	break;
      case 'l':
	strcpy(debugf,optarg);
	break;
      case 'a':
	{
	  extern BOOL append_log;
	  append_log = !append_log;
	}
	break;
      case 'D':
	is_daemon = True;
	break;
      case 'd':
	if (*optarg == 'A')
	  DEBUGLEVEL = 10000;
	else
	  DEBUGLEVEL = atoi(optarg);
	break;
      case 'p':
	port = atoi(optarg);
	break;
      case 'h':
	usage(argv[0]);
	exit(0);
	break;
      default:
	usage(argv[0]);
	exit(1);
      }

  reopen_logs();

  DEBUG(2,("%s smbd version %s started\n",timestring(),VERSION));
  DEBUG(2,("Copyright Andrew Tridgell 1992-1995\n"));

#ifndef NO_GETRLIMIT
#ifdef RLIMIT_NOFILE
  {
    struct rlimit rlp;
    getrlimit(RLIMIT_NOFILE, &rlp);
    rlp.rlim_cur = (MAX_OPEN_FILES>rlp.rlim_max)? rlp.rlim_max:MAX_OPEN_FILES;
    setrlimit(RLIMIT_NOFILE, &rlp);
    getrlimit(RLIMIT_NOFILE, &rlp);
    DEBUG(3,("Maximum number of open files per session is %d\n",rlp.rlim_cur));
  }
#endif
#endif

  
  DEBUG(2,("uid=%d gid=%d euid=%d egid=%d\n",
	getuid(),getgid(),geteuid(),getegid()));

  if (sizeof(uint16) < 2 || sizeof(uint32) < 4)
    {
      DEBUG(0,("ERROR: Samba is not configured correctly for the word size on your machine\n"));
      exit(1);
    }

  init_structs();

  if (!reload_services(False))
    return(-1);	

#ifndef NO_SIGNAL_TEST
  signal(SIGHUP,SIGNAL_CAST sig_hup);
#endif
  
  DEBUG(3,("%s loaded services\n",timestring()));

  if (!is_daemon && !is_a_socket(0))
    {
      DEBUG(0,("standard input is not a socket, assuming -D option\n"));
      is_daemon = True;
    }

  if (is_daemon)
    {
      DEBUG(3,("%s becoming a daemon\n",timestring()));
      become_daemon();
    }

  if (!open_sockets(is_daemon,port))
    exit(1);

#if FAST_SHARE_MODES
  if (!start_share_mode_mgmt())
    exit(1);
#endif

  /* possibly reload the services file. */
  reload_services(True);

  maxxmit = MIN(lp_maxxmit(),BUFFER_SIZE);

  if (*lp_rootdir())
    {
      if (sys_chroot(lp_rootdir()) == 0)
	DEBUG(2,("%s changed root to %s\n",timestring(),lp_rootdir()));
    }

  process();
  close_sockets();

#if FAST_SHARE_MODES
  stop_share_mode_mgmt();
#endif

  exit_server("normal exit");
  return(0);
}


