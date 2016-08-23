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

// $Revision: 1.1.1.3 $
// $Date: 2001/06/15 00:22:15 $
#include "../../inc/tools/config.h"
#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <genlib/net/netreader.h>
#include <genlib/util/utilall.h>
#include <genlib/net/netexception.h>
#include <sys/socket.h>
#include <unistd.h>

NetReader::NetReader( int socketfd )
{
    sockfd = socketfd;
    datalen = 0;
    offset = 0;
    buf_eof = false;
}
    
NetReader::~NetReader()
{
}
    
char NetReader::getChar()
{
    if ( !bufferHasData() )
    {
        refillBuffer();
        
        if ( !bufferHasData() )
        {
            return 0;
        }
    }
    
    // read next char	
    return data[offset++];
}
    
// throws OutOfBoundsException
void NetReader::pushBack()
{
    if ( offset == 0 )
    {
        // can't move further back
        throw OutOfBoundsException( "NetReader::pushback()" );
    }
    
    offset--;
}

int NetReader::read( void* buffer, int bufferSize )
{
    int copyLen;
    int dataLeft;
    
    assert( buffer != NULL );
    
    // no more data left
    if ( buf_eof )
    {
        return 0;
    }

    if ( !bufferHasData() )
    {
        refillBuffer();
        if ( !bufferHasData() )
        {
            return 0;
        }
    }	
    
    dataLeft = datalen - offset;
    
    // copyLen = min( dataLeft, bufferSize )
    copyLen = bufferSize < dataLeft ? bufferSize : dataLeft;

    memcpy( buffer, &data[offset], copyLen );
    
    offset += copyLen;
    
    return copyLen;	
}

// throws NetException
void NetReader::refillBuffer()
{
    int numSaveChars;
    int numRead;
    
    // already reached EOF
    if ( buf_eof )
    {
        throw EOFException( "NetBuffer2::refillbuffer()" );
    }
    
    // save MAX_PUSHBACK chars at end, to start of data buffer
    
    // numSaveChars = min( MAX_PUSHBACK, datalen )
    numSaveChars = MAX_PUSHBACK < datalen ? MAX_PUSHBACK : datalen;
    
    // save
    memmove( &data[0], &data[datalen-numSaveChars], numSaveChars );
    
    offset = numSaveChars;	// point after saved chars
    
    // read rest from network
    numRead = recv( sockfd, &data[offset], BUFSIZE, 0 );
    
    if ( numRead < 0 )
    {
        NetException e( "NetReader::refillBuffer()" );
        e.setErrorCode( numRead );
        throw e;
    }
    
    if ( numRead == 0 )
    {
        buf_eof = true;
    }

    datalen = numSaveChars + numRead;	
}

bool NetReader::bufferHasData() const
{
    return offset < datalen;
}

#endif
#endif
