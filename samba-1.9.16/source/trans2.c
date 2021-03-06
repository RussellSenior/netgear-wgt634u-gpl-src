/* 
   Unix SMB/Netbios implementation.
   Version 1.9.
   SMB transaction2 handling
   Copyright (C) Jeremy Allison 1994

   Extensively modified by Andrew Tridgell, 1995

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

extern int DEBUGLEVEL;
extern int Protocol;
extern connection_struct Connections[];
extern files_struct Files[];
extern BOOL case_sensitive;
extern int Client;

/****************************************************************************
  Send the required number of replies back.
  We assume all fields other than the data fields are
  set correctly for the type of call.
  HACK ! Always assumes smb_setup field is zero.
****************************************************************************/
static int send_trans2_replies(char *outbuf, int bufsize, char *params, 
			 int paramsize, char *pdata, int datasize)
{
  /* As we are using a protocol > LANMAN1 then the maxxmit
     variable must have been set in the sessetupX call.
     This takes precedence over the max_xmit field in the
     global struct. These different max_xmit variables should
     be merged as this is now too confusing */

  extern int maxxmit;
  int data_to_send = datasize;
  int params_to_send = paramsize;
  int useable_space;
  char *pp = params;
  char *pd = pdata;
  int params_sent_thistime, data_sent_thistime, total_sent_thistime;
  int alignment_offset = 1;

  /* Initially set the wcnt area to be 10 - this is true for all
     trans2 replies */
  set_message(outbuf,10,0,True);

  /* If there genuinely are no parameters or data to send just send
     the empty packet */
  if(params_to_send == 0 && data_to_send == 0)
    {
      send_smb(Client,outbuf);
      return 0;
    }

  /* Space is bufsize minus Netbios over TCP header minus SMB header */
  /* The + 1 is to align the param and data bytes on an even byte
     boundary. NT 4.0 Beta needs this to work correctly. */
  useable_space = bufsize - ((smb_buf(outbuf)+alignment_offset) - outbuf);
  useable_space = MIN(useable_space, maxxmit); /* XXX is this needed? correct? */

  while( params_to_send || data_to_send)
    {
      /* Calculate whether we will totally or partially fill this packet */
      total_sent_thistime = params_to_send + data_to_send + alignment_offset;
      total_sent_thistime = MIN(total_sent_thistime, useable_space);

      set_message(outbuf, 10, total_sent_thistime, True);

      /* Set total params and data to be sent */
      SSVAL(outbuf,smb_tprcnt,paramsize);
      SSVAL(outbuf,smb_tdrcnt,datasize);

      /* Calculate how many parameters and data we can fit into
	 this packet. Parameters get precedence */

      params_sent_thistime = MIN(params_to_send,useable_space);
      data_sent_thistime = useable_space - params_sent_thistime;
      data_sent_thistime = MIN(data_sent_thistime,data_to_send);

      SSVAL(outbuf,smb_prcnt, params_sent_thistime);
      if(params_sent_thistime == 0)
	{
	  SSVAL(outbuf,smb_proff,0);
	  SSVAL(outbuf,smb_prdisp,0);
	} else {
	  /* smb_proff is the offset from the start of the SMB header to the
	     parameter bytes, however the first 4 bytes of outbuf are
	     the Netbios over TCP header. Thus use smb_base() to subtract
	     them from the calculation */
	  SSVAL(outbuf,smb_proff,((smb_buf(outbuf)+alignment_offset) - smb_base(outbuf)));
	  /* Absolute displacement of param bytes sent in this packet */
	  SSVAL(outbuf,smb_prdisp,pp - params);
	}

      SSVAL(outbuf,smb_drcnt, data_sent_thistime);
      if(data_sent_thistime == 0)
	{
	  SSVAL(outbuf,smb_droff,0);
	  SSVAL(outbuf,smb_drdisp, 0);
	} else {
	  /* The offset of the data bytes is the offset of the
	     parameter bytes plus the number of parameters being sent this time */
	  SSVAL(outbuf,smb_droff,((smb_buf(outbuf)+alignment_offset) - 
				  smb_base(outbuf)) + params_sent_thistime);
	  SSVAL(outbuf,smb_drdisp, pd - pdata);
	}

      /* Copy the param bytes into the packet */
      if(params_sent_thistime)
	memcpy((smb_buf(outbuf)+alignment_offset),pp,params_sent_thistime);
      /* Copy in the data bytes */
      if(data_sent_thistime)
	memcpy(smb_buf(outbuf)+alignment_offset+params_sent_thistime,pd,data_sent_thistime);

      DEBUG(9,("t2_rep: params_sent_thistime = %d, data_sent_thistime = %d, useable_space = %d\n",
	       params_sent_thistime, data_sent_thistime, useable_space));
      DEBUG(9,("t2_rep: params_to_send = %d, data_to_send = %d, paramsize = %d, datasize = %d\n",
	       params_to_send, data_to_send, paramsize, datasize));

      /* Send the packet */
      send_smb(Client,outbuf);

      pp += params_sent_thistime;
      pd += data_sent_thistime;

      params_to_send -= params_sent_thistime;
      data_to_send -= data_sent_thistime;

      /* Sanity check */
      if(params_to_send < 0 || data_to_send < 0)
	{
	  DEBUG(2,("send_trans2_replies failed sanity check pts = %d, dts = %d\n!!!",
		   params_to_send, data_to_send));
	  return -1;
	}
    }

  return 0;
}


/****************************************************************************
  reply to a TRANSACT2_OPEN
****************************************************************************/
static int call_trans2open(char *inbuf, char *outbuf, int bufsize, int cnum, 
		    char **pparams, char **ppdata)
{
  char *params = *pparams;
  int16 open_mode = SVAL(params, 2);
  int16 open_attr = SVAL(params,6);
#if 0
  BOOL return_additional_info = BITSETW(params,0);
  int16 open_sattr = SVAL(params, 4);
  time_t open_time = make_unix_date3(params+8);
#endif
  int16 open_ofun = SVAL(params,12);
  int32 open_size = IVAL(params,14);
  char *pname = &params[28];
  int16 namelen = strlen(pname)+1;

  pstring fname;
  int fnum = -1;
  int unixmode;
  int size=0,fmode=0,mtime=0,rmode;
  int32 inode = 0;
  struct stat sbuf;
  int smb_action = 0;

  StrnCpy(fname,pname,namelen);

  DEBUG(3,("trans2open %s cnum=%d mode=%d attr=%d ofun=%d size=%d\n",
	   fname,cnum,open_mode, open_attr, open_ofun, open_size));

  /* XXXX we need to handle passed times, sattr and flags */

  unix_convert(fname,cnum);
    
  fnum = find_free_file();
  if (fnum < 0)
    return(ERROR(ERRSRV,ERRnofids));

  if (!check_name(fname,cnum))
    return(UNIXERROR(ERRDOS,ERRnoaccess));

  unixmode = unix_mode(cnum,open_attr | aARCH);
      
      
  open_file_shared(fnum,cnum,fname,open_mode,open_ofun,unixmode,
		   &rmode,&smb_action);
      
  if (!Files[fnum].open)
    return(UNIXERROR(ERRDOS,ERRnoaccess));

  if (fstat(Files[fnum].fd,&sbuf) != 0) {
    close_file(fnum);
    return(ERROR(ERRDOS,ERRnoaccess));
  }
    
  size = sbuf.st_size;
  fmode = dos_mode(cnum,fname,&sbuf);
  mtime = sbuf.st_mtime;
  inode = sbuf.st_ino;
  if (fmode & aDIR) {
    close_file(fnum);
    return(ERROR(ERRDOS,ERRnoaccess));
  }

  /* Realloc the size of parameters and data we will return */
  params = *pparams = Realloc(*pparams, 28);
  if(params == NULL)
    return(ERROR(ERRDOS,ERRnomem));

  bzero(params,28);
  SSVAL(params,0,fnum);
  SSVAL(params,2,fmode);
  put_dos_date2(params,4, mtime);
  SIVAL(params,8, size);
  SSVAL(params,12,rmode);

  SSVAL(params,18,smb_action);
  SIVAL(params,20,inode);
 
  /* Send the required number of replies */
  send_trans2_replies(outbuf, bufsize, params, 28, *ppdata, 0);

  return -1;
}

