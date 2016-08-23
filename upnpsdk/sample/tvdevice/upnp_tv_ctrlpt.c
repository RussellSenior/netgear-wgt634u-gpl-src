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
// $Date: 2001/06/15 00:21:32 $
//


#include "upnp_tv_ctrlpt.h"


/* Mutex for protecting the global device list
   in a multi-threaded, asynchronous environment.
   All functions should lock this mutex before reading
   or writing the device list. */
pthread_mutex_t DeviceListMutex = PTHREAD_MUTEX_INITIALIZER;

UpnpClient_Handle ctrlpt_handle = -1;

char TvDeviceType[] = "urn:schemas-upnp-org:device:tvdevice:1";
char *TvServiceType[] = {"urn:schemas-upnp-org:service:tvcontrol:1", "urn:schemas-upnp-org:service:tvpicture:1"};
char *TvServiceName[] = {"Control", "Picture"};

/* Global arrays for storing variable names and counts for 
   TvControl and TvPicture services */
char *TvVarName[TV_SERVICE_SERVCOUNT][TV_MAXVARS] = {
    {"Power","Channel","Volume",""}, 
    {"Color","Tint","Contrast","Brightness"}
};
char TvVarCount[TV_SERVICE_SERVCOUNT] = {TV_CONTROL_VARCOUNT, TV_PICTURE_VARCOUNT};

/* Timeout to request during subscriptions */
int default_timeout = 1800;

/* The first node in the global device list, or NULL
   if empty */
struct TvDeviceNode *GlobalDeviceList=NULL;

/* Tags for valid commands issued at the command prompt */
enum cmdloop_tvcmds {
    PRTHELP=0,
    POWON,
    POWOFF,
    SETCHAN,
    SETVOL,
    SETCOL,
    SETTINT,
    SETCONT,
    SETBRT,
    CTRLACTION,
    PICTACTION,
    CTRLGETVAR,
    PICTGETVAR,
    PRTDEV,
    LSTDEV,
    REFRESH,
    EXITCMD
};


/* Data structure for parsing commands from
   the command line */
struct cmdloop_commands  {
    char  *str;         // the string 
    int    cmdnum;      // the command
    char  *args;        // the args
} cmdloop_commands;




/* Mappings between command text names, command tag,
   and required command arguments for command line
   commands */
static struct cmdloop_commands cmdloop_cmdlist[]= {
    {"Help", PRTHELP,             ""}, 
    {"ListDev", LSTDEV,           ""},
    {"Refresh", REFRESH,          ""},
    {"PrintDev", PRTDEV,          "<devnum>"},
    {"PowerOn", POWON,            "<devnum>"}, 
    {"PowerOff", POWOFF,          "<devnum>"},
    {"SetChannel", SETCHAN,       "<devnum> <channel (int)>"},
    {"SetVolume", SETVOL,         "<devnum> <volume (int)>"},
    {"SetColor", SETCOL,          "<devnum> <color (int)>"},
    {"SetTint", SETTINT,          "<devnum> <tint (int)>"},
    {"SetContrast", SETCONT,      "<devnum> <contrast (int)>"},
    {"SetBrightness", SETBRT,     "<devnum> <brightness (int)>"},
    {"CtrlAction",  CTRLACTION,   "<devnum> <action (string)>"},
    {"PictAction",  PICTACTION,   "<devnum> <action (string)>"},
    {"CtrlGetVar",  CTRLGETVAR,   "<devnum> <varname (string)>"},
    {"PictGetVar",  PICTGETVAR,   "<devnum> <varname (string)>"},
    {"Exit", EXITCMD,             ""}
};





/********************************************************************************
 * TvCtrlPointPrintHelp
 *
 * Description: 
 *       Print help info for this application.
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
int TvCtrlPointPrintHelp() 
{
    printf("\n\n");
    printf("******************************\n");
    printf("* TV Control Point Help Info *\n");
    printf("******************************\n");
    printf("\n\n");
    printf("This sample control point application automatically searches\n");
    printf("for and subscribes to the services of television device emulator\n");
    printf("devices, described in the tvdevicedesc.xml description document.\n");
    printf("\n\n");
    printf("Commands:\n\n");
    printf("  Help\n");
    printf("       Print this help info.\n\n");
    printf("  ListDev\n");
    printf("       Print the current list of TV Device Emulators that this\n");
    printf("         control point is aware of.  Each device is preceded by a\n");
    printf("         device number which corresponds to the devnum argument of\n");
    printf("         commands listed below.\n\n");
    printf("  Refresh\n");
    printf("       Delete all of the devices from the device list and issue new\n");
    printf("         search request to rebuild the list from scratch.\n\n");
    printf("  PrintDev       <devnum>\n");
    printf("       Print the state table for the device <devnum>.\n");
    printf("         e.g., 'PrintDev 1' prints the state table for the first\n");
    printf("         device in the device list.\n\n");
    printf("  PowerOn        <devnum>\n");
    printf("       Sends the PowerOn action to the Control Service of\n");
    printf("         device <devnum>.\n\n");
    printf("  PowerOff       <devnum>\n");
    printf("       Sends the PowerOff action to the Control Service of\n");
    printf("         device <devnum>.\n\n");
    printf("  SetChannel     <devnum> <channel>\n");
    printf("       Sends the SetChannel action to the Control Service of\n");
    printf("         device <devnum>, requesting the channel to be changed\n");
    printf("         to <channel>.\n\n");
    printf("  SetVolume      <devnum> <volume>\n");
    printf("       Sends the SetVolume action to the Control Service of\n");
    printf("         device <devnum>, requesting the volume to be changed\n");
    printf("         to <volume>.\n\n");
    printf("  SetColor       <devnum> <color>\n");
    printf("       Sends the SetColor action to the Control Service of\n");
    printf("         device <devnum>, requesting the color to be changed\n");
    printf("         to <color>.\n\n");
    printf("  SetTint        <devnum> <tint>\n");
    printf("       Sends the SetTint action to the Control Service of\n");
    printf("         device <devnum>, requesting the tint to be changed\n");
    printf("         to <tint>.\n\n");
    printf("  SetContrast    <devnum> <contrast>\n");
    printf("       Sends the SetContrast action to the Control Service of\n");
    printf("         device <devnum>, requesting the contrast to be changed\n");
    printf("         to <contrast>.\n\n");
    printf("  SetBrightness  <devnum> <brightness>\n");
    printf("       Sends the SetBrightness action to the Control Service of\n");
    printf("         device <devnum>, requesting the brightness to be changed\n");
    printf("         to <brightness>.\n\n");
    printf("  CtrlAction     <devnum> <action>\n");
    printf("       Sends an action request specified by the string <action>\n");
    printf("         to the Control Service of device <devnum>.  This command\n");
    printf("         only works for actions that have no arguments.\n");
    printf("         (e.g., \"CtrlAction 1 IncreaseChannel\")\n\n");
    printf("  PictAction     <devnum> <action>\n");
    printf("       Sends an action request specified by the string <action>\n");
    printf("         to the Picture Service of device <devnum>.  This command\n");
    printf("         only works for actions that have no arguments.\n");
    printf("         (e.g., \"PictAction 1 DecreaseContrast\")\n\n");
    printf("  CtrlGetVar     <devnum> <varname>\n");
    printf("       Requests the value of a variable specified by the string <varname>\n");
    printf("         from the Control Service of device <devnum>.\n");
    printf("         (e.g., \"CtrlGetVar 1 Volume\")\n\n");
    printf("  PictGetVar     <devnum> <action>\n");
    printf("       Requests the value of a variable specified by the string <varname>\n");
    printf("         from the Picture Service of device <devnum>.\n");
    printf("         (e.g., \"PictGetVar 1 Tint\")\n\n");
    printf("  Exit\n");
    printf("       Exits the control point application.\n");

    return(1);
}


/********************************************************************************
 * TvCtrlPointDeleteNode
 *
 * Description: 
 *       Delete a device node from the global device list.  Note that this
 *       function is NOT thread safe, and should be called from another
 *       function that has already locked the global device list.
 *
 * Parameters:
 *   node -- The device node
 *
 ********************************************************************************/
