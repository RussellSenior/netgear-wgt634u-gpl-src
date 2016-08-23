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
// $Date: 2001/06/15 00:21:33 $
//

#include "upnp_tv_device.h"



char TvDeviceType[] = "urn:schemas-upnp-org:device:tvdevice:1";
char *TvServiceType[] = {"urn:schemas-upnp-org:service:tvcontrol:1", "urn:schemas-upnp-org:service:tvpicture:1"};

/* Global arrays for storing Tv Control Service
   variable names, values, and defaults */
char *tvc_varname[] = {"Power","Channel","Volume"};
char tvc_varval[TV_CONTROL_VARCOUNT][TV_MAX_VAL_LEN];
char *tvc_varval_def[] = {"0", "1", "5"};

/* Global arrays for storing Tv Picture Service
   variable names, values, and defaults */
char *tvp_varname[] = {"Color","Tint","Contrast","Brightness"};
char tvp_varval[TV_PICTURE_VARCOUNT][TV_MAX_VAL_LEN];
char *tvp_varval_def[] = {"5","5","5","5"};

/* The amount of time before advertisements
   will expire */
int default_advr_expire=1800;

/* Global structure for storing the state table for this device */
struct TvService tv_service_table[2];


UpnpDevice_Handle device_handle=-1;


/* Mutex for protecting the global state table data
   in a multi-threaded, asynchronous environment.
   All functions should lock this mutex before reading
   or writing the state table data. */
pthread_mutex_t TVDevMutex= PTHREAD_MUTEX_INITIALIZER;




/********************************************************************************
 * TvDeviceStateTableInit
 *
 * Description: 
 *       Initialize the device state table for 
 * 	 this TvDevice, pulling identifier info
 *       from the description Document.  Note that 
 *       knowledge of the service description is
 *       assumed.  State table variables and default
 *       values are currently hard coded in this file
 *       rather than being read from service description
 *       documents.
 *
 * Parameters:
 *   DescDocURL -- The description document URL
 *
 ********************************************************************************/
int TvDeviceStateTableInit(char* DescDocURL) 
{
    Upnp_Document DescDoc=NULL;
    int ret = UPNP_E_SUCCESS;
    char *servid_ctrl=NULL, *evnturl_ctrl=NULL, *ctrlurl_ctrl=NULL;
    char *servid_pict=NULL, *evnturl_pict=NULL, *ctrlurl_pict=NULL;
    char *udn=NULL;
    int i;

    if (UpnpDownloadXmlDoc(DescDocURL, &DescDoc) != UPNP_E_SUCCESS) {
	printf("TvDeviceStateTableInit -- Error Parsing %s\n", DescDocURL);
	ret = UPNP_E_INVALID_DESC;
    }

    /* Find the Tv Control Service identifiers */
    if (!SampleUtil_FindAndParseService(DescDoc, DescDocURL, TvServiceType[TV_SERVICE_CONTROL], &servid_ctrl, &evnturl_ctrl, &ctrlurl_ctrl)) {
	printf("TvDeviceStateTableInit -- Error: Could not find Service: %s\n", TvServiceType[TV_SERVICE_CONTROL]);

	if (servid_ctrl) free(servid_ctrl);
	if (evnturl_ctrl) free(evnturl_ctrl);
	if (ctrlurl_ctrl) free(ctrlurl_ctrl);

	return(UPNP_E_INVALID_DESC);
    }

    /* Find the Tv Picture Service identifiers */
    if (!SampleUtil_FindAndParseService(DescDoc, DescDocURL, TvServiceType[TV_SERVICE_PICTURE], &servid_pict, &evnturl_pict, &ctrlurl_pict)) {
	printf("TvDeviceStateTableInit -- Error: Could not find Service: %s\n", TvServiceType[TV_SERVICE_PICTURE]);
      
	if (servid_ctrl) free(servid_ctrl);
	if (evnturl_ctrl) free(evnturl_ctrl);
	if (ctrlurl_ctrl) free(ctrlurl_ctrl);
	if (servid_pict) free(servid_pict);
	if (evnturl_pict) free(evnturl_pict);
	if (ctrlurl_pict) free(ctrlurl_pict);

	return(UPNP_E_INVALID_DESC);
    }

    udn = SampleUtil_GetFirstDocumentItem(DescDoc, "UDN");


    strcpy(tv_service_table[TV_SERVICE_CONTROL].UDN, udn);
    strcpy(tv_service_table[TV_SERVICE_CONTROL].ServiceId, servid_ctrl);
    strcpy(tv_service_table[TV_SERVICE_CONTROL].ServiceType, TvServiceType[TV_SERVICE_CONTROL]);
    tv_service_table[TV_SERVICE_CONTROL].VariableCount=TV_CONTROL_VARCOUNT;
    for (i=0; i<tv_service_table[TV_SERVICE_CONTROL].VariableCount; i++) {
	tv_service_table[TV_SERVICE_CONTROL].VariableName[i] = tvc_varname[i];
	tv_service_table[TV_SERVICE_CONTROL].VariableStrVal[i] = tvc_varval[i];
	strcpy(tv_service_table[TV_SERVICE_CONTROL].VariableStrVal[i], tvc_varval_def[i]);
    }

    strcpy(tv_service_table[TV_SERVICE_PICTURE].UDN, udn);
    strcpy(tv_service_table[TV_SERVICE_PICTURE].ServiceId, servid_pict);
    strcpy(tv_service_table[TV_SERVICE_PICTURE].ServiceType, TvServiceType[TV_SERVICE_PICTURE]);
    tv_service_table[TV_SERVICE_PICTURE].VariableCount=TV_PICTURE_VARCOUNT;
    for (i=0; i<tv_service_table[TV_SERVICE_PICTURE].VariableCount; i++) {
	tv_service_table[TV_SERVICE_PICTURE].VariableName[i] = tvp_varname[i];
	tv_service_table[TV_SERVICE_PICTURE].VariableStrVal[i] = tvp_varval[i];
	strcpy(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal[i], tvp_varval_def[i]);
    }

    if (udn) free(udn);
    if (servid_ctrl) free(servid_ctrl);
    if (evnturl_ctrl) free(evnturl_ctrl);
    if (ctrlurl_ctrl) free(ctrlurl_ctrl);
    if (servid_pict) free(servid_pict);
    if (evnturl_pict) free(evnturl_pict);
    if (ctrlurl_pict) free(ctrlurl_pict);

    return(ret);
}