/****************************************************************************
  get a level dependent lanman2 dir entry.
****************************************************************************/
static int get_lanman2_dir_entry(int cnum,char *path_mask,int dirtype,int info_level,
				 int requires_resume_key,
				 BOOL dont_descend,char **ppdata, 
				 char *base_data, int space_remaining, 
				 BOOL *out_of_space,
				 int *last_name_off)
{
  char *dname;
  BOOL found = False;
  struct stat sbuf;
  pstring mask;
  pstring pathreal;
  pstring fname;
  BOOL matched;
  char *p, *pdata = *ppdata;
  int reskey=0, prev_dirpos=0;
  int mode=0;
  uint32 size=0,len;
  uint32 mdate=0, adate=0, cdate=0;
  char *nameptr;
  BOOL isrootdir = (strequal(Connections[cnum].dirpath,"./") ||
		    strequal(Connections[cnum].dirpath,".") ||
		    strequal(Connections[cnum].dirpath,"/"));
  BOOL was_8_3;

  *fname = 0;
  *out_of_space = False;

  if (!Connections[cnum].dirptr)
    return(False);

  p = strrchr(path_mask,'/');
  if(p != NULL)
    {
      if(p[1] == '\0')
	strcpy(mask,"*.*");
      else
	strcpy(mask, p+1);
    }
  else
    strcpy(mask, path_mask);

  while (!found)
    {
      /* Needed if we run out of space */
      prev_dirpos = TellDir(Connections[cnum].dirptr);
      dname = ReadDirName(Connections[cnum].dirptr);

      reskey = TellDir(Connections[cnum].dirptr);

      DEBUG(6,("get_lanman2_dir_entry:readdir on dirptr 0x%x now at offset %d\n",
	       Connections[cnum].dirptr,TellDir(Connections[cnum].dirptr)));
      
      if (!dname) 
	return(False);

      matched = False;

      strcpy(fname,dname);      

      if(mask_match(fname, mask, case_sensitive, True))
	{
	  BOOL isdots = (strequal(fname,"..") || strequal(fname,"."));
	  if (dont_descend && !isdots)
	    continue;
	  
	  if (isrootdir && isdots)
	    continue;

	  strcpy(pathreal,Connections[cnum].dirpath);
	  strcat(pathreal,"/");
	  strcat(pathreal,fname);
	  if (sys_stat(pathreal,&sbuf) != 0) 
	    {
			if ( errno!=ENOENT ) {
				DEBUG(5,("get_lanman2_dir_entry:Couldn't stat [%s] (%s)\n",pathreal,strerror(errno)));
				continue;
			}
			if (sys_lstat(pathreal,&sbuf) != 0) {
				DEBUG(5,("get_lanman2_dir_entry:Couldn't lstat [%s] (%s)\n",pathreal,strerror(errno)));
				continue;
			} 
			else {
				sbuf.st_mode=(S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);
			}
	    }

	  mode = dos_mode(cnum,pathreal,&sbuf);

	  if (!dir_check_ftype(cnum,mode,&sbuf,dirtype)) {
	    DEBUG(5,("[%s] attribs didn't match %x\n",fname,dirtype));
	    continue;
	  }

	  size = sbuf.st_size;
	  mdate = sbuf.st_mtime;
	  adate = sbuf.st_atime;
	  cdate = sbuf.st_ctime;
	  if(mode & aDIR)
	    size = 0;

	  DEBUG(5,("get_lanman2_dir_entry found %s fname=%s\n",pathreal,fname));
	  
	  found = True;
	}
    }


#ifndef KANJI
  unix2dos_format(fname, True);
#endif

  p = pdata;
  nameptr = p;

  name_map_mangle(fname,False,SNUM(cnum));

  switch (info_level)
    {
    case 1:
      if(requires_resume_key) {
	SIVAL(p,0,reskey);
	p += 4;
      }
      put_dos_date2(p,l1_fdateCreation,cdate);
      put_dos_date2(p,l1_fdateLastAccess,adate);
      put_dos_date2(p,l1_fdateLastWrite,mdate);
      SIVAL(p,l1_cbFile,size);
      SIVAL(p,l1_cbFileAlloc,ROUNDUP(size,1024));
      SSVAL(p,l1_attrFile,mode);
      SCVAL(p,l1_cchName,strlen(fname));
      strcpy(p + l1_achName, fname);
      nameptr = p + l1_achName;
      p += l1_achName + strlen(fname) + 1;
      break;

    case 2:
      /* info_level 2 */
      if(requires_resume_key) {
	SIVAL(p,0,reskey);
	p += 4;
      }
      put_dos_date2(p,l2_fdateCreation,cdate);
      put_dos_date2(p,l2_fdateLastAccess,adate);
      put_dos_date2(p,l2_fdateLastWrite,mdate);
      SIVAL(p,l2_cbFile,size);
      SIVAL(p,l2_cbFileAlloc,ROUNDUP(size,1024));
      SSVAL(p,l2_attrFile,mode);
      SIVAL(p,l2_cbList,0); /* No extended attributes */
      SCVAL(p,l2_cchName,strlen(fname));
      strcpy(p + l2_achName, fname);
      nameptr = p + l2_achName;
      p += l2_achName + strlen(fname) + 1;
      break;

    case 3:
      SIVAL(p,0,reskey);
      put_dos_date2(p,4,cdate);
      put_dos_date2(p,8,adate);
      put_dos_date2(p,12,mdate);
      SIVAL(p,16,size);
      SIVAL(p,20,ROUNDUP(size,1024));
      SSVAL(p,24,mode);
      SIVAL(p,26,4);
      CVAL(p,30) = strlen(fname);
      strcpy(p+31, fname);
      nameptr = p+31;
      p += 31 + strlen(fname) + 1;
      break;

    case 4:
      if(requires_resume_key) {
	SIVAL(p,0,reskey);
	p += 4;
      }
      SIVAL(p,0,33+strlen(fname)+1);
      put_dos_date2(p,4,cdate);
      put_dos_date2(p,8,adate);
      put_dos_date2(p,12,mdate);
      SIVAL(p,16,size);
      SIVAL(p,20,ROUNDUP(size,1024));
      SSVAL(p,24,mode);
      CVAL(p,32) = strlen(fname);
      strcpy(p + 33, fname);
      nameptr = p+33;
      p += 33 + strlen(fname) + 1;
      break;

    case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
      was_8_3 = is_8_3(fname);
      len = 94+strlen(fname);
      len = (len + 3) & ~3;
      SIVAL(p,0,len); p += 4;
      SIVAL(p,0,reskey); p += 4;
      put_long_date(p,cdate); p += 8;
      put_long_date(p,adate); p += 8;
      put_long_date(p,mdate); p += 8;
      put_long_date(p,mdate); p += 8;
      SIVAL(p,0,size); p += 8;
      SIVAL(p,0,size); p += 8;
      SIVAL(p,0,mode); p += 4;
      SIVAL(p,0,strlen(fname)); p += 4;
      SIVAL(p,0,0); p += 4;
      if (!was_8_3) {
#ifndef KANJI
	strcpy(p+2,unix2dos_format(fname,False));
#else 
	strcpy(p+2,fname);
#endif
	if (!name_map_mangle(p+2,True,SNUM(cnum)))
	  (p+2)[12] = 0;
      } else
	*(p+2) = 0;
      strupper(p+2);
      SSVAL(p,0,strlen(p+2));
      p += 2 + 24;
      /* nameptr = p;  */
      strcpy(p,fname); p += strlen(p);
      p = pdata + len;
      break;

    case SMB_FIND_FILE_DIRECTORY_INFO:
      len = 64+strlen(fname);
      len = (len + 3) & ~3;
      SIVAL(p,0,len); p += 4;
      SIVAL(p,0,reskey); p += 4;
      put_long_date(p,cdate); p += 8;
      put_long_date(p,adate); p += 8;
      put_long_date(p,mdate); p += 8;
      put_long_date(p,mdate); p += 8;
      SIVAL(p,0,size); p += 8;
      SIVAL(p,0,size); p += 8;
      SIVAL(p,0,mode); p += 4;
      SIVAL(p,0,strlen(fname)); p += 4;
      strcpy(p,fname);
      p = pdata + len;
      break;
      
      
    case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
      len = 68+strlen(fname);
      len = (len + 3) & ~3;
      SIVAL(p,0,len); p += 4;
      SIVAL(p,0,reskey); p += 4;
      put_long_date(p,cdate); p += 8;
      put_long_date(p,adate); p += 8;
      put_long_date(p,mdate); p += 8;
      put_long_date(p,mdate); p += 8;
      SIVAL(p,0,size); p += 8;
      SIVAL(p,0,size); p += 8;
      SIVAL(p,0,mode); p += 4;
      SIVAL(p,0,strlen(fname)); p += 4;
      SIVAL(p,0,0); p += 4;
      strcpy(p,fname);
      p = pdata + len;
      break;

    case SMB_FIND_FILE_NAMES_INFO:
      len = 12+strlen(fname);
      len = (len + 3) & ~3;
      SIVAL(p,0,len); p += 4;
      SIVAL(p,0,reskey); p += 4;
      SIVAL(p,0,strlen(fname)); p += 4;
      strcpy(p,fname);
      p = pdata + len;
      break;

    default:      
      return(False);
    }


  if (PTR_DIFF(p,pdata) > space_remaining) {
    /* Move the dirptr back to prev_dirpos */
    SeekDir(Connections[cnum].dirptr, prev_dirpos);
    *out_of_space = True;
    DEBUG(9,("get_lanman2_dir_entry: out of space\n"));
    return False; /* Not finished - just out of space */
  }

  /* Setup the last_filename pointer, as an offset from base_data */
  *last_name_off = PTR_DIFF(nameptr,base_data);
  /* Advance the data pointer to the next slot */
  *ppdata = p;
  return(found);
}
  
