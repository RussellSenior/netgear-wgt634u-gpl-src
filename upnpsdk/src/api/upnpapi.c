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
//    

//File upnpapi.c
#include "../../inc/tools/config.h"
#include <assert.h>
#include <signal.h>
#include "upnpapi.h"
#include "../inc/genlib/tpool/interrupts.h"
#include "../inc/genlib/tpool/scheduler.h"
#include <stdlib.h>
#include<string.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
//************************************
//Needed for GENA
#include "../inc/gena/gena.h"
#include "../inc/genlib/service_table/service_table.h"
#include "../inc/genlib/miniserver/miniserver.h"
//*******************************************

/* ********************* */
#ifdef INTERNAL_WEB_SERVER
#include "../inc/genlib/net/http/server.h"
#include "../inc/urlconfig/urlconfig.h"
#endif /* INTERNAL_WEB_SERVER */
/* ****************** */

pthread_mutex_t GlobalHndMutex = PTHREAD_MUTEX_INITIALIZER;
#include "../inc/genlib/timer_thread/timer_thread.h"

int UpnpSdkInit = 0; // Global variable to denote the state of Upnp SDK
                     // = 0 if uninitialized, = 1 if initialized.

int UpnpInit(IN const char *HostIP, IN unsigned short DestPort)
{
    int retVal=0;


    DBGONLY(if( InitLog() != UPNP_E_SUCCESS) 
	    return UPNP_E_INIT_FAILED;);


    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpInit \n");)

    HandleLock();
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
   
    if(UpnpSdkInit != 0)
    {
        HandleUnlock();
        return UPNP_E_INIT;
    }

    InitHandleList();
    HandleUnlock();
     
    tpool_SetMaxThreads(MAX_THREADS + 3); // 3 threads are required for running
                                    // miniserver, ssdp.
    if (tintr_Init(SIGUSR1) != 0)
	   return UPNP_E_INIT_FAILED;
    UpnpSdkInit = 1; 
    #if EXCLUDE_SOAP == 0
    InitSoap();
    #endif
    #if EXCLUDE_GENA == 0
    SetGenaCallback(genaCallback);
    #endif

    if ((retVal= InitTimerThread(&GLOBAL_TIMER_THREAD))!=UPNP_E_SUCCESS)
    {
	       UpnpSdkInit=0;
	       UpnpFinish();
	       return retVal;
    }
    #if EXCLUDE_SSDP == 0
    syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d \n","UPnPInit:afterif",__LINE__);
    if ((retVal = InitSsdpLib(SsdpCallbackEventHandler)) != UPNP_E_SUCCESS)
    {
        UpnpSdkInit = 0; 
        if (retVal != -1) 
            return retVal; 
        else // if ssdp is already running for unknown reasons!
            return UPNP_E_INIT_FAILED;
    }
    #endif
   
    #if EXCLUDE_MINISERVER == 0
    if ((retVal = StartMiniServer(DestPort))<=0)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"Miniserver failed to start");)
        UpnpFinish();
        UpnpSdkInit = 0; 
        if (retVal != -1) 
            return retVal; 
        else // if miniserver is already running for unknown reasons!
            return UPNP_E_INIT_FAILED;

    }
    #endif
    
    DestPort=retVal;
    LOCAL_PORT=DestPort; 
    
    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Host Ip: %s Host Port: %d\n",LOCAL_HOST,LOCAL_PORT));
    
    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Exiting UpnpInit \n");)
      
    return UPNP_E_SUCCESS; 
} /***************** end of UpnpInit ******************/ 

int UpnpFinish()
{
    DEVICEONLY(UpnpDevice_Handle  device_handle;)
    CLIENTONLY(UpnpClient_Handle  client_handle;)
    struct Handle_Info * temp;
    DBGONLY(
    int retVal1 = 1; 
    int retVal2 = 1;)
    if (UpnpSdkInit != 1)
        return UPNP_E_FINISH;

  
    DBGONLY(
    
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpFinish : UpnpSdkInit is :%d:\n", UpnpSdkInit);
    if (UpnpSdkInit == 1)
    {
      UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpFinish : UpnpSdkInit is ONE\n");
    }
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Jobs pending = %d : Threads running = %d\n",
       tpool_GetNumJobsPending(), tpool_GetNumThreadsRunning());)

  
    UpnpSdkInit = 0; 

    #ifdef INCLUDE_DEVICE_APIS
    if (GetDeviceHandleInfo(&device_handle, &temp)==HND_DEVICE)
      UpnpUnRegisterRootDevice(device_handle);
    #endif

    #ifdef INCLUDE_CLIENT_APIS
    if (GetClientHandleInfo(&client_handle,&temp)==HND_CLIENT)
      UpnpUnRegisterClient(client_handle);
    #endif
    
    StopTimerThread(&GLOBAL_TIMER_THREAD);
    #if EXCLUDE_SSDP == 0
    DeInitSsdpLib();
    #endif
    sleep(3);

    StopMiniServer(); 
    tintr_Done();

    DBGONLY(
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Exiting UpnpFinish : UpnpSdkInit is :%d:\n", UpnpSdkInit);
    while (retVal1 || retVal2)
    {
      sleep(3);
      retVal1 = tpool_GetNumJobsPending();
      retVal2 = tpool_GetNumThreadsRunning();
      UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Jobs pending = %d : Threads running = %d\n",retVal1, retVal2);
      break;
    }
)
      DBGONLY(CloseLog(););
    return UPNP_E_SUCCESS;
}  /********************* End of  UpnpFinish  *************************/

#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice (
    IN const char *DescUrl,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie,
    OUT UpnpDevice_Handle *Hnd
    )
{

    struct Handle_Info * HInfo; 
    int retVal=0;

    DBGONLY( UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpRegisterRootDevice \n");)

    if(UpnpSdkInit != 1)
    {
        HandleUnlock();
        return UPNP_E_FINISH;
    }
    if (Hnd == NULL || Fun == NULL || DescUrl == NULL || strlen(DescUrl) == 0) 
        return UPNP_E_INVALID_PARAM;

    HandleLock();
    if ((*Hnd = GetFreeHandle()) == UPNP_E_OUTOF_HANDLE) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY; 
    }

    HInfo = (struct Handle_Info *) malloc (sizeof(struct Handle_Info));
    if (HInfo == NULL) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }
    HandleTable[*Hnd] = HInfo;

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Root device URL is %s\n", DescUrl);)


    HInfo->HType = HND_DEVICE;
    strcpy(HInfo->DescURL,DescUrl);
    HInfo->Callback = Fun;
    HInfo->Cookie = (void *) Cookie;
    HInfo->MaxAge = DEFAULT_MAXAGE;
    HInfo->DeviceList = NULL;
    HInfo->ServiceList = NULL;
    HInfo->DescDocument = NULL;
    CLIENTONLY(HInfo->ClientSubList=NULL;)
    HInfo->MaxSubscriptions=UPNP_INFINITE;
    HInfo->MaxSubscriptionTimeOut=UPNP_INFINITE;
    if ((retVal=UpnpDownloadXmlDoc(HInfo->DescURL, &(HInfo->DescDocument))) 
        != UPNP_E_SUCCESS)
    {
        FreeHandle(*Hnd);
        HandleUnlock();
        return retVal;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: Valid Description\n");
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: DescURL : %s\n", HInfo->DescURL);)

    HInfo->DeviceList  = UpnpDocument_getElementsByTagName(HInfo->DescDocument
                                                            , "device");
    if (HInfo->DeviceList == NULL)
    {
        FreeHandle(*Hnd);
        HandleUnlock();

        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: No devices found for RootDevice\n");)

        return UPNP_E_INVALID_DESC;
    }

    HInfo->ServiceList = UpnpDocument_getElementsByTagName(HInfo->DescDocument
                                                            , "serviceList");  
    if (HInfo->ServiceList == NULL)
    {
        FreeHandle(*Hnd);
        HandleUnlock();

        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: No services found for RootDevice\n");)

        return UPNP_E_INVALID_DESC;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: Gena Check\n");)

    //*******************************
    //GENA SET UP
    //*******************************
    if (getServiceTable(HInfo->DescDocument, &HInfo->ServiceTable,
		     HInfo->DescURL))
    {

       DBGONLY(
       UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: GENA Service Table \n");
       UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Here are the known services: \n");
       printServiceTable(&HInfo->ServiceTable,UPNP_INFO,API);)

    }
    else
    {
       DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"\nUpnpRegisterRootDevice: No Eventing Support Found \n");)
    }

    HandleUnlock();


    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Exiting RegisterRootDevice Successfully\n");)

    return UPNP_E_SUCCESS;
}  /****************** End of UpnpRegisterRootDevice  *********************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS
int UpnpUnRegisterRootDevice(IN UpnpDevice_Handle Hnd)
{
    int retVal = 0;
    struct Handle_Info *HInfo;
    struct Handle_Info *info;

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpUnRegisterRootDevice \n");)

    #if EXCLUDE_GENA == 0
    if (genaUnregisterDevice(Hnd) != UPNP_E_SUCCESS)
        return UPNP_E_INVALID_HANDLE;
    #endif

    HandleLock();
    if (GetHandleInfo(Hnd, &HInfo) == UPNP_E_INVALID_HANDLE)
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }     
    HandleUnlock();

    #if EXCLUDE_SSDP == 0
    retVal = AdvertiseAndReply(-1, Hnd, 0, (struct sockaddr_in *) NULL, 
                               (char *) NULL, (char *) NULL, (char *) NULL, 
                               HInfo->MaxAge);
    #endif

    info = (struct Handle_Info *) HandleTable[Hnd];
    UpnpNodeList_free( info->DeviceList );
    UpnpNodeList_free( info->ServiceList );
    UpnpDocument_free( info->DescDocument );

    #ifdef INTERNAL_WEB_SERVER
    if ( info->aliasInstalled )
    {
        http_RemoveAlias( info->DescAlias );
    }
    #endif // INTERNAL_WEB_SERVER

    HandleLock();
    FreeHandle(Hnd);
    HandleUnlock();

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Exiting UpnpUnRegisterRootDevice \n");
)

    return  retVal;
}  /****************** End of UpnpUnRegisterRootDevice *********************/