/********************************************************************************
 * TvDeviceHandleSubscriptionRequest
 *
 * Description: 
 *       Called during a subscription request callback.  If the
 *       subscription request is for this device and either its
 *       control service or picture service, then accept it.
 *
 * Parameters:
 *   sr_event -- The subscription request event structure
 *
 ********************************************************************************/
int TvDeviceHandleSubscriptionRequest(struct Upnp_Subscription_Request *sr_event) 
{
    int i,j;
    Upnp_Document PropSet;

    pthread_mutex_lock(&TVDevMutex);
  
    for (i=0; i<TV_SERVICE_SERVCOUNT; i++) {
	if ((strcmp(sr_event->UDN,tv_service_table[i].UDN) == 0) &&
	    (strcmp(sr_event->ServiceId,tv_service_table[i].ServiceId) == 0)) {



	    /* This is a request for the TvDevice Control Service */
	   /* UpnpAcceptSubscription(device_handle,
				   sr_event->UDN,
				   sr_event->ServiceId,
				   (char **)tv_service_table[i].VariableName,
				   (char **)tv_service_table[i].VariableStrVal,
				   tv_service_table[i].VariableCount,
				   sr_event->Sid);*/


            //Another way of doing the same thing.
            PropSet = NULL;

            for (j=0; j< tv_service_table[i].VariableCount; j++)
            UpnpAddToPropertySet(&PropSet, tv_service_table[i].VariableName[j],tv_service_table[i].VariableStrVal[j]);

            UpnpAcceptSubscriptionExt(device_handle, sr_event->UDN, sr_event->ServiceId,PropSet,sr_event->Sid);
            UpnpDocument_free(PropSet);

	}
    }

    pthread_mutex_unlock(&TVDevMutex);	

    return(1);
}





/********************************************************************************
 * TvDeviceHandleGetVarRequest
 *
 * Description: 
 *       Called during a get variable request callback.  If the
 *       request is for this device and either its control service
 *       or picture service, then respond with the variable value.
 *
 * Parameters:
 *   cgv_event -- The control get variable request event structure
 *
 ********************************************************************************/
