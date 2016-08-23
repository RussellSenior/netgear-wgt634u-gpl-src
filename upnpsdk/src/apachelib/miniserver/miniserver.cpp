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

// $Revision: 1.4 $
// $Date: 2002/01/25 00:18:23 $
#include "../../inc/tools/config.h"
#if EXCLUDE_MINISERVER == 0
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <signal.h>

#include <genlib/util/utilall.h>
#include <genlib/util/util.h>
#include <genlib/miniserver/miniserver.h>
#include <genlib/tpool/scheduler.h>
#include <genlib/tpool/interrupts.h>
#include "upnp.h"

#include "tools/config.h"

extern "C" {
#include "../../api/upnpapi.h"
}

#include "gena/gena.h"
#include "genlib/timer_thread/timer_thread.h"
#include <mod_upnp/mod_upnp.h>

// read timeout
#define TIMEOUT_SECS 30
enum MiniServerState { MSERV_IDLE, MSERV_RUNNING, MSERV_STOPPING };

CREATE_NEW_EXCEPTION_TYPE( MiniServerReadException, BasicException, "MiniServerReadException" )

enum READ_EXCEPTION_CODE {
        RCODE_SUCCESS               =  0,
        RCODE_NETWORK_READ_ERROR    = -1,
        RCODE_MALFORMED_LINE        = -2,
        RCODE_LENGTH_NOT_SPECIFIED  = -3,
        RCODE_METHOD_NOT_ALLOWED    = -4,
        RCODE_INTERNAL_SERVER_ERROR = -5,
        RCODE_METHOD_NOT_IMPLEMENTED = -6,
        RCODE_TIMEDOUT              = -7,
        };
        
enum HTTP_COMMAND_TYPE { CMD_HTTP_GET,
        CMD_SOAP_POST, CMD_SOAP_MPOST,
        CMD_GENA_SUBSCRIBE, CMD_GENA_UNSUBSCRIBE, CMD_GENA_NOTIFY,
        CMD_HTTP_UNKNOWN,
        CMD_HTTP_MALFORMED };

enum UPNP_TYPE{
	UPNP_CONTROLPOINT,
	UPNP_DEVICE};

// module vars

static MiniServerCallback gGetCallback = NULL;
static MiniServerCallback gSoapCallback = NULL;
static MiniServerCallback gGenaCallback = NULL;
static MiniServerWrite gWriteCallback = NULL;

static MiniServerState gMServState = MSERV_IDLE;
static pthread_t gMServThread = 0;

extern int UpnpSdkInit;

#define UPNP_IPC_TMP P_tmpdir"/.upnp"
#define UPNP_IPC_SHM				10
#define UPNP_IPC_SEM				20
const int MAX_INSTANCES=4;
const int MAX_MESSAGE_SIZE=8192;
const int MAX_TIMEOUT=100000;
const int NUM_SEMAPHORES=2+MAX_INSTANCES;/* 1 for server, and 1 for locking of shared memory */

static key_t shmkey=-1;
static key_t semkey=-1;
static int shmid=-1;
static int semid=-1;
const int serverEvent=0;
const int shmLock=1;

// Shared Memory
struct eventList{
	int semNum;
	int port;
	UPNP_TYPE type;
};
struct SharedMessage{
	eventList eventsRegistered[MAX_INSTANCES];//right now only support 4..	
	HTTP_COMMAND_TYPE cmdType;
	char message[MAX_MESSAGE_SIZE+1];
	int cbParams;
};
static SharedMessage *shMsg;
// Semaphore
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
	int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short int *array;  /* array for GETALL, SETALL */
    struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#endif

void ClientSignaler(HTTP_COMMAND_TYPE cmd, const char *doc, int rec);
void SoapSignaler(const char *doc, int rec);
void GenaSignaler(const char *doc, int rec);
//////////////

void SetSemaphoreUPnPType()
{
	if((int)shMsg > 0){
		for(int i=0;i<MAX_INSTANCES;i++){
			if(shMsg->eventsRegistered[i].port==LOCAL_PORT){
				// only device should have LOCAL_PORT set AND be calling this
				shMsg->eventsRegistered[i].type=UPNP_DEVICE;
				break;
			}
		}
	}
}

void SetHTTPGetCallback( MiniServerCallback callback )
{
    gGetCallback = callback;
	SetSemaphoreUPnPType();//only called for a device
}

MiniServerCallback GetHTTPGetCallback( void )
{
    return gGetCallback;
}

void SetSoapCallback( MiniServerCallback callback )
{
    gSoapCallback = callback;
}

MiniServerCallback GetSoapCallback( void )
{
    return gSoapCallback;
}