int TvCtrlPointDeleteNode(struct TvDeviceNode *node) 
{
    int ret=UPNP_E_SUCCESS;
    int service, var;

    if (!node) {
	printf("error in TvCtrlPointDeleteNode: node is empty\n");
	return(1);
    }

    for (service=0; service<TV_SERVICE_SERVCOUNT; service++) {
	/* If we have a valid control SID, then unsubscribe */
	if (strcmp(node->device.TvService[service].SID, "") != 0) {
	    ret=UpnpUnSubscribe(ctrlpt_handle, node->device.TvService[service].SID);
	    if (ret==UPNP_E_SUCCESS)
		printf("Unsubscribed from Tv %s EventURL with SID=%s\n", TvServiceName[service], node->device.TvService[service].SID);
	    else
		printf("Error unsubscribing to Tv %s EventURL -- %d\n", TvServiceName[service], ret);
	}

	for (var = 0; var < TvVarCount[service]; var++) {
	    if(node->device.TvService[service].VariableStrVal[var]) 
		free(node->device.TvService[service].VariableStrVal[var]);
	}
    }

    free(node);
    node = NULL;

    return(1);
}



/********************************************************************************
 * TvCtrlPointRemoveDevice
 *
 * Description: 
 *       Remove a device from the global device list.
 *
 * Parameters:
 *   UDN -- The Unique Device Name for the device to remove
 *
 ********************************************************************************/
int TvCtrlPointRemoveDevice(char* UDN) 
{
    struct TvDeviceNode *curdevnode, *prevdevnode;
    int ret=0;


    pthread_mutex_lock(&DeviceListMutex);


    curdevnode = GlobalDeviceList;
  
    if (!curdevnode) {
	printf("Warning in TvCtrlPointRemoveDevice -- Device list empty\n");
    } else {

	if (strcmp(curdevnode->device.UDN,UDN) == 0) {
	    GlobalDeviceList = curdevnode->next;
	    TvCtrlPointDeleteNode(curdevnode);
	} else {
	    prevdevnode = curdevnode;
	    curdevnode = curdevnode->next;

	    while (curdevnode) {
		if (strcmp(curdevnode->device.UDN,UDN) == 0) {
		    prevdevnode->next = curdevnode->next;
		    TvCtrlPointDeleteNode(curdevnode);
		    break;
		}
		prevdevnode = curdevnode;
		curdevnode = curdevnode->next;
	    }
	}
    }

    pthread_mutex_unlock(&DeviceListMutex);

    return ret;
}


/********************************************************************************
 * TvCtrlPointRemoveAll
 *
 * Description: 
 *       Remove all devices from the global device list.
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
int TvCtrlPointRemoveAll() 
{
    struct TvDeviceNode *curdevnode, *next;

    pthread_mutex_lock(&DeviceListMutex);

    curdevnode = GlobalDeviceList;
    GlobalDeviceList = NULL;
  
    while (curdevnode) {
	next = curdevnode->next;
	TvCtrlPointDeleteNode(curdevnode);
	curdevnode = next;
    }

    pthread_mutex_unlock(&DeviceListMutex);

    return(1);
}


/********************************************************************************
 * TvCtrlPointRefresh
 *
 * Description: 
 *       Clear the current global device list and issue new search
 *	 requests to build it up again from scratch.
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
int TvCtrlPointRefresh() 
{
    int ret;

    TvCtrlPointRemoveAll();

    /* Search for device with particular UDN, 
       waiting for up to 5 seconds for the response */
    /* ret = UpnpSearchAsync(ctrlpt_handle, 5, "uuid:Upnp-TVEmulator-1_0-1234567890001", NULL); */

    /* Search for all devices of type tvdevice version 1, 
       waiting for up to 5 seconds for the response */
    ret = UpnpSearchAsync(ctrlpt_handle, 5, TvDeviceType, NULL);

    /* Search for all services of type tvcontrol version 1, 
       waiting for up to 5 seconds for the response */
    /* ret = UpnpSearchAsync(ctrlpt_handle, 5, TvServiceType[TV_SERVICE_CONTROL], NULL); */

    /* Search for all root devices, 
       waiting for up to 5 seconds for the response */
    /* ret = UpnpSearchAsync(ctrlpt_handle, 5, "upnp:rootdevice", NULL); */
    
    if (ret != UPNP_E_SUCCESS)
	printf("Error sending search request%d\n", ret);

    return(1);
}