/****************************************************************************
  reply to a TRANS2_FINDFIRST
****************************************************************************/
static int call_trans2findfirst(char *inbuf, char *outbuf, int bufsize, int cnum, 
			 char **pparams, char **ppdata)
{
  /* We must be careful here that we don't return more than the
     allowed number of data bytes. If this means returning fewer than
     maxentries then so be it. We assume that the redirector has
     enough room for the fixed number of parameter bytes it has
     requested. */
  uint32 max_data_bytes = SVAL(inbuf, smb_mdrcnt);
  char *params = *pparams;
  char *pdata = *ppdata;
  int dirtype = SVAL(params,0);
  int maxentries = SVAL(params,2);
  BOOL close_after_first = BITSETW(params+4,0);
  BOOL close_if_end = BITSETW(params+4,1);
  BOOL requires_resume_key = BITSETW(params+4,2);
  int info_level = SVAL(params,6);
  pstring directory;
  pstring mask;
  char *p, *wcard;
  int last_name_off=0;
  int dptr_num = -1;
  int numentries = 0;
  int i;
  BOOL finished = False;
  BOOL dont_descend = False;
  BOOL out_of_space = False;
  int space_remaining;

  *directory = *mask = 0;

  DEBUG(3,("call_trans2findfirst: dirtype = %d, maxentries = %d, close_after_first=%d, close_if_end = %d requires_resume_key = %d level = %d, max_data_bytes = %d\n",
	   dirtype, maxentries, close_after_first, close_if_end, requires_resume_key,
	   info_level, max_data_bytes));
  
  switch (info_level) 
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case SMB_FIND_FILE_DIRECTORY_INFO:
    case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_NAMES_INFO:
    case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
      break;
    default:
      return(ERROR(ERRDOS,ERRunknownlevel));
    }

  strcpy(directory, params + 12); /* Complete directory path with 
				     wildcard mask appended */

  DEBUG(5,("path=%s\n",directory));

  unix_convert(directory,cnum);
  if(!check_name(directory,cnum)) {
    return(ERROR(ERRDOS,ERRbadpath));
  }

  p = strrchr(directory,'/');
  if(p == NULL) {
    strcpy(mask,directory);
    strcpy(directory,"./");
  } else {
    strcpy(mask,p+1);
    *p = 0;
  }

  DEBUG(5,("dir=%s, mask = %s\n",directory, mask));

  pdata = *ppdata = Realloc(*ppdata, max_data_bytes + 1024);
  if(!*ppdata)
    return(ERROR(ERRDOS,ERRnomem));
  bzero(pdata,max_data_bytes);

  /* Realloc the params space */
  params = *pparams = Realloc(*pparams, 10);
  if(params == NULL)
    return(ERROR(ERRDOS,ERRnomem));

  dptr_num = dptr_create(cnum,directory, True ,SVAL(inbuf,smb_pid));
  if (dptr_num < 0)
    return(ERROR(ERRDOS,ERRbadpath));

  /* convert the formatted masks */
  {
    p = mask;
    while (*p) {
      if (*p == '<') *p = '*';
      if (*p == '>') *p = '?';
      if (*p == '"') *p = '.';
      p++;
    }
  }
  
  /* a special case for 16 bit apps */
  if (strequal(mask,"????????.???")) strcpy(mask,"*");

  /* handle broken clients that send us old 8.3 format */
  string_sub(mask,"????????","*");
  string_sub(mask,".???",".*");

  /* Save the wildcard match and attribs we are using on this directory - 
     needed as lanman2 assumes these are being saved between calls */

  if(!(wcard = strdup(mask))) {
    dptr_close(dptr_num);
    return(ERROR(ERRDOS,ERRnomem));
  }

  dptr_set_wcard(dptr_num, wcard);
  dptr_set_attr(dptr_num, dirtype);

  DEBUG(4,("dptr_num is %d, wcard = %s, attr = %d\n",dptr_num, wcard, dirtype));

  /* We don't need to check for VOL here as this is returned by 
     a different TRANS2 call. */
  
  DEBUG(8,("dirpath=<%s> dontdescend=<%s>\n",
	   Connections[cnum].dirpath,lp_dontdescend(SNUM(cnum))));
  if (in_list(Connections[cnum].dirpath,lp_dontdescend(SNUM(cnum)),case_sensitive))
    dont_descend = True;
    
  p = pdata;
  space_remaining = max_data_bytes;
  out_of_space = False;

  for (i=0;(i<maxentries) && !finished && !out_of_space;i++)
    {

      /* this is a heuristic to avoid seeking the dirptr except when 
	 absolutely necessary. It allows for a filename of about 40 chars */
      if (space_remaining < DIRLEN_GUESS && numentries > 0)
	{
	  out_of_space = True;
	  finished = False;
	}
      else
	{
	  finished = 
	    !get_lanman2_dir_entry(cnum,mask,dirtype,info_level,
				   requires_resume_key,dont_descend,
				   &p,pdata,space_remaining, &out_of_space,
				   &last_name_off);
	}

      if (finished && out_of_space)
	finished = False;

      if (!finished && !out_of_space)
	numentries++;
      space_remaining = max_data_bytes - PTR_DIFF(p,pdata);
    }
  
  /* Check if we can close the dirptr */
  if(close_after_first || (finished && close_if_end))
    {
      dptr_close(dptr_num);
      DEBUG(5,("call_trans2findfirst - (2) closing dptr_num %d\n", dptr_num));
      dptr_num = -1;
    }

  /* At this point pdata points to numentries directory entries. */

  /* Set up the return parameter block */
  SSVAL(params,0,dptr_num);
  SSVAL(params,2,numentries);
  SSVAL(params,4,finished);
  SSVAL(params,6,0); /* Never an EA error */
  SSVAL(params,8,last_name_off);

  send_trans2_replies( outbuf, bufsize, params, 10, pdata, PTR_DIFF(p,pdata));

  if ((! *directory) && dptr_path(dptr_num))
    sprintf(directory,"(%s)",dptr_path(dptr_num));

  DEBUG(4,("%s %s mask=%s directory=%s cnum=%d dirtype=%d numentries=%d\n",
	timestring(),
	smb_fn_name(CVAL(inbuf,smb_com)), 
	mask,directory,cnum,dirtype,numentries));

  return(-1);
}