int TvDeviceHandleGetVarRequest(struct Upnp_State_Var_Request *cgv_event) 
{
    int i, j;
    int getvar_succeeded = 0;

    cgv_event->CurrentVal = NULL;
  
    pthread_mutex_lock(&TVDevMutex);
  
    for (i=0; i<TV_SERVICE_SERVCOUNT; i++) {
	if ((strcmp(cgv_event->DevUDN,tv_service_table[i].UDN)==0) &&
	    (strcmp(cgv_event->ServiceID,tv_service_table[i].ServiceId)==0)) {
	    /* Request for variable in the TvDevice Service */
	    for (j=0; j< tv_service_table[i].VariableCount; j++) {
		if (strcmp(cgv_event->StateVarName, tv_service_table[i].VariableName[j])==0) {
		    getvar_succeeded = 1;
		    cgv_event->CurrentVal = (Upnp_DOMString) malloc(sizeof(tv_service_table[i].VariableStrVal[j]));
		    strcpy(cgv_event->CurrentVal, tv_service_table[i].VariableStrVal[j]);
		    break;
		}
	    } 
	}
    }
  
    if (getvar_succeeded) {
	cgv_event->ErrCode = UPNP_E_SUCCESS;
    } else {
	printf("Error in UPNP_CONTROL_GET_VAR_REQUEST callback:\n"); 
	printf("   Unknown variable name = %s\n",  cgv_event->StateVarName); 
	cgv_event->ErrCode = 404;
	strcpy(cgv_event->ErrStr, "Invalid Variable");
    }

    pthread_mutex_unlock(&TVDevMutex);
  
    return(cgv_event->ErrCode == UPNP_E_SUCCESS);
}


/********************************************************************************
 * TvDeviceHandleActionRequest
 *
 * Description: 
 *       Called during an action request callback.  If the
 *       request is for this device and either its control service
 *       or picture service, then perform the action and respond.
 *
 * Parameters:
 *   ca_event -- The control action request event structure
 *
 ********************************************************************************/