void SetGenaCallback( MiniServerCallback callback )
{
    gGenaCallback = callback;
}

MiniServerCallback GetGenaCallback( void )
{
    return gGenaCallback;
}

void SetWriteToClient(MiniServerWrite callback)
{
   gWriteCallback=callback;
}

int WriteToClient(int sockfd, const char *bytes, size_t len, int timeout)
{
   	int lenWritten=0;
	if(gWriteCallback!=NULL){
    	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
					"Going out to APACHE \n");)
		lenWritten=gWriteCallback(((CBParams *)sockfd)->rec,bytes,len,0);
	}else{
    	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
					"Going out shared memory \n");)
		// we're in the device
		lenWritten=MAX_MESSAGE_SIZE-strlen(shMsg->message);
		lenWritten=(lenWritten > (int)len ?(int) len : lenWritten);
		strncat(shMsg->message,bytes,lenWritten);
	}

   return lenWritten;
}

// setup all the callbacks, w/o the SSDP multicast listener
int MiniServerInit(char *webRoot)
{
	UpnpSdkInit=1;

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside MiniServerInit \n");)

    HandleLock();
	LOCAL_PORT=0;
	/*
    if (HostIP!=NULL)
        strcpy(LOCAL_HOST,HostIP);
    else
    {
        if (getlocalhostname(LOCAL_HOST)!=UPNP_E_SUCCESS)
        {
            HandleUnlock();
            return UPNP_E_INIT_FAILED;
        }
    }
   
    if(MiniServerInit != 0)
    {
        HandleUnlock();
        return UPNP_E_INIT;
    }
	*/

    InitHandleList();
    HandleUnlock();
     
    tpool_SetMaxThreads(MAX_THREADS + 3); // 3 threads are required for running
                                    // miniserver, ssdp.
    if (tintr_Init(SIGUSR1) != 0)
	   return UPNP_E_INIT_FAILED;
    #if EXCLUDE_SOAP == 0
    //InitSoap();
    gSoapCallback=SoapSignaler;
    #endif
    #if EXCLUDE_GENA == 0
	// override
    gGenaCallback=GenaSignaler;
    #endif
	
	close(open(UPNP_IPC_TMP,O_CREAT|O_WRONLY,0));
	
	if(semkey==-1)
		semkey=ftok(UPNP_IPC_TMP,UPNP_IPC_SEM);

	if(shmkey==-1)
		shmkey=ftok(UPNP_IPC_TMP,UPNP_IPC_SHM);

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"semkey: %i, shmkey: %i \n",semkey,shmkey);)
	
	if(semkey!=-1){
		short initialized=0;
		semid=semget(semkey,NUM_SEMAPHORES,IPC_CREAT|IPC_EXCL|0666);
		if(semid==-1 && errno==EEXIST){
			semid=semget(semkey,NUM_SEMAPHORES,IPC_CREAT|0666);
			initialized=1;
		}

    	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
							"semid: %i,err: %i\n",semid,errno);)
		if(semid!=-1 && !initialized){
			semun upnpSem;
			upnpSem.val=1;//unlocked
			for(int i=0;i<MAX_INSTANCES;i++){
				if(semctl(semid,i,SETVAL,upnpSem)==-1){
    				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
								"semctl error: %i\n",errno);)
				}
				if(i==1)
					upnpSem.val=0;//first 2 unlocked, all the others are locked
			}
		}
	}

	if(shmkey!=-1){
		short initialized=0;
		shmid=shmget(shmkey,sizeof(SharedMessage),IPC_CREAT|IPC_EXCL|SHM_DEST|SHM_R|SHM_W);

		if(shmid==-1 && errno==EEXIST){
			shmid=shmget(shmkey,sizeof(SharedMessage),IPC_CREAT|SHM_DEST|SHM_R|SHM_W);
			initialized=1;
		}

		if(shmid!=-1){
			// ok, created shared memory
			shMsg=(SharedMessage *)shmat(shmid,0,0);
    		DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"shMsg: %i \n",(int)shMsg);)
			if((int)shMsg!=-1 && !initialized){
				memset(shMsg,'\0',sizeof(SharedMessage));
			}else{/*
				//error
    			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
							"Shared Memory invalid, id: %i, errno: %i\n",shmid,errno);)
							*/
			}
		}
	}
	
	UpnpSetWebServerRootDir(webRoot);


    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"semid: %i, shmid: %i \n",semid,shmid);)

    return UPNP_E_SUCCESS;
}