#endif //INCLUDE_DEVICE_APIS


// *************************************************************
#ifdef INCLUDE_DEVICE_APIS
#ifdef INTERNAL_WEB_SERVER

// determines alias for given name which is a file name or URL.
//
// return codes:
//   UPNP_E_SUCCESS
//   UPNP_E_EXT_NOT_XML
static int GetNameForAlias( IN char *name, OUT char** alias )
{
    char *ext;
    char *al;

    ext = strrchr( name, '.' );
    if ( ext == NULL || strcasecmp(ext, ".xml") != 0 )
    {
        return UPNP_E_EXT_NOT_XML;
    }

    al = strrchr( name, '/' );
    if ( al == NULL )
    {
        *alias = name;
    }
    else
    {
        *alias = al;
    }

    return UPNP_E_SUCCESS;
}

static void get_server_addr( OUT struct sockaddr_in* serverAddr )
{
    memset( serverAddr, 0, sizeof(struct sockaddr_in) );

    serverAddr->sin_family = AF_INET;
    serverAddr->sin_port = htons( LOCAL_PORT );
    //inet_aton( LOCAL_HOST, &serverAddr->sin_addr );
    serverAddr->sin_addr.s_addr = inet_addr(LOCAL_HOST);
}

// return codes:
//   UPNP_E_SUCCESS
//   UPNP_E_OUTOF_MEMORY
//   UPNP_E_URL_TOO_BIG
//   UPNP_E_INVALID_PARAM
//   UPNP_E_FILE_NOT_FOUND
//   UPNP_E_FILE_READ_ERROR
//   UPNP_E_INVALID_URL
//   UPNP_E_INVALID_DESC
//   UPNP_E_EXT_NOT_XML
//   UPNP_E_NETWORK_ERROR
//   UPNP_E_SOCKET_WRITE
//   UPNP_E_SOCKET_READ
//   UPNP_E_SOCKET_BIND
//   UPNP_E_SOCKET_CONNECT
//   UPNP_E_OUTOF_SOCKET
static int GetDescDocumentAndURL( IN Upnp_DescType descriptionType,
    IN char* description,
    IN unsigned int bufferLen,
    IN int config_baseURL,
    OUT Upnp_Document* xmlDoc,
    OUT char* descURL,
    OUT char* descAlias )
{
    int retVal;
    char *membuf = NULL;
    char *descStr = NULL;
    char *actualAliasStr = NULL;
    int descIsMalloked = 0;

    if ( description == NULL )
    {
        return UPNP_E_INVALID_PARAM;
    }

    // non-URL description must have configuration specified
    if ( descriptionType != UPNPREG_URL_DESC && (!config_baseURL) )
    {
        return UPNP_E_INVALID_PARAM;
    }

    if ( descriptionType == UPNPREG_URL_DESC )
    {
        if ((retVal=UpnpDownloadXmlDoc(description, xmlDoc))!=UPNP_E_SUCCESS)
        {
            return retVal;
        }
    }
    else if ( descriptionType == UPNPREG_FILENAME_DESC )
    {
        FILE *fp;
        unsigned fileLen;
        unsigned num_read;

        if ( (fp = fopen(description, "rb")) == NULL )
        {
            return UPNP_E_FILE_NOT_FOUND;
        }
        fseek( fp, 0, SEEK_END );
        fileLen = ftell( fp );
        rewind( fp );

        if ( (membuf = (char *)malloc(fileLen + 1)) == NULL )
        {
            fclose( fp );
            return UPNP_E_OUTOF_MEMORY;
        }

        num_read = fread( membuf, 1, fileLen, fp );
        if ( num_read != fileLen )
        {
            fclose( fp );
            free( membuf );
            return UPNP_E_FILE_READ_ERROR;
        }

        membuf[fileLen] = 0;
        fclose( fp );
        *xmlDoc = UpnpParse_Buffer( membuf );
        free( membuf );
    }
    else if ( descriptionType == UPNPREG_BUF_DESC )
    {
        *xmlDoc = UpnpParse_Buffer( description );
    }
    else
    {
        return UPNP_E_INVALID_PARAM;
    }

    if ( *xmlDoc == NULL )
    {
        return UPNP_E_INVALID_DESC;
    }

    // determine alias
    if ( config_baseURL )
    {
        char *aliasStr;
        struct sockaddr_in serverAddr;

        if ( descriptionType == UPNPREG_BUF_DESC )
        {
            aliasStr = "description.xml";
        }
        else  // URL or filename
        {
            retVal = GetNameForAlias( description, &aliasStr );
            if ( retVal != UPNP_E_SUCCESS )
            {
                UpnpDocument_free( *xmlDoc );
                return retVal;
            }
        }

        get_server_addr( &serverAddr );

        // config
        retVal = Configure_Urlbase( *xmlDoc, &serverAddr,
            aliasStr, &actualAliasStr, &descStr );
        if ( retVal != UPNP_E_SUCCESS )
        {
            UpnpDocument_free( *xmlDoc );
            return retVal;
        }
        descIsMalloked = 1;
    }
    else // manual
    {
        descStr = description;
    }

    if ( strlen(descStr) > (LINE_SIZE - 1) )
    {
        UpnpDocument_free( *xmlDoc );
        if ( descIsMalloked )
        {
            free( descStr );
            free( actualAliasStr );
        }
        return UPNP_E_URL_TOO_BIG;
    }

    strcpy( descURL, descStr );

    if ( descIsMalloked )
    {
        strcpy( descAlias, actualAliasStr );
        free( descStr );
        free( actualAliasStr );
    }

    assert( *xmlDoc != NULL );

    return UPNP_E_SUCCESS;
}

#else   // no web server

// returns:
//   UPNP_E_SUCCESS
//   UPNP_E_NO_WEB_SERVER
//   UPNP_E_INVALID_PARAM
//   UPNP_E_URL_TOO_LONG
static int GetDescDocumentAndURL( IN Upnp_DescType descriptionType,
    IN char* description,
    IN unsigned int bufferLen,
    IN int config_baseURL,
    OUT Upnp_Document* xmlDoc,
    OUT char* descURL,
    OUT char* descAlias )
{
    int retVal;

    if ( (descriptionType != UPNPREG_URL_DESC) || config_baseURL )
    {
        return UPNP_E_NO_WEB_SERVER;
    }

    if ( description == NULL )
    {
        return UPNP_E_INVALID_PARAM;
    }

    if ( strlen(description) > (LINE_SIZE - 1) )
    {
        return UPNP_E_URL_TOO_BIG;
    }
    strcpy( descURL, description );

    if ((retVal=UpnpDownloadXmlDoc(description, xmlDoc))!=UPNP_E_SUCCESS)
    {
        return retVal;
    }

    return UPNP_E_SUCCESS;
}


#endif // INTERNAL_WEB_SERVER
// ********************************************************