/****************************************************************************
  reply to a TRANS2_FINDNEXT
****************************************************************************/
static int call_trans2findnext(char *inbuf, char *outbuf, int length, int bufsize,
			int cnum, char **pparams, char **ppdata)
{
  /* We must be careful here that we don't return more than the
     allowed number of data bytes. If this means returning fewer than
     maxentries then so be it. We assume that the redirector has
     enough room for the fixed number of parameter bytes it has
     requested. */
  int max_data_bytes = SVAL(inbuf, smb_mdrcnt);
  char *params = *pparams;
  char *pdata = *ppdata;
  int16 dptr_num = SVAL(params,0);
  int maxentries = SVAL(params,2);
  uint16 info_level = SVAL(params,4);
  uint32 resume_key = IVAL(params,6);
  BOOL close_after_request = BITSETW(params+10,0);
  BOOL close_if_end = BITSETW(params+10,1);
  BOOL requires_resume_key = BITSETW(params+10,2);
  BOOL continue_bit = BITSETW(params+10,3);
  pstring mask;
  pstring directory;
  char *p;
  uint16 dirtype;
  int numentries = 0;
  int i, last_name_off=0;
  BOOL finished = False;
  BOOL dont_descend = False;
  BOOL out_of_space = False;
  int space_remaining;

  *mask = *directory = 0;

  DEBUG(3,("call_trans2findnext: dirhandle = %d, max_data_bytes = %d, maxentries = %d, close_after_request=%d, close_if_end = %d requires_resume_key = %d resume_key = %d continue=%d level = %d\n",
	   dptr_num, max_data_bytes, maxentries, close_after_request, close_if_end, 
	   requires_resume_key, resume_key, continue_bit, info_level));

  switch (info_level) 
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case SMB_FIND_FILE_DIRECTORY_INFO:
    case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_NAMES_INFO:
    case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
      break;
    default:
      return(ERROR(ERRDOS,ERRunknownlevel));
    }

  pdata = *ppdata = Realloc( *ppdata, max_data_bytes + 1024);
  if(!*ppdata)
    return(ERROR(ERRDOS,ERRnomem));
  bzero(pdata,max_data_bytes);

  /* Realloc the params space */
  params = *pparams = Realloc(*pparams, 6*SIZEOFWORD);
  if(!params)
    return(ERROR(ERRDOS,ERRnomem));

  /* Check that the dptr is valid */
  if(!(Connections[cnum].dirptr = dptr_fetch_lanman2(params, dptr_num)))
    return(ERROR(ERRDOS,ERRnofiles));

  string_set(&Connections[cnum].dirpath,dptr_path(dptr_num));

  /* Get the wildcard mask from the dptr */
  if((p = dptr_wcard(dptr_num))== NULL) {
    DEBUG(2,("dptr_num %d has no wildcard\n", dptr_num));
    return (ERROR(ERRDOS,ERRnofiles));
  }
  strcpy(mask, p);
  strcpy(directory,Connections[cnum].dirpath);

  /* Get the attr mask from the dptr */
  dirtype = dptr_attr(dptr_num);

  DEBUG(3,("dptr_num is %d, mask = %s, attr = %x, dirptr=(0x%X,%d)\n",
	   dptr_num, mask, dirtype, 
	   Connections[cnum].dirptr,
	   TellDir(Connections[cnum].dirptr)));

  /* We don't need to check for VOL here as this is returned by 
     a different TRANS2 call. */

  DEBUG(8,("dirpath=<%s> dontdescend=<%s>\n",Connections[cnum].dirpath,lp_dontdescend(SNUM(cnum))));
  if (in_list(Connections[cnum].dirpath,lp_dontdescend(SNUM(cnum)),case_sensitive))
    dont_descend = True;
    
  p = pdata;
  space_remaining = max_data_bytes;
  out_of_space = False;

  /* If we have a resume key - seek to the correct position. */
  if(requires_resume_key && !continue_bit)
    SeekDir(Connections[cnum].dirptr, resume_key);

  for (i=0;(i<(int)maxentries) && !finished && !out_of_space ;i++)
    {
      /* this is a heuristic to avoid seeking the dirptr except when 
	 absolutely necessary. It allows for a filename of about 40 chars */
      if (space_remaining < DIRLEN_GUESS && numentries > 0)
	{
	  out_of_space = True;
	  finished = False;
	}
      else
	{
	  finished = 
	    !get_lanman2_dir_entry(cnum,mask,dirtype,info_level,
				   requires_resume_key,dont_descend,
				   &p,pdata,space_remaining, &out_of_space,
				   &last_name_off);
	}

      if (finished && out_of_space)
	finished = False;

      if (!finished && !out_of_space)
	numentries++;
      space_remaining = max_data_bytes - PTR_DIFF(p,pdata);
    }
  
  /* Check if we can close the dirptr */
  if(close_after_request || (finished && close_if_end))
    {
      dptr_close(dptr_num); /* This frees up the saved mask */
      DEBUG(5,("call_trans2findnext: closing dptr_num = %d\n", dptr_num));
      dptr_num = -1;
    }


  /* Set up the return parameter block */
  SSVAL(params,0,numentries);
  SSVAL(params,2,finished);
  SSVAL(params,4,0); /* Never an EA error */
  SSVAL(params,6,last_name_off);

  send_trans2_replies( outbuf, bufsize, params, 8, pdata, PTR_DIFF(p,pdata));

  if ((! *directory) && dptr_path(dptr_num))
    sprintf(directory,"(%s)",dptr_path(dptr_num));

  DEBUG(3,("%s %s mask=%s directory=%s cnum=%d dirtype=%d numentries=%d\n",
	   timestring(),
	   smb_fn_name(CVAL(inbuf,smb_com)), 
	   mask,directory,cnum,dirtype,numentries));

  return(-1);
}