/********************************************************************************
 * TvCtrlPointGetVar
 *
 * Description: 
 *       Send a GetVar request to the specified service of a device.
 *
 * Parameters:
 *   service -- The service
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   varname -- The name of the variable to request.
 *
 ********************************************************************************/
int TvCtrlPointGetVar(int service, int devnum, char* varname)
{
    struct TvDeviceNode *devnode;
    int ret=0;

    pthread_mutex_lock(&DeviceListMutex);

    if (!TvCtrlPointGetDevice(devnum, &devnode)) {
	ret = 0;;
    } else {

	ret = UpnpGetServiceVarStatusAsync( ctrlpt_handle, devnode->device.TvService[service].ControlURL, varname,  
			      TvCtrlPointCallbackEventHandler, NULL);
	if (ret != UPNP_E_SUCCESS)
	    printf("Error in UpnpGetServiceVarStatusAsync -- %d\n", ret);

    }

    pthread_mutex_unlock(&DeviceListMutex);

    return(ret);
}


/********************************************************************************
 * TvCtrlPointSendAction
 *
 * Description: 
 *       Send an Action request to the specified service of a device.
 *
 * Parameters:
 *   service -- The service
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   actionname -- The name of the action.
 *   param_name -- An array of parameter names
 *   param_val -- The corresponding parameter values
 *   param_count -- The number of parameters
 *
 ********************************************************************************/
int TvCtrlPointSendAction(int service, int devnum, char *actionname, char **param_name, char **param_val, int param_count)
{
    struct TvDeviceNode *devnode;
    char ActionXml[250];
    char tmpstr[100];  
    Upnp_Document actionNode=NULL;
    int ret=0;
    int param;

    pthread_mutex_lock(&DeviceListMutex);

    if (!TvCtrlPointGetDevice(devnum, &devnode)) {
	ret = 0;;
    } else {

	/*sprintf(ActionXml, "<u:%s xmlns:u=\"%s\">", actionname, TvServiceType[service]);
	for (param = 0; param < param_count; param++) {
	    sprintf(tmpstr, "<%s>%s</%s>", param_name[param], param_val[param], param_name[param]);
	    strcat(ActionXml, tmpstr);
	}
	sprintf(tmpstr, "</u:%s>", actionname);
	strcat(ActionXml, tmpstr);
	actionNode = UpnpParse_Buffer( ActionXml);*/


       if( param_count== 0)
          //UpnpAddToAction(&actionNode,actionname,TvServiceType[service],NULL,NULL);  OR
          actionNode = UpnpMakeAction(actionname,TvServiceType[service],0,NULL);
       else
       for (param = 0; param < param_count; param++)
       {
           if( UpnpAddToAction(&actionNode,actionname,TvServiceType[service],param_name[param], param_val[param]) != UPNP_E_SUCCESS)
           {
                printf("Error in adding action parameter !!!!!!!!!!!!!!!!\n");
                return -1;
           }
       }

	ret = UpnpSendActionAsync( ctrlpt_handle, devnode->device.TvService[service].ControlURL, TvServiceType[service],
			      devnode->device.UDN, actionNode, TvCtrlPointCallbackEventHandler, NULL);
	if (ret != UPNP_E_SUCCESS)
	    printf("Error in UpnpSendActionAsync -- %d\n", ret);
    }

    pthread_mutex_unlock(&DeviceListMutex);

    if (actionNode) UpnpDocument_free(actionNode);
    return(ret);
}



/********************************************************************************
 * TvCtrlPointSendSetChannel
 *
 * Description: 
 *       Send a SetChannel action to a device in
 *       the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   channel -- The channel to change to
 *
 ********************************************************************************/
int TvCtrlPointSendSetChannel(int devnum, int channel)
{
    int ret=0;
    char param_name_a[50];
    char param_val_a[50];
    char *param_name = param_name_a;
    char *param_val = param_val_a;

    sprintf(param_name_a, "Channel");
    sprintf(param_val_a, "%d", channel);

    ret = TvCtrlPointSendAction(TV_SERVICE_CONTROL, devnum, "SetChannel", &param_name, &param_val, 1);

    return(ret);
}


/********************************************************************************
 * TvCtrlPointSendSetVolume
 *
 * Description: 
 *       Send a SetVolume action to a device in
 *       the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   volume -- The volume to change to
 *
 ********************************************************************************/
int TvCtrlPointSendSetVolume(int devnum, int volume)
{
    int ret=0;
    char param_name_a[50];
    char param_val_a[50];
    char *param_name = param_name_a;
    char *param_val = param_val_a;

    sprintf(param_name_a, "Volume");
    sprintf(param_val_a, "%d", volume);

    ret = TvCtrlPointSendAction(TV_SERVICE_CONTROL, devnum, "SetVolume", &param_name, &param_val, 1);

    return(ret);
}



/********************************************************************************
 * TvCtrlPointSendSetColor
 *
 * Description: 
 *       Send a SetColor action to a device in
 *       the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   color -- The color to change to
 *
 ********************************************************************************/
int TvCtrlPointSendSetColor(int devnum, int color)
{
    int ret=0;
    char param_name_a[50];
    char param_val_a[50];
    char *param_name = param_name_a;
    char *param_val = param_val_a;

    sprintf(param_name_a, "Color");
    sprintf(param_val_a, "%d", color);

    ret = TvCtrlPointSendAction(TV_SERVICE_PICTURE, devnum, "SetColor", &param_name, &param_val, 1);

    return(ret);
}



/********************************************************************************
 * TvCtrlPointSendSetTint
 *
 * Description: 
 *       Send a SetTint action to a device in
 *       the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   tint -- The tint to change to
 *
 ********************************************************************************/
int TvCtrlPointSendSetTint(int devnum, int tint)
{
    int ret=0;
    char param_name_a[50];
    char param_val_a[50];
    char *param_name = param_name_a;
    char *param_val = param_val_a;

    sprintf(param_name_a, "Tint");
    sprintf(param_val_a, "%d", tint);

    ret = TvCtrlPointSendAction(TV_SERVICE_PICTURE, devnum, "SetTint", &param_name, &param_val, 1);

    return(ret);
}



