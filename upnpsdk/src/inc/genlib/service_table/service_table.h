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
// * Neither name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
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
// $Revision: 1.1.1.5 $
// $Date: 2001/06/15 00:22:16 $
//     


//Functions for generating a Service Table from a Dom Node
//and for traversing that list 
// also included are functions to work with Subscription Lists


#ifndef _SERVICE_TABLE
#define _SERVICE_TABLE

#include "./tools/config.h"
#include "./genlib/http_client/http_client.h"
#include "./upnpdom/all.h"
#include "./upnpdom/domCif.h"
#include "upnp.h"
#include <stdio.h>
#include <malloc.h>
#include <time.h>

#include <pthread.h>
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else 
#define EXTERN_C 
#endif
#define SID_SIZE  41

DEVICEONLY(

typedef struct SUBSCRIPTION {
  Upnp_SID sid;
  int eventKey;
  int ToSendEventKey;
  time_t expireTime;
  int active;
  URL_list DeliveryURLs;
  struct SUBSCRIPTION *next;
} subscription;


typedef struct SERVICE_INFO {
  Upnp_DOMString serviceType;
  Upnp_DOMString serviceId;
  char * SCPDURL ;
  char * controlURL;
  char * eventURL;
  Upnp_DOMString UDN;
  int active;
  int TotalSubscriptions;
  subscription *subscriptionList;
  struct SERVICE_INFO * next;
} service_info;

typedef struct SERVICE_TABLE {
  Upnp_DOMString URLBase;
  service_info *serviceList;
} service_table;



//finds the serviceID/DeviceID pair in the service table
//returns a pointer to the service info (pointer should NOT be deallocated)
//returns the device handle of the root device for the found service
//returns HND_INVALID if not found
EXTERN_C service_info *FindServiceId( service_table * table, 
			     const char * serviceId, const char * UDN);

//finds the eventURLPath in the service table
// paths are matched EXACTLY (including a query portion) and the initial /
//returns HND_INVALID if not found
EXTERN_C service_info * FindServiceEventURLPath( service_table *table,
					  char * eventURLPath
					 );

//returns HND_INVALID if not found
EXTERN_C service_info * FindServiceControlURLPath( service_table *table,
					  char * controlURLPath);

//for testing
DBGONLY(EXTERN_C void printService(service_info *service,Dbg_Level
				   level,
				   Dbg_Module module));

DBGONLY(EXTERN_C void printServiceList(service_info *service,
				       Dbg_Level level, Dbg_Module module));

DBGONLY(EXTERN_C void printServiceTable(service_table *
					table,Dbg_Level
					level,Dbg_Module module));

//for deallocation (when necessary)

//frees service list (including head)
EXTERN_C void freeServiceList(service_info * head);

//frees dynamic memory in table, (does not free table, only memory within the structure)
EXTERN_C void freeServiceTable(service_table * table);

//to generate the service table from a dom node and handle
//returns a 1 if successful 0 otherwise
//all services are initially active and have empty subscriptionLists
EXTERN_C int getServiceTable(Upnp_Node node,  
		    service_table * out, char * DefaultURLBase);


//functions for Subscriptions

//removes the subscription with the SID from the subscription list pointed to by head
//returns the new list in head
EXTERN_C void RemoveSubscriptionSID(Upnp_SID sid, service_info * service);

//returns a pointer to the subscription with the SID, NULL if not found
EXTERN_C subscription * GetSubscriptionSID(Upnp_SID sid,service_info * service);   


//returns a pointer to the first subscription
EXTERN_C subscription * GetFirstSubscription(service_info *service);

//returns the next ACTIVE subscription that has not expired
// Expired subscriptions are removed
EXTERN_C subscription * GetNextSubscription(service_info * service, subscription *current);

//frees subscriptionList (including head)
EXTERN_C void freeSubscriptionList(subscription * head);

//Misc helper functions
EXTERN_C int getSubElement(const char * element_name,Upnp_Node node, 
		  Upnp_Node *out);

EXTERN_C Upnp_DOMString getElementValue(Upnp_Node node);

EXTERN_C int copy_subscription(subscription *in, subscription *out);

EXTERN_C void freeSubscription(subscription * sub);
)
#endif