int MiniServerDeInit()
{
	if(shmid!=-1){
		shmid_ds dsBuf;
		shmctl(shmid,IPC_STAT,&dsBuf);
		shmctl(shmid,IPC_RMID,&dsBuf);
		shmdt(&shmid);
	}
	if(semid!=-1){
		semun upnpSem;
		upnpSem.val=1;
		semctl(semkey,0,IPC_RMID,upnpSem);
	}

	return UPNP_E_SUCCESS;
}

static void semaphoreLock(int id, int semNum)
{
	int bResult=0;
	sembuf upnpSem={semNum,-1,0};
	while(semop(id,&upnpSem,1)==-1){
		if(errno!=EINTR){// wait even when getting signals..
   			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
						"sem: %i lock FAILED, err: %i\n",semNum,errno);)
			bResult=-1;
			break;
		}else if(gMServState==MSERV_STOPPING){// we've been signaled to stop
   			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
						"-- stop signal");)
			break;
		}
	}
	if(bResult==0){
	    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"semaphore locked\n");)
	}
}

static void semaphoreUnlock(int id, int semNum)
{
	int bResult=0;
	sembuf upnpSem={semNum,1,0};
	bResult=semop(id,&upnpSem,1);
	if(bResult==0){
		DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__, "semaphore unlocked");)
	}else{
		DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"semaphore unlock FAILED, err: %i\n",errno);)
	}
}

DBGONLY(
static int isUnlocked(int id, int semNum)
{
	return semctl(id,semNum,GETVAL,0);
}
)

void GenaSignaler(const char *doc, int rec)
{
	ClientSignaler(CMD_GENA_SUBSCRIBE/* doesnt really matter*/,doc,rec);
}

void SoapSignaler(const char *doc, int rec)
{
	ClientSignaler(CMD_SOAP_POST/* doesnt really matter*/,doc,rec);
}

void ClientSignaler(HTTP_COMMAND_TYPE cmd, const char *doc, int cbParams)
{
	long timeout=0;
    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"client signaler, semid: %i, shMsg: %i\n",semid,(int)shMsg);)
	CBParams *cb=(CBParams *)cbParams;
	// control points only receive gena notify's right now...
	UPNP_TYPE cbUPnPType=(cb->cmdType==CMD_GENA_NOTIFY ? UPNP_CONTROLPOINT : UPNP_DEVICE);
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Command Type: %i\n",cbUPnPType);)

	if(semid!=-1 && ((int)shMsg)!=-1){
		int clientToLock=0;
		int len=strlen(doc);
		//locked
		semaphoreLock(semid,shmLock);
		semaphoreLock(semid,serverEvent);
    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"semaphore unlocked? %i\n",isUnlocked(semid,serverEvent));)
		shMsg->cmdType=cmd;
		shMsg->cbParams=cbParams;
		// clear the memory..
		memset(shMsg->message,'\0',sizeof(shMsg->message));
		strncpy(shMsg->message,doc,(len > MAX_MESSAGE_SIZE? MAX_MESSAGE_SIZE:len));
		//unlock
		semaphoreUnlock(semid,serverEvent);

		//signal waiting process (only 1 supported now, but could be more)
		for(int i=0;i<MAX_INSTANCES;i++){
			if((shMsg->eventsRegistered[i].semNum!=0) && 
					(shMsg->eventsRegistered[i].type==cbUPnPType)){
				semaphoreUnlock(semid,shMsg->eventsRegistered[i].semNum);
				clientToLock=shMsg->eventsRegistered[i].semNum;
DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"waking up: %i\n",clientToLock);)
				break;// done
			}
		}

		if(clientToLock){
			timeout=0;
			// now wait for them to lock....
			while((semctl(semid,serverEvent,GETPID,0)==getpid())/*&&
					(timeout<MAX_TIMEOUT)*/){
				usleep(10);
				timeout++;
			}
	
    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"semaphore done waiting \n");)
			//wait on lock
			semaphoreLock(semid,serverEvent);

			semaphoreLock(semid,clientToLock);
    	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"message out: %s\n",shMsg->message);)

			WriteToClient(shMsg->cbParams,shMsg->message,strlen(shMsg->message),0);

			//unlock, all done
			semaphoreUnlock(semid,serverEvent);
		}
		// unlock for others
		semaphoreUnlock(semid,shmLock);
	}
}
//////////////////////////


