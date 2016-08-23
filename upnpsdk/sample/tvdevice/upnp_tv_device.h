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
// $Date: 2001/06/15 00:21:09 $
//

#ifndef UPNP_TV_DEVICE_H
#define UPNP_TV_DEVICE_H

#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "upnp.h"
#include "sample_util.h"


#define TV_SERVICE_SERVCOUNT  2
#define TV_SERVICE_CONTROL    0
#define TV_SERVICE_PICTURE    1

#define TV_CONTROL_VARCOUNT   3
#define TV_CONTROL_POWER      0
#define TV_CONTROL_CHANNEL    1
#define TV_CONTROL_VOLUME     2

#define TV_PICTURE_VARCOUNT   4
#define TV_PICTURE_COLOR      0
#define TV_PICTURE_TINT       1
#define TV_PICTURE_CONTRAST   2
#define TV_PICTURE_BRIGHTNESS 3

#define TV_MAX_VAL_LEN 5

/* This should be the maximum VARCOUNT from above */
#define TV_MAXVARS TV_PICTURE_VARCOUNT

extern char TvDeviceType[];
extern char *TvServiceType[];


/* Structure for storing Tv Service
   identifiers and state table */
struct TvService {
    char UDN[NAME_SIZE]; /* Universally Unique Device Name */
    char ServiceId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    char *VariableName[TV_MAXVARS]; 
    char *VariableStrVal[TV_MAXVARS];
    int  VariableCount;
};

extern struct TvService tv_service_table[];


extern UpnpDevice_Handle device_handle;


/* Mutex for protecting the global state table data
   in a multi-threaded, asynchronous environment.
   All functions should lock this mutex before reading
   or writing the state table data. */
extern pthread_mutex_t TVDevMutex;





void TvDeviceShutdownHdlr(int);

int TvDeviceStateTableInit(char*);
int TvDeviceHandleSubscriptionRequest(struct Upnp_Subscription_Request *);
int TvDeviceHandleGetVarRequest(struct Upnp_State_Var_Request *);
int TvDeviceHandleActionRequest(struct Upnp_Action_Request *);
int TvDeviceCallbackEventHandler(Upnp_EventType, void*, void*);


int TvDeviceSetServiceTableVar(unsigned int, unsigned int, char*);
int TvDeviceSetPower(int);
int TvDevicePowerOn();
int TvDevicePowerOff();
int TvDeviceSetChannel(int);
int TvDeviceIncrChannel(int);
int TvDeviceSetVolume(int);
int TvDeviceIncrVolume(int);
int TvDeviceSetColor(int);
int TvDeviceIncrColor(int);
int TvDeviceSetTint(int);
int TvDeviceIncrTint(int);
int TvDeviceSetContrast(int);
int TvDeviceIncrContrast(int);
int TvDeviceSetBrightness(int);
int TvDeviceIncrBrightness(int);




#endif
