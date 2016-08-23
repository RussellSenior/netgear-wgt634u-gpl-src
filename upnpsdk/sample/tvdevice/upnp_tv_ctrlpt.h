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


#ifndef UPNP_TV_CTRLPT_H
#define UPNP_TV_CTRLPT_H


#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "upnp.h"
#include "upnptools.h"
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
extern char *TvServiceName[];

extern char *TvVarName[][];
extern char TvVarCount[];


struct tv_service {
    char ServiceId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    char *VariableStrVal[TV_MAXVARS];
    char EventURL[NAME_SIZE];
    char ControlURL[NAME_SIZE];
    char SID[NAME_SIZE];
};


extern struct TvDeviceNode *GlobalDeviceList;

struct TvDevice {
    char UDN[250];
    char DescDocURL[250];
    char FriendlyName[250];
    char PresURL[250];
    int  AdvrTimeOut;
    struct tv_service TvService[TV_SERVICE_SERVCOUNT];
};

struct TvDeviceNode {
    struct TvDevice device;
    struct TvDeviceNode *next;
};


extern pthread_mutex_t DeviceListMutex;

extern UpnpClient_Handle ctrlpt_handle;





int TvCtrlPointPrintHelp();
int TvCtrlPointDeleteNode(struct TvDeviceNode*);
int TvCtrlPointRemoveDevice(char*);
int TvCtrlPointRemoveAll();
int TvCtrlPointRefresh();

int TvCtrlPointGetVar(int, int, char*);
int TvCtrlPointSendAction(int, int, char *, char **, char **, int);

int TvCtrlPointSendSetChannel(int, int);
int TvCtrlPointSendSetVolume(int, int);
int TvCtrlPointSendSetColor(int, int);
int TvCtrlPointSendSetTint(int, int);
int TvCtrlPointSendSetContrast(int, int);
int TvCtrlPointSendSetBrightness(int, int);

int TvCtrlPointGetDevice(int, struct TvDeviceNode **);
int TvCtrlPointPrintList();
int TvCtrlPointPrintDevice(int);
void TvCtrlPointAddDevice (Upnp_Document, char *, int); 
void TvStateUpdate(int, Upnp_Document, char **);
void TvCtrlPointHandleEvent(Upnp_SID, int, Upnp_Document); 
void TvCtrlPointHandleSubscribeUpdate(char *, Upnp_SID, int); 
int TvCtrlPointCallbackEventHandler(Upnp_EventType, void *, void *);
void TvCtrlPointVerifyTimeouts(int);
void TvCtrlPointPrintCommands();
void* TvCtrlPointCommandLoop(void*);


#endif