/****************************************************************************
  reply to a TRANS2_QFSINFO (query filesystem info)
****************************************************************************/
static int call_trans2qfsinfo(char *inbuf, char *outbuf, int length, int bufsize,
			int cnum, char **pparams, char **ppdata)
{
  char *pdata = *ppdata;
  char *params = *pparams;
  uint16 info_level = SVAL(params,0);
  int data_len;
  struct stat st;
  char *vname = volume_label(SNUM(cnum));
  

  DEBUG(3,("call_trans2qfsinfo: cnum = %d, level = %d\n", cnum, info_level));

  if(sys_stat(".",&st)!=0) {
    DEBUG(2,("call_trans2qfsinfo: stat of . failed (%s)\n", strerror(errno)));
    return (ERROR(ERRSRV,ERRinvdevice));
  }

  pdata = *ppdata = Realloc(*ppdata, 1024); bzero(pdata,1024);

  switch (info_level) 
    {
    case 1:
      {
	int dfree,dsize,bsize;
	data_len = 18;
	bsize = cnum;
	dfree = bsize + 1;
	dsize = bsize - 1;
	sys_disk_free(".",&bsize,&dfree,&dsize);	
	SIVAL(pdata,l1_idFileSystem,st.st_dev);
	SIVAL(pdata,l1_cSectorUnit,bsize/512);
	SIVAL(pdata,l1_cUnit,dsize);
	SIVAL(pdata,l1_cUnitAvail,dfree);
	SSVAL(pdata,l1_cbSector,512);
	DEBUG(5,("call_trans2qfsinfo : bsize=%d, id=%x, cSectorUnit=%d, cUnit=%d, cUnitAvail=%d, cbSector=%d\n",
		 bsize, st.st_dev, bsize/512, dsize, dfree, 512));
	break;
    }
    case 2:
    { 
      /* Return volume name */
      int volname_len = MIN(strlen(vname),11);
      data_len = l2_vol_szVolLabel + volname_len + 1;
      put_dos_date2(pdata,l2_vol_fdateCreation,st.st_ctime);
      SCVAL(pdata,l2_vol_cch,volname_len);
      StrnCpy(pdata+l2_vol_szVolLabel,vname,volname_len);
      DEBUG(5,("call_trans2qfsinfo : time = %x, namelen = %d, name = %s\n",st.st_ctime,volname_len,
	       pdata+l2_vol_szVolLabel));
      break;
    }
    case SMB_QUERY_FS_ATTRIBUTE_INFO:
      data_len = 12 + 2*strlen(FSTYPE_STRING);
      SIVAL(pdata,0,0x4006); /* FS ATTRIBUTES == long filenames supported? */
      SIVAL(pdata,4,128); /* Max filename component length */
      SIVAL(pdata,8,2*strlen(FSTYPE_STRING));
      PutUniCode(pdata+12,FSTYPE_STRING);
      break;
    case SMB_QUERY_FS_LABEL_INFO:
      data_len = 4 + strlen(vname);
      SIVAL(pdata,0,strlen(vname));
      strcpy(pdata+4,vname);      
      break;
    case SMB_QUERY_FS_VOLUME_INFO:      
      data_len = 17 + strlen(vname);
      SIVAL(pdata,12,strlen(vname));
      strcpy(pdata+17,vname);      
      break;
    case SMB_QUERY_FS_SIZE_INFO:
      {
	int dfree,dsize,bsize;
	data_len = 24;
	sys_disk_free(".",&bsize,&dfree,&dsize);	
	SIVAL(pdata,0,dsize);
	SIVAL(pdata,8,dfree);
	SIVAL(pdata,16,bsize/512);
	SIVAL(pdata,20,512);
      }
      break;
    case SMB_QUERY_FS_DEVICE_INFO:
      data_len = 8;
      SIVAL(pdata,0,0); /* dev type */
      SIVAL(pdata,4,0); /* characteristics */
      break;
    default:
      return(ERROR(ERRDOS,ERRunknownlevel));
    }


  send_trans2_replies( outbuf, bufsize, params, 0, pdata, data_len);

  DEBUG(4,("%s %s info_level =%d\n",timestring(),smb_fn_name(CVAL(inbuf,smb_com)), info_level));

  return -1;
}

/****************************************************************************
  reply to a TRANS2_SETFSINFO (set filesystem info)
****************************************************************************/
static int call_trans2setfsinfo(char *inbuf, char *outbuf, int length, int bufsize,
			int cnum, char **pparams, char **ppdata)
{
  /* Just say yes we did it - there is nothing that
     can be set here so it doesn't matter. */
  int outsize;
  DEBUG(3,("call_trans2setfsinfo\n"));

  if (!CAN_WRITE(cnum))
    return(ERROR(ERRSRV,ERRaccess));

  outsize = set_message(outbuf,10,0,True);

  return outsize;
}