// return codes:
//   UPNP_E_SUCCESS
//   UPNP_E_OUTOF_MEMORY
//   UPNP_E_URL_TOO_BIG
//   UPNP_E_INVALID_PARAM
//   UPNP_E_FILE_NOT_FOUND
//   UPNP_E_FILE_READ_ERROR
//   UPNP_E_INVALID_URL
//   UPNP_E_INVALID_DESC
//   UPNP_E_EXT_NOT_XML
//   UPNP_E_NETWORK_ERROR
//   UPNP_E_SOCKET_WRITE
//   UPNP_E_SOCKET_READ
//   UPNP_E_SOCKET_BIND
//   UPNP_E_SOCKET_CONNECT
//   UPNP_E_OUTOF_SOCKET
//   UPNP_E_NO_WEB_SERVER
int UpnpRegisterRootDevice2( IN Upnp_DescType descriptionType,
        IN const char* description_const,
        IN size_t bufferLen,    // ignored unless descType == UPNPREG_BUF_DESC
        IN int config_baseURL,
        IN Upnp_FunPtr Fun,
        IN const void* Cookie,
        OUT UpnpDevice_Handle* Hnd )
{
    struct Handle_Info * HInfo;
    int retVal=0;
    char *description = (char *)description_const;

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpRegisterRootDevice2 \n");)

    if(UpnpSdkInit != 1)
    {
        return UPNP_E_FINISH;
    }
    if ( Hnd == NULL || Fun == NULL )
    {
        return UPNP_E_INVALID_PARAM;
    }

    HandleLock();
    if ((*Hnd = GetFreeHandle()) == UPNP_E_OUTOF_HANDLE)
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }

    HInfo = (struct Handle_Info *) malloc (sizeof(struct Handle_Info));
    if (HInfo == NULL)
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }
    HandleTable[*Hnd] = HInfo;

    // prevent accidental removal of a non-existent alias
    HInfo->aliasInstalled = 0;

    retVal = GetDescDocumentAndURL( descriptionType, description,
        bufferLen, config_baseURL, &HInfo->DescDocument,
        HInfo->DescURL, HInfo->DescAlias );

    if ( retVal != UPNP_E_SUCCESS )
    {
        FreeHandle(*Hnd);
        HandleUnlock();
        return retVal;
    }

    HInfo->aliasInstalled = (config_baseURL != 0);

    HInfo->HType = HND_DEVICE;
    HInfo->Callback = Fun;
    HInfo->Cookie = (void *)Cookie;
    HInfo->MaxAge = DEFAULT_MAXAGE;
    HInfo->DeviceList = NULL;
    HInfo->ServiceList = NULL;
    CLIENTONLY(HInfo->ClientSubList=NULL;)
    HInfo->MaxSubscriptions=UPNP_INFINITE;
    HInfo->MaxSubscriptionTimeOut=UPNP_INFINITE;


    DBGONLY(
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: Valid Description\n");
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: DescURL : %s\n", HInfo->DescURL);)

    HInfo->DeviceList  = UpnpDocument_getElementsByTagName(HInfo->DescDocument, "device");

    if (HInfo->DeviceList == NULL)
    {
        FreeHandle(*Hnd);
        HandleUnlock();

        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: No devices found for RootDevice\n");)

        return UPNP_E_INVALID_DESC;
    }

    HInfo->ServiceList = UpnpDocument_getElementsByTagName(HInfo->DescDocument , "serviceList");

    if (HInfo->ServiceList == NULL)
    {
        FreeHandle(*Hnd);
        HandleUnlock();

        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: No services found for RootDevice\n");)

        return UPNP_E_INVALID_DESC;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: Gena Check\n");)

    //*******************************
    //GENA SET UP
    //*******************************

    if (getServiceTable(HInfo->DescDocument, &HInfo->ServiceTable,HInfo->DescURL))
    {

       DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: GENA Service Table \n");)

    }
    else
    {
       DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"\nUpnpRegisterRootDevice2: No Eventing Support Found \n");)
    }
    HandleUnlock();


    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting RegisterRootDevice2 Successfully\n");)

    return UPNP_E_SUCCESS;
}  /****************** End of UpnpRegisterRootDevice2  *********************/


#endif //INCLUDE_DEVICE_APIS

#ifdef INCLUDE_CLIENT_APIS
int UpnpRegisterClient(IN Upnp_FunPtr Fun,
                IN const void * Cookie,
                OUT UpnpClient_Handle *Hnd)
{
    struct Handle_Info * HInfo;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpRegisterClient \n");)

    if(UpnpSdkInit != 1)
    {
        HandleUnlock();
        return UPNP_E_FINISH;
    }
    if (Fun == NULL || Hnd == NULL) 
        return UPNP_E_INVALID_PARAM;

    HandleLock();
    if ((*Hnd = GetFreeHandle()) == UPNP_E_OUTOF_HANDLE) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY; 
    }
    HInfo = (struct Handle_Info *) malloc (sizeof(struct Handle_Info));
    if (HInfo == NULL) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }

    HInfo->HType = HND_CLIENT;
    HInfo->Callback = Fun;
    HInfo->Cookie = (void *) Cookie;
    HInfo->MaxAge = 0;
    HInfo->ClientSubList=NULL;
    DEVICEONLY(HInfo->MaxSubscriptions=UPNP_INFINITE;)
    DEVICEONLY(HInfo->MaxSubscriptionTimeOut=UPNP_INFINITE;)
    
    HandleTable[*Hnd] = HInfo;

    HandleUnlock();

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpRegisterClient \n");
)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpRegisterClient   *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
int UpnpUnRegisterClient(IN UpnpClient_Handle Hnd)
{
    struct Handle_Info *HInfo;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpUnRegisterClient \n");)
    #if EXCLUDE_GENA == 0
    if (genaUnregisterClient(Hnd) != UPNP_E_SUCCESS)
        return UPNP_E_INVALID_HANDLE;
    #endif
    HandleLock();
    if (GetHandleInfo(Hnd, &HInfo) == UPNP_E_INVALID_HANDLE)
    {
       HandleUnlock();
       return UPNP_E_INVALID_HANDLE;
    }     
    FreeHandle(Hnd);
    HandleUnlock();

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpUnRegisterClient \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpUnRegisterClient *********************/
#endif // INCLUDE_CLIENT_APIS

//-----------------------------------------------------------------------------
//
//                                   SSDP interface
//
//-----------------------------------------------------------------------------

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

int UpnpSendAdvertisement(IN UpnpDevice_Handle Hnd, IN int Exp, IN int Ttl)
{
    struct Handle_Info *  SInfo=NULL; 
    int retVal = 0, *ptrMx;
    upnp_timeout *adEvent;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSendAdvertisement \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE)
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (Exp < 1)
	   Exp = DEFAULT_MAXAGE;
    SInfo->MaxAge = Exp; 
    HandleUnlock(); 
    retVal = AdvertiseAndReply(Ttl, Hnd, 0, (struct sockaddr_in *) NULL, 
                             (char *) NULL, (char *) NULL, (char *) NULL, 
                             Exp);
    
    if (retVal != UPNP_E_SUCCESS)
        return retVal;
    ptrMx = (int *) malloc (sizeof(int));
    if (ptrMx == NULL)
        return UPNP_E_OUTOF_MEMORY;
    adEvent = (upnp_timeout *) malloc (sizeof(upnp_timeout));
    
    if (adEvent == NULL)
    {   
        free(ptrMx); 
        return UPNP_E_OUTOF_MEMORY;
    }
    *ptrMx = Exp;
    adEvent->handle = Hnd;
    adEvent->ttl = Ttl;
    adEvent->Event = ptrMx;
    
    if ((retVal = ScheduleTimerEvent(Exp - AUTO_ADVERTISEMENT_TIME, 
                  AutoAdvertise, adEvent, &GLOBAL_TIMER_THREAD, 
                  &(adEvent->eventId))) !=UPNP_E_SUCCESS)
    {
        free(adEvent);
        free(ptrMx);
        return retVal;
    }   

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSendAdvertisement \n");
)

    return retVal;
}  /****************** End of UpnpSendAdvertisement *********************/
#endif // INCLUDE_DEVICE_APIS
#endif
#if EXCLUDE_SSDP == 0
#ifdef INCLUDE_CLIENT_APIS
int UpnpSearchAsync(IN UpnpClient_Handle Hnd,
    IN int Mx,
    IN const char *Target_const,
    IN const void *Cookie_const
    )
{
    struct Handle_Info *  SInfo=NULL;
    char *Target = (char *)Target_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSearchAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( Mx < 1) 
        Mx = DEFAULT_MX;

    if (Target == NULL )
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    SearchByTarget(Mx, Target, (void*) Cookie_const);
    
    HandleUnlock();

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSearchAsync \n");)

    return  UPNP_E_SUCCESS;

}  /****************** End of UpnpSearchAsync *********************/
#endif // INCLUDE_CLIENT_APIS
#endif
//-----------------------------------------------------------------------------
//
//                                   GENA interface 
//
//-----------------------------------------------------------------------------

