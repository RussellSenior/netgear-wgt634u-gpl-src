///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////
//
// $Revision: 1.1.1.3 $
// $Date: 2001/06/15 00:22:14 $
//

#include "../../inc/tools/config.h"
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
pthread_mutex_t GlobalDebugMutex=PTHREAD_MUTEX_INITIALIZER;

#include "upnp.h"
#include <stdarg.h>

FILE *ErrFileHnd=NULL;
FILE *InfoFileHnd=NULL;

void UpnpDisplayBanner(FILE * fd, char **lines, int size, int starLength);

int InitLog()
{
	if ( (ErrFileHnd  = fopen ("ErrFileName.txt" , "a")) == NULL) return -1;
	if ( (InfoFileHnd = fopen ("InfoFileName.txt", "a")) == NULL) return -1;
	return UPNP_E_SUCCESS;
}

void CloseLog()
{
  fclose(ErrFileHnd);
  fclose(InfoFileHnd);
}

     



DBGONLY(

void UpnpPrintf(Dbg_Level DLevel, Dbg_Module Module,char *DbgFileName, int DbgLineNo,char * FmtStr, ... )
{
   
   
  
   va_list ArgList;
   
  
  

   va_start(ArgList, FmtStr);
   
   
   if( DEBUG_LEVEL < DLevel) return;

   if(DEBUG_ALL == 0)
   {
      switch(Module)
      {
         case SSDP:if(DEBUG_SSDP == 1)  break;
                   else return;
         case SOAP:if(DEBUG_SOAP == 1)  break;
                   else return;
         case GENA:if(DEBUG_GENA == 1)  break;
                   else return;
         case TPOOL:if(DEBUG_TPOOL== 1)  break;
                   else return;
         case MSERV:if(DEBUG_MSERV == 1)  break;
                   else return;
         case DOM:if(DEBUG_DOM == 1)  break;
                   else return;
         case API:if(DEBUG_API == 1)  break;
                   else return;
         default : return;
      }
   }
 
   pthread_mutex_lock(&GlobalDebugMutex);
   
   if(DEBUG_TARGET == 0) 
   {
     if (DbgFileName)
       UpnpDisplayFileAndLine(stdout,DbgFileName,DbgLineNo);
      vfprintf(stdout,FmtStr,ArgList);
      fflush(stdout);      
   }
   else 
   {
      if(DLevel == 0)
      {
	if (DbgFileName)
	  UpnpDisplayFileAndLine(ErrFileHnd,DbgFileName,DbgLineNo);
          vfprintf(ErrFileHnd,FmtStr,ArgList);
	      fflush(ErrFileHnd);
      }
      else
      {
	if (DbgFileName)
	   UpnpDisplayFileAndLine(InfoFileHnd,DbgFileName,DbgLineNo);
          vfprintf(InfoFileHnd,FmtStr,ArgList);
	      fflush(InfoFileHnd);   
      }

   }
  
   va_end(ArgList);
   pthread_mutex_unlock(&GlobalDebugMutex);
   
}


)


DBGONLY(
FILE * GetDebugFile(Dbg_Level DLevel, Dbg_Module Module)
{
  if( DEBUG_LEVEL < DLevel) return NULL;

   if(DEBUG_ALL == 0)
   {
      switch(Module)
      {
         case SSDP:if(DEBUG_SSDP == 1)  break;
                   else return NULL;
         case SOAP:if(DEBUG_SOAP == 1)  break;
                   else return NULL;
         case GENA:if(DEBUG_GENA == 1)  break;
                   else return NULL;
         case TPOOL:if(DEBUG_TPOOL== 1)  break;
                   else return NULL;
         case MSERV:if(DEBUG_MSERV == 1)  break;
                   else return NULL;
         case DOM:if(DEBUG_DOM == 1)  break;
                   else return NULL;
         case API:if(DEBUG_API == 1)  break;
                   else return NULL;
         default : return NULL;
      }
   }
  
   if(DEBUG_TARGET == 0) 
   {
     return stdout;
      
   }
   else 
   {
      if(DLevel == 0)
      {
          return ErrFileHnd;
      }
      else
      {
         return InfoFileHnd;
      }

   }
 
}
)

DBGONLY(

void UpnpDisplayFileAndLine(FILE *fd,char * DbgFileName, int DbgLineNo)
{
   int starlength=66;
   char *lines[2];
   char FileAndLine[500]; 
   lines[0]="DEBUG";
   if (DbgFileName)
     {
      sprintf(FileAndLine,"FILE: %s, LINE: %d",DbgFileName,DbgLineNo); 
      lines[1]=FileAndLine;
     }
    UpnpDisplayBanner(fd,lines,2,starlength);
	fflush(fd);
}
)

DBGONLY(

void UpnpDisplayBanner(FILE * fd, char **lines, int size, int starLength)
{
  char *stars= (char *) malloc(starLength+1);
 
 
  char *line =NULL;
  int leftMarginLength =  starLength/2 + 1; 
  int rightMarginLength = starLength/2 + 1;
  char *leftMargin = (char *) malloc (leftMarginLength);
  char *rightMargin = (char *) malloc (rightMarginLength);
  
  int i=0;
  int LineSize=0;
  char *currentLine=(char *) malloc (starLength+1);

  memset(stars,'*',starLength);
  stars[starLength]=0;
  memset( leftMargin, 0, leftMarginLength);
  memset( rightMargin, 0, rightMarginLength);

  fprintf(fd,"\n%s\n",stars);
  for (i=0;i<size;i++)
    {
      LineSize=strlen(lines[i]);
      line=lines[i];
      while(LineSize>(starLength-2))
	{ 
	  memcpy(currentLine,line,(starLength-2));
	  currentLine[(starLength-2)]=0;
	  fprintf(fd,"*%s*\n",currentLine);
	  LineSize-=(starLength-2);
	  line+=(starLength-2);
	}
      if (LineSize % 2 == 0)
      {
         leftMarginLength = rightMarginLength = ((starLength-2)-LineSize)/2;
      }
      else
      {
         leftMarginLength = ((starLength-2)-LineSize)/2;
         rightMarginLength= ((starLength-2)-LineSize)/2+1;
      }		

      memset(leftMargin, ' ', leftMarginLength);
      memset(rightMargin, ' ', rightMarginLength);
      leftMargin[leftMarginLength] = 0;
      rightMargin[rightMarginLength] =0;

      fprintf(fd,"*%s%s%s*\n",leftMargin, line, rightMargin);
    }
  fprintf(fd,"%s\n\n",stars);
  free (leftMargin);
  free (rightMargin);
  free (stars);
  free( currentLine);
})

