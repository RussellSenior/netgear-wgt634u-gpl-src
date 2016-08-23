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


#ifndef _CLIENT_TABLE
#define _CLIENT_TABLE

#include "upnp.h"

#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include "genlib/http_client/http_client.h"
#include "genlib/service_table/service_table.h"

#include "genlib/timer_thread/timer_thread.h"
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else 
#define EXTERN_C 
#endif

CLIENTONLY(
typedef struct CLIENT_SUBSCRIPTION {
  Upnp_SID sid;
  char * ActualSID;
  char * EventURL;
  int RenewEventId;
  struct CLIENT_SUBSCRIPTION * next;
} client_subscription;





EXTERN_C void freeClientSubList(client_subscription * list);

EXTERN_C void freeClientSubList(client_subscription * list);

EXTERN_C void RemoveClientSubClientSID(client_subscription **head, 
				       const Upnp_SID sid);

EXTERN_C client_subscription * GetClientSubClientSID(client_subscription *head
						     , const Upnp_SID sid);

EXTERN_C client_subscription * GetClientSubActualSID(client_subscription *head
						     , token * sid);

EXTERN_C int copy_client_subscription(client_subscription * in, client_subscription * out);

EXTERN_C void free_client_subscription(client_subscription * sub);
)
#endif