int TvDeviceHandleActionRequest(struct Upnp_Action_Request *ca_event) 
{
    char result_str[500];
    char service_type[500];
    char *value=NULL;

    /* Defaults if action not found */
    int action_succeeded = -1;
    int err=401;

    ca_event->ErrCode = 0;
    ca_event->ActionResult = NULL;
  
    if ((strcmp(ca_event->DevUDN,tv_service_table[TV_SERVICE_CONTROL].UDN)==0) &&
	(strcmp(ca_event->ServiceID,tv_service_table[TV_SERVICE_CONTROL].ServiceId)==0)) {
    
	/* Request for action in the TvDevice Control Service */

	strcpy(service_type, tv_service_table[TV_SERVICE_CONTROL].ServiceType);
    
	if (strcmp(ca_event->ActionName, "PowerOn") == 0) {
	    action_succeeded = TvDevicePowerOn();
	} else if (strcmp(ca_event->ActionName, "PowerOff") == 0) {
	    action_succeeded = TvDevicePowerOff();
	} else if (strcmp(ca_event->ActionName, "SetChannel") == 0) {
	    if ((value = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "Channel"))) {
		action_succeeded = TvDeviceSetChannel(atoi(value));
	    } else {
		// invalid args error
		err = 402;
		action_succeeded = 0;
	    }
	} else if (strcmp(ca_event->ActionName, "IncreaseChannel") == 0) {
	    action_succeeded = TvDeviceIncrChannel(1);
	} else if (strcmp(ca_event->ActionName, "DecreaseChannel") == 0) {
	    action_succeeded = TvDeviceIncrChannel(-1);
	} else if (strcmp(ca_event->ActionName, "SetVolume") == 0) {
	    if ((value = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "Volume"))) {
		action_succeeded = TvDeviceSetVolume(atoi(value));
	    } else {
		// invalid args error
		err = 402;
		action_succeeded = 0;
	    }
	} else if (strcmp(ca_event->ActionName, "IncreaseVolume") == 0) {
	    action_succeeded = TvDeviceIncrVolume(1);
	} else if (strcmp(ca_event->ActionName, "DecreaseVolume") == 0) {
	    action_succeeded = TvDeviceIncrVolume(-1);
	}
			
    } else if ((strcmp(ca_event->DevUDN,tv_service_table[TV_SERVICE_PICTURE].UDN)==0) &&
	       (strcmp(ca_event->ServiceID,tv_service_table[TV_SERVICE_PICTURE].ServiceId)==0)) {
    
	/* Request for action in the TvDevice Picture Service */

	strcpy(service_type, tv_service_table[TV_SERVICE_PICTURE].ServiceType);
    
	if (strcmp(ca_event->ActionName, "SetColor") == 0) {
	    if ((value = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "Color"))) {
		action_succeeded = TvDeviceSetColor(atoi(value));
	    } else {
		// invalid args error
		err = 402;
		action_succeeded = 0;
	    }
	} else if (strcmp(ca_event->ActionName, "IncreaseColor") == 0) {
	    action_succeeded = TvDeviceIncrColor(1);
	} else if (strcmp(ca_event->ActionName, "DecreaseColor") == 0) {
	    action_succeeded = TvDeviceIncrColor(-1);
	} else if (strcmp(ca_event->ActionName, "SetTint") == 0) {
	    if ((value = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "Tint"))) {
		action_succeeded = TvDeviceSetTint(atoi(value));
	    } else {
		// invalid args error
		err = 402;
		action_succeeded = 0;
	    }
	} else if (strcmp(ca_event->ActionName, "IncreaseTint") == 0) {
	    action_succeeded = TvDeviceIncrTint(1);
	} else if (strcmp(ca_event->ActionName, "DecreaseTint") == 0) {
	    action_succeeded = TvDeviceIncrTint(-1);
	} else if (strcmp(ca_event->ActionName, "SetContrast") == 0) {
	    if ((value = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "Contrast"))) {
		action_succeeded = TvDeviceSetContrast(atoi(value));
	    } else {
		// invalid args error
		err = 402;
		action_succeeded = 0;
	    }
	} else if (strcmp(ca_event->ActionName, "IncreaseContrast") == 0) {
	    action_succeeded = TvDeviceIncrContrast(1);
	} else if (strcmp(ca_event->ActionName, "DecreaseContrast") == 0) {
	    action_succeeded = TvDeviceIncrContrast(-1);
	} else if (strcmp(ca_event->ActionName, "SetBrightness") == 0) {
	    if ((value = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "Brightness"))) {
		action_succeeded = TvDeviceSetBrightness(atoi(value));
	    } else {
		// invalid args error
		err = 402;
		action_succeeded = 0;
	    }
	} else if (strcmp(ca_event->ActionName, "IncreaseBrightness") == 0) {
	    action_succeeded = TvDeviceIncrBrightness(1);
	} else if (strcmp(ca_event->ActionName, "DecreaseBrightness") == 0) {
	    action_succeeded = TvDeviceIncrBrightness(-1);
	}
    
    } else {
	printf("Error in UPNP_CONTROL_ACTION_REQUEST callback:\n"); 
	printf("   Unknown UDN = %s or ServiceId = %s\n",  ca_event->DevUDN, ca_event->ServiceID ); 
    }
  
    if (action_succeeded > 0) {
	ca_event->ErrCode = UPNP_E_SUCCESS;
    
	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
		ca_event->ActionName,
		service_type,
		ca_event->ActionName);
	ca_event->ActionResult = UpnpParse_Buffer(result_str);
    } else if (action_succeeded == -1) {
	printf("Error in UPNP_CONTROL_ACTION_REQUEST callback:\n"); 
	printf("   Unknown ActionName = %s\n",  ca_event->ActionName); 

	ca_event->ErrCode = 401;
	strcpy(ca_event->ErrStr, "Invalid Action");
    	ca_event->ActionResult = NULL;
    
    } else {
	printf("Error in UPNP_CONTROL_ACTION_REQUEST callback:\n"); 
	printf("   Failure while running %s\n",  ca_event->ActionName); 

	ca_event->ErrCode = err;
	strcpy(ca_event->ErrStr, "Invalid Args");
    	ca_event->ActionResult = NULL;
    }

    if (value) free(value);

    return(ca_event->ErrCode);
}






/********************************************************************************
 * TvDeviceSetServiceTableVar
 *
 * Description: 
 *       Update the TvDevice service state table, and notify all subscribed 
 *       control points of the updated state.  Note that since this function
 *       blocks on the mutex TVDevMutex, to avoid a hang this function should 
 *       not be called within any other function that currently has this mutex 
 *       locked.
 *
 * Parameters:
 *   service -- The service number (TV_SERVICE_CONTROL or TV_SERVICE_PICTURE)
 *   variable -- The variable number (TV_CONTROL_POWER, TV_CONTROL_CHANNEL,
 *                   TV_CONTROL_VOLUME, TV_PICTURE_COLOR, TV_PICTURE_TINT,
 *                   TV_PICTURE_CONTRAST, or TV_PICTURE_BRIGHTNESS)
 *   value -- The string representation of the new value
 *
 ********************************************************************************/