/****************************************************************************
  reply to a TRANS2_QFILEINFO (query file info by fileid)
****************************************************************************/
static int call_trans2qfilepathinfo(char *inbuf, char *outbuf, int length, 
				    int bufsize,int cnum,
				    char **pparams,char **ppdata,
				    int total_data)
{
  char *params = *pparams;
  char *pdata = *ppdata;
  uint16 tran_call = SVAL(inbuf, smb_setup0);
  uint16 info_level;
  int mode=0;
  int size=0;
  unsigned int data_size;
  struct stat sbuf;
  pstring fname1;
  char *fname;
  char *p;
  int l,pos;


  if (tran_call == TRANSACT2_QFILEINFO) {
    int16 fnum = SVALS(params,0);
    info_level = SVAL(params,2);

    CHECK_FNUM(fnum,cnum);
    CHECK_ERROR(fnum);

    fname = Files[fnum].name;
    if (fstat(Files[fnum].fd,&sbuf) != 0) {
      DEBUG(3,("fstat of fnum %d failed (%s)\n",fnum, strerror(errno)));
      return(UNIXERROR(ERRDOS,ERRbadfid));
    }
    pos = lseek(Files[fnum].fd,0,SEEK_CUR);
  } else {
    /* qpathinfo */
    info_level = SVAL(params,0);
    fname = &fname1[0];
    strcpy(fname,&params[6]);
    unix_convert(fname,cnum);
    if (!check_name(fname,cnum) || sys_stat(fname,&sbuf)) {
      DEBUG(3,("fileinfo of %s failed (%s)\n",fname,strerror(errno)));
      return(UNIXERROR(ERRDOS,ERRbadpath));
    }
    pos = 0;
  }


  DEBUG(3,("call_trans2qfilepathinfo %s level=%d call=%d total_data=%d\n",
	   fname,info_level,tran_call,total_data));

  p = strrchr(fname,'/'); 
  if (!p) 
    p = fname;
  else
    p++;
  l = strlen(p);  
  mode = dos_mode(cnum,fname,&sbuf);
  size = sbuf.st_size;
  if (mode & aDIR) size = 0;
  
  params = *pparams = Realloc(*pparams,2); bzero(params,2);
  data_size = 1024;
  pdata = *ppdata = Realloc(*ppdata, data_size); 

  if (total_data > 0 && IVAL(pdata,0) == total_data) {
    /* uggh, EAs for OS2 */
    DEBUG(4,("Rejecting EA request with total_data=%d\n",total_data));
#if 0
    SSVAL(params,0,ERROR_EAS_NOT_SUPPORTED);
    send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
    return(-1);
#else
    return(ERROR(ERRDOS,ERROR_EAS_NOT_SUPPORTED));
#endif
  }

  bzero(pdata,data_size);

  switch (info_level) 
    {
    case 1:
    case 2:
      data_size = (info_level==1?22:26);
      put_dos_date2(pdata,l1_fdateCreation,sbuf.st_ctime);
      put_dos_date2(pdata,l1_fdateLastAccess,sbuf.st_atime);
      put_dos_date2(pdata,l1_fdateLastWrite,sbuf.st_mtime);
      SIVAL(pdata,l1_cbFile,size);
      SIVAL(pdata,l1_cbFileAlloc,ROUNDUP(size,1024));
      SSVAL(pdata,l1_attrFile,mode);
      SIVAL(pdata,l1_attrFile+2,4); /* this is what OS2 does */
      break;

    case 3:
      data_size = 24;
      put_dos_date2(pdata,0,sbuf.st_ctime);
      put_dos_date2(pdata,4,sbuf.st_atime);
      put_dos_date2(pdata,8,sbuf.st_mtime);
      SIVAL(pdata,12,size);
      SIVAL(pdata,16,ROUNDUP(size,1024));
      SIVAL(pdata,20,mode);
      break;

    case 4:
      data_size = 4;
      SIVAL(pdata,0,data_size);
      break;

    case 6:
      return(ERROR(ERRDOS,ERRbadfunc)); /* os/2 needs this */      

    case SMB_QUERY_FILE_BASIC_INFO:
      data_size = 36;
      put_long_date(pdata,sbuf.st_ctime);
      put_long_date(pdata+8,sbuf.st_atime);
      put_long_date(pdata+16,sbuf.st_mtime);
      put_long_date(pdata+24,sbuf.st_mtime);
      SIVAL(pdata,32,mode);
      break;

    case SMB_QUERY_FILE_STANDARD_INFO:
      data_size = 22;
      SIVAL(pdata,0,size);
      SIVAL(pdata,8,size);
      SIVAL(pdata,16,sbuf.st_nlink);
      CVAL(pdata,20) = 0;
      CVAL(pdata,21) = (mode&aDIR)?1:0;
      break;

    case SMB_QUERY_FILE_EA_INFO:
      data_size = 4;
      break;

    case SMB_QUERY_FILE_NAME_INFO:
    case SMB_QUERY_FILE_ALT_NAME_INFO:
      data_size = 4 + l;
      SIVAL(pdata,0,l);
      strcpy(pdata+4,fname);
      break;
    case SMB_QUERY_FILE_ALLOCATION_INFO:
    case SMB_QUERY_FILE_END_OF_FILEINFO:
      data_size = 8;
      SIVAL(pdata,0,size);
      break;

    case SMB_QUERY_FILE_ALL_INFO:
      put_long_date(pdata,sbuf.st_ctime);
      put_long_date(pdata+8,sbuf.st_atime);
      put_long_date(pdata+16,sbuf.st_mtime);
      put_long_date(pdata+24,sbuf.st_mtime);
      SIVAL(pdata,32,mode);
      pdata += 40;
      SIVAL(pdata,0,size);
      SIVAL(pdata,8,size);
      SIVAL(pdata,16,sbuf.st_nlink);
      CVAL(pdata,20) = 0;
      CVAL(pdata,21) = (mode&aDIR)?1:0;
      pdata += 24;
      pdata += 8; /* index number */
      pdata += 4; /* EA info */
      if (mode & aRONLY)
	SIVAL(pdata,0,0xA9);
      else
	SIVAL(pdata,0,0xd01BF);
      pdata += 4;
      SIVAL(pdata,0,pos); /* current offset */
      pdata += 8;
      SIVAL(pdata,0,mode); /* is this the right sort of mode info? */
      pdata += 4;
      pdata += 4; /* alignment */
      SIVAL(pdata,0,l);
      strcpy(pdata+4,fname);
      pdata += 4 + l;
      data_size = PTR_DIFF(pdata,(*ppdata));
      break;

    case SMB_QUERY_FILE_STREAM_INFO:
      data_size = 24 + l;
      SIVAL(pdata,0,pos);
      SIVAL(pdata,4,size);
      SIVAL(pdata,12,size);
      SIVAL(pdata,20,l);	
      strcpy(pdata+24,fname);
      break;
    default:
      return(ERROR(ERRDOS,ERRunknownlevel));
    }

  send_trans2_replies( outbuf, bufsize, params, 2, *ppdata, data_size);

  return(-1);
}