/********************************************************************************
 * TvCtrlPointSendSetContrast
 *
 * Description: 
 *       Send a SetContrast action to a device in
 *       the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   contrast -- The contrast to change to
 *
 ********************************************************************************/
int TvCtrlPointSendSetContrast(int devnum, int contrast)
{
    int ret=0;
    char param_name_a[50];
    char param_val_a[50];
    char *param_name = param_name_a;
    char *param_val = param_val_a;

    sprintf(param_name_a, "Contrast");
    sprintf(param_val_a, "%d", contrast);

    ret = TvCtrlPointSendAction(TV_SERVICE_PICTURE, devnum, "SetContrast", &param_name, &param_val, 1);

    return(ret);
}



/********************************************************************************
 * TvCtrlPointSendSetBrightness
 *
 * Description: 
 *       Send a SetBrightness action to a device in
 *       the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   brightness -- The brightness to change to
 *
 ********************************************************************************/
int TvCtrlPointSendSetBrightness(int devnum, int brightness)
{
    int ret=0;
    char param_name_a[50];
    char param_val_a[50];
    char *param_name = param_name_a;
    char *param_val = param_val_a;

    sprintf(param_name_a, "Brightness");
    sprintf(param_val_a, "%d", brightness);

    ret = TvCtrlPointSendAction(TV_SERVICE_PICTURE, devnum, "SetBrightness", &param_name, &param_val, 1);

    return(ret);
}




/********************************************************************************
 * TvCtrlPointGetDevice
 *
 * Description: 
 *       Given a list number, returns the pointer to the device
 *       node at that position in the global device list.  Note
 *       that this function is not thread safe.  It must be called 
 *       from a function that has locked the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   devnode -- The output device node pointer
 *
 ********************************************************************************/
int TvCtrlPointGetDevice(int devnum, struct TvDeviceNode **devnode) 
{
    int count = devnum;
    struct TvDeviceNode *tmpdevnode=NULL;

    if (count) tmpdevnode = GlobalDeviceList;

    while (--count && tmpdevnode) {
	tmpdevnode = tmpdevnode->next;
    }

    if (tmpdevnode) {
	*devnode = tmpdevnode;
	return(1);
    } else {
	printf("Error finding TvDevice number -- %d\n", devnum);
	return(0);
    }
}




/********************************************************************************
 * TvCtrlPointPrintList
 *
 * Description: 
 *       Print the universal device names for each device in the global device list
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
int TvCtrlPointPrintList() 
{
    struct TvDeviceNode *tmpdevnode;
    int i=0;

    pthread_mutex_lock(&DeviceListMutex);

    printf("\nTvCtrlPointPrintList:\n");
    tmpdevnode = GlobalDeviceList;
    while (tmpdevnode) {
	printf(" %3d -- %s\n", ++i, tmpdevnode->device.UDN);
	tmpdevnode = tmpdevnode->next;
    }
    printf("\n");
    pthread_mutex_unlock(&DeviceListMutex);

    return(1);
}

/********************************************************************************
 * TvCtrlPointPrintDevice
 *
 * Description: 
 *       Print the identifiers and state table for a device from
 *       the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *
 ********************************************************************************/
int TvCtrlPointPrintDevice(int devnum) 
{
    struct TvDeviceNode *tmpdevnode;
    int i=0, service, var;
    char spacer[15];

    if (devnum <= 0) {
	printf("Error in TvCtrlPointPrintDevice: invalid devnum = %d\n", devnum);
	return(0);
    }

    pthread_mutex_lock(&DeviceListMutex);

    printf("\nTvCtrlPointPrintDevice:\n");
    tmpdevnode = GlobalDeviceList;
    while (tmpdevnode) {
	i++;
	if (i == devnum)
	    break;
	tmpdevnode = tmpdevnode->next;
    }
	
    if (!tmpdevnode) {
	printf("Error in TvCtrlPointPrintDevice: invalid devnum = %d  --  actual device count = %d\n", devnum, i);
    } else {
	printf("  TvDevice -- %d\n", devnum);
	printf("    |                  \n");
	printf("    +- UDN        = %s\n", tmpdevnode->device.UDN);
	printf("    +- DescDocURL     = %s\n", tmpdevnode->device.DescDocURL);
	printf("    +- FriendlyName   = %s\n", tmpdevnode->device.FriendlyName);
	printf("    +- PresURL        = %s\n", tmpdevnode->device.PresURL);
	printf("    +- Adver. TimeOut = %d\n", tmpdevnode->device.AdvrTimeOut);
	for (service=0; service<TV_SERVICE_SERVCOUNT; service++) {
	    if (service < TV_SERVICE_SERVCOUNT-1) 
		sprintf(spacer, "    |    ");
	    else 
		sprintf(spacer, "         ");
	    printf("    |                  \n");
	    printf("    +- Tv %s Service\n", TvServiceName[service]);
	    printf("%s+- ServiceId       = %s\n", spacer, tmpdevnode->device.TvService[service].ServiceId);
	    printf("%s+- ServiceType     = %s\n", spacer, tmpdevnode->device.TvService[service].ServiceType);
	    printf("%s+- EventURL        = %s\n", spacer, tmpdevnode->device.TvService[service].EventURL);
	    printf("%s+- ControlURL      = %s\n", spacer, tmpdevnode->device.TvService[service].ControlURL);
	    printf("%s+- SID             = %s\n", spacer, tmpdevnode->device.TvService[service].SID);
	    printf("%s+- ServiceStateTable\n", spacer);
	    for (var=0; var<TvVarCount[service]; var++) {
		printf("%s     +- %-10s = %s\n", spacer, TvVarName[service][var], tmpdevnode->device.TvService[service].VariableStrVal[var]);
	    }
	}
    }
    printf("\n");
    pthread_mutex_unlock(&DeviceListMutex);

    return(1);
}