#if EXCLUDE_GENA == 0
#ifdef INCLUDE_DEVICE_APIS
int UpnpSetMaxSubscriptions(  
    IN UpnpDevice_Handle Hnd, /** The handle of the device for which
				  the maximum subscriptions is being set. */
    IN int MaxSubscriptions   /** The maximum number of subscriptions to be
				  allowed per service. */
    )
{
  struct Handle_Info * SInfo=NULL;

  DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSetMaxSubscriptions \n");)

  HandleLock();
  if ( ( (MaxSubscriptions!=UPNP_INFINITE) && (MaxSubscriptions<0))
       || (GetHandleInfo(Hnd,&SInfo)!=HND_DEVICE))
    {
      HandleUnlock();
      return UPNP_E_INVALID_HANDLE;
    }
  SInfo->MaxSubscriptions=MaxSubscriptions;
  HandleUnlock();

  DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSetMaxSubscriptions \n");)

  return UPNP_E_SUCCESS;
}  /****************** End of UpnpSetMaxSubscriptions *********************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS
int UpnpSetMaxSubscriptionTimeOut(  
    IN UpnpDevice_Handle Hnd,       /** The handle of the device for which
				     the maximum subscription time-out is being set. */
    IN int MaxSubscriptionTimeOut   /** The maximum subscription time-out to be
				     accepted. */
    )
{
   struct Handle_Info * SInfo=NULL;

   DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSetMaxSubscriptionTimeOut \n");)

   HandleLock();

   if ( ( ( MaxSubscriptionTimeOut!=UPNP_INFINITE) && (MaxSubscriptionTimeOut<0)) 
            || (GetHandleInfo(Hnd,&SInfo)!=HND_DEVICE))
   {
      HandleUnlock();
      return UPNP_E_INVALID_HANDLE;
   }

   SInfo->MaxSubscriptionTimeOut=MaxSubscriptionTimeOut;
   HandleUnlock();

   DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSetMaxSubscriptionTimeOut \n");)

   return UPNP_E_SUCCESS;
  
}  /****************** End of UpnpSetMaxSubscriptionTimeOut ******************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_CLIENT_APIS
int UpnpSubscribeAsync(IN UpnpClient_Handle Hnd,
    IN const char * EvtUrl_const,
    IN int TimeOut,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const
    )
{
    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam  * Param;
    char *EvtUrl = (char *)EvtUrl_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSubscribeAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (EvtUrl == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_URL;
    }
    if (TimeOut!=UPNP_INFINITE && TimeOut < 1)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if (Fun == NULL) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }
    HandleUnlock();

    Param->FunName = SUBSCRIBE;
    Param->Handle = Hnd;
    strcpy(Param->Url,EvtUrl);    
    Param->TimeOut = TimeOut;
    Param->Fun = Fun;
    Param->Cookie = (void*) Cookie_const;

    tpool_Schedule( (void *)UpnpThreadDistribution , Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSubscribeAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpSubscribeAsync *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS

int  UpnpSubscribe (IN UpnpClient_Handle Hnd,
    IN const char * EvtUrl_const,
    INOUT int *TimeOut,
    OUT Upnp_SID SubsId)
{
    struct Handle_Info * SInfo=NULL; 
    int RetVal;
    char *EvtUrl = (char *)EvtUrl_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSubscribe \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (EvtUrl == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_URL;
    }
    if (TimeOut == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    HandleUnlock();
    RetVal = genaSubscribe(Hnd, EvtUrl, TimeOut, SubsId);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSubscribe \n");)

    return RetVal;

}  /****************** End of UpnpSubscribe  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
int  UpnpUnSubscribe (IN UpnpClient_Handle Hnd, IN Upnp_SID SubsId)
{
    struct Handle_Info * SInfo=NULL; 
    int RetVal;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpUnSubscribe \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    HandleUnlock();
    RetVal = genaUnSubscribe(Hnd, SubsId);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpUnSubscribe \n");)

    return RetVal;

}  /****************** End of UpnpUnSubscribe  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
int UpnpUnSubscribeAsync(IN UpnpClient_Handle Hnd,
    IN Upnp_SID SubsId,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const
    )
{

    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam  * Param;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpUnSubscribeAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    if (Fun == NULL) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    HandleUnlock();
    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
        return UPNP_E_OUTOF_MEMORY;

    Param->FunName = UNSUBSCRIBE;
    Param->Handle = Hnd;
    strcpy(Param->SubsId,SubsId);  
    Param->Fun = Fun;
    Param->Cookie = (void*) Cookie_const;

    tpool_Schedule((void *) UpnpThreadDistribution, Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpUnSubscribeAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpUnSubscribeAsync  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
int  UpnpRenewSubscription (IN UpnpClient_Handle Hnd, INOUT int *TimeOut, 
                            IN Upnp_SID SubsId)
{
    struct Handle_Info * SInfo=NULL; 
    int RetVal;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpRenewSubscription \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (TimeOut == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    HandleUnlock();
    RetVal = genaRenewSubscription(Hnd, SubsId, TimeOut);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpRenewSubscription \n");)

    return RetVal;

}  /****************** End of UpnpRenewSubscription  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
int UpnpRenewSubscriptionAsync(IN UpnpClient_Handle Hnd,
    INOUT int TimeOut,
    IN Upnp_SID SubsId,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const )
{

    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam  * Param;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpRenewSubscriptionAsync \n");)

    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (TimeOut!=UPNP_INFINITE && TimeOut < 1)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    if (Fun == NULL) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    HandleUnlock();

    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
        return UPNP_E_OUTOF_MEMORY;

    Param->FunName = RENEW;
    Param->Handle = Hnd;
    strcpy(Param->SubsId,SubsId);  
    Param->Fun = Fun;
    Param->Cookie = (void*) Cookie_const;
    Param->TimeOut = TimeOut;

    tpool_Schedule((void *) UpnpThreadDistribution, Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpRenewSubscriptionAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpRenewSubscriptionAsync *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_DEVICE_APIS
int UpnpNotify(IN UpnpDevice_Handle Hnd ,
    IN const char *DevID_const,
    IN const char *ServName_const,
    IN const char ** VarName_const,
    IN const char ** NewVal_const,
    IN int cVariables)
{

    struct Handle_Info *SInfo=NULL; 
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;
    char **VarName = (char **)VarName_const;
    char **NewVal = (char **)NewVal_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpNotify \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (DevID == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (ServName == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (VarName == NULL || NewVal == NULL || cVariables < 0)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    
    HandleUnlock();
    retVal = genaNotifyAll (Hnd,DevID,ServName, VarName, NewVal, cVariables);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpNotify \n");)

    return retVal;

}  /****************** End of UpnpNotify *********************/


