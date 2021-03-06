///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation `
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


#ifndef SAMPLE_UTIL_H
#define SAMPLE_UTIL_H

#include "stdlib.h"
#include "upnp.h"
#include "upnptools.h"

char* SampleUtil_GetElementValue(Upnp_Node node);
Upnp_NodeList SampleUtil_GetFirstServiceList(Upnp_Node);
char* SampleUtil_GetFirstDocumentItem(Upnp_Node, char *);
char* SampleUtil_GetFirstElementItem(Upnp_Element, char *);
void SampleUtil_PrintEventType(Upnp_EventType);
int SampleUtil_PrintEvent(Upnp_EventType, void *);
int SampleUtil_FindAndParseService (Upnp_Document, char*, char*, char**, char**, char**);


#endif /* UPNPSDK_UTIL_H */










