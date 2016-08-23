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

// $Revision: 1.1.1.4 $
// $Date: 2001/06/15 00:22:15 $

// readwrite.cpp
#include "../../inc/tools/config.h"
#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0

#include <stdio.h>
#include <genlib/net/netreader.h>
#include <genlib/net/netexception.h>
#include <genlib/net/http/parseutil.h>
#include <genlib/net/http/readwrite.h>
#include <genlib/net/http/statuscodes.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <strings.h>

// return codes:
//   0: success
//  -1: std error; check errno
//  HTTP_E_OUT_OF_MEMORY
//  HTTP_E_BAD_MSG_FORMAT
//  HTTP_E_TIMEDOUT
int http_RecvMessage( IN int tcpsockfd, OUT HttpMessage& message,
    UpnpMethodType requestMethod, int timeoutSecs )
{
    int retCode = 0;
    
    try
    {
        NetReader reader( tcpsockfd );
        Tokenizer scanner( reader );
            
        if ( requestMethod == HTTP_UNKNOWN_METHOD )
        {
            // reading a request
            message.loadRequest( scanner, &reader );
        }
        else
        {
            // read response
            message.loadResponse( scanner, &reader, requestMethod );
        }
                
        retCode = 0;
    }
    catch ( HttpParseException& e )
    {
        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "http_RecvMessage():ERROR %s\n", e.getMessage()); )

        retCode = HTTP_E_BAD_MSG_FORMAT;
    }
    catch ( OutOfMemoryException& e )
    {
        DBG(
            UpnpPrintf(UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
                "http_RecvMessage():mem exception\n"); )
        //DBG( e.print(); )
        retCode = HTTP_E_OUT_OF_MEMORY;
    }
    catch ( NetException& e )
    {
        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "http_RecvMessage():...net excep\n"); )
        //DBG( e.print(); )
        retCode = -1;
    }
    catch ( TimeoutException& e )
    {
        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "http_RecvMessage():...timeout excep\n"); )
        DBG( e.print(); )
        retCode = HTTP_E_TIMEDOUT;
    }
    catch ( ... )
    {
        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "uncaught exception in http_RecvMessage()"); )
    }

    return retCode;
}


// returns -1 on system error
//  HTTP_E_FILE_READ
static int SendFile( IN int tcpsockfd, IN const char* filename,
    int timeoutSecs )
{
    FILE *fp = NULL;
    const int BUFSIZE = 2 * 1024;
    char buf[BUFSIZE];
    int numRead;
    int numWritten;
    int code = 0;

    try
    {
        fp = fopen( filename, "rb" );
        if ( fp == NULL )
        {
            throw -1;
        }

        while ( true )
        {
            // read data
            numRead = fread( buf, sizeof(char), BUFSIZE, fp );
            if ( numRead == 0 )
            {
                if ( ferror(fp) )
                {
                    throw HTTP_E_FILE_READ;
                }
                break;
            }
        
            // write data
            numWritten = send( tcpsockfd, buf, numRead, MSG_NOSIGNAL );
            if ( numWritten == -1 )
            {
                throw -1;
            }
        }
    }
    catch ( int catchCode )
    {
        code = catchCode;
    }

    if ( fp != NULL )
    {
        fclose( fp );
    }

    return code;
}


// return codes:
//   0: success
//  -1: std error; check errno
//  HTTP_E_OUT_OF_MEMORY
//  HTTP_E_TIMEDOUT
int http_SendMessage( IN int tcpsockfd, IN HttpMessage& message,
    int timeoutSecs )
{
    int retCode = 0;

    assert( tcpsockfd > 0 );

    try
    {
        xstring s;
        ssize_t numWritten;
        int status;
        
        // send headers
        message.startLineAndHeadersToString( s );
        
        numWritten = send( tcpsockfd, s.c_str(), s.length(), MSG_NOSIGNAL );
        if ( numWritten == -1 )
            throw -1;
            
        // send optional body
        HttpEntity &entity = message.entity;
        HttpEntity::EntityType etype;
        
        etype = entity.getType();
        switch ( etype )
        {
            case HttpEntity::EMPTY:
                // nothing to send
                break;
                
            case HttpEntity::TEXT:
            case HttpEntity::TEXT_PTR:
            {
                // precond: content-length is in header and
                //   no transfer-encoding

                const void* entityData;
                entityData = entity.getEntity();
                if ( entityData != NULL )
                {
                    numWritten = send( tcpsockfd, (const char *)entity.getEntity(),
                        entity.getEntityLen(), MSG_NOSIGNAL );
                    if ( numWritten == -1 )
                    {
                        throw -1;
                    }
                }
                break;
            }
                
            case HttpEntity::FILENAME:
                status = SendFile( tcpsockfd, entity.getFileName(),
                    timeoutSecs );
                if ( status == -1 )
                {
                    throw status;
                }
                break;
            
            default:
                DBG(
                    UpnpPrintf( UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
                        "http_SendMessage(): unknown HttpEntity type %d", etype ); )
                break;  
        }
    }
    catch ( int code )
    {
        retCode = code;
    }
    catch ( ... )
    {
        DBG(
            UpnpPrintf(UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
                "uncaught exception in http_SendMessage" ); )
        throw;
    }
    
    return retCode;
}

// on success, returns socket connect to server
// on failure, returns -1, check errno
int http_Connect( const char* resourceURL )
{
    UriType uri;
    int sockfd;
    int status;
    sockaddr_in address;
    
    sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sockfd == -1 )
        return -1;
        
    uri.setUri( resourceURL );
    bzero( &address, sizeof(sockaddr_in) );
    uri.getIPAddress( address );

    status = connect( sockfd, (sockaddr*) &address, sizeof(sockaddr_in) );
    if ( status == -1 )
    {
        close( sockfd );
        return -1;
    }

    return sockfd;
}

// return codes
//   0: success
//  -1: std error; check errno
//  -2: out of memory
//  -3: timeout
int http_Download( IN const char* resourceURL, OUT HttpMessage& resource,
    IN int timeoutSecs )
{
    int retCode = 0;
    int connfd = 0;
    int status = 0;
    HttpMessage request;

    try
    {
        try
        {
            // build request
            request.isRequest = true;
        
            HttpRequestLine& requestLine = request.requestLine;
            requestLine.method = HTTP_GET;
            requestLine.uri.setUri( resourceURL );
            requestLine.majorVersion = 1;
            requestLine.minorVersion = 1;

            // connect
            connfd = http_Connect( resourceURL );
            if ( connfd == -1 )
                throw -1;
            
            // send request
            status = http_SendMessage( connfd, request, timeoutSecs );
            if ( status < 0 )
            {
                throw status;
            }
                
            // read reply
            status = http_RecvMessage( connfd, resource,
                requestLine.method, timeoutSecs );
            if ( status < 0 )
            {
                throw status;
            }
                
            // free resources
            close( connfd );
            retCode = 0;
        }
        catch ( ... )
        {
            // free resources
            if ( close != 0 )
            {
                close( connfd );
            }
            throw;
        }
    }
    catch ( int code )
    {
        retCode = code;
    }
    catch ( OutOfMemoryException& /* e */ )
    {
        retCode = HTTP_E_OUT_OF_MEMORY;
    }
    catch ( TimeoutException& /* e */ )
    {
        retCode = HTTP_E_TIMEDOUT;
    }
    catch ( ... )
    {
        DBG(
            UpnpPrintf( UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
                "uncaught exception in http_Download()"); )
        throw;
    }

    return retCode;
}

#endif
#endif