/********************************************************************************
 * TvCtrlPointAddDevice
 *
 * Description: 
 *       If the device is not already included in the global device list,
 *       add it.  Otherwise, update its advertisement expiration timeout.
 *
 * Parameters:
 *   DescDoc -- The description document for the device
 *   location -- The location of the description document URL
 *   expires -- The expiration time for this advertisement
 *
 ********************************************************************************/
void TvCtrlPointAddDevice (Upnp_Document DescDoc, char *location, int expires) 
{
    char *deviceType=NULL;
    char *friendlyName=NULL;
    char presURL[200];
    char *baseURL=NULL;
    char *relURL=NULL;
    char *UDN=NULL;
    char *serviceId[TV_SERVICE_SERVCOUNT] = {NULL, NULL};
    char *eventURL[TV_SERVICE_SERVCOUNT] = {NULL, NULL};
    char *controlURL[TV_SERVICE_SERVCOUNT] = {NULL, NULL};
    Upnp_SID eventSID[TV_SERVICE_SERVCOUNT];
    int TimeOut[TV_SERVICE_SERVCOUNT] = {default_timeout, default_timeout};
    struct TvDeviceNode *deviceNode;
    struct TvDeviceNode *tmpdevnode;
    int ret=1;
    int found=0;
    int service, var;
  
    pthread_mutex_lock(&DeviceListMutex);
  
    /* Read key elements from description document */
    UDN = SampleUtil_GetFirstDocumentItem(DescDoc, "UDN");
    deviceType = SampleUtil_GetFirstDocumentItem(DescDoc, "deviceType");
    friendlyName = SampleUtil_GetFirstDocumentItem(DescDoc, "friendlyName");
    baseURL = SampleUtil_GetFirstDocumentItem(DescDoc, "URLBase");
    relURL = SampleUtil_GetFirstDocumentItem(DescDoc, "presentationURL");

    if (baseURL)
	ret = UpnpResolveURL(baseURL, relURL, presURL);
    else
	ret = UpnpResolveURL(location, relURL, presURL);
    if (ret!=UPNP_E_SUCCESS)
	printf("Error generating presURL from %s + %s\n", baseURL, relURL);
  
    if (strcmp(deviceType, TvDeviceType) == 0) {
	printf("Found Tv device!!!\n");
    
	// Check if this device is already in the list
	tmpdevnode = GlobalDeviceList;
	while (tmpdevnode) {
	    if (strcmp(tmpdevnode->device.UDN,UDN) == 0) {
		found=1;
		break;
	    }
	    tmpdevnode = tmpdevnode->next;
	}
    
	if (found) {
	    // The device is already there, so just update 
	    // the advertisement timeout field
	    tmpdevnode->device.AdvrTimeOut = expires;
	} else {
	    
	    for (service = 0; service<TV_SERVICE_SERVCOUNT; service++) {
		if (SampleUtil_FindAndParseService(DescDoc, location, TvServiceType[service], &serviceId[service], &eventURL[service], &controlURL[service])) {
		    printf("Subscribing to EventURL %s...\n", eventURL[service]);
		    ret=UpnpSubscribe(ctrlpt_handle,eventURL[service],
				      &TimeOut[service],eventSID[service]);
		    if (ret==UPNP_E_SUCCESS) {
			printf("Subscribed to EventURL with SID=%s\n", eventSID[service]);
		    } else {
			printf("Error Subscribing to EventURL -- %d\n", ret);
			strcpy(eventSID[service], "");
		    }
		} else {
		    printf("Error: Could not find Service: %s\n", TvServiceType[service]);
		}
	    }
      
	    /* Create a new device node */
	    deviceNode = (struct TvDeviceNode *) malloc (sizeof(struct TvDeviceNode));
	    strcpy(deviceNode->device.UDN, UDN);
	    strcpy(deviceNode->device.DescDocURL, location);
	    strcpy(deviceNode->device.FriendlyName, friendlyName);
	    strcpy(deviceNode->device.PresURL, presURL);
	    deviceNode->device.AdvrTimeOut = expires;

	    for (service = 0; service<TV_SERVICE_SERVCOUNT; service++) {
		strcpy(deviceNode->device.TvService[service].ServiceId, serviceId[service]);
		strcpy(deviceNode->device.TvService[service].ServiceType, TvServiceType[service]);
		strcpy(deviceNode->device.TvService[service].ControlURL, controlURL[service]);
		strcpy(deviceNode->device.TvService[service].EventURL, eventURL[service]);
		strcpy(deviceNode->device.TvService[service].SID, eventSID[service]);
		for (var = 0; var < TvVarCount[service]; var++) {
		    deviceNode->device.TvService[service].VariableStrVal[var] = (char *) malloc (TV_MAX_VAL_LEN);
		    strcpy(deviceNode->device.TvService[service].VariableStrVal[var], "");
		}
	    }
      
	    deviceNode->next = NULL;
      
      
	    // Insert the new device node in the list
	    if ((tmpdevnode = GlobalDeviceList)) {
	
		while (tmpdevnode) {
		    if (tmpdevnode->next) {
			tmpdevnode = tmpdevnode->next;
		    } else {
			tmpdevnode->next = deviceNode;
			break;
		    }
		}
	    } else {
		GlobalDeviceList = deviceNode;
	    }
	}
    }
  
  
    pthread_mutex_unlock(&DeviceListMutex);
  
    if (deviceType) free(deviceType);
    if (friendlyName) free(friendlyName);
    if (UDN) free(UDN);
    if (baseURL) free(baseURL);
    if (relURL) free(relURL);
  
    for (service = 0; service<TV_SERVICE_SERVCOUNT; service++) {
	if (serviceId[service]) free(serviceId[service]);
	if (controlURL[service]) free(controlURL[service]);
	if (eventURL[service]) free(eventURL[service]);
    }
}



/********************************************************************************
 * TvStateUpdate
 *
 * Description: 
 *       Update a Tv state table.  Called when an event is
 *       received.  Note: this function is NOT thread save.  It must be
 *       called from another function that has locked the global device list.
 *
 * Parameters:
 *   Service -- The service state table to update
 *   ChangedVariables -- DOM document representing the XML received
 *                       with the event
 *   State -- pointer to the state table for the Tv  service
 *            to update
 *
 ********************************************************************************/