int TvDeviceSetServiceTableVar(unsigned int service, unsigned int variable, char *value) {
 Upnp_Document  PropSet= NULL;
    if ((service >= TV_SERVICE_SERVCOUNT) || (variable >= tv_service_table[service].VariableCount) 
	|| (strlen(value) >= TV_MAX_VAL_LEN)) {
	
	return(0);
    }

    pthread_mutex_lock(&TVDevMutex);
    
    strcpy(tv_service_table[service].VariableStrVal[variable], value);

    // Using the first utility api
    /*UpnpAddToPropertySet(&PropSet, tv_service_table[service].VariableName[variable], tv_service_table[service].VariableStrVal[variable]);
    UpnpNotifyExt(device_handle, tv_service_table[service].UDN, tv_service_table[service].ServiceId,PropSet);
    UpnpDocument_free(PropSet); */


    //Using second utility API
    PropSet= UpnpCreatePropertySet(1,tv_service_table[service].VariableName[variable], tv_service_table[service].VariableStrVal[variable]);
    UpnpNotifyExt(device_handle, tv_service_table[service].UDN, tv_service_table[service].ServiceId,PropSet);
    UpnpDocument_free(PropSet);


    /* Send updated power setting notification
       to subscribed control points */
   /* UpnpNotify(device_handle,
	       tv_service_table[service].UDN,
	       tv_service_table[service].ServiceId,
	       (char **)&tv_service_table[service].VariableName[variable],
	       (char **)&tv_service_table[service].VariableStrVal[variable],
	       1); */

    pthread_mutex_unlock(&TVDevMutex);

    return(1);

}


/********************************************************************************
 * TvDeviceSetPower
 *
 * Description: 
 *       Turn the power on/off, update the TvDevice control service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 *
 * Parameters:
 *   on -- If 1, turn power on.  If 0, turn power off.
 *
 ********************************************************************************/
int TvDeviceSetPower(int on) 
{
    char value[TV_MAX_VAL_LEN];
    int ret=0;

    if (on != 0  &&  on != 1) {
	printf("error: can't set power to value %d\n", on);
	return(0);
    }

    /* Vendor-specific code to turn the power on/off goes here */

    sprintf(value, "%d", on);
    ret = TvDeviceSetServiceTableVar(TV_SERVICE_CONTROL, TV_CONTROL_POWER, value);

    return(ret);
}

/********************************************************************************
 * TvDevicePowerOn
 *
 * Description: 
 *       Turn the power on.
 *
 * Parameters:
 *
 ********************************************************************************/
int TvDevicePowerOn() 
{
    return(TvDeviceSetPower(1));
}

/********************************************************************************
 * TvDevicePowerOff
 *
 * Description: 
 *       Turn the power off.
 *
 * Parameters:
 *
 ********************************************************************************/
int TvDevicePowerOff() 
{
    return(TvDeviceSetPower(0));
}

/********************************************************************************
 * TvDeviceSetChannel
 *
 * Description: 
 *       Change the channel, update the TvDevice control service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 *
 * Parameters:
 *   channel -- The channel number to change to.
 *
 ********************************************************************************/
int TvDeviceSetChannel(int channel) 
{
    char value[TV_MAX_VAL_LEN];
    int ret=0;

    if (channel < 1  ||  channel > 100) {
	printf("error: can't change to channel %d\n", channel);
	return(0);
    }

    /* Vendor-specific code to set the channel goes here */

    sprintf(value, "%d", channel);
    ret = TvDeviceSetServiceTableVar(TV_SERVICE_CONTROL, TV_CONTROL_CHANNEL, value);

    return(ret);
}

/********************************************************************************
 * TvDeviceIncrChannel
 *
 * Description: 
 *       Increment the channel.  Read the current channel from the state
 *       table, add the increment, and then change the channel.
 *
 * Parameters:
 *   incr -- The increment by which to change the channel.
 *
 ********************************************************************************/
int TvDeviceIncrChannel(int incr) 
{
    int curchannel, newchannel;
    int ret;

    pthread_mutex_lock(&TVDevMutex);
    curchannel = atoi(tv_service_table[TV_SERVICE_CONTROL].VariableStrVal[TV_CONTROL_CHANNEL]);
    pthread_mutex_unlock(&TVDevMutex);

    newchannel = curchannel + incr;
    ret = TvDeviceSetChannel(newchannel);

    return(ret);
}

