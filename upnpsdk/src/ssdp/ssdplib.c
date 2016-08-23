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
// $Revision: 1.2 $
// $Date: 2001/08/15 18:17:31 $
//  


#include "../../inc/tools/config.h"
#include "ssdplib.h"
#include <stdio.h>
#include "../inc/genlib/tpool/scheduler.h"
#include "../inc/genlib/tpool/interrupts.h"
#include "../inc/interface.h"
#include <sys/utsname.h>
#include <syslog.h>
#include "arpping.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#define MAX_TIME_TOREAD  45
void RequestHandler();
Event  ErrotEvt;
enum Listener{Idle,Stopping,Running}ListenerState=Idle;
pthread_t  ListenerThread = 0;

static long StartupTime;
extern int mSearchNum;
extern char lan_ip_address[16];
extern char lan_mac_address[24];
extern char *lan_if_name;
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetLocalHost(char * Ip)
 // Description : Returns the local IP address string.
 // Parameters  : Ip : Local IP address.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if EXCLUDE_SSDP == 0

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void SendErrorEvent(int ErrCode)
 // Description : This function is used to send the critical error notification to the callback function like memory
 //               allocation, network error etc.
 //
 // Parameters  : ErrCode : Error code to be passed back to the callback function
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 void SendErrorEvent(int ErrCode)
 {
   DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation !!!\n");)
   ErrotEvt.ErrCode = ErrCode;
   CallBackFn((Event *)&ErrotEvt);
 }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void GetRandomNumber(int Max)
 // Description : This function generates a milisecond delay random no.
 //
 // Parameters  : Max : Max delay in seconds.
 //
 // Return value: Randon number.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 int GetRandomNumber(int Max)
  {
   unsigned long  MaxLt;
   MaxLt = Max*1000000;
   gettimeofday(&trt,NULL);
   StartupTime = trt.tv_usec - 500000 ;
   srand(StartupTime);
   return (rand() % MaxLt);
 }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void RemoveThreadData(ThreadData *ThData)
 // Description : This function does  the cleanup job, it removes any data allocated by fn PutThreadData().
 //
 // Parameters  : ThData : Data packet to be passed back to the Thread.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 void RemoveThreadData(ThreadData *ThData)
 {

     free(ThData->Data);
     free(ThData);

 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int PutThreadData(ThreadData *ThData,char * Rqst, struct sockaddr_in * DestAddr, int Mx)
 // Description : This function stored the data to be used by the independent thread.
 //
 // Parameters  : ThData : Data packet to be passed back to the Thread.
 //
 // Return value: 1 if successfull , -1 otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int PutThreadData(ThreadData *ThData,char * Rqst, struct sockaddr_in * DestAddr, int Mx)
 {

   ThData->Data = (char *)malloc(strlen(Rqst)+1);
   if( ThData->Data == NULL)
   {
       SendErrorEvent( UPNP_E_OUTOF_MEMORY);
       return -1;
   }

   strcpy(ThData->Data,Rqst);
   ThData->Mx = Mx;

   if(DestAddr != NULL)
   {
       ThData->DestAddr.sin_family = DestAddr->sin_family;
       ThData->DestAddr.sin_addr.s_addr = DestAddr->sin_addr.s_addr;
       ThData->DestAddr.sin_port = DestAddr->sin_port;
   }
   else  ThData->DestAddr.sin_port =0;

   return 1;

 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void TransferResEvent( ThreadData *ThData)
 // Description : This function process the HTTP data packet received by the multicast channel and pass it back to the
 //               callback function.
 //
 // Parameters  : ThData : Data packet to be passed back to the Thread.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TransferResEvent( ThreadData *ThData)
{
    Event * Evt = (Event *)malloc(sizeof(Event));
    Evt->ErrCode = NO_ERROR_FOUND;
    if( Evt == NULL)
    {
         SendErrorEvent( UPNP_E_OUTOF_MEMORY);
         return;
    }
    else
    {
         if (ThData->Data != NULL)
         {
              Evt->DestAddr =  &(ThData->DestAddr);
              if (AnalyzeCommand(ThData->Data,Evt) > 0)
              {
                 if(Evt->Cmd == SEARCH)
                 {
                    if (Evt->Mx < 0 || !strlen(Evt->Man))
                    {
                      DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in parsing !!!\n");)
                      goto end;
                    }
                 }
                 if(Evt->Cmd == SEARCH && Evt->Mx > 1)
                 {
                    DBGONLY(UpnpPrintf(UPNP_ALL,SSDP,__FILE__,__LINE__,"Sleeping for %d milisecond!!!!!!!\n",GetRandomNumber(Evt->Mx));)
                    usleep(GetRandomNumber(Evt->Mx));
                 }
 
                 CallBackFn(Evt);

              }
              else
		            {
                 DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in parsing !!!\n");)
              }
         }

    }

end:
    RemoveThreadData(ThData);
    free(Evt);
    return;
 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int StartEventHandler(char *EventBuf, struct sockaddr_in * DestAddr)
 // Description : This function  starts the TransferResEvent thread, which later process this Event packet.
 //
 // Parameters  : EventBuf : Raw HTTP packet received in multicast channel.
 //               DestAddr : Address of the client from which this packet is received.
 // Return value: 1 if successfull , -1 otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int StartEventHandler(char *EventBuf, struct sockaddr_in * DestAddr)
 {
     ThreadData *ThData;

     ThData = (ThreadData *)malloc(sizeof(struct TData));
     if(ThData  == NULL)
     {

         SendErrorEvent( UPNP_E_OUTOF_MEMORY);
         return -1;
     }

     PutThreadData(ThData,EventBuf,DestAddr,0);
     tpool_Schedule((ScheduleFunc)TransferResEvent,ThData);
     return 1;

 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void  ListenMulticastChannel()
 // Description : This function run as a independent thread listen for the response coming on the multicast channel.
 //               It starts during the lib initialization and run until closed by the DeInit() functin call.
 // Parameters  : SsdpSock : Multicast Socket on which it will listen for the request.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 void  ListenMulticastChannel(int SsdpSock)
 {
   int  socklen,ByteReceived;
   struct sockaddr_in ClientAddr;
   fd_set RdSet;
   char	RequestBuf[BUFSIZE];

   ListenerThread = pthread_self();
   ListenerState = Running;
   DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Multicast listener started...\n");)

   bzero((char *)&RequestBuf, BUFSIZE);
   bzero((char *)&ClientAddr, sizeof(struct sockaddr_in));

   for(;;)
   {


       FD_ZERO(&RdSet);
       FD_SET(SsdpSock,&RdSet);

       if (ListenerState == Stopping) break;

       if (select(SsdpSock+1,&RdSet,NULL,NULL,NULL) == -1)
       {
           if (errno == EINTR && ListenerState == Stopping )
           {
              DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"SSDP got stop signal, Exiting!!!!!!!!!!!!!!\n");)
              break;
           }
           else
           {
              DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select call !!!!!!!!!!!! \n");)
              if ( errno == EBADF)
		            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
              else if ( errno ==  EINTR)
		            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught. \n");)}
              else if ( errno ==  EINVAL )
		            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
              else if ( errno ==   ENOMEM )
		            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Select was unable to allocate memory for internal tables.\n");)}
              break;
           }

       }
       else
       {

          if(FD_ISSET(SsdpSock,&RdSet))
          {
               socklen = sizeof(struct sockaddr_in);
               ByteReceived = recvfrom(SsdpSock, RequestBuf, BUFSIZE, 0,(struct sockaddr*)&ClientAddr, &socklen);
               if(ByteReceived > 0 )
               {
                  RequestBuf[ByteReceived] = '\0';
                  DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Received response !!!  %s From host %s \n", RequestBuf,inet_ntoa(ClientAddr.sin_addr));)
                  DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Received multicast packet: \n %s\n",RequestBuf);)
                  StartEventHandler(RequestBuf,&ClientAddr );
               }

            }
       }


   }

   close(SsdpSock);
   ListenerState = Idle;
   return;

 }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void DeInitSsdpLib()
 // Description : This function stops  the multicast thread.
 // Parameters  : None
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 void DeInitSsdpLib()
 {
 	int code;
 	
 	if ( ListenerState == Idle )
 		return;	// not running
 		
 	ListenerState = Stopping;
 	
 	
 	// keep sending signals until server stops
 	while ( 1 )
 	{
 		  if ( ListenerState == Idle )
 		  {
 			   break;
 		  }
 		
 		  DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Sending interrupt\n");)
 		
 		  code = tintr_Interrupt( ListenerThread );
 		  if (code < 0)
 		  { 
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Interrupt Failed............");)
 		  }
 		
 		  if(ListenerState == Idle)
 		  {
 			    break;
 		  }
 		
 		  sleep( 1 );		// pause before sending signals again
 	}
 }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int InitSsdpLib(SsdpFunPtr Fn)
 // Description : This is the first function to be called in the SSDP library, it creates the multicats socket and starts
 //               the multicast listener thread.
 // Parameters  : FunPtr: Callback function which will receive all the responses.
 //
 // Return value: Less than zero if fails, UPNP_E_SUCCESS  otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 int InitSsdpLib(SsdpFunPtr Fn)
 {
    int SsdpSock,On=1,val;
    u_char Ttl=4;
    struct ip_mreq SsdpMcastAddr;
    struct sockaddr_in SelfAddr;

    StartupTime = time(NULL);

    InitParser();

    if ( ListenerState != Idle )
    {
    	return -1;		// already running
    }

    SsdpSock = socket(AF_INET, SOCK_DGRAM, 0);
    val = fcntl(SsdpSock,F_GETFL,0);
    fcntl(SsdpSock,F_SETFL,val|O_NONBLOCK);

    if ( SsdpSock == -1 || val == -1)
    {
       SendErrorEvent(UPNP_E_NETWORK_ERROR);
       DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n");)
       return UPNP_E_OUTOF_SOCKET;
    }

    if ( setsockopt(SsdpSock, SOL_SOCKET, SO_REUSEADDR, &On, sizeof(On)) == -1)
    return UPNP_E_OUTOF_SOCKET;

    bzero(&SsdpMcastAddr, sizeof(struct ip_mreq ));
    SsdpMcastAddr.imr_interface.s_addr = htonl(INADDR_ANY);
    SsdpMcastAddr.imr_multiaddr.s_addr = inet_addr(SSDP_IP);


    bzero((char *)&SelfAddr, sizeof(struct sockaddr_in));

    SelfAddr.sin_family = AF_INET;
    SelfAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    SelfAddr.sin_port = htons(SSDP_PORT);
    if (bind( SsdpSock, (struct sockaddr *) &SelfAddr, sizeof(SelfAddr)) != 0)
    {
       SendErrorEvent(UPNP_E_NETWORK_ERROR);
       DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in binding !!!\n");)
       return UPNP_E_SOCKET_BIND;
    }
    setsockopt(SsdpSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &SsdpMcastAddr, sizeof(struct ip_mreq));
    setsockopt(SsdpSock, IPPROTO_IP, IP_MULTICAST_TTL, &Ttl, sizeof(Ttl));

    syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d \n","Initssdplib",__LINE__);
    tpool_Schedule((ScheduleFunc)ListenMulticastChannel, (void*)SsdpSock);
    CallBackFn = Fn;

    // wait SSDP to Start
    while(ListenerState != Running )
    {
       sleep(1);
    }
    return UPNP_E_SUCCESS ;

 }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void RequestHandler(ThreadData *ThData)
 // Description : This function works as a request handler which passes the HTTP request string to multicast channel then
 //               wait for the response. Once it is received, it is passed back to callback function.
 // Parameters  : ThData : Data Packet which contains all the parameter which is requied to send the request on the
 //               multicatst channel.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 void RequestHandler(ThreadData *ThData)
 {
      int socklen=sizeof( struct sockaddr_in),RetVal,TryIdx=0, RqstSock, ByteReceived,TimeTillRead;
      struct timeval tmout;
      fd_set WrSet,RdSet;
      struct sockaddr_in DestAddr;
      char RequestBuf[BUFSIZE];
      u_char Ttl=4;
      time_t StartTime,CurrentTime,ElapsedTime;
      Event * Evt = (Event *)malloc(sizeof(Event));
      if(Evt  == NULL)
      {

         SendErrorEvent( UPNP_E_OUTOF_MEMORY);
         return;
      }

      RqstSock = socket(AF_INET, SOCK_DGRAM, 0);
      setsockopt(RqstSock, IPPROTO_IP, IP_MULTICAST_TTL, &Ttl, sizeof(Ttl));
      RetVal = fcntl(RqstSock,F_GETFL,0);
      fcntl(RqstSock,F_SETFL,RetVal|O_NONBLOCK);
      if ( RqstSock == -1 || RetVal == -1)
      {
            SendErrorEvent(UPNP_E_NETWORK_ERROR);
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in creating socket !!!\n");)
            return;
      }

      TimeTillRead = ThData->Mx;

      if (TimeTillRead <= 1) TimeTillRead = 2;
      else if (TimeTillRead > MAX_TIME_TOREAD) TimeTillRead = MAX_TIME_TOREAD ;

       Evt->ErrCode = NO_ERROR_FOUND;
       bzero((char *)&DestAddr, sizeof(struct sockaddr_in));
       if(ThData->DestAddr.sin_port == 0)
       {
            DestAddr.sin_family = AF_INET;
            DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
            DestAddr.sin_port = htons(SSDP_PORT);
       }
       else
       {
           DestAddr.sin_family = ThData->DestAddr.sin_family;
           DestAddr.sin_addr.s_addr = ThData->DestAddr.sin_addr.s_addr;
           DestAddr.sin_port = ThData->DestAddr.sin_port;
        }


        while(TryIdx < NUM_TRY)
        {
             FD_ZERO(&WrSet);
             FD_SET(RqstSock,&WrSet);
             tmout.tv_sec = 1;
             tmout.tv_usec = 1;

             DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending packet : \n%s\n",ThData->Data);)
             sendto(RqstSock,ThData->Data,strlen(ThData->Data),0,(struct sockaddr *)&DestAddr, socklen);
             if ( select(RqstSock+1,NULL,&WrSet,NULL,NULL) == -1)
             {

                 if ( errno == EBADF)
		               {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
                 else if ( errno ==  EINTR)
		               {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught.    \n");)}
                 else if ( errno ==  EINVAL )
		               {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
                 else if ( errno ==   ENOMEM )
		               {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Select was unable to allocate memory for internal tables.\n");)}

                 SendErrorEvent(UPNP_E_NETWORK_ERROR);
                 RemoveThreadData(ThData);
                 free(Evt);
                 close(RqstSock);
                 return;
            }
             else if(FD_ISSET(RqstSock,&WrSet))
             {
                 RetVal= 1;
                 break;
             }
             TryIdx++;
          }


          StartTime = time(NULL);
          CurrentTime = StartTime;
          ElapsedTime = 0;

          while(ElapsedTime < TimeTillRead+2)
          {
             FD_ZERO(&RdSet);
             FD_SET(RqstSock,&RdSet);

             CurrentTime = time(NULL);
             ElapsedTime = CurrentTime-StartTime;

             tmout.tv_sec = TimeTillRead+2-ElapsedTime;
             tmout.tv_usec = TimeTillRead+2-ElapsedTime;

             if ( select(RqstSock+1,&RdSet,NULL,NULL,&tmout) == -1)
             {
                DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select call!!!!!!!!");)
                Evt->ErrCode = E_SOCKET;
                RemoveThreadData(ThData);
                free(Evt);
                return;
             }
             else if(FD_ISSET(RqstSock,&RdSet))
             {

                 socklen = sizeof(struct sockaddr_in);
                 ByteReceived = recvfrom(RqstSock, RequestBuf, BUFSIZE, 0,(struct sockaddr*)&DestAddr, &socklen);
                 if(ByteReceived > 0 )
                 {
                    RequestBuf[ByteReceived] = '\0';
                    DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Received buffer%s\n",RequestBuf);)
                    if( AnalyzeCommand(RequestBuf,Evt) >=0)
                    {
                       Evt->Cookie = ThData->Cookie;
                       CallBackFn(Evt);
                    }

                 }

             }

           }

          Evt->Cookie = ThData->Cookie;
          Evt->Cmd=TIMEOUT;
          CallBackFn(Evt);
          RemoveThreadData(ThData);
          free(Evt);
          close(RqstSock);
          return;


 }


#ifdef INCLUDE_CLIENT_APIS
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void CreateClientRequestPacket(char * RqstBuf,int Mx, char *SearchTarget)
 // Description : This function creates a HTTP search request packet depending on the input parameter.
 // Parameters  : RqstBuf : Output string in HTTP format.
 //               SearchTarget : Search Taget
 //               Mx : Number of seconds to wait to collect all the responses.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 void CreateClientRequestPacket(char * RqstBuf,int Mx, char *SearchTarget)
 {

   char TempBuf[COMMAND_LEN];
   int Port;


   strcpy( RqstBuf,"M-SEARCH * HTTP/1.1\r\n");


   Port = SSDP_PORT;
   strcpy(TempBuf,"HOST:");
   strcat(TempBuf,SSDP_IP);
   sprintf(TempBuf,"%s:%d\r\n",TempBuf,Port);
   strcat(RqstBuf,TempBuf);

   strcat(RqstBuf,"MAN:\"ssdp:discover\"\r\n");


   if ( Mx > 0)
   {
      sprintf(TempBuf,"MX:%d\r\n",Mx);
      strcat(RqstBuf,TempBuf);
   }


  if (SearchTarget != NULL)
  {
     sprintf(TempBuf,"ST:%s\r\n", SearchTarget);
     strcat(RqstBuf,TempBuf);
  }
  strcat(RqstBuf,"\r\n");

 }




 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int SearchByTarget()
 // Description : Creates and send the search request.
 // Parameters  : Mx : Number of seconds to wait, to collect all the responses.
 //               St : Search target.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int SearchByTarget(int Mx, char * St, void * Cookie)
 {

    char * ReqBuf;
    ThreadData *ThData;

    ReqBuf = (char *)malloc(BUFSIZE);
    if (ReqBuf == NULL) return UPNP_E_OUTOF_MEMORY;

    CreateClientRequestPacket(ReqBuf,Mx,St);
    DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending request buffer = \n%s\n",ReqBuf);)

    ThData = (ThreadData *)malloc(sizeof(struct TData));
    if (ThData == NULL) return UPNP_E_OUTOF_MEMORY;
    PutThreadData(ThData,ReqBuf, NULL, Mx);
    ThData->Cookie = Cookie;
    tpool_Schedule((ScheduleFunc)RequestHandler,ThData);

    free(ReqBuf);
    return 1;

 }

#endif

#ifdef INCLUDE_DEVICE_APIS
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ServiceRequestHandler(struct sockaddr_in * DestAddr, char * szStr)
 // Description : This function works as a request handler which passes the HTTP request string to multicast channel then
 //               wait for the response, once it received, it is passed back to callback function.
 // Parameters  : szSt : Response string in HTTP format.
 //               DestAddr : Ip address, to send the reply.
 //
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 int ServiceRequestHandler(struct sockaddr_in * DestAddr, char * szStr)
 {
      int ReplySock, socklen=sizeof( struct sockaddr_in),RetVal,TryIdx=0;
      struct timeval tmout;
      fd_set WrSet;

      Event * Evt = (Event *) malloc(sizeof(Event));
      if ( Evt == NULL)
      {
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation : \n"));
         return -1;
      }
      else Evt->ErrCode =  NO_ERROR_FOUND;


      ReplySock = socket(AF_INET, SOCK_DGRAM, 0);

      RetVal = fcntl(ReplySock,F_GETFL,0);
      fcntl(ReplySock,F_SETFL,RetVal|O_NONBLOCK);

      if( ReplySock == -1 || RetVal == -1)
      {
         SendErrorEvent(UPNP_E_NETWORK_ERROR);
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n"));
         free(Evt);
      }


      while(TryIdx < NUM_TRY)
      {

          FD_ZERO(&WrSet);
          FD_SET(ReplySock,&WrSet);

          tmout.tv_sec = 1;
          tmout.tv_usec = 1;

          sendto(ReplySock,szStr,strlen(szStr),0,(struct sockaddr *)DestAddr, socklen);
          if ( select(ReplySock+1,NULL,&WrSet,NULL,NULL) == -1)
          {
             DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select !!!!!!!!\n"));
             if ( errno == EBADF)
	            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
             else if ( errno ==  EINTR)
	            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught.    \n");)}
             else if ( errno ==  EINVAL )
	            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
				         else if ( errno ==   ENOMEM )
				         {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"select was unable to allocate memory for internal tables.\n");)}

             SendErrorEvent(UPNP_E_NETWORK_ERROR);
             free(Evt);
             break;
          }
          else if(FD_ISSET(ReplySock,&WrSet))
          {
              RetVal= 1;
              break;
          }
          TryIdx++;
      }

      free(Evt);
      close(ReplySock);
     return 1;
 }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int NewRequestHandler(struct sockaddr_in * DestAddr, int NumPacket, char ** RqPacket)
 // Description : This function works as a request handler which passes the HTTP request string to multicast channel then
 //               wait for the response, once it received, it is passed back to callback function.
 // Parameters  : RqPacket : Request packet in HTTP format.
 //               DestAddr : Ip address, to send the reply.
 //               NumPacket: Number of packet to be sent.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 int NewRequestHandler(struct sockaddr_in * DestAddr, int NumPacket, char ** RqPacket)
 {
      int ReplySock, socklen=sizeof( struct sockaddr_in),RetVal,TryIdx=0;
      struct timeval tmout;
      fd_set WrSet;
      int NumCopy,Index;
      int ret;
      struct in_addr myIPAddr;
      int arpRet;
      
      Event * Evt = (Event *) malloc(sizeof(Event));
      if ( Evt == NULL)
      {
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation : \n"));
         return UPNP_E_OUTOF_MEMORY;
      }
      else Evt->ErrCode =  NO_ERROR_FOUND;


      ReplySock = socket(AF_INET, SOCK_DGRAM, 0);

      RetVal = fcntl(ReplySock,F_GETFL,0);
      fcntl(ReplySock,F_SETFL,RetVal|O_NONBLOCK);

      if( ReplySock == -1 || RetVal == -1)
      {
         SendErrorEvent(UPNP_E_NETWORK_ERROR);
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n"));
         free(Evt);
         return UPNP_E_OUTOF_SOCKET;

      }

      // Send ARP prior to sending DISCOVER SEARCH:ALL reply -- David Lin
      inet_aton(lan_ip_address,&myIPAddr); 	// translate alpha IP to network bit order
      if( (lan_ip_address!=NULL) && (lan_mac_address!=NULL) &&(DestAddr->sin_addr.s_addr &0x0000FF)!=239 )
      {
	      // send ARP packet
	      arpRet = arpping(DestAddr->sin_addr.s_addr,myIPAddr.s_addr, lan_mac_address, lan_if_name);
	      // output result of ARP
	      syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s:%d address%s arpRet:%d\n","NewRequestHandler:ARP",__LINE__,inet_ntoa(DestAddr->sin_addr),arpRet);
      }
      // end ARP check

      for(Index=0;Index< NumPacket;Index++)
      {

          NumCopy =0;
          TryIdx =0;

          while(TryIdx < NUM_TRY && NumCopy < NUM_SSDP_COPY)
          {

             FD_ZERO(&WrSet);
             FD_SET(ReplySock,&WrSet);

             tmout.tv_sec = 1;
             tmout.tv_usec = 1;

             DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending reply %s\n",*(RqPacket+Index));)
		     
             ret=sendto(ReplySock,*(RqPacket+Index),strlen(*(RqPacket+Index)),0,(struct sockaddr *)DestAddr, socklen);

   	     syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d port:%d\n bufferlen: %d sendtoRet:%d buffer:%s","NewRequestHandler",__LINE__,ntohs(DestAddr->sin_port),strlen(*(RqPacket+Index)),ret,*(RqPacket+Index));
             if ( select(ReplySock+1,NULL,&WrSet,NULL,NULL) == -1)
             {
                DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select !!!!!!!!\n")) ;
                if ( errno == EBADF)
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
                else if ( errno ==  EINTR)
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught.    \n");)}
                else if ( errno ==  EINVAL )
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
                else if ( errno ==   ENOMEM )
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"select was unable to allocate memory for internal tables.\n");)}
                SendErrorEvent(UPNP_E_NETWORK_ERROR);
                break;
             }
             else if(FD_ISSET(ReplySock,&WrSet))
             {
                 ++NumCopy;
             }
             else TryIdx++;
         }
       }

       free(Evt);
       close(ReplySock);
       return UPNP_E_SUCCESS;
 }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int NewRequestHandlerTtl(struct sockaddr_in * DestAddr, int NumPacket, char ** RqPacket, int Ttl)
 // Description : This function is the same as NewRequestHandler() except for another parameter to control packet ttl attribute
 // Date        : 2004/04/12
 // Author      : light lu 
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 int NewRequestHandlerTtl(struct sockaddr_in * DestAddr, int NumPacket, char ** RqPacket, int Ttl)
 {
      int ReplySock, socklen=sizeof( struct sockaddr_in),RetVal,TryIdx=0;
      struct timeval tmout;
      fd_set WrSet;
      int NumCopy,Index;
      int ret;
      struct in_addr myIPAddr;
      int arpRet;
      
      Event * Evt = (Event *) malloc(sizeof(Event));
      if ( Evt == NULL)
      {
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation : \n"));
         return UPNP_E_OUTOF_MEMORY;
      }
      else Evt->ErrCode =  NO_ERROR_FOUND;


      ReplySock = socket(AF_INET, SOCK_DGRAM, 0);

      RetVal = fcntl(ReplySock,F_GETFL,0);
      fcntl(ReplySock,F_SETFL,RetVal|O_NONBLOCK);

      if( ReplySock == -1 || RetVal == -1)
      {
         SendErrorEvent(UPNP_E_NETWORK_ERROR);
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n"));
         free(Evt);
         return UPNP_E_OUTOF_SOCKET;

      }

      // Send ARP prior to sending DISCOVER SEARCH:ALL reply -- David Lin
      inet_aton(lan_ip_address,&myIPAddr); 	// translate alpha IP to network bit order
      if( (lan_ip_address!=NULL) && (lan_mac_address!=NULL) &&(DestAddr->sin_addr.s_addr &0x0000FF)!=239 )
      {
	      // send ARP packet
	      arpRet = arpping(DestAddr->sin_addr.s_addr,myIPAddr.s_addr, lan_mac_address, lan_if_name);
	      // output result of ARP
	      syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s:%d address%s arpRet:%d\n","NewRequestHandler:ARP",__LINE__,inet_ntoa(DestAddr->sin_addr),arpRet);
      }
      // end ARP check

      for(Index=0;Index< NumPacket;Index++)
      {

          NumCopy =0;
          TryIdx =0;

          while(TryIdx < NUM_TRY && NumCopy < NUM_SSDP_COPY)
          {

             FD_ZERO(&WrSet);
             FD_SET(ReplySock,&WrSet);

             tmout.tv_sec = 1;
             tmout.tv_usec = 1;

             DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending reply %s\n",*(RqPacket+Index));)
		     
			setsockopt(ReplySock, IPPROTO_IP, IP_MULTICAST_TTL, &Ttl, sizeof(Ttl));
             ret=sendto(ReplySock,*(RqPacket+Index),strlen(*(RqPacket+Index)),0,(struct sockaddr *)DestAddr, socklen);

   	     syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d port:%d\n bufferlen: %d sendtoRet:%d buffer:%s","NewRequestHandler",__LINE__,ntohs(DestAddr->sin_port),strlen(*(RqPacket+Index)),ret,*(RqPacket+Index));
             if ( select(ReplySock+1,NULL,&WrSet,NULL,NULL) == -1)
             {
                DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select !!!!!!!!\n")) ;
                if ( errno == EBADF)
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
                else if ( errno ==  EINTR)
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught.    \n");)}
                else if ( errno ==  EINVAL )
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
                else if ( errno ==   ENOMEM )
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"select was unable to allocate memory for internal tables.\n");)}
                SendErrorEvent(UPNP_E_NETWORK_ERROR);
                break;
             }
             else if(FD_ISSET(ReplySock,&WrSet))
             {
                 ++NumCopy;
             }
             else TryIdx++;
         }
       }

       free(Evt);
       close(ReplySock);
       return UPNP_E_SUCCESS;
 }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    :  void CreateServiceRequestPacket(int Notf,char *RqstBuf,char * NtSt,char *Usn,char *Server,char * S,char * Location,int  Duration)
 // Description : This function creates a HTTP request packet.  Depending on the input parameter it either creates a service advertisement
 //               request or service shutdown request etc.
 // Parameters  : Notf : Specify the type of notification, either advertisement or shutdown etc.
 //               RqstBuf  : Output buffer filled with HTTP statement.
 //               RqstId   : Same ID as send with the request HTTP statement.
 //               ServType : Service Type or category
 //               ServId   :Service unique name or ID
 //               Location : Location URL.
 //               Duration : Service duration in sec.
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


 void CreateServiceRequestPacket(int Notf,char *RqstBuf,char * NtSt,char *Usn,char *Server,char * Location,int  Duration)
 {
   char TempBuf[COMMAND_LEN];
   int  Port = SSDP_PORT;
   char Date[40];
   struct utsname sys_info;
   
   //Note  Notf=0 means service shutdown, Notf=1 means service advertisement, Notf =2 means reply
   currentTmToHttpDate(Date);

   memset(&sys_info,0x00,sizeof(sys_info));
   uname(&sys_info);

   if(Notf ==2)
   {
   syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d \n","CreateServiceRequestPacket",__LINE__);
     strcpy(RqstBuf,"HTTP/1.1 200 OK\r\n");
     sprintf(TempBuf,"CACHE-CONTROL: max-age=%d\r\n",Duration);
     strcat(RqstBuf,TempBuf);
     strcat(RqstBuf,Date);
     strcat(RqstBuf,"EXT:\r\n");
     sprintf(TempBuf,"LOCATION: %s\r\n",Location);
     strcat(RqstBuf,TempBuf);
     sprintf(TempBuf,"SERVER: %s/%s UPnP/1.0 Intel UPnP SDK/1.0\r\n",
             sys_info.sysname, sys_info.release);
     strcat(RqstBuf,TempBuf);
     sprintf(TempBuf,"ST: %s\r\n",NtSt);
     strcat(RqstBuf,TempBuf);


   }
   else if(Notf ==1)
   {
      strcpy(RqstBuf,"NOTIFY * HTTP/1.1\r\n");
      strcpy(TempBuf,"HOST:");
      strcat(TempBuf,SSDP_IP);
      sprintf(TempBuf,"%s:%d\r\n",TempBuf,Port);
      strcat(RqstBuf,TempBuf);
      sprintf(TempBuf,"Cache-Control: max-age= %d\r\n",Duration);
      strcat(RqstBuf,TempBuf);
      sprintf(TempBuf,"LOCATION: %s\r\n",Location);
      strcat(RqstBuf,TempBuf);
      sprintf(TempBuf,"NT:%s\r\n",NtSt);
      strcat(RqstBuf,TempBuf);
      strcat(RqstBuf,"NTS:ssdp:alive\r\n");
      sprintf(TempBuf,"SERVER: %s/%s UPnP/1.0 Intel UPnP SDK/1.0\r\n",
             sys_info.sysname, sys_info.release);
      strcat(RqstBuf,TempBuf);
   }
   else if(Notf ==0)
   {
      strcpy(RqstBuf,"NOTIFY * HTTP/1.1\r\n");
      strcpy(TempBuf,"HOST: ");
      strcat(TempBuf,SSDP_IP);
      sprintf(TempBuf,"%s:%d\r\n",TempBuf,Port);
      strcat(RqstBuf,TempBuf);

      // Following two header is added to interop with Windows Millenium but this is not
      // a part of UPNP spec 1.0 
      sprintf(TempBuf,"CACHE-CONTROL: max-age=%d\r\n",Duration);
      strcat(RqstBuf,TempBuf);
      sprintf(TempBuf,"LOCATION: %s\r\n",Location);
      strcat(RqstBuf,TempBuf);


      sprintf(TempBuf,"NT: %s\r\n",NtSt);
      strcat(RqstBuf,TempBuf);
      strcat(RqstBuf,"NTS: ssdp:byebye\r\n");  

   }

   sprintf(TempBuf,"USN: %s\r\n",Usn);
   strcat(RqstBuf,TempBuf);
   strcat(RqstBuf,"\r\n\r\n");

 }



 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int DeviceAdvertisement(int RootDev,char * Udn, char * DevType,char *Server,char * Location,int  Duration)
 // Description : This function creates the device advertisement request based on the input parameter, and send it to the
 //               multicast channel.
 // Parameters  : RootDev : 1 means root device 0 means embedded device.
 //               Udm : Device UDN
 //               DevType : Device Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int DeviceAdvertisement(char * DevType, int RootDev,char * Udn, char *Server, char * Location, int  Duration, int Ttl)
{

    struct sockaddr_in DestAddr;
    char *szReq[3], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal;


    DBGONLY(UpnpPrintf(UPNP_ALL,SSDP,__FILE__,__LINE__,"In function SendDeviceAdvertisemenrt\n");)

    DestAddr.sin_family = AF_INET;
    DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
    DestAddr.sin_port = htons(SSDP_PORT);

    if(RootDev) //If deviceis a root device , here we need to send 3 advertisement or reply
    {
       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       szReq[2] = (char*)malloc(BUFSIZE);

       if (szReq[0] == NULL || szReq[1] == NULL || szReq[2] == NULL) return UPNP_E_OUTOF_MEMORY ;

       strcpy(Mil_Nt,"upnp:rootdevice");
       sprintf(Mil_Usn,"%s::upnp:rootdevice",Udn);
       CreateServiceRequestPacket(1,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(1,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(1,szReq[2],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandlerTtl(&DestAddr,3, szReq, Ttl) ;

       free(szReq[0]);
       free(szReq[1]);
       free(szReq[2]);


    }
    else //If device is not a root device then it is a sub-device., here we need to send 2 advertisement or reply
    {

       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL || szReq[1] == NULL ) return UPNP_E_OUTOF_MEMORY ;

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(1,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(1,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandlerTtl(&DestAddr,2, szReq, Ttl);
       free(szReq[0]);
       free(szReq[1]);


     }


    return RetVal;


}

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int DeviceReply(int RootDev, char *DevType,char * Udn,struct sockaddr_in * DestAddr,char *Server,char * Location,int  Duration)
 // Description : This function creates the reply packet based on the input parameter, and send it to the client addesss
 //               given in its input parameter DestAddr.
 // Parameters  : RootDev : 1 means root device 0 means embedded device.
 //               Udn : Device UDN
 //               DevType : Device Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 //               DestAddr : destination IP address.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int SendReply(struct sockaddr_in * DestAddr, char *DevType, int RootDev, char * Udn, char *Server, char * Location, int  Duration, int ByType)
 {

    char *szReq[1], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal;


    if(RootDev) //If deviceis a root device , here we need to send 3 advertisement or reply
    {
       szReq[0] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY ;

       strcpy(Mil_Nt,"upnp:rootdevice");
       sprintf(Mil_Usn,"%s::upnp:rootdevice",Udn);
       CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandler(DestAddr,1, szReq) ;

       free(szReq[0]);

    }
    else //If device is not a root device then it is a sub-device., here we need to send 2 advertisement or reply
    {

        szReq[0] = (char*)malloc(BUFSIZE);
        if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY;


        if(ByType == 0)
        {
           sprintf(Mil_Nt,"%s",Udn);
           sprintf(Mil_Usn,"%s",Udn);
           CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
           RetVal = NewRequestHandler(DestAddr,1, szReq);
        }
        else
        {
           sprintf(Mil_Nt,"%s",DevType);
           sprintf(Mil_Usn,"%s::%s",Udn,DevType);
           CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
           RetVal = NewRequestHandler(DestAddr,1, szReq);

        }

        free(szReq[0]);
    }

    return RetVal;

 }



 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int DeviceReply(int RootDev, char *DevType,char * Udn,struct sockaddr_in * DestAddr,char *Server,char * Location,int  Duration)
 // Description : This function creates the reply packet based on the input parameter, and send it to the client addesss
 //               given in its input parameter DestAddr.
 // Parameters  : RootDev : 1 means root device 0 means embedded device.
 //               Udn : Device UDN
 //               DevType : Device Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 //               DestAddr : destination IP address.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  int DeviceReply(struct sockaddr_in * DestAddr, char *DevType, int RootDev, char * Udn, char *Server, char * Location, int  Duration)
 {

    char *szReq[3], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal;


    if(RootDev) //If deviceis a root device , here we need to send 3 advertisement or reply
    {
       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       szReq[2] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL || szReq[1] == NULL|| szReq[2] == NULL ) return UPNP_E_OUTOF_MEMORY ;

       syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d, MSearch:%d Port:%d \n","DeviceReply",__LINE__,mSearchNum, ntohs(DestAddr->sin_port));
       
       strcpy(Mil_Nt,"upnp:rootdevice");
       sprintf(Mil_Usn,"%s::upnp:rootdevice",Udn);
       CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(2,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(2,szReq[2],Mil_Nt,Mil_Usn,Server,Location,Duration);


       RetVal = NewRequestHandler(DestAddr,3, szReq) ;

       free(szReq[0]);
       free(szReq[1]);
       free(szReq[2]);


    }
    else //If device is not a root device then it is a sub-device., here we need to send 2 advertisement or reply
    {

       syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d \n","DeviceReply:non root",__LINE__);
       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL || szReq[1] == NULL ) return UPNP_E_OUTOF_MEMORY ;

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(2,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);


       RetVal = NewRequestHandler(DestAddr,2, szReq);
       free(szReq[0]);
       free(szReq[1]);


     }

    return RetVal;
 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ServiceAdvertisement( char * Udn, char * ServType,char *Server,char * Location,int  Duration)
 // Description : This function creates the advertisement packet based on the input parameter, and send it to the
 //               multicast channel.
 // Parameters  : Server : Os.
 //               Udn : Device UDN
 //               ServType : Service Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   int ServiceAdvertisement( char * Udn, char * ServType,char *Server,char * Location,int  Duration,int Ttl)
  {
       char Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
       char * szReq[1];
       struct sockaddr_in DestAddr;
       int RetVal;


       fflush(stdout);


       szReq[0] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY ;

       DestAddr.sin_family = AF_INET;
       DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
       DestAddr.sin_port = htons(SSDP_PORT);


       sprintf(Mil_Nt,"%s",ServType);
       sprintf(Mil_Usn,"%s::%s",Udn,ServType);
       CreateServiceRequestPacket(1,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
       RetVal = NewRequestHandlerTtl(&DestAddr,1, szReq, Ttl);


       free(szReq[0]);
       return RetVal;
  }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ServiceReply( char * Udn, char * ServType,char *Server,char * Location,int  Duration,struct sockaddr_in *DestAddr)
 // Description : This function creates the advertisement packet based on the input parameter, and send it to the
 //               multicast channel.
 // Parameters  : Server : Os
 //               Udn : Device UDN
 //               ServType : Service Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 //               DestAddr : Client IP address.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   int ServiceReply(struct sockaddr_in *DestAddr,  char * ServType, char * Udn, char *Server,char * Location,int  Duration)
  {
       char Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
       char * szReq[1];
       int RetVal;

       szReq[0] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY ;

       sprintf(Mil_Nt,"%s",ServType);
       sprintf(Mil_Usn,"%s::%s",Udn,ServType);
       CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
       RetVal = NewRequestHandler(DestAddr,1, szReq);

       free(szReq[0]);
       return RetVal;

  }



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ServiceShutdown(int RootDev, char *DevType,char * Udn,struct sockaddr_in * DestAddr,char *Server,char * Location,int  Duration)
 // Description : This function creates a HTTP service shutdown request packet and sent it to the multicast channel through
 //               RequestHandler.
 // Parameters  : RootDev  : 1 means root device.
 //               DevType  : Device Type or category
 //               Udn      : Device UDN
 //               Location : Location URL.
 //               Duration : Service duration in sec.
 //
 // Return value: None
 ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////
  int ServiceShutdown( char * Udn, char * ServType,char *Server,char * Location,int  Duration)
{
        
       char Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
       char * szReq[1];
       struct sockaddr_in DestAddr;
       int RetVal;


       szReq[0] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY ;

       DestAddr.sin_family = AF_INET;
       DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
       DestAddr.sin_port = htons(SSDP_PORT);


       sprintf(Mil_Nt,"%s",ServType);
       sprintf(Mil_Usn,"%s::%s",Udn,ServType);
       CreateServiceRequestPacket(0,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
       RetVal = NewRequestHandler(&DestAddr,1, szReq);


       free(szReq[0]);
       return RetVal;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int DeviceShutdown(int RootDev, char *DevType,char * Udn,struct sockaddr_in * DestAddr,char *Server,char * Location,int  Duration)
 // Description : This function creates a HTTP service shutdown request packet and sent it to the multicast channel through
 //               RequestHandler.
 // Parameters  : RootDev  : 1 means root device.
 //               DevType  : Device Type or category
 //               Udn      : Device UDN
 //               Location : Location URL.
 //               Duration : Service duration in sec.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int DeviceShutdown(char * DevType, int RootDev,char * Udn, char *Server, char * Location, int  Duration)
 {

    struct sockaddr_in DestAddr;
    char *szReq[3], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal;

    DBGONLY(UpnpPrintf(UPNP_ALL,SSDP,__FILE__,__LINE__,"In function SendDeviceAdvertisemenrt\n");)


    DestAddr.sin_family = AF_INET;
    DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
    DestAddr.sin_port = htons(SSDP_PORT);

    if(RootDev) //If deviceis a root device , here we need to send 3 advertisement or reply
    {
       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       szReq[2] = (char*)malloc(BUFSIZE);

       if (szReq[0] == NULL || szReq[1] == NULL || szReq[2] == NULL) return UPNP_E_OUTOF_MEMORY ;

       strcpy(Mil_Nt,"upnp:rootdevice");
       sprintf(Mil_Usn,"%s::upnp:rootdevice",Udn);
       CreateServiceRequestPacket(0,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(0,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(0,szReq[2],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandler(&DestAddr,3, szReq) ;


       free(szReq[0]);
       free(szReq[1]);
       free(szReq[2]);


    }
    else //If device is not a root device then it is a sub-device., here we need to send 2 advertisement or reply
    {

       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL || szReq[1] == NULL ) return UPNP_E_OUTOF_MEMORY ;

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(0,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(0,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandler(&DestAddr,2, szReq);

       free(szReq[0]);
       free(szReq[1]);


     }
     return RetVal;

}

#endif
#endif