int UpnpNotifyExt(IN UpnpDevice_Handle Hnd,
    IN const char *DevID_const,
    IN const char *ServName_const,
    IN Upnp_Document PropSet)
{

    struct Handle_Info *SInfo=NULL; 
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpNotify \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (DevID == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (ServName == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    
    HandleUnlock();
    retVal = genaNotifyAllExt(Hnd,DevID,ServName,PropSet);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpNotify \n");)

    return retVal;

}  /****************** End of UpnpNotify *********************/

#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS
int UpnpAcceptSubscription(IN UpnpDevice_Handle Hnd ,
    IN const char *DevID_const,
    IN const char *ServName_const,
    IN const char ** VarName_const,
    IN const char ** NewVal_const,
    int cVariables,
    IN Upnp_SID SubsId)
{
    struct Handle_Info *SInfo=NULL; 
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;
    char **VarName = (char **)VarName_const;
    char **NewVal = (char **)NewVal_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpAcceptSubscription \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (DevID == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (ServName == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    if (VarName == NULL || NewVal == NULL || cVariables < 0)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    
    HandleUnlock();
    retVal = genaInitNotify(Hnd,DevID,ServName, VarName, NewVal, cVariables, 
                            SubsId);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpAcceptSubscription \n");)

    return retVal;

}  /****************** End of UpnpAcceptSubscription *********************/


int UpnpAcceptSubscriptionExt(IN UpnpDevice_Handle Hnd ,
    IN const char *DevID_const,
    IN const char *ServName_const,
    IN Upnp_Document PropSet,
    IN Upnp_SID SubsId)
{
    struct Handle_Info *SInfo=NULL; 
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpAcceptSubscription \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (DevID == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (ServName == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }

    if (PropSet == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    
    HandleUnlock();
    retVal = genaInitNotifyExt(Hnd,DevID,ServName,PropSet,SubsId);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpAcceptSubscription \n");)

    return retVal;

}  /****************** End of UpnpAcceptSubscription *********************/

#endif // INCLUDE_DEVICE_APIS
#endif // EXCLUDE_GENA == 0


//-----------------------------------------------------------------------------
//
//                                   SOAP interface 
//
//-----------------------------------------------------------------------------
#if EXCLUDE_SOAP == 0
#ifdef INCLUDE_CLIENT_APIS
int UpnpSendAction( IN UpnpClient_Handle Hnd,
    IN const char *ActionURL_const,
    IN const char *ServiceType_const,
    IN const char *DevUDN_const,
    IN Upnp_Document Action,
    OUT Upnp_Document *RespNodePtr)
{
    struct Handle_Info * SInfo=NULL; 
    int retVal = 0;
    char *ActionURL = (char *)ActionURL_const;
    char *ServiceType = (char *)ServiceType_const;
    //char *DevUDN = (char *)DevUDN_const;  // udn not used?

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSendAction \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if (ActionURL == NULL)
    {
        return UPNP_E_INVALID_URL;
    }    
    if (ServiceType == NULL || Action == NULL || RespNodePtr == NULL)
    {
        return UPNP_E_INVALID_PARAM;
    }    

    retVal = SoapSendAction(ActionURL, ServiceType, Action, 
                            RespNodePtr);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSendAction \n");)

    return retVal;
}  /****************** End of UpnpSendAction *********************/
 // INCLUDE_CLIENT_APIS

int UpnpSendActionAsync(IN UpnpClient_Handle Hnd,
    IN const char *ActionURL_const,
    IN const char *ServiceType_const,
    IN const char * DevUDN_const,
    IN Upnp_Document Act,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const)
{
    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam  * Param;
    Upnp_DOMString tmpStr;
    char *ActionURL = (char *)ActionURL_const;
    char *ServiceType = (char *)ServiceType_const;
    //char *DevUDN = (char *)DevUDN_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSendActionAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if (ActionURL == NULL)
    {
        return UPNP_E_INVALID_URL;
    }    
    if (ServiceType == NULL || Act == NULL || Fun == NULL)
    {
        return UPNP_E_INVALID_PARAM;
    }    
    tmpStr = UpnpNewPrintDocument(Act);
    if (tmpStr == NULL)
    {
        return UPNP_E_INVALID_ACTION;
    }
    
    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
        return UPNP_E_OUTOF_MEMORY;

    Param->FunName = ACTION;
    Param->Handle = Hnd;
    strcpy(Param->Url,ActionURL); 
    strcpy(Param->ServiceType,ServiceType);

    Param->Act = UpnpParse_Buffer(tmpStr);
    free(tmpStr);
    Param->Cookie = (void*) Cookie_const;
    Param->Fun = Fun;

    tpool_Schedule((void *) UpnpThreadDistribution, Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSendActionAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpSendActionAsync *********************/


int UpnpGetServiceVarStatusAsync(IN UpnpClient_Handle Hnd,
    IN const char *ActionURL_const,
    IN const char *VarName_const,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const
    )
{
    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam * Param;
    char *ActionURL = (char *)ActionURL_const;
    char *VarName = (char *)VarName_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpGetServiceVarStatusAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT)
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if (ActionURL == NULL)
    {
        return UPNP_E_INVALID_URL;
    }    
    if (VarName == NULL || Fun == NULL ) 
        return UPNP_E_INVALID_PARAM;

    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
        return UPNP_E_OUTOF_MEMORY;

    Param->FunName = STATUS;
    Param->Handle = Hnd;
    strcpy(Param->Url,ActionURL);     
    strcpy(Param->VarName,VarName);         
    Param->Fun = Fun;
    Param->Cookie = (void*) Cookie_const;

    tpool_Schedule((void *) UpnpThreadDistribution, Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpGetServiceVarStatusAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpGetServiceVarStatusAsync ****************/

int UpnpGetServiceVarStatus(IN UpnpClient_Handle Hnd,
    IN const char *ActionURL_const,
    IN const char *VarName_const,
    OUT Upnp_DOMString* StVar)
{
    struct Handle_Info * SInfo=NULL; 
    int retVal = 0;
    char *StVarPtr;
    char *ActionURL = (char *)ActionURL_const;
    char *VarName = (char *)VarName_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpGetServiceVarStatus \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if (ActionURL == NULL)
    {
        return UPNP_E_INVALID_URL;
    }    
    if (VarName == NULL || StVar == NULL ) 
        return UPNP_E_INVALID_PARAM;

    retVal = SoapGetServiceVarStatus(ActionURL, VarName, &StVarPtr); 
    *StVar = StVarPtr;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpGetServiceVarStatus \n");)

    return retVal;
}  /****************** End of UpnpGetServiceVarStatus *********************/
#endif // INCLUDE_CLIENT_APIS
#endif // EXCLUDE_SOAP

//-----------------------------------------------------------------------------
//
//                                   Client API's 
//
//-----------------------------------------------------------------------------

int UpnpDownloadUrlItem( const char *url_const,
    char **outBuf, char *contentType)
{
    char *tmpBuf;
    int retVal = 0, bufLen = 0;
    http_message msgBuf;
    token ctBuf;
    char *url = (char *)url_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpDownloadUrlItem \n");)

    if (url == NULL || outBuf == NULL || contentType == NULL)
        return UPNP_E_INVALID_PARAM;

    if (contentType !=  NULL)
        strcpy(contentType,"");
    
    if ((retVal = transferHTTP("GET", "\r\n", 2,&tmpBuf, url)) != HTTP_SUCCESS)
    {
        return retVal;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: tmpBuf is = %s\n",tmpBuf);
            UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"************************END OF OUTBUF**************************\n");)
    bufLen = strlen(tmpBuf);

    if ((retVal = parse_http_response(tmpBuf, &msgBuf, bufLen)) != HTTP_SUCCESS)
    {
        free(tmpBuf);
        return retVal;
    }

    if (msgBuf.content.size == 0)
    {

    DBGONLY(
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: Content Size is ZERO\n");
    //UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: Printing msgBuf.content.buff |%s| and the ptr is |%x|\n", msgBuf.content.buff, msgBuf.content.buff);
    msgBuf.content.buff = NULL;
    //UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: Printing msgBuf.content.buff |%s| and the ptr is |%x|\n", msgBuf.content.buff, msgBuf.content.buff);
    )

        free(tmpBuf);
        return UPNP_E_INVALID_URL;
    }
    if (strncasecmp(msgBuf.status.status_code.buff, "200", strlen("200")))
    {
        free(tmpBuf);

        DBGONLY(
        UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: HTTP ERROR IN GETTING THE DOC\n");
        print_token(&msgBuf.status.status_code,UPNP_CRITICAL,API,__FILE__,__LINE__);)

        return UPNP_E_INVALID_URL;
    }
    (*outBuf) = (char *) malloc (msgBuf.content.size + 1);
    if (*outBuf == NULL)
    {
        return UPNP_E_OUTOF_MEMORY;
    }
    strcpy(*outBuf, msgBuf.content.buff);
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: copied the buffer\n");)
    if (search_for_header(&msgBuf,"Content-Type",&ctBuf))
        strncpy(contentType, ctBuf.buff, LINE_SIZE - 1 > ctBuf.size ? 
                ctBuf.size : LINE_SIZE - 1);

    DBGONLY(
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: OutBuf is = %s\n",*outBuf);
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"************************END OF OUTBUF**************************\n");)

    free_http_message(&msgBuf); // free the headers inside msgBuf
    free(tmpBuf);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpDownloadUrlItem \n");)

    return UPNP_E_SUCCESS;
}  /****************** End of UpnpDownloadUrlItem *********************/

int UpnpDownloadXmlDoc(const char *url_const,
    Upnp_Document *xmlDoc)
{
    char *xmlBuf = NULL;
    char contentType[LINE_SIZE] = "";
    int retVal = 0;
    char *url = (char *)url_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpDownloadXmlDoc \n");)

    if (url == NULL || xmlDoc == NULL)
    {
        return UPNP_E_INVALID_PARAM;
    }
    if ((retVal = UpnpDownloadUrlItem(url, &xmlBuf, contentType)) != 
         UPNP_E_SUCCESS)
    {
        return retVal;
    }

    if (strncasecmp(contentType, "text/xml",strlen("text/xml")))
    {
        free(xmlBuf);
        return UPNP_E_INVALID_DESC;
    }
    
    DBGONLY(
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Got the Xml file . \n  %s ", xmlBuf);
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"********************* END OF XML FILE*******************\n");
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Going to parse it\n");)

    *xmlDoc = UpnpParse_Buffer(xmlBuf);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Going to free xmlBuf \n");)

    free(xmlBuf);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"xmlBuf FREED\n");)

    if (*xmlDoc == NULL)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"Invalid desc\n");)
        return UPNP_E_INVALID_DESC;
    }
    else
    {
        DBGONLY(
        xmlBuf = UpnpNewPrintDocument(*xmlDoc);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Printing the Parsed xml document \n %s\n", xmlBuf);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"********************* END OF Parsed XML Doc*******************\n");
        free(xmlBuf);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpDownloadXmlDoc\n");)
        return UPNP_E_SUCCESS;
    }
}  /****************** End of UpnpDownloadXmlDoc *********************/
                 
//-----------------------------------------------------------------------------
//
//                          UPNP-API  Internal function implementation
//
//-----------------------------------------------------------------------------

//********************************************************
//* Name: AdvertiseAndReply
//* Description:  Function to send SSDP advertisements, replies and
//*               shutdown messages.
//* Called by:    UpnpSendAdvertisement, SsdpCallbackHandler, 
//*               UpnpUnRegisterRootDevice.
//* In:           int AdFlag :
//*                           AdFlag = -1 : Send Shutdown 
//*                           AdFlag = 0 : Send Reply
//*                           AdFlag >= 1 : Send Advertisement and AdFlag stands for TTL
//*               UpnpDevice_Handle Hnd : Device handle
//*               enum SsdpSearchType SearchType : Search type for sending
//*                                                replies.
//*               struct sockaddr_in *DestAddr : Destination address
//*               char *DeviceType : Device type
//*               char *DeviceUDN : Device UDN
//*               char *ServiceType : Service type
//*               int Exp : Advertisement age
//* Out:          none
//* Return Codes: UPNP_E_SUCCESS
//* Error Codes:  UPNP_E_INVALID_HANDLE
//*
//********************************************************
#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

int AdvertiseAndReply(int AdFlag, UpnpDevice_Handle Hnd, 
                  enum SsdpSearchType SearchType, struct sockaddr_in *DestAddr,
                      char *DeviceType, char *DeviceUDN, 
                      char *ServiceType, int Exp)
{
    int i, j, cmp;
	int currServiceIndex=0;
    int defaultExp = DEFAULT_MAXAGE;
    struct Handle_Info *  SInfo=NULL; 
    char  UDNstr[100], devType[100], servType[100];
    Upnp_NodeList NodeList = NULL; 
    Upnp_NodeList tmpNodeList = NULL; 
    Upnp_Node tmpNode = NULL;
    Upnp_Node tmpNode2 = NULL;
    Upnp_Node textNode = NULL;
    Upnp_Node parentNode = NULL;
    Upnp_DOMException err; 
    Upnp_DOMString tmpStr;

    DBGONLY(
    Upnp_DOMString dbgStr;
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside AdvertiseAndReply with AdFlag = %d\n", AdFlag);)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    defaultExp = SInfo->MaxAge;

    // Modified to prevent more than one thread accessing the same UpnpDocument
    //   HandleUnlock();

    // parse the device list and send advertisements/replies 
    for (i = 0; ; i++)
    {

        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Entering new device list with i = %d\n\n", i);)

        UpnpNode_free(tmpNode);
        tmpNode = UpnpNodeList_item(SInfo->DeviceList, i);
        if (tmpNode == NULL)
        {
            DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting new device list with i = %d\n\n", i);)
            break;
        }

        DBGONLY(
        dbgStr = UpnpNode_getNodeName(tmpNode);
        UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Extracting device type once for %s\n", dbgStr);
        free(dbgStr);)
        // extract device type 

        NodeList = UpnpElement_getElementsByTagName(tmpNode, "deviceType"); 
        if (NodeList == NULL)
        {
            continue;
        }

        DBGONLY(
        dbgStr = UpnpNode_getNodeName(tmpNode);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting UDN for %s\n", dbgStr);
        free(dbgStr);)

        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting device type\n");)

        tmpNode2 = UpnpNodeList_item(NodeList, 0);
        UpnpNodeList_free(NodeList);
        if (tmpNode2 == NULL)
        {
            continue;
        }
        textNode = UpnpNode_getFirstChild(tmpNode2);
        UpnpNode_free(tmpNode2);
        if (textNode == NULL)
        {
            continue;
        }

        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting device type \n");)

        tmpStr = UpnpNode_getNodeValue(textNode, &err);
        UpnpNode_free(textNode);
        if (tmpStr == NULL) 
        {
            continue;
        }
        strcpy(devType , tmpStr);
        free(tmpStr);
        if (devType == NULL)
        {
            continue;
        }

        DBGONLY(
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting device type = %s\n", devType);
        if (tmpNode == NULL)
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"TempNode is NULL\n");
        dbgStr =  UpnpNode_getNodeName(tmpNode);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting UDN for %s\n",dbgStr);
        free(dbgStr);)

        // extract UDN 

        NodeList = UpnpElement_getElementsByTagName(tmpNode, "UDN"); 
        if (NodeList == NULL)
        {

            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UDN not found!!!\n");)
            continue;
        }
        tmpNode2 = UpnpNodeList_item(NodeList, 0);
        UpnpNodeList_free(NodeList);
        if (tmpNode2 == NULL)
        {

            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UDN not found!!!\n");)
            continue;
        }
        textNode = UpnpNode_getFirstChild(tmpNode2);
        UpnpNode_free(tmpNode2);
        if (textNode == NULL)
        {

            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UDN not found!!!\n");)
            continue;
        }
        tmpStr = UpnpNode_getNodeValue(textNode, &err);
        UpnpNode_free(textNode);
        if (tmpStr == NULL)
        {
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UDN not found!!!!\n");)
            continue;
        }
        strcpy(UDNstr, tmpStr);
        free(tmpStr);
        if (UDNstr == NULL)
        {
            continue;
        }

        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Sending UDNStr = %s \n", UDNstr);)

        if (AdFlag)
        {
            if (AdFlag == -1)
            {
                DeviceShutdown(devType, i==0 , UDNstr, SERVER, SInfo->DescURL, Exp); 
            }
            else // send the device advertisement 
            {
                DeviceAdvertisement(devType, i==0 , UDNstr, SERVER,SInfo->DescURL, Exp, AdFlag); 
            }
        }
        else
        {
            switch (SearchType)
            {
                case SSDP_ALL :
		      syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d \n","AdvertiseAndReply:SSDP_ALL",__LINE__);
                    DeviceReply(DestAddr, devType, i==0, UDNstr, SERVER,SInfo->DescURL, defaultExp);
                    break;

                case SSDP_ROOTDEVICE :

                    if (i==0)
                    {
                        SendReply(DestAddr, devType, 1, UDNstr, SERVER,SInfo->DescURL, defaultExp, 0);
                    }
                    break;
                case SSDP_DEVICE :
                    if (DeviceUDN != NULL && strlen(DeviceUDN) != 0) 
                    {
                        if (strcasecmp(DeviceUDN, UDNstr))
                        {
                            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"DeviceUDN=%s and search UDN=%s did not match\n",UDNstr, DeviceUDN);)
                            break;
                        }
                        else
                        {

                            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"DeviceUDN=%s and search UDN=%s MATCH\n",UDNstr, DeviceUDN);)
                            SendReply(DestAddr, devType, 0, UDNstr,SERVER, SInfo->DescURL, defaultExp, 0);
                            break;
                        }
  
                    }
                    if (!strcasecmp(DeviceType, devType))
                    {

                        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"DeviceType=%s and search devType=%s MATCH\n",devType,DeviceType);)
                        SendReply(DestAddr, devType, 0, UDNstr,SERVER, SInfo->DescURL, defaultExp, 1);
                    }

                    DBGONLY(else UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"DeviceType=%s and search devType=%s DID NOT MATCH\n",devType,DeviceType);)

                    break;
                default : break;
            }
        }

        // send service advertisements for services corresponding to the same device 
        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Sending service Advertisement\n");)

        UpnpNode_free(tmpNode);
        //tmpNode = UpnpNodeList_item(SInfo->ServiceList, i);
        tmpNode = UpnpNodeList_item(SInfo->ServiceList, currServiceIndex);
        if (tmpNode == NULL)
            continue;

		//
		// verify this service belongs to the Device being advertised above
		//
		parentNode = UpnpNode_getParentNode(tmpNode);
		if (parentNode == NULL)
		{
            continue;
		}
        NodeList = UpnpElement_getElementsByTagName(parentNode, "UDN"); 
        if (NodeList == NULL)
        {
            continue;
        }
        tmpNode2 = UpnpNodeList_item(NodeList, 0);
        UpnpNodeList_free(NodeList);
        if (tmpNode2 == NULL)
        {
            continue;
        }
        textNode = UpnpNode_getFirstChild(tmpNode2);
        UpnpNode_free(tmpNode2);
        if (textNode == NULL)
        {
            continue;
        }
        tmpStr = UpnpNode_getNodeValue(textNode, &err);
        UpnpNode_free(textNode);
        if (tmpStr == NULL)
        {
            continue;
        }
		printf("checking service node: parent UDN is: %s, device UDN is: %s\n",
				tmpStr, UDNstr);

		cmp=strcmp(tmpStr, UDNstr);
       	free(tmpStr);

		if (cmp!=0) 
			continue;

		printf("strings match.... service is part of this device\n");
		currServiceIndex++;

        NodeList = UpnpElement_getElementsByTagName(tmpNode, "service"); 
        if (NodeList == NULL)
        {
            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Service not found 3\n");)
            continue;
        }
        for (j = 0; ; j++)
        {
            UpnpNode_free(tmpNode);
            tmpNode = UpnpNodeList_item(NodeList, j);
            if (tmpNode == NULL)
                break;

            tmpNodeList = UpnpElement_getElementsByTagName(tmpNode,"serviceType");

            if (tmpNodeList == NULL)
            {
                DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"ServiceType not found \n");)
                continue;
            }
            tmpNode2 = UpnpNodeList_item(tmpNodeList, 0);
            UpnpNodeList_free(tmpNodeList);
            if (tmpNode2 == NULL)
            {
                continue;
            }
            textNode = UpnpNode_getFirstChild(tmpNode2);
            UpnpNode_free(tmpNode2);
            if (textNode == NULL)
            {
                continue;
            }

            // servType is of format Servicetype:ServiceVersion

            tmpStr = UpnpNode_getNodeValue(textNode, &err);
            UpnpNode_free(textNode);
            if (tmpStr == NULL)
            {
                continue;
            }
            strcpy(servType, tmpStr); 
            free(tmpStr);
            if (servType == NULL)
            {
                continue;
            }

            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"ServiceType = %s\n", servType);)

            if (AdFlag)
            {
                if (AdFlag == -1)
                {
                    ServiceShutdown(UDNstr, servType, SERVER,SInfo->DescURL, Exp);
                }
                else 
                {
                    ServiceAdvertisement(UDNstr, servType,SERVER, SInfo->DescURL, Exp, AdFlag);
                }
                
            }
            else
            {
                switch(SearchType)
                {
                    case SSDP_ALL :

                        ServiceReply(DestAddr, servType, UDNstr,SERVER,  SInfo->DescURL, defaultExp);
                        break;

                    case SSDP_SERVICE :

                        if (ServiceType != NULL)
                        {
                            if (!strcasecmp(ServiceType, servType))
                            {
                                ServiceReply(DestAddr, servType,UDNstr, SERVER, SInfo->DescURL, defaultExp);
                            }
                        }
                        break;
                    default : break;
                } // switch(SearchType)               

            }
        }
    }
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting AdvertiseAndReply : \n");)

      HandleUnlock();// Modified to prevent more than one 
                     //thread accessing the same UpnpDocument

    return  UPNP_E_SUCCESS; 
 
}  /****************** End of AdvertiseAndReply *********************/