static void RunMiniServer(void *args)
{
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
				"-------- thread started ----------\n");)
	int clientEvent=(int)args;
	gMServThread=pthread_self();
	gMServState=MSERV_RUNNING;
	long timeout=0;
    
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
				"sem unlocked? %i\n",isUnlocked(semid,clientEvent));)
	while(1){
		DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
					"starting wait,event: %i\n",clientEvent);)

		//wait on our event
		semaphoreLock(semid,clientEvent);
	
		if(gMServState==MSERV_STOPPING)
			break;
	
		//
		MiniServerCallback callback=NULL;
	    //we're gonna wait for a wakeup call    
		if(semid!=-1 && ((int)shMsg)!=-1){
			semaphoreLock(semid,serverEvent);
	
			char messageIn[MAX_MESSAGE_SIZE+1]={0};

			//now process the callback from shared memory
			switch(shMsg->cmdType){
			case CMD_SOAP_POST:
			case CMD_SOAP_MPOST:
    			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"soap cb\n");)
				callback=gSoapCallback;
				break;
			case CMD_GENA_NOTIFY:
			case CMD_GENA_SUBSCRIBE:
			case CMD_GENA_UNSUBSCRIBE:
    			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"gena cb\n");)
				callback=gGenaCallback;
				break;
			default:
				callback=NULL;
			}
			if(callback!=NULL){
				if(strlen(shMsg->message)){
					strcpy(messageIn,shMsg->message);
    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"message sent to callback: %s\n",messageIn);)
					memset(shMsg->message,'\0',sizeof(shMsg->message));
					callback(messageIn,shMsg->cbParams);
				}
			}
			// unlock and keep chugging
			semaphoreUnlock(semid,serverEvent);
	
		}
		semaphoreUnlock(semid,clientEvent);

		timeout=0;
		// now wait for them to lock....
		while((semctl(semid,clientEvent,GETPID,0)==getpid())/*&&
				(timeout < MAX_TIMEOUT)*/){
			usleep(10);
			timeout++;
		}
	}
	// remove our selves
	for(int i=0;i<MAX_INSTANCES;i++){
		if(shMsg->eventsRegistered[i].semNum==clientEvent){
			shMsg->eventsRegistered[i].semNum=0;
    		DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"FOUND MYSELF: %i,REMOVING\n",clientEvent);)
			break;
		}
	}
	gMServState=MSERV_IDLE;
}



// if listen port is 0, port is dynamically picked
// returns:
//   on success: actual port socket is bound to
//   on error:   a negative number UPNP_E_XXX
int StartMiniServer( unsigned short listen_port )
{
	gMServState=MSERV_IDLE;
	int clientEvent=0;

	// semaphore setup
	semkey=ftok(UPNP_IPC_TMP,UPNP_IPC_SEM);
	semid=semget(semkey,NUM_SEMAPHORES,IPC_CREAT/*|IPC_EXCL*/|0666);

	// setup the shared memory
	shmkey=ftok(UPNP_IPC_TMP,UPNP_IPC_SHM);
	shmid=shmget(shmkey,sizeof(SharedMessage),0);

	if((shmid==-1 && errno==EEXIST)||shmid!=-1){
		// ok, found shared memory
		shMsg=(SharedMessage *)shmat(shmid,0,0);
    	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"--!! shMsg: %i \n",(int)shMsg);)
		if((int)shMsg!=-1){
			for(int i=0;i<MAX_INSTANCES;i++){
				if(shMsg->eventsRegistered[i].semNum ==0){//all good, take it
					clientEvent=shMsg->eventsRegistered[i].semNum=i+2;// +2 reserved
					shMsg->eventsRegistered[i].port=listen_port;
    	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"--!! I am: %i \n",i+2);)
					break;// done
				}
			}
			if(clientEvent)//spawn our controlling thread
				tpool_Schedule(RunMiniServer,(void *)clientEvent);
			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"MESSAGE: %s\n",shMsg->message);)
		}
	}

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"semid: %i, shmid: %i \n",semid,shmid);)

	return (shmid ? 80: -1);// default web port
}

// returns 0: success; -2 if miniserver is idle
int StopMiniServer( void )
{
	if(gMServState==MSERV_IDLE)
		return -2;

	// signal the thread to stop
	gMServState=MSERV_STOPPING;
	// keep sending signals until server stops
    while ( true )
    {
        if ( gMServState == MSERV_IDLE )
        {
            break;
        }

        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "StopMiniServer(): sending interrupt\n"); )

        int code = tintr_Interrupt( gMServThread );
        if ( code < 0 )
        {
            DBG(
                UpnpPrintf( UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
                    "%s: StopMiniServer(): interrupt failed",
                    strerror(errno) ); )

            //DBG( perror("StopMiniServer(): interrupt failed"); )
        }

        if ( gMServState == MSERV_IDLE )
        {
            break;
        }

        sleep( 1 );     // pause before signalling again
    }


	if(shmid!=-1){
		shmdt(&shmid);
	}
    return 0;
}

#endif