/********************************************************************************
 * TvDeviceSetVolume
 *
 * Description: 
 *       Change the volume, update the TvDevice control service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 *
 * Parameters:
 *   volume -- The volume value to change to.
 *
 ********************************************************************************/
int TvDeviceSetVolume(int volume) 
{
    char value[TV_MAX_VAL_LEN];
    int ret=0;

    if (volume < 0  ||  volume > 10) {
	printf("error: can't change to volume %d\n", volume);
	return(0);
    }

    /* Vendor-specific code to set the volume goes here */

    sprintf(value, "%d", volume);
    ret = TvDeviceSetServiceTableVar(TV_SERVICE_CONTROL, TV_CONTROL_VOLUME, value);

    return(ret);
}

/********************************************************************************
 * TvDeviceIncrVolume
 *
 * Description: 
 *       Increment the volume.  Read the current volume from the state
 *       table, add the increment, and then change the volume.
 *
 * Parameters:
 *   incr -- The increment by which to change the volume.
 *
 ********************************************************************************/
int TvDeviceIncrVolume(int incr) 
{
    int curvolume, newvolume;
    int ret;

    pthread_mutex_lock(&TVDevMutex);
    curvolume = atoi(tv_service_table[TV_SERVICE_CONTROL].VariableStrVal[TV_CONTROL_VOLUME]);
    pthread_mutex_unlock(&TVDevMutex);

    newvolume = curvolume + incr;
    ret = TvDeviceSetVolume(newvolume);

    return(ret);
}

/********************************************************************************
 * TvDeviceSetColor
 *
 * Description: 
 *       Change the color, update the TvDevice picture service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 *
 * Parameters:
 *   color -- The color value to change to.
 *
 ********************************************************************************/
int TvDeviceSetColor(int color) 
{
    char value[TV_MAX_VAL_LEN];
    int ret=0;

    if (color < 1  ||  color > 10) {
	printf("error: can't change to color %d\n", color);
	return(0);
    }

    /* Vendor-specific code to set the color goes here */

    sprintf(value, "%d", color);
    ret = TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE, TV_PICTURE_COLOR, value);

    return(ret);
}

/********************************************************************************
 * TvDeviceIncrColor
 *
 * Description: 
 *       Increment the color.  Read the current color from the state
 *       table, add the increment, and then change the color.
 *
 * Parameters:
 *   incr -- The increment by which to change the color.
 *
 ********************************************************************************/
int TvDeviceIncrColor(int incr) 
{
    int curcolor, newcolor;
    int ret;

    pthread_mutex_lock(&TVDevMutex);
    curcolor = atoi(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal[TV_PICTURE_COLOR]);
    pthread_mutex_unlock(&TVDevMutex);

    newcolor = curcolor + incr;
    ret = TvDeviceSetColor(newcolor);

    return(ret);
}

/********************************************************************************
 * TvDeviceSetTint
 *
 * Description: 
 *       Change the tint, update the TvDevice picture service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 *
 * Parameters:
 *   tint -- The tint value to change to.
 *
 ********************************************************************************/
int TvDeviceSetTint(int tint) 
{
    char value[TV_MAX_VAL_LEN];
    int ret=0;

    if (tint < 1  ||  tint > 10) {
	printf("error: can't change to tint %d\n", tint);
	return(0);
    }

    /* Vendor-specific code to set the tint goes here */


    sprintf(value, "%d", tint);
    ret = TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE, TV_PICTURE_TINT, value);

    return(ret);
}

/********************************************************************************
 * TvDeviceIncrTint
 *
 * Description: 
 *       Increment the tint.  Read the current tint from the state
 *       table, add the increment, and then change the tint.
 *
 * Parameters:
 *   incr -- The increment by which to change the tint.
 *
 ********************************************************************************/
int TvDeviceIncrTint(int incr) 
{
    int curtint, newtint;
    int ret;

    pthread_mutex_lock(&TVDevMutex);
    curtint = atoi(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal[TV_PICTURE_TINT]);
    pthread_mutex_unlock(&TVDevMutex);

    newtint = curtint + incr;
    ret = TvDeviceSetTint(newtint);

    return(ret);
}

/********************************************************************************
 * TvDeviceSetContrast
 *
 * Description: 
 *       Change the contrast, update the TvDevice picture service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 *
 * Parameters:
 *   contrast -- The contrast value to change to.
 *
 ********************************************************************************/