#endif
#endif
#ifdef INCLUDE_CLIENT_APIS

//********************************************************
//* Name: UpnpThreadDistribution
//* Description:  Function to schedule async functions in threadpool.
//* Called by:    threadpool functions
//* In:           struct UpnpNonblockParam * Param
//* Out:          none
//* Return Codes: none
//* Error Codes:  none
//********************************************************

void UpnpThreadDistribution(struct UpnpNonblockParam * Param)
{


    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpThreadDistribution \n");)

    switch(Param->FunName)
    {
    #if EXCLUDE_GENA == 0
    CLIENTONLY(  
    case SUBSCRIBE: 
        {
            struct Upnp_Event_Subscribe Evt;
            Evt.ErrCode = genaSubscribe (Param->Handle, Param->Url, 
                                (int *)&(Param->TimeOut), (char *) Evt.Sid);

            strcpy(Evt.PublisherUrl, Param->Url);
            Evt.TimeOut = Param->TimeOut;
            Param->Fun(UPNP_EVENT_SUBSCRIBE_COMPLETE,&Evt,Param->Cookie);
            free(Param);
            break;
        }
    case UNSUBSCRIBE:
        {
            struct Upnp_Event_Subscribe Evt;
            Evt.ErrCode = genaUnSubscribe (Param->Handle, Param->SubsId);
            strcpy((char *) Evt.Sid, Param->SubsId);
            Param->Fun(UPNP_EVENT_UNSUBSCRIBE_COMPLETE,&Evt,Param->Cookie);
            free(Param);
            break;

        }

    case RENEW      :
        {
            struct Upnp_Event_Subscribe Evt;
            Evt.ErrCode = genaRenewSubscription (Param->Handle, Param->SubsId,
                                                 &(Param->TimeOut));

            Evt.TimeOut = Param->TimeOut;
            strcpy((char *) Evt.Sid, Param->SubsId);
            Param->Fun(UPNP_EVENT_RENEWAL_COMPLETE,&Evt,Param->Cookie);
            free(Param);
            break;
        })
    #endif
    #if EXCLUDE_SOAP == 0
    case ACTION:    
        {
            struct Upnp_Action_Complete Evt;

            Evt.ActionResult = NULL;
            #ifdef INCLUDE_CLIENT_APIS

            Evt.ErrCode = SoapSendAction (Param->Url,Param->ServiceType, 
                           Param->Act, &Evt.ActionResult);
            #endif

            Evt.ActionRequest = Param->Act;
            strcpy(Evt.CtrlUrl, Param->Url);

            Param->Fun(UPNP_CONTROL_ACTION_COMPLETE,&Evt,Param->Cookie);

            UpnpDocument_free(Evt.ActionRequest);
            UpnpDocument_free(Evt.ActionResult);
            free(Param);
            break;
        }
    case STATUS:    
        {
            struct Upnp_State_Var_Complete Evt;
            #ifdef INCLUDE_CLIENT_APIS

            Evt.ErrCode = SoapGetServiceVarStatus (Param->Url, Param->VarName,
                                                   &(Evt.CurrentVal));
            #endif
            strcpy(Evt.StateVarName, Param->VarName);
            strcpy(Evt.CtrlUrl, Param->Url);
             
            Param->Fun(UPNP_CONTROL_GET_VAR_COMPLETE,&Evt,Param->Cookie);
            free(Evt.CurrentVal);
            free(Param);
            break;
        }
    #endif //EXCLUDE_SOAP
    default : break;
    } // end of switch(Param->FunName)


    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpThreadDistribution \n");)

}  /****************** End of UpnpThreadDistribution  *********************/
#endif