void TvStateUpdate(int Service, Upnp_Document ChangedVariables, char **State)
{
  
    Upnp_NodeList properties;
    Upnp_NodeList variables;
    int length;
    int length1;
    int i;
    Upnp_Node property;
    Upnp_Node variable;
    int j;
    char *tmpstate=NULL;

    printf("Tv State Update:\n");

    /* Find all of the e:property tags in the document */
    properties=UpnpDocument_getElementsByTagName(ChangedVariables, 
						 "e:property");
    if (properties!=NULL) {
     
	length= UpnpNodeList_getLength(properties);

	/* Loop through each property change found */ 
	for (i=0;i<(length);i++) { 

	    property =  UpnpNodeList_item(properties,i);

	    /* For each variable name in the state table, check if this
	       is a corresponding property change */
	    for (j=0;j<TvVarCount[Service];j++) {
		variables= UpnpElement_getElementsByTagName(property,
							    TvVarName[Service][j]);
		
		/* If a match is found, extract the value, and update
		   the state table */
		if (variables) {
		    length1= UpnpNodeList_getLength(variables);
		    if (length1) {
			variable=UpnpNodeList_item(variables,0);
		     
			tmpstate=SampleUtil_GetElementValue(variable);
			if (tmpstate) {
			    strcpy(State[j], tmpstate);
			    printf("\tVariable Name: %s New Value:'%s'\n",
				   TvVarName[Service][j],State[j]);
			}
			UpnpNode_free(variable);
			variable=NULL;
			if (tmpstate) free(tmpstate);
			tmpstate=NULL;
		    }
		    UpnpNodeList_free(variables);
		    variables=NULL;
		   
		}
	    }
	    UpnpNode_free(property);
	}
	UpnpNodeList_free(properties);
    }
   
}



/********************************************************************************
 * TvCtrlPointHandleEvent
 *
 * Description: 
 *       Handle a UPnP event that was received.  Process the event and update
 *       the appropriate service state table.
 *
 * Parameters:
 *   sid -- The subscription id for the event
 *   eventkey -- The eventkey number for the event
 *   changes -- The DOM document representing the changes
 *
 ********************************************************************************/
void TvCtrlPointHandleEvent(Upnp_SID sid, int evntkey, Upnp_Document changes) 
{
    struct TvDeviceNode *tmpdevnode;
    int service;

    pthread_mutex_lock(&DeviceListMutex);

    tmpdevnode = GlobalDeviceList;

    while (tmpdevnode) {
	for (service=0; service<TV_SERVICE_SERVCOUNT; service++) {
	    if (strcmp(tmpdevnode->device.TvService[service].SID,sid) == 0) {
		printf("Received Tv %s Event: %d for SID %s\n", TvServiceName[service], evntkey, sid);
		TvStateUpdate(service,changes,(char **)&tmpdevnode->device.TvService[service].VariableStrVal);
		break;
	    }
	}
    
	tmpdevnode = tmpdevnode->next;
    }

    pthread_mutex_unlock(&DeviceListMutex);
}



/********************************************************************************
 * TvCtrlPointHandleSubscribeUpdate
 *
 * Description: 
 *       Handle a UPnP subscription update that was received.  Find the 
 *       service the update belongs to, and update its subscription
 *       timeout.
 *
 * Parameters:
 *   eventURL -- The event URL for the subscription
 *   sid -- The subscription id for the subscription
 *   timeout  -- The new timeout for the subscription
 *
 ********************************************************************************/
void TvCtrlPointHandleSubscribeUpdate(char *eventURL, Upnp_SID sid, int timeout) 
{
    struct TvDeviceNode *tmpdevnode;
    int service;

    pthread_mutex_lock(&DeviceListMutex);

    tmpdevnode = GlobalDeviceList;
    while (tmpdevnode) {
	for (service = 0; service<TV_SERVICE_SERVCOUNT; service++) {

	    if (strcmp(tmpdevnode->device.TvService[service].EventURL,eventURL) == 0) {
		printf("Received Tv %s Event Renewal for eventURL %s\n", TvServiceName[service], eventURL);
		strcpy(tmpdevnode->device.TvService[service].SID, sid);
		break;
	    }
	}
    
	tmpdevnode = tmpdevnode->next;
    }

    pthread_mutex_unlock(&DeviceListMutex);
}



/********************************************************************************
 * TvCtrlPointCallbackEventHandler
 *
 * Description: 
 *       The callback handler registered with the SDK while registering
 *       the control point.  Detects the type of callback, and passes the 
 *       request on to the appropriate function.
 *
 * Parameters:
 *   EventType -- The type of callback event
 *   Event -- Data structure containing event data
 *   Cookie -- Optional data specified during callback registration
 *
 ********************************************************************************/