/****************************************************************************
  reply to a TRANS2_SETFILEINFO (set file info by fileid)
****************************************************************************/
static int call_trans2setfilepathinfo(char *inbuf, char *outbuf, int length, 
				      int bufsize, int cnum, char **pparams, 
				      char **ppdata, int total_data)
{
  char *params = *pparams;
  char *pdata = *ppdata;
  uint16 tran_call = SVAL(inbuf, smb_setup0);
  uint16 info_level;
  int mode=0;
  int size=0;
  struct utimbuf tvs;
  struct stat st;
  pstring fname1;
  char *fname;
  int fd = -1;

  if (!CAN_WRITE(cnum))
    return(ERROR(ERRSRV,ERRaccess));

  if (tran_call == TRANSACT2_SETFILEINFO) {
    int16 fnum = SVALS(params,0);
    info_level = SVAL(params,2);    

    CHECK_FNUM(fnum,cnum);
    CHECK_ERROR(fnum);

    fname = Files[fnum].name;
    fd = Files[fnum].fd;

    if(fstat(fd,&st)!=0) {
      DEBUG(3,("fstat of %s failed (%s)\n", fname, strerror(errno)));
      return(ERROR(ERRDOS,ERRbadpath));
    }
  } else {
    /* set path info */
    info_level = SVAL(params,0);    
    fname = fname1;
    strcpy(fname,&params[6]);
    unix_convert(fname,cnum);
    if(!check_name(fname, cnum))
      return(ERROR(ERRDOS,ERRbadpath));
    
    if(sys_stat(fname,&st)!=0) {
      DEBUG(3,("stat of %s failed (%s)\n", fname, strerror(errno)));
      return(ERROR(ERRDOS,ERRbadpath));
    }    
  }

  DEBUG(3,("call_trans2setfilepathinfo(%d) %s info_level=%d totdata=%d\n",
	   tran_call,fname,info_level,total_data));

  /* Realloc the parameter and data sizes */
  params = *pparams = Realloc(*pparams,2); SSVAL(params,0,0);
  if(params == NULL)
    return(ERROR(ERRDOS,ERRnomem));

  size = st.st_size;
  tvs.modtime = st.st_mtime;
  tvs.actime = st.st_atime;
  mode = dos_mode(cnum,fname,&st);

  if (total_data > 0 && IVAL(pdata,0) == total_data) {
    /* uggh, EAs for OS2 */
    DEBUG(4,("Rejecting EA request with total_data=%d\n",total_data));
    SSVAL(params,0,ERROR_EAS_NOT_SUPPORTED);

    send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
  
    return(-1);    
  }

  switch (info_level)
    {
    case 1:
      tvs.actime = make_unix_date2(pdata+l1_fdateLastAccess);
      tvs.modtime = make_unix_date2(pdata+l1_fdateLastWrite);
      mode = SVAL(pdata,l1_attrFile);
      size = IVAL(pdata,l1_cbFile);
      break;

    case 2:
      tvs.actime = make_unix_date2(pdata+l1_fdateLastAccess);
      tvs.modtime = make_unix_date2(pdata+l1_fdateLastWrite);
      mode = SVAL(pdata,l1_attrFile);
      size = IVAL(pdata,l1_cbFile);
      break;

    case 3:
      tvs.actime = make_unix_date2(pdata+8);
      tvs.modtime = make_unix_date2(pdata+12);
      size = IVAL(pdata,16);
      mode = IVAL(pdata,24);
      break;

    case 4:
      tvs.actime = make_unix_date2(pdata+8);
      tvs.modtime = make_unix_date2(pdata+12);
      size = IVAL(pdata,16);
      mode = IVAL(pdata,24);
      break;

    case SMB_SET_FILE_BASIC_INFO:
      pdata += 8;		/* create time */
      tvs.actime = interpret_long_date(pdata); pdata += 8;
      tvs.modtime=MAX(interpret_long_date(pdata),interpret_long_date(pdata+8));
      pdata += 16;
      mode = IVAL(pdata,0);
      break;

    case SMB_SET_FILE_END_OF_FILE_INFO:
      if (IVAL(pdata,4) != 0)	/* more than 32 bits? */
	return(ERROR(ERRDOS,ERRunknownlevel));
      size = IVAL(pdata,0);
      break;

    case SMB_SET_FILE_DISPOSITION_INFO: /* not supported yet */
    case SMB_SET_FILE_ALLOCATION_INFO: /* not supported yet */
    default:
      return(ERROR(ERRDOS,ERRunknownlevel));
    }


  if (!tvs.actime) tvs.actime = st.st_atime;
  if (!tvs.modtime) tvs.modtime = st.st_mtime;
  if (!size) size = st.st_size;

  /* Try and set the times, size and mode of this file - if they are different 
   from the current values */
  if(st.st_mtime != tvs.modtime || st.st_atime != tvs.actime) {
    if(sys_utime(fname, &tvs)!=0)
      return(ERROR(ERRDOS,ERRnoaccess));
  }
  if(mode != dos_mode(cnum,fname,&st) && dos_chmod(cnum,fname,mode,NULL)) {
    DEBUG(2,("chmod of %s failed (%s)\n", fname, strerror(errno)));
    return(ERROR(ERRDOS,ERRnoaccess));
  }
  if(size != st.st_size) {
    if (fd == -1) {
      fd = sys_open(fname,O_RDWR,0);
      if (fd == -1)
	return(ERROR(ERRDOS,ERRbadpath));
      set_filelen(fd, size);
      close(fd);
    } else {
      set_filelen(fd, size);
    }
  }

  SSVAL(params,0,0);

  send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
  
  return(-1);
}

/****************************************************************************
  reply to a TRANS2_MKDIR (make directory with extended attributes).
****************************************************************************/
static int call_trans2mkdir(char *inbuf, char *outbuf, int length, int bufsize,
			int cnum, char **pparams, char **ppdata)
{
  char *params = *pparams;
  pstring directory;
  int ret = -1;

  if (!CAN_WRITE(cnum))
    return(ERROR(ERRSRV,ERRaccess));

  strcpy(directory, &params[4]);

  DEBUG(3,("call_trans2mkdir : name = %s\n", directory));

  unix_convert(directory,cnum);
  if (check_name(directory,cnum))
    ret = sys_mkdir(directory,unix_mode(cnum,aDIR));
  
  if(ret < 0)
    {
      DEBUG(5,("call_trans2mkdir error (%s)\n", strerror(errno)));
      return(UNIXERROR(ERRDOS,ERRnoaccess));
    }

  /* Realloc the parameter and data sizes */
  params = *pparams = Realloc(*pparams,2);
  if(params == NULL)
    return(ERROR(ERRDOS,ERRnomem));

  SSVAL(params,0,0);

  send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
  
  return(-1);
}

/****************************************************************************
  reply to a TRANS2_FINDNOTIFYFIRST (start monitoring a directory for changes)
  We don't actually do this - we just send a null response.
****************************************************************************/
static int call_trans2findnotifyfirst(char *inbuf, char *outbuf, int length, int bufsize,
			int cnum, char **pparams, char **ppdata)
{
  static uint16 fnf_handle = 257;
  char *params = *pparams;
  uint16 info_level = SVAL(params,4);

  DEBUG(3,("call_trans2findnotifyfirst - info_level %d\n", info_level));

  switch (info_level) 
    {
    case 1:
    case 2:
      break;
    default:
      return(ERROR(ERRDOS,ERRunknownlevel));
    }

  /* Realloc the parameter and data sizes */
  params = *pparams = Realloc(*pparams,6);
  if(params == NULL)
    return(ERROR(ERRDOS,ERRnomem));

  SSVAL(params,0,fnf_handle);
  SSVAL(params,2,0); /* No changes */
  SSVAL(params,4,0); /* No EA errors */

  fnf_handle++;

  if(fnf_handle == 0)
    fnf_handle = 257;

  send_trans2_replies(outbuf, bufsize, params, 6, *ppdata, 0);
  
  return(-1);
}

/****************************************************************************
  reply to a TRANS2_FINDNOTIFYNEXT (continue monitoring a directory for 
  changes). Currently this does nothing.
****************************************************************************/
static int call_trans2findnotifynext(char *inbuf, char *outbuf, int length, int bufsize,
			int cnum, char **pparams, char **ppdata)
{
  char *params = *pparams;

  DEBUG(3,("call_trans2findnotifynext\n"));

  /* Realloc the parameter and data sizes */
  params = *pparams = Realloc(*pparams,4);
  if(params == NULL)
    return(ERROR(ERRDOS,ERRnomem));

  SSVAL(params,0,0); /* No changes */
  SSVAL(params,2,0); /* No EA errors */

  send_trans2_replies(outbuf, bufsize, params, 4, *ppdata, 0);
  
  return(-1);
}

/****************************************************************************
  reply to a SMBfindclose (stop trans2 directory search)
****************************************************************************/
int reply_findclose(char *inbuf,char *outbuf,int length,int bufsize)
{
  int cnum;
  int outsize = 0;
  int16 dptr_num=SVALS(inbuf,smb_vwv0);

  cnum = SVAL(inbuf,smb_tid);

  DEBUG(3,("reply_findclose, cnum = %d, dptr_num = %d\n", cnum, dptr_num));

  dptr_close(dptr_num);

  outsize = set_message(outbuf,0,0,True);

  DEBUG(3,("%s SMBfindclose cnum=%d, dptr_num = %d\n",timestring(),cnum,dptr_num));

  return(outsize);
}