int TvDeviceSetContrast(int contrast) 
{
    char value[TV_MAX_VAL_LEN];
    int ret=0;

    if (contrast < 1  ||  contrast > 10) {
	printf("error: can't change to contrast %d\n", contrast);
	return(0);
    }

    /* Vendor-specific code to set the contrast goes here */

    sprintf(value, "%d", contrast);
    ret = TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE, TV_PICTURE_CONTRAST, value);

    return(ret);
}

/********************************************************************************
 * TvDeviceIncrContrast
 *
 * Description: 
 *       Increment the contrast.  Read the current contrast from the state
 *       table, add the increment, and then change the contrast.
 *
 * Parameters:
 *   incr -- The increment by which to change the contrast.
 *
 ********************************************************************************/
int TvDeviceIncrContrast(int incr) 
{
    int curcontrast, newcontrast;
    int ret;

    /* Get the current contrast value from the
       state table */
    pthread_mutex_lock(&TVDevMutex);
    curcontrast = atoi(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal[TV_PICTURE_CONTRAST]);
    pthread_mutex_unlock(&TVDevMutex);

    newcontrast = curcontrast + incr;
    ret = TvDeviceSetContrast(newcontrast);

    return(ret);
}


/********************************************************************************
 * TvDeviceSetBrightness
 *
 * Description: 
 *       Change the brightness, update the TvDevice picture service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 *
 * Parameters:
 *   brightness -- The brightness value to change to.
 *
 ********************************************************************************/
int TvDeviceSetBrightness(int brightness) 
{
    char value[TV_MAX_VAL_LEN];
    int ret=0;

    if (brightness < 1  ||  brightness > 10) {
	printf("error: can't change to brightness %d\n", brightness);
	return(0);
    }

    /* Vendor-specific code to set the brightness goes here */

    sprintf(value, "%d", brightness);
    ret = TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE, TV_PICTURE_BRIGHTNESS, value);

    return(ret);
}

/********************************************************************************
 * TvDeviceIncrBrightness
 *
 * Description: 
 *       Increment the brightness.  Read the current brightness from the state
 *       table, add the increment, and then change the brightness.
 *
 * Parameters:
 *   incr -- The increment by which to change the brightness.
 *
 ********************************************************************************/
int TvDeviceIncrBrightness(int incr) 
{
    int curbrightness, newbrightness;
    int ret;

    pthread_mutex_lock(&TVDevMutex);
    curbrightness = atoi(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal[TV_PICTURE_BRIGHTNESS]);
    pthread_mutex_unlock(&TVDevMutex);

    newbrightness = curbrightness + incr;
    ret = TvDeviceSetBrightness(newbrightness);

    return(ret);
}


















/********************************************************************************
 * TvDeviceCallbackEventHandler
 *
 * Description: 
 *       The callback handler registered with the SDK while registering
 *       root device or sending a search request.  Detects the type of 
 *       callback, and passes the request on to the appropriate procedure.
 *
 * Parameters:
 *   EventType -- The type of callback event
 *   Event -- Data structure containing event data
 *   Cookie -- Optional data specified during callback registration
 *
 ********************************************************************************/
int TvDeviceCallbackEventHandler(Upnp_EventType EventType, 
			 void *Event, 
			 void *Cookie)
{
    /* Print a summary of the event received */
    SampleUtil_PrintEvent(EventType, Event);
  
    switch ( EventType) {

    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
	TvDeviceHandleSubscriptionRequest(
	    (struct Upnp_Subscription_Request *) Event);
	break;

    case UPNP_CONTROL_GET_VAR_REQUEST:
	TvDeviceHandleGetVarRequest(
	    (struct Upnp_State_Var_Request *) Event);
	break;

    case UPNP_CONTROL_ACTION_REQUEST:
	TvDeviceHandleActionRequest(
	    (struct Upnp_Action_Request *) Event);
	break;

	/* ignore these cases, since this is not a control point */
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    case UPNP_DISCOVERY_SEARCH_TIMEOUT:
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    case UPNP_CONTROL_ACTION_COMPLETE:
    case UPNP_CONTROL_GET_VAR_COMPLETE:
    case UPNP_EVENT_RECEIVED:
    case UPNP_EVENT_RENEWAL_COMPLETE:
    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
	    break;
	    
    default:
	    printf("Error in TvDeviceCallbackEventHandler: unknown event type %d\n", EventType);
    }

    return(0);
}