//********************************************************
//* Name: GetCallBackFn
//* Description:  Function to get callback function ptr from a handle
//* Called by:    
//* In:           UpnpClient_Handle Hnd
//* Out:          callback function ptr
//********************************************************

Upnp_FunPtr GetCallBackFn(UpnpClient_Handle Hnd)
{
    return ((struct Handle_Info*)HandleTable[Hnd])->Callback;
}  /****************** End of GetCallBackFn *********************/

//********************************************************
//* Name: InitHandleList
//* Description:  Function to initialize handle table
//* Called by:    UpnpInit
//* In:           none
//* Out:          none
//********************************************************

void InitHandleList()
{
    int i;

    for(i=0;i<NUM_HANDLE;i++)
        HandleTable[i] = NULL;
}  /****************** End of InitHandleList *********************/

//********************************************************
//* Name: GetFreeHandle
//* Description:  Function to get a free handle
//* Called by:    UpnpRegisterRootDevice, UpnpRegisterClient
//* In:           none
//* Out:          handle
//* Error code:   UPNP_E_OUTOF_HANDLE
//********************************************************

int GetFreeHandle()
{
    int i=1;

/* Handle 0 is not used as NULL translates to 0 when passed as a handle */
    while(i < NUM_HANDLE)
    {
        if(HandleTable[i++] == NULL) break;
    }
  
    if ( i == NUM_HANDLE) 
        return UPNP_E_OUTOF_HANDLE; //Error
    else 
        return --i; 

}  /****************** End of GetFreeHandle *********************/

//********************************************************
//* Name: GetClientHandleInfo
//* Description:  Function to get client handle info
//* Called by:    
//* In:           UpnpClient_Handle *client_handle_out
//*               struct Handle_Info **HndInfo
//* Out:          client handle and its info
//* Return codes: HND_CLIENT
//* Error codes:  HND_INVALID 
//********************************************************

//Assumes at most one client
Upnp_Handle_Type GetClientHandleInfo(UpnpClient_Handle * client_handle_out, struct Handle_Info **HndInfo)
{
  (*client_handle_out)=1;
  if (GetHandleInfo(1,HndInfo)==HND_CLIENT)
    return HND_CLIENT;
  (*client_handle_out)=2;
  if (GetHandleInfo(2,HndInfo)==HND_CLIENT)
    return HND_CLIENT;
  (*client_handle_out)=-1;
  return HND_INVALID;
}  /****************** End of GetClientHandleInfo *********************/

//********************************************************
//* Name: GetDeviceHandleInfo
//* Description:  Function to get device handle info
//* Called by:
//* In:           UpnpClient_Handle *device_handle_out
//*               struct Handle_Info **HndInfo
//* Out:          device handle and its info
//* Return codes: HND_DEVICE
//* Error codes:  HND_INVALID
//********************************************************   
//Assumes at most one device

Upnp_Handle_Type GetDeviceHandleInfo(UpnpDevice_Handle * device_handle_out, struct Handle_Info **HndInfo)
{
  (*device_handle_out)=1;
  if (GetHandleInfo(1,HndInfo)==HND_DEVICE)
    return HND_DEVICE;
  
  (*device_handle_out)=2;
  if (GetHandleInfo(2,HndInfo)==HND_DEVICE)
    return HND_DEVICE;  
  (*device_handle_out)=-1;

  return HND_INVALID;
}  /****************** End of GetDeviceHandleInfo *********************/


//********************************************************
//* Name: GetHandleInfo
//* Description:  Function to get handle info
//* Called by:
//* In:           UpnpClient_Handle Hnd
//*               struct Handle_Info **HndInfo
//* Out:          handle info
//* Return codes: Handle type
//* Error codes:  UPNP_E_INVALID_HANDLE
//********************************************************  

Upnp_Handle_Type GetHandleInfo(UpnpClient_Handle Hnd, 
                               struct Handle_Info **HndInfo)
{

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"GetHandleInfo: Handle is %d\n", Hnd);)

    if (Hnd < 1 || Hnd >= NUM_HANDLE)
    {

        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"GetHandleInfo : Handle out of range\n");)
        return UPNP_E_INVALID_HANDLE;
    }
    if (HandleTable[Hnd] != NULL) 
    {

        *HndInfo = (struct Handle_Info *) HandleTable[Hnd];
        return ((struct Handle_Info *) *HndInfo)->HType;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"GetHandleInfo : exiting\n");)

    return UPNP_E_INVALID_HANDLE;
}  /****************** End of GetHandleInfo *********************/


int PrintHandleInfo(UpnpClient_Handle Hnd)
{
    struct Handle_Info *HndInfo;
    if (HandleTable[Hnd] != NULL)
    {
        HndInfo = HandleTable[Hnd];
        DBGONLY(
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Printing information for Handle_%d\n", Hnd);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"HType_%d\n", HndInfo->HType);
        if (HndInfo->HType!=HND_CLIENT)UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"DescURL_%s\n", HndInfo->DescURL);)
    }
    else  return UPNP_E_INVALID_HANDLE;

    return UPNP_E_SUCCESS;
}  /****************** End of PrintHandleInfo *********************/

//********************************************************
//* Name: FreeHandle
//* Description:  Function to free handle info
//* Called by:    UpnpUnRegister functions
//* In:           int Upnp_Handle
//* Out:          none
//* Return codes: UPNP_E_SUCCESS
//* Error codes:  UPNP_E_INVALID_HANDLE
//********************************************************

int FreeHandle(int Upnp_Handle)
{
    if (Upnp_Handle < 1 || Upnp_Handle >= NUM_HANDLE)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"FreeHandleInfo : Handle out of range\n");)
        return UPNP_E_INVALID_HANDLE;
    }

    if (HandleTable[Upnp_Handle] == NULL)
        return UPNP_E_INVALID_HANDLE;

    free( HandleTable[Upnp_Handle] );
    HandleTable[Upnp_Handle] = NULL;
    return  UPNP_E_SUCCESS;
}  /****************** End of FreeHandle *********************/


void printNodes(Upnp_Node tmpRoot, int depth)
{
    int i;
    Upnp_NodeList NodeList1;
    Upnp_Node ChildNode1;
    unsigned short NodeType;
    Upnp_DOMString NodeValue;
    Upnp_DOMString NodeName;
    Upnp_DOMException err; 

    NodeList1= UpnpNode_getChildNodes(tmpRoot);
    for (i=0; i< 100; i++)
    {
        ChildNode1 = UpnpNodeList_item(NodeList1,i);
        if (ChildNode1 == NULL)
        {
            break;
        }

    printNodes(ChildNode1, depth + 1);

    NodeType = UpnpNode_getNodeType((Upnp_Node)ChildNode1);
    NodeValue = UpnpNode_getNodeValue(ChildNode1, &err);
    NodeName = UpnpNode_getNodeName(ChildNode1);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"DEPTH-%2d-Node Type %d, Node Name: %s, Node Value: %s\n", 
           depth, NodeType, NodeName, NodeValue);)
    }
}  /****************** End of printNodes *********************/