int TvCtrlPointCallbackEventHandler(Upnp_EventType EventType, 
			 void *Event, 
			 void *Cookie)
{
  
    SampleUtil_PrintEvent(EventType, Event);
  
    switch ( EventType) {
      
	/* SSDP Stuff */
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    {
	struct Upnp_Discovery *d_event = (struct Upnp_Discovery * ) Event;
	Upnp_Document DescDoc=NULL;
	int ret;

	if (d_event->ErrCode != UPNP_E_SUCCESS)
	    printf("Error in Discovery Callback -- %d\n", d_event->ErrCode);

	if ((ret=UpnpDownloadXmlDoc(d_event->Location, &DescDoc)) != UPNP_E_SUCCESS) {
	    printf("Error obtaining device description from %s -- error = %d\n", d_event->Location, ret );
	} else {
	    TvCtrlPointAddDevice(DescDoc, d_event->Location, d_event->Expires);
	}

	if (DescDoc) UpnpDocument_free(DescDoc);

	TvCtrlPointPrintList();
    }
    break;

    case UPNP_DISCOVERY_SEARCH_TIMEOUT:
	/* Nothing to do here... */
	break;

    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    {
	struct Upnp_Discovery *d_event = (struct Upnp_Discovery * ) Event;

	if (d_event->ErrCode != UPNP_E_SUCCESS)
	    printf("Error in Discovery ByeBye Callback -- %d\n", d_event->ErrCode);

	printf("Received ByeBye for Device: %s\n", d_event->DeviceId);
	TvCtrlPointRemoveDevice(d_event->DeviceId);

	printf("After byebye:\n");
	TvCtrlPointPrintList();

    }
    break;

    /* SOAP Stuff */
    case UPNP_CONTROL_ACTION_COMPLETE:
    {
	struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;

	if (a_event->ErrCode != UPNP_E_SUCCESS)
	    printf("Error in  Action Complete Callback -- %d\n", a_event->ErrCode);

	/* No need for any processing here, just print out results.  Service state
	   table updates are handled by events. */
    }
    break;
      
    case UPNP_CONTROL_GET_VAR_COMPLETE:
    {
	struct Upnp_State_Var_Complete *sv_event = (struct Upnp_State_Var_Complete * ) Event;

	if (sv_event->ErrCode != UPNP_E_SUCCESS)
	    printf("Error in Get Var Complete Callback -- %d\n", sv_event->ErrCode);

    }
    break;
      
    /* GENA Stuff */
    case UPNP_EVENT_RECEIVED:
    {
	struct Upnp_Event *e_event = (struct Upnp_Event * ) Event;

	TvCtrlPointHandleEvent(e_event->Sid, e_event->EventKey, e_event->ChangedVariables);

    }
    break;

    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
    case UPNP_EVENT_RENEWAL_COMPLETE:
    {
	struct Upnp_Event_Subscribe *es_event = (struct Upnp_Event_Subscribe * ) Event;

	if (es_event->ErrCode != UPNP_E_SUCCESS)
	    printf("Error in Event Subscribe Callback -- %d\n", es_event->ErrCode);
	else
	    TvCtrlPointHandleSubscribeUpdate(es_event->PublisherUrl, es_event->Sid, es_event->TimeOut);
    }
    break;

    case UPNP_EVENT_AUTORENEWAL_FAILED:
    case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
    {
	int TimeOut=default_timeout;
	Upnp_SID newSID;
	int ret;
	struct Upnp_Event_Subscribe *es_event = (struct Upnp_Event_Subscribe * ) Event;

	ret=UpnpSubscribe(ctrlpt_handle, es_event->PublisherUrl,
			  &TimeOut, newSID);
	if (ret==UPNP_E_SUCCESS) {
	    printf("Subscribed to EventURL with SID=%s\n", newSID);
	    TvCtrlPointHandleSubscribeUpdate(es_event->PublisherUrl, newSID, TimeOut);
	} else {
	    printf("Error Subscribing to EventURL -- %d\n", ret);
	}
    }
    break;


    /* ignore these cases, since this is not a device */
    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
    case UPNP_CONTROL_GET_VAR_REQUEST:
    case UPNP_CONTROL_ACTION_REQUEST:
	break;


    }

    return(0);
}



/********************************************************************************
 * TvCtrlPointVerifyTimeouts
 *
 * Description: 
 *       Checks the advertisement and subscription timeouts for each device
 *       and service in the global device list.  If an advertisement expires,
 *       the device is removed from the list.  If an advertisement is about to
 *       expire, a search request is sent for that device.  If a subscription
 *       expires, request a new one.  If a subscription is about to expire,
 *       try to renew it.
 *
 * Parameters:
 *    incr -- The increment to subtract from the timeouts each time the
 *            function is called.
 *
 ********************************************************************************/
void TvCtrlPointVerifyTimeouts(int incr)
{
    struct TvDeviceNode *prevdevnode, *curdevnode;
    int ret;

    pthread_mutex_lock(&DeviceListMutex);

    prevdevnode = curdevnode = GlobalDeviceList;
    while (curdevnode) {
	curdevnode->device.AdvrTimeOut -= incr;
	//printf("Advertisement Timeout: %d\n", curdevnode->device.AdvrTimeOut);
	
	if (curdevnode->device.AdvrTimeOut <= 0) {
	    /* This advertisement has expired, so we should remove the device
	       from the list */
	    if (GlobalDeviceList == curdevnode) 
		GlobalDeviceList = curdevnode->next;
	    prevdevnode->next = curdevnode->next;
	    TvCtrlPointDeleteNode(curdevnode);
	} else {
	    
	    if (curdevnode->device.AdvrTimeOut < 2*incr) {
		/* This advertisement is about to expire, so send
		   out a search request for this device UDN to 
		   try to renew */
		ret = UpnpSearchAsync(ctrlpt_handle, incr, curdevnode->device.UDN, NULL);
		if (ret != UPNP_E_SUCCESS)
		    printf("Error sending search request for Device UDN: %s -- err = %d\n", curdevnode->device.UDN, ret);
		
	    }

	    prevdevnode = curdevnode;

	}	
	
	curdevnode = curdevnode->next;

    }
    pthread_mutex_unlock(&DeviceListMutex);

}








/********************************************************************************
 * TvCtrlPointPrintCommands
 *
 * Description: 
 *       Print the list of valid command line commands to the user
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
void TvCtrlPointPrintCommands() 
{
    int i;
    int numofcmds=sizeof(cmdloop_cmdlist)/sizeof(cmdloop_commands);

    printf("\nValid Commands:\n");
    for (i=0; i<numofcmds; i++) {
	printf("  %-14s %s\n", cmdloop_cmdlist[i].str, cmdloop_cmdlist[i].args);
    }
    printf("\n");
}






/********************************************************************************
 * TvCtrlPointCommandLoop
 *
 * Description: 
 *       Function that receives commands from the user at the command prompt
 *       during the lifetime of the control point, and calls the appropriate
 *       functions for those commands.
 *
 * Parameters:
 *    None
 *
 ********************************************************************************/