/********************************************************************************
 * TvDeviceCommandLoop
 *
 * Description: 
 *       Function that receives commands from the user at the command prompt
 *       during the lifetime of the device, and calls the appropriate
 *       functions for those commands.
 *
 * Parameters:
 *    None
 *
 ********************************************************************************/
void* TvDeviceCommandLoop(void *args)
{
    int stoploop=0;
    char cmdline[100];
    char cmd[100];

    while (!stoploop) {
	sprintf(cmdline, " ");
	sprintf(cmd, " ");

	printf("\n>> ");

	// Get a command line
	fgets(cmdline, 100, stdin);

	sscanf(cmdline, "%s", cmd);

	if (strcasecmp(cmd, "exit") == 0) {
	    printf("Shutting down...\n");
	    UpnpUnRegisterRootDevice(device_handle);
	    UpnpFinish();
	    exit(0);
	} else {
	    printf("\n   Unknown command: %s\n\n", cmd);
	    printf("   Valid Commands:\n");
	    printf("     Exit\n\n");
	}

    }

    return NULL;
}



int main(int argc, char** argv)
{

    int ret=1;
    int port;
    char *ip_address=NULL, *desc_doc_name=NULL, *web_dir_path=NULL;
    char desc_doc_url[200];
    pthread_t cmdloop_thread;
    int code;
    int sig;
    sigset_t sigs_to_catch;

    if (argc!=5) {
	printf("wrong number of arguments\n Usage: %s ipaddress port desc_doc_name web_dir_path\n", argv[0]);
	printf("\tipaddress:     IP address of the device (must match desc. doc)\n");
	printf("\t\te.g.: 192.168.0.4\n");
	printf("\tport:          Port number to use for receiving UPnP messages (must match desc. doc)\n");
	printf("\t\te.g.: 5431\n");
	printf("\tdesc_doc_name: name of device description document\n");
	printf("\t\te.g.: tvdevicedesc.xml\n");
	printf("\tweb_dir_path: Filesystem path where web files related to the device are stored\n");
	printf("\t\te.g.: /upnp/sample/tvdevice/web\n");
	exit(1);
    }

    ip_address=argv[1];  
    sscanf(argv[2],"%d",&port);
    desc_doc_name=argv[3];  
    web_dir_path=argv[4];  

    sprintf(desc_doc_url, "http://%s:%d/%s", ip_address, port, desc_doc_name);

    printf("Intializing UPnP \n\twith desc_doc_url=%s\n\t", desc_doc_url);
    printf("\t     ipaddress=%s port=%d\n", ip_address, port);
    printf("\t     web_dir_path=%s\n", web_dir_path);
    if ((ret = UpnpInit(ip_address, port)) != UPNP_E_SUCCESS) {
	printf("Error with UpnpInit -- %d\n", ret);
	UpnpFinish();
	exit(1);
    }
    printf("UPnP Initialized\n");
   
    printf("Specifying the webserver root directory -- %s\n", web_dir_path);
    if ((ret = UpnpSetWebServerRootDir(web_dir_path)) != UPNP_E_SUCCESS) {
	printf("Error specifying webserver root directory -- %s: %d\n", web_dir_path, ret);
	UpnpFinish();
	exit(1);
    }

    printf("Registering the RootDevice\n");
    if ((ret = UpnpRegisterRootDevice(desc_doc_url, TvDeviceCallbackEventHandler, &device_handle, &device_handle)) != UPNP_E_SUCCESS) {
	printf("Error registering the rootdevice : %d\n", ret);
	UpnpFinish();
	exit(1);
    } else {
	printf("RootDevice Registered\n");
     
	printf("Initializing State Table\n");
	TvDeviceStateTableInit(desc_doc_url);
	printf("State Table Initialized\n");

	if ((ret = UpnpSendAdvertisement(device_handle, default_advr_expire)) != UPNP_E_SUCCESS) {
	    printf("Error sending advertisements : %d\n", ret);
	    UpnpFinish();
	    exit(1);
	}

	printf("Advertisements Sent\n");
    }
   

    /* start a command loop thread */
    code = pthread_create( &cmdloop_thread, NULL, TvDeviceCommandLoop,
			   NULL );



    /* Catch Ctrl-C and properly shutdown */
    sigemptyset(&sigs_to_catch);
    sigaddset(&sigs_to_catch, SIGINT);
    sigwait(&sigs_to_catch, &sig);
    printf("Shutting down on signal %d...\n", sig);
    UpnpUnRegisterRootDevice(device_handle);
    UpnpFinish();

    exit(0);
}



