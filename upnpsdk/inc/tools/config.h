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
// $Revision: 1.1.1.4 $
// $Date: 2001/06/15 00:22:14 $
//

#ifndef CONFIG_H
#define CONFIG_H 

#include <stdio.h>
#include <pthread.h>

/** @name Compile time configuration options
 *  The UPnP SDK for Linux contains some parameters modifiable at compile time
 *  that effect the behavior of the UPnP library.  All configuration options 
 *  are located in {\tt inc/tools/config.h}.
 */
 
//@{

/** @name MAX_THREADS
 *  The {\tt MAX_THREADS} constant defines the maximum number of threads the
 *  thread pool inside the UPnP library will create.  These threads are used
 *  for both callbacks into applications built on top of the SDK and also for
 *  making connections to other control points and devices.  It is not 
 *  recommended that this value be much below 10, since the threads are 
 *  necessary for correct operation.  This value can be increased for greater
 *  performance in operation at the expense of greater memory overhead.
 */
//@{

#define MAX_THREADS 10 
//@}


/** @name HTTP_READ_BYTES
 * HTTP Responses will read at most HTTP_READ_BYTES.  This prevents devices
 * that have a misbehaving web server to send a large amount of data to
 * the control point causing it to crash.  A value of -1 means there is no 
 * max.
 */
//@{
#define HTTP_READ_BYTES       -1
//@}


/** @name SSDP_COPY
 * This configuration parameter will decides how many copies of each SSDP 
 * advertisement packet will be sent. By default it will send two copies of 
 * every packet.  
 */
//@{
#define NUM_SSDP_COPY  2
//@}


/** @name AUTO_RENEW_TIME
 * The {\tt AUTO_RENEW_TIME} is the time, in seconds, before a subscription
 * expires that the UPnP library automatically resubscribes.  The default 
 * value is 35 seconds.  Setting this value too low can result in the 
 * subscription renewal not making it to the device in time, causing the 
 * subscription to timeout.
 */
//@{

#define AUTO_RENEW_TIME 35

//@}

/** @name AUTO_ADVERTISEMENT_TIME
 *  The {\tt AUTO_ADVERTISEMENT_TIME} is the time, in seconds, before an
 *  device advertisements expires before a renewed advertisement is sent.
 *  The default time is 30 seconds.
 */
//@{

#define AUTO_ADVERTISEMENT_TIME 30

//@}

//@}


/** @name Exclude Module
 *  Depending on the requirements, the user can selectively discard any of 
 *  the given module like SOAP, GENA, SSDP or Internal web server. By 
 *  default everything is included inside the library.  By setting any of
 *  the values below to 0, that component will not be included in the final
 *  libupnp library.
 *  \begin{itemize}
 *  \item {\tt EXCLUDE_SOAP[0,1]}
 *  \item {\tt EXCLUDE_GENA[0,1]}
 *  \item {\tt EXCLUDE_SSDP[0,1]}
 *  \item {\tt EXCLUDE_DOM [0,1]}
 *  \item {\tt EXCLUDE_WEB_SERVER[0,1]}
 *  \end{itemize}
 *
 */

//@{
#define EXCLUDE_SSDP 0
#define EXCLUDE_SOAP 0
#define EXCLUDE_GENA 0
#define EXCLUDE_DOM  0
#define EXCLUDE_MINISERVER 0
#define EXCLUDE_WEB_SERVER 1
//@}



/** @name {\tt DEBUG_LEVEL}
 *  The user has a option to select 4 different types of debugging level. 
 *  The critical level will show only those messages which can halt the 
 *  normal processing of library, like memory allocation errors. The 
 *  remaining three levels are just for debugging purpose.  Packet level 
 *  will display all the  incoming and outgoing pakcets that are flowing 
 *  between the control point and the device. Info Level displays the 
 *  other important operational information regarding the working of 
 *  libaray. If the user select All, then library displays all the debugging 
 *  information that it has.
 *  \begin{itemize}
 *  \item {\tt Critical [0]}
 *  \item {\tt Packet Level[1]}
 *  \item {\tt Info Level[2]}
 *  \item {\tt All[3]}
 *  \end{itemize}
 */

//@{

 #define DEBUG_LEVEL	     3 
    
//@}


/** @name {\tt DEBUG_TARGET}
 *  The user has the option to redirect the library output debug messages 
 *  to either the screen or to a log file.  All the output messages with 
 *  debug level 0 will go to upnp.err and the message with debug level 
 *  greater than zero will be redireced to upnp.out.
 */
//@{

#define DEBUG_TARGET	    0   

//@}


#define DEBUG_ALL	  0    
#define DEBUG_SSDP	  0    
#define DEBUG_SOAP	  0    
#define DEBUG_GENA	  0    
#define DEBUG_TPOOL	 0     
#define DEBUG_MSERV	 0
#define DEBUG_DOM	   0
#define DEBUG_API    0    




///////////////////////////////Do not change, Internal purpose only//////////

#ifdef DEBUG
#define DBGONLY(x) x
#else
#define DBGONLY(x)  
#endif

//#include "../../src/inc/upnp_debug.h"


typedef enum Upnp_Module {SSDP,SOAP,GENA,TPOOL,MSERV,DOM,API} Dbg_Module;
typedef enum DBG_LVL {UPNP_CRITICAL,UPNP_PACKET,UPNP_INFO,UPNP_ALL} Dbg_Level;

#ifdef __cplusplus
extern "C" {
#endif

  FILE * GetDebugFile(Dbg_Level level, Dbg_Module module);

  DBGONLY(void UpnpPrintf(Dbg_Level DLevel, Dbg_Module Module,char
			*DbgFileName, int DbgLineNo,char * FmtStr,
			...);
  void UpnpDisplayBanner(FILE *fd,
			 char **lines, int size, int starlength);
  void UpnpDisplayFileAndLine(FILE *fd,char *DbgFileName, int DbgLineNo);)
#ifdef __cplusplus
}
#endif

int InitLog();
void CloseLog();

#ifdef  INTERNAL_WEB_SERVER
#undef  EXCLUDE_WEB_SERVER 
#undef  EXCLUDE_MINISERVER 
#define EXCLUDE_WEB_SERVER 0
#define EXCLUDE_MINISERVER 0
#endif

#if EXCLUDE_GENA == 1 && EXCLUDE_SOAP == 1 && EXCLUDE_WEB_SERVER == 1
#undef  EXCLUDE_MINISERVER 
#undef  INTERNAL_WEB_SERVER
#define EXCLUDE_MINISERVER 1
#endif

#if EXCLUDE_GENA == 0 || EXCLUDE_SOAP == 0 || EXCLUDE_WEB_SERVER == 0
#undef  EXCLUDE_MINISERVER 
#define EXCLUDE_MINISERVER 0
#endif

#ifndef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0
#define INTERNAL_WEB_SERVER
#endif
#endif


#ifdef INCLUDE_CLIENT_APIS
#define CLIENTONLY(x) x
#else 
#define CLIENTONLY(x)
#endif

#ifdef INCLUDE_DEVICE_APIS
#define DEVICEONLY(x) x
#else 
#define DEVICEONLY(x) 
#endif

DBGONLY( extern pthread_mutex_t GlobalDebugMutex;)

#endif
