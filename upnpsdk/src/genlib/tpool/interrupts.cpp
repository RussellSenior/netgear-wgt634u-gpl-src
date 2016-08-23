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

// $Revision: 1.1.1.3 $
// $Date: 2001/06/15 00:22:16 $

#include <genlib/tpool/scheduler.h>
#include <genlib/tpool/tpool.h>
#include <memory.h>
#include <stdio.h>
#include <signal.h>
#include <genlib/tpool/interrupts.h>
#include <genlib/util/miscexceptions.h>
#include "../../../inc/tools/config.h"

#include <pthread.h>
#include <unistd.h>



// module vars //////////////////////////////

static int                  g_SignalNum = 0;
static struct sigaction     g_OldAction;

/////////////////////////////////////////////

// uninitializes interrupt handler
// returns:
//   0: success
// TINTR_E_NOT_INITIALIZED
int tintr_Done( void )
{
    struct sigaction dummy;
    sigaction( g_SignalNum, &g_OldAction, &dummy );
    g_SignalNum = 0;
    return 0;
}


//////////////////////////////////////////////////////
////////////////////////////////////////////////////////

static void signal_handler_alpha( int arg )
{
    DBG(
        UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
            "tintr: signal_handler_alpha( %d ) called\n", arg); )
}

#define SA_NOMASK      0x40000000

static int set_signal_handler( int signalNum )
{
    struct sigaction newset;
    int code;

    newset.sa_handler = signal_handler_alpha;
    newset.sa_flags = SA_NOMASK;
    code = sigaction( signalNum, &newset, &g_OldAction );
    if ( code < 0 )
    {
        DBG( perror("tintr: sigaction() failed"); )
        DBG(
            UpnpPrintf(UPNP_CRITICAL, TPOOL, __FILE__, __LINE__,
                "tintr: sigaction() failed: %s\n"); )
    }
    return code;
}


// initializes signal handler
// returns: 0 on success
//   -1 on error; check errno
int tintr_Init( int signalNum )
{
    int code;
    
    code = set_signal_handler( signalNum );
    
    g_SignalNum = signalNum;
    
    return code;
}

// returns 0 on success, -1 on error
int tintr_Interrupt( IN pthread_t thread )
{
    DBG(
        UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
            "sending signal %d\n", g_SignalNum); )
    DBG(
        UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
            "intr: thread = %ld\n", (long)thread); )
    
    int code;
    
    code = pthread_kill( thread, g_SignalNum );
    return code;
}

