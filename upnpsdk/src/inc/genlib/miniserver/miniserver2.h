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

// $Revision: 1.1.1.1 $
// $Date: 2001/06/14 23:52:28 $

#ifndef GENLIB_MINISERVER_MINISERVER2_H
#define GENLIB_MINISERVER_MINISERVER2_H

// MiniServer 2

// to handle HTTP messages
typedef void (*minisvr_Callback) ( IN HttpMessage& request, IN int sockfd );

// to handle logs
typedef void (*minisvr_LogCallback) ( IN sockaddr_in* client,
    IN const char* resource, IN int result );


// async'ly starts miniserver listening at listen_port
// returns: 0 on success; -1 on error (check errno)
//  -2 server is stopping/running; wait until it stops to restart
int minisvr_Start( unsigned short listen_port );

// stops miniserver; miniserver may not shutdown immediately
// returns 0: stop msg sent
//        -2: miniserver not running
int minisvr_Stop();

void minisvr_SetSoapHandler( minisvr_Callback callback );
minisvr_Callback minsvr_GetSoapHandler();

void minisvr_SetGenaHandler( minisvr_Callback callback );
minisvr_Callback minisvr_GetGenaHandler();

void minisvr_SetHttpGetHandler( minisvr_Callback callback );
minisvr_Callback minisvr_GetHttpGetHandler();

void minisvr_SetLogHandler( minisvr_LogCallback callback );
minisvr_LogCallback minisvr_GetLogHandler();

#endif /* GENLIB_MINISERVER_MINISERVER2_H */