void* TvCtrlPointCommandLoop(void *args)
{
    int stoploop=0;
    char cmdline[100];
    char cmd[100];
    char strarg[100];
    int arg1;
    int arg2;
    int cmdnum=-1;
    int numofcmds=sizeof(cmdloop_cmdlist)/sizeof(cmdloop_commands);
    int cmdfound=0;
    int i;
    int invalid_args;
    int arg_val_err=-99999;


    while (!stoploop) {
	cmdfound=0;
	sprintf(cmdline, " ");
	sprintf(cmd, " ");
	arg1 = arg_val_err;
	arg2 = arg_val_err;
	invalid_args=0;

	printf("\n>> ");

	// Get a command line
	fgets(cmdline, 100, stdin);

	/* Get the command name (first string of command line).
	   Also, assume two integer arguments and parse those. */
	sscanf(cmdline, "%s %d %d", cmd, &arg1, &arg2);

	for(i = 0; i < numofcmds; i++) {
	    if (strcasecmp(cmd, cmdloop_cmdlist[i].str) == 0) {
		cmdnum = cmdloop_cmdlist[i].cmdnum;
		cmdfound = 1;
		break;
	    }
	}

	if (cmdfound) {
	    switch(cmdnum) {
	    case PRTHELP:
		TvCtrlPointPrintHelp();
		break;
	    case POWON:
		if (arg1 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendAction(TV_SERVICE_CONTROL, arg1, "PowerOn", NULL, NULL, 0);
		break;
	    case POWOFF:
		if (arg1 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendAction(TV_SERVICE_CONTROL, arg1, "PowerOff", NULL, NULL, 0);
		break;
	    case SETCHAN:
		if (arg1 == arg_val_err  ||  arg2 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendSetChannel(arg1, arg2);
		break;
	    case SETVOL:
		if (arg1 == arg_val_err  ||  arg2 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendSetVolume(arg1, arg2);
		break;
	    case SETCOL:
		if (arg1 == arg_val_err  ||  arg2 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendSetColor(arg1, arg2);
		break;
	    case SETTINT:
		if (arg1 == arg_val_err  ||  arg2 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendSetTint(arg1, arg2);
		break;
	    case SETCONT:
		if (arg1 == arg_val_err  ||  arg2 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendSetContrast(arg1, arg2);
		break;
	    case SETBRT:
		if (arg1 == arg_val_err  ||  arg2 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendSetBrightness(arg1, arg2);
		break;
	    case CTRLACTION:
		/* re-parse commandline since second arg is string */
		sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (arg1 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendAction(TV_SERVICE_CONTROL, arg1, strarg, NULL, NULL, 0);
		break;
	    case PICTACTION:
		/* re-parse commandline since second arg is string */
		sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (arg1 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointSendAction(TV_SERVICE_PICTURE, arg1, strarg, NULL, NULL, 0);
		break;
	    case CTRLGETVAR:
		/* re-parse commandline since second arg is string */
		sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (arg1 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointGetVar(TV_SERVICE_CONTROL, arg1, strarg);
		break;
	    case PICTGETVAR:
		/* re-parse commandline since second arg is string */
		sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (arg1 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointGetVar(TV_SERVICE_PICTURE, arg1, strarg);
		break;
	    case PRTDEV:
		if (arg1 == arg_val_err)
		    invalid_args = 1;
		else
		    TvCtrlPointPrintDevice(arg1);
		break;
	    case LSTDEV:
		TvCtrlPointPrintList();
		break;
	    case REFRESH:
		TvCtrlPointRefresh();
		break;
	    case EXITCMD:
		printf("Shutting down...\n");
		UpnpUnRegisterClient(ctrlpt_handle);
		UpnpFinish();
		exit(0);
		break;
	    default:
		printf("command not yet implemented: %s\n", cmd);
		TvCtrlPointPrintCommands();
	    }

	    if (invalid_args) {
		printf("Invalid args in command: %s\n", cmd);
		TvCtrlPointPrintCommands();
	    }
	} else {
	    printf("Unknown command: %s\n", cmd);
	    TvCtrlPointPrintCommands();
	}

    }

    return(NULL);
}



/********************************************************************************
 * TvCtrlPointTimerLoop
 *
 * Description: 
 *       Function that runs in its own thread and monitors advertisement
 *       and subscription timeouts for devices in the global device list.
 *
 * Parameters:
 *    None
 *
 ********************************************************************************/
void* TvCtrlPointTimerLoop(void *args)
{

    int incr = 30; // how often to verify the timeouts

    while (1) {
	sleep(incr);
	TvCtrlPointVerifyTimeouts(incr);
    }
    
}





int main(int argc, char** argv)
{

    int ret=1;
    pthread_t timer_thread, cmdloop_thread;
    int code;
    int port;
    char *ip_address;
    int sig;
    sigset_t sigs_to_catch;

    if (argc!=3) {
	printf("wrong number of arguments\n Usage: %s ipaddress port\n", argv[0]);
	printf("\tipaddress:     IP address of the device (must match desc. doc)\n");
	printf("\t\te.g.: 192.168.0.2\n");
	printf("\tport:          Port number to use for receiving UPnP messages (must match desc. doc)\n");
	printf("\t\te.g.: 5432\n");
	exit(1);
    }

    ip_address=argv[1];  
    sscanf(argv[2],"%d",&port);


    printf("Intializing UPnP \n\twith ipaddress=%s port=%d \n", ip_address, port);
    if ((ret = UpnpInit(ip_address, port))) {
	printf("Error with UpnpInit -- %d\n", ret);
	UpnpFinish();
	exit(1);
    }
   
    printf("UPnP Initialized\n");
    printf("Registering the Control Point\n");
    if ((ret = UpnpRegisterClient(TvCtrlPointCallbackEventHandler, &ctrlpt_handle, &ctrlpt_handle)))
    {
	printf("Error registering control point : %d\n", ret);
	UpnpFinish();
	exit(1);
    }
    printf("Control Point Registered\n");

    TvCtrlPointRefresh();

    // start a command loop thread
    code = pthread_create( &cmdloop_thread, NULL, TvCtrlPointCommandLoop,
			   NULL );

    // start a timer thread
    code = pthread_create( &timer_thread, NULL, TvCtrlPointTimerLoop,
			   NULL );


    /* Catch Ctrl-C and properly shutdown */
    sigemptyset(&sigs_to_catch);
    sigaddset(&sigs_to_catch, SIGINT);
    sigwait(&sigs_to_catch, &sig);
    printf("Shutting down on signal %d...\n", sig);
    UpnpUnRegisterClient(ctrlpt_handle);
    UpnpFinish();

    exit(0);
}