/****************************************************************************
  reply to a SMBfindnclose (stop FINDNOTIFYFIRST directory search)
****************************************************************************/
int reply_findnclose(char *inbuf,char *outbuf,int length,int bufsize)
{
  int cnum;
  int outsize = 0;
  int dptr_num= -1;

  cnum = SVAL(inbuf,smb_tid);
  dptr_num = SVAL(inbuf,smb_vwv0);

  DEBUG(3,("reply_findnclose, cnum = %d, dptr_num = %d\n", cnum, dptr_num));

  /* We never give out valid handles for a 
     findnotifyfirst - so any dptr_num is ok here. 
     Just ignore it. */

  outsize = set_message(outbuf,0,0,True);

  DEBUG(3,("%s SMB_findnclose cnum=%d, dptr_num = %d\n",timestring(),cnum,dptr_num));

  return(outsize);
}


/****************************************************************************
  reply to a SMBtranss2 - just ignore it!
****************************************************************************/
int reply_transs2(char *inbuf,char *outbuf,int length,int bufsize)
{
  DEBUG(4,("Ignoring transs2 of length %d\n",length));
  return(-1);
}

/****************************************************************************
  reply to a SMBtrans2
****************************************************************************/
int reply_trans2(char *inbuf,char *outbuf,int length,int bufsize)
{
  int outsize = 0;
  int cnum = SVAL(inbuf,smb_tid);
  unsigned int total_params = SVAL(inbuf, smb_tpscnt);
  unsigned int total_data =SVAL(inbuf, smb_tdscnt);
#if 0
  unsigned int max_param_reply = SVAL(inbuf, smb_mprcnt);
  unsigned int max_data_reply = SVAL(inbuf, smb_mdrcnt);
  unsigned int max_setup_fields = SVAL(inbuf, smb_msrcnt);
  BOOL close_tid = BITSETW(inbuf+smb_flags,0);
  BOOL no_final_response = BITSETW(inbuf+smb_flags,1);
  int32 timeout = IVALS(inbuf,smb_timeout);
#endif
  unsigned int suwcnt = SVAL(inbuf, smb_suwcnt);
  unsigned int tran_call = SVAL(inbuf, smb_setup0);
  char *params = NULL, *data = NULL;
  int num_params, num_params_sofar, num_data, num_data_sofar;

  outsize = set_message(outbuf,0,0,True);

  /* All trans2 messages we handle have smb_sucnt == 1 - ensure this
     is so as a sanity check */
  if(suwcnt != 1 )
    {
      DEBUG(2,("Invalid smb_sucnt in trans2 call\n"));
      return(ERROR(ERRSRV,ERRerror));
    }
    
  /* Allocate the space for the maximum needed parameters and data */
  if (total_params > 0)
    params = (char *)malloc(total_params);
  if (total_data > 0)
    data = (char *)malloc(total_data);
  
  if ((total_params && !params)  || (total_data && !data))
    {
      DEBUG(2,("Out of memory in reply_trans2\n"));
      return(ERROR(ERRDOS,ERRnomem));
    }

  /* Copy the param and data bytes sent with this request into
     the params buffer */
  num_params = num_params_sofar = SVAL(inbuf,smb_pscnt);
  num_data = num_data_sofar = SVAL(inbuf, smb_dscnt);

  memcpy( params, smb_base(inbuf) + SVAL(inbuf, smb_psoff), num_params);
  memcpy( data, smb_base(inbuf) + SVAL(inbuf, smb_dsoff), num_data);

  if(num_data_sofar < total_data || num_params_sofar < total_params)
    {
    /* We need to send an interim response then receive the rest
       of the parameter/data bytes */
      outsize = set_message(outbuf,0,0,True);
      send_smb(Client,outbuf);

      while( num_data_sofar < total_data || num_params_sofar < total_params)
	{
	  if(!receive_smb(Client,inbuf, SMB_SECONDARY_WAIT) ||
	     CVAL(inbuf, smb_com) != SMBtranss2)
	    {
	      outsize = set_message(outbuf,0,0,True);
	      DEBUG(2,("Invalid secondary trans2 packet\n"));
	      free(params);
	      free(data);
	      return(ERROR(ERRSRV,ERRerror));
	    }
      
	  /* Revise total_params and total_data in case they have changed downwards */
	  total_params = SVAL(inbuf, smb_tpscnt);
	  total_data = SVAL(inbuf, smb_tdscnt);
	  num_params_sofar += (num_params = SVAL(inbuf,smb_spscnt));
	  num_data_sofar += ( num_data = SVAL(inbuf, smb_sdscnt));
	  memcpy( &params[ SVAL(inbuf, smb_spsdisp)], 
		 smb_base(inbuf) + SVAL(inbuf, smb_spsoff), num_params);
	  memcpy( &data[SVAL(inbuf, smb_sdsdisp)],
		 smb_base(inbuf)+ SVAL(inbuf, smb_sdsoff), num_data);
	}
    }

  if (Protocol >= PROTOCOL_NT1) {
    uint16 flg2 = SVAL(outbuf,smb_flg2);
    SSVAL(outbuf,smb_flg2,flg2 | 0x40); /* IS_LONG_NAME */
  }

  /* Now we must call the relevant TRANS2 function */
  switch(tran_call) 
    {
    case TRANSACT2_OPEN:
      outsize = call_trans2open(inbuf, outbuf, bufsize, cnum, &params, &data);
      break;
    case TRANSACT2_FINDFIRST:
      outsize = call_trans2findfirst(inbuf, outbuf, bufsize, cnum, &params, &data);
      break;
    case TRANSACT2_FINDNEXT:
      outsize = call_trans2findnext(inbuf, outbuf, length, bufsize, cnum, &params, &data);
      break;
    case TRANSACT2_QFSINFO:
      outsize = call_trans2qfsinfo(inbuf, outbuf, length, bufsize, cnum, &params, &data);
      break;
    case TRANSACT2_SETFSINFO:
      outsize = call_trans2setfsinfo(inbuf, outbuf, length, bufsize, cnum, &params, &data);
      break;
    case TRANSACT2_QPATHINFO:
    case TRANSACT2_QFILEINFO:
      outsize = call_trans2qfilepathinfo(inbuf, outbuf, length, bufsize, cnum, &params, &data, total_data);
      break;
    case TRANSACT2_SETPATHINFO:
    case TRANSACT2_SETFILEINFO:
      outsize = call_trans2setfilepathinfo(inbuf, outbuf, length, bufsize, cnum, &params, &data, total_data);
      break;
    case TRANSACT2_FINDNOTIFYFIRST:
      outsize = call_trans2findnotifyfirst(inbuf, outbuf, length, bufsize, cnum, &params, &data);
      break;
    case TRANSACT2_FINDNOTIFYNEXT:
      outsize = call_trans2findnotifynext(inbuf, outbuf, length, bufsize, cnum, &params, &data);
      break;
    case TRANSACT2_MKDIR:
      outsize = call_trans2mkdir(inbuf, outbuf, length, bufsize, cnum, &params, &data);
      break;
    default:
      /* Error in request */
      DEBUG(2,("%s Unknown request %d in trans2 call\n",timestring(), tran_call));
      if(params)
	free(params);
      if(data)
	free(data);
      return (ERROR(ERRSRV,ERRerror));
    }

  /* As we do not know how many data packets will need to be
     returned here the various call_trans2xxxx calls
     must send their own. Thus a call_trans2xxx routine only
     returns a value other than -1 when it wants to send
     an error packet. 
  */

  if(params)
    free(params);
  if(data)
    free(data);
  return outsize; /* If a correct response was needed the call_trans2xxx 
		     calls have already sent it. If outsize != -1 then it is
		     returning an error packet. */
}
