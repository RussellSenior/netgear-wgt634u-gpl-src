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
// $Date: 2001/06/15 00:22:16 $
#include "../../inc/tools/config.h"
#if EXCLUDE_MINISERVER == 0
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <genlib/util/util.h>
#include <genlib/util/memreader.h>
#include <genlib/util/miscexceptions.h>

MemReader::MemReader( const char *str )
{
    assert( str != NULL );
    
    buf = (char *)str;
    index = 0;
    len = strlen( str );    
}

MemReader::MemReader( const void* binaryData, int length )
{
    assert( binaryData != NULL );
    assert( length >= 0 );
    
    buf = (char *)binaryData;
    index = 0;
    len = length;
}

char MemReader::getChar()
{
    if ( index >= len )
    {
        throw OutOfBoundsException( "MemReader::getChar()" );
    }
    
    return buf[index++];
}

void MemReader::pushBack()
{
    if ( index <= 0 )
    {
        throw OutOfBoundsException( "MemReader::pushBack()" );
    }
    
    index--;
}

int MemReader::read( INOUT void* databuf, IN int bufferSize )
{
    int copyLen;
    int dataLeftLen;

    assert( databuf != NULL );
    assert( bufferSize > 0 );
        
    // how much data left in buffer
    dataLeftLen = len - index;
    
    // pick smaller value
    copyLen = MinVal( bufferSize, dataLeftLen );
    
    // copy data to caller's buffer
    memcpy( databuf, &buf[index], copyLen );

    // point to next char in buf
    index += copyLen;
    
    return copyLen;
}

bool MemReader::endOfStream()
{
    return index >= len;
}

#endif