#if EXCLUDE_SSDP == 0

//********************************************************
//* Name: SsdpCallbackEventHandler
//* Description:  Ssdp callback handler function for upnp api layer
//* Called by:    ssdp layer
//* In:           SsdpEvent * Evt
//********************************************************

void SsdpCallbackEventHandler(SsdpEvent * Evt)
{
    int h;
    struct Upnp_Discovery *param;
    struct Handle_Info *SInfo = NULL;
    Upnp_EventType retEventType = UPNP_E_SUCCESS;
    char *cptr;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside SsdpCallbackEventHandler \n");)

    if( Evt!= NULL && Evt->ErrCode == 0)
    {
        switch(Evt->Cmd)
        {

            case SSDP_OK:      

            case SSDP_ALIVE:   

            case SSDP_BYEBYE: 
 
            case SSDP_TIMEOUT:                
                 DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"SsdpCallbackEventHandler with Cmd %d \n", Evt->Cmd);)

                /* we are assuming that there can be only one client supported at a time */
                if(GetClientHandleInfo(&h, &SInfo) != HND_CLIENT)
                    break;
                if (Evt->Cmd == SSDP_TIMEOUT)
                {
                    SInfo->Callback(UPNP_DISCOVERY_SEARCH_TIMEOUT, NULL, 
                                Evt->Cookie);
                    break;
                }
                /* callback on client with Upnp_Discovery */

                param = (struct Upnp_Discovery *) malloc (sizeof(struct Upnp_Discovery)); 

                param->ErrCode = Evt->ErrCode;
                param->Expires = Evt->MaxAge;
                strcpy(param->DeviceType, Evt->DeviceType);
                strcpy(param->DeviceId, Evt->UDN);
                strcpy(param->ServiceType, Evt->ServiceType);
                // fix to eliminate leading spaces in location
                for (cptr = Evt->Location; *cptr == ' '; cptr++);
                strcpy(param->Location, cptr);
                strcpy(param->Os, Evt->Os);
                strcpy(param->Date, Evt->Date);
                strcpy(param->Ext, Evt->Ext);
                param->DestAddr = (struct sockaddr_in *)Evt->DestAddr;
                switch (Evt->Cmd)
                {
                    case SSDP_OK :
                                      retEventType = UPNP_DISCOVERY_SEARCH_RESULT;
                                      break;

                    case SSDP_ALIVE : Evt->Cookie =  SInfo->Cookie;
                                      retEventType = UPNP_DISCOVERY_ADVERTISEMENT_ALIVE;
                                      break;

                    case SSDP_BYEBYE : Evt->Cookie =  SInfo->Cookie;
                                       retEventType = UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE;
                                       break;
                    default : break;
                }
                SInfo->Callback(retEventType, param,Evt->Cookie);
                free(param);

                DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"SsdpCallbackEventHandler : after client callback \n");)

                break;

            #ifdef INCLUDE_DEVICE_APIS
            case SSDP_SEARCH: 
                DBGONLY(
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"SsdpCallbackEventHandler with Cmd %d SEARCH\n", Evt->Cmd);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"MAX-AGE     =  %d\n",Evt->MaxAge);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"MX     =  %d\n",Evt->Mx);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"DeviceType   =  %s\n",Evt->DeviceType);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"DeviceUuid   =  %s\n",Evt->UDN);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"ServiceType =  %s\n",Evt->ServiceType);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Location    =  %s\n",Evt->Location);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"HostAddr    =  %s\n",Evt->HostAddr);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"OS    =  %s\n",Evt->Os);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Date    =  %s\n",Evt->Date);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Ext    =  %s\n",Evt->Ext);
                fflush(stdout);)

                if (GetDeviceHandleInfo(&h, &SInfo) != HND_DEVICE)
                    break; 
	      	
       	  syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING),__FILE__ "%s ,%d \n","SsdpCallbackEventHandler",__LINE__);
                AdvertiseAndReply(0, h, Evt->RequestType, Evt->DestAddr, 
                Evt->DeviceType, Evt->UDN, Evt->ServiceType, SInfo->MaxAge);
                break;
             #endif

            default : break;
        } // END OF switch (Evt->Cmd)

    }  // END OF if ( Evt!= NULL && Evt->ErrCode == 0)


    DBGONLY(
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"MAX-AGE     =  %d\n",Evt->MaxAge);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"DeviceType   =  %s\n",Evt->DeviceType);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"DeviceUuid   =  %s\n",Evt->UDN);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"ServiceType =  %s\n",Evt->ServiceType);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Location    =  %s\n",Evt->Location);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"HostAddr    =  %s\n",Evt->HostAddr);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"OS    =  %s\n",Evt->Os);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Date    =  %s\n",Evt->Date);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Ext    =  %s\n",Evt->Ext);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Exiting SsdpCallbakEventHandler\n");)

}  /****************** End of SsdpCallbackEventHandler  *********************/
#endif // EXCLUDE_SSDP


//********************************************************
 //* Name: getlocalhostname
 //* Description:  Function to get local IP address
 //*               Gets the ip address for the DEFAULT_INTERFACE 
 //*               interface which is up and not a loopback
 //*               assumes at most MAX_INTERFACES interfaces
 //* Called by:    UpnpInit
 //* In:           char *out
 //* Out:          Ip address
 //* Return codes: UPNP_E_SUCCESS
 //* Error codes:  UPNP_E_INIT
 //********************************************************
     
int getlocalhostname(char *out)
{

    char szBuffer[MAX_INTERFACES*sizeof(struct ifreq)]; 
    struct ifconf ifConf;
    struct ifreq ifReq;
    int nResult;
    int i;
    int LocalSock;
    struct sockaddr_in LocalAddr;
    int j=0;
    

    // Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. 
    if ((LocalSock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
      {
        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Can't create addrlist socket\n");)
        return UPNP_E_INIT;
      }
    // Get the interface configuration information... 
    ifConf.ifc_len = sizeof szBuffer;
    ifConf.ifc_ifcu.ifcu_buf = (caddr_t)szBuffer;
    nResult = ioctl(LocalSock, SIOCGIFCONF, &ifConf);

    if (nResult < 0)
      {
        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"DiscoverInterfaces: SIOCGIFCONF returned error\n");)
     
       return UPNP_E_INIT;
      }

   // Cycle through the list of interfaces looking for IP addresses. 

   for (i = 0; ( (i < ifConf.ifc_len) && (j<DEFAULT_INTERFACE) );) 
     {
       struct ifreq *pifReq = (struct ifreq *)((caddr_t)ifConf.ifc_req + i);
       i += sizeof *pifReq;

       // See if this is the sort of interface we want to deal with.
       strcpy (ifReq.ifr_name, pifReq -> ifr_name);
       if (ioctl (LocalSock, SIOCGIFFLAGS, &ifReq) < 0)
         {
           DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Can't get interface flags for %s:\n", ifReq.ifr_name);)
     
         }
       // Skip loopback, point-to-point and down interfaces, except don't skip down interfaces
       // if we're trying to get a list of configurable interfaces. 
       if ((ifReq.ifr_flags & IFF_LOOPBACK) || (!(ifReq.ifr_flags & IFF_UP)))
         {
           continue;
         }	
       if (pifReq -> ifr_addr.sa_family == AF_INET) 
         {

			// Get a pointer to the address...
           memcpy (&LocalAddr, &pifReq -> ifr_addr, sizeof pifReq -> ifr_addr);

			// We don't want the loopback interface. 
           if (LocalAddr.sin_addr.s_addr == htonl (INADDR_LOOPBACK))
             continue;
     
         }
     //increment j if we found an address which is not loopback
     //and is up
       j++;
       
     }	
   close (LocalSock);
  
   strncpy(out,inet_ntoa( LocalAddr.sin_addr),LINE_SIZE);

   DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside getlocalhostname : after strncpy %s\n",
	     out);)
   return UPNP_E_SUCCESS;
}

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

void AutoAdvertise(void *input)
{
    upnp_timeout *event =(upnp_timeout *) input;
    UpnpSendAdvertisement(event->handle, *((int *) event->Event), event->ttl);
    free_upnp_timeout(event);
}
#endif //INCLUDE_DEVICE_APIS
#endif

/* **************************** */
#ifdef INTERNAL_WEB_SERVER
int UpnpSetWebServerRootDir( IN const char* rootDir )
{
    http_SetRootDir( rootDir );

    if ( rootDir == NULL )
    {
        // Disable GET command
        SetHTTPGetCallback( NULL );
    }
    else
    {
        // enable GET command
        SetHTTPGetCallback( http_OldServerCallback );
    }
    return UPNP_E_SUCCESS;
}
#endif /* INTERNAL_WEB_SERVER */
/* *************************** */

#ifdef INCLUDE_CLIENT_APIS
int UpnpSetEventCallbackUrl(IN const char* url)
{
	genaSetEventCallbackUrl(url);	
	return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */
                       
/*********************** END OF FILE upnpapi.c :) ************************/
