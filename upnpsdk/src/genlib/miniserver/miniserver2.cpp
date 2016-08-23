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

// $Revision: 1.1.1.2 $
// $Date: 2001/06/15 00:21:34 $

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <genlib/util/utilall.h>
#include <genlib/util/util.h>
#include <genlib/tpool/scheduler.h>
#include <genlib/net/netexception.h>
#include <genlib/net/http/statuscodes.h>
#include <genlib/net/http/parseutil2.h>
#include <genlib/net/http/readwrite.h>

#include <genlib/miniserver/miniserver2.h>

enum MiniServerState { MSERV_IDLE, MSERV_RUNNING, MSERV_STOPPING };

CREATE_NEW_EXCEPTION_TYPE( MiniServerException, BasicException, "MiniServerReadException" )

enum READ_EXCEPTION_CODE {
        RCODE_SUCCESS               =  0,
        RCODE_NETWORK_READ_ERROR    = -1,
        RCODE_BAD_FORMAT            = -2,
        RCODE_LENGTH_NOT_SPECIFIED  = -3,
        RCODE_METHOD_NOT_ALLOWED    = -4,
        RCODE_INTERNAL_SERVER_ERROR = -5,
        RCODE_METHOD_NOT_IMPLEMENTED = -6,
        };

// module vars ////////////////////

static MiniServerState gMServState = MSERV_IDLE;

static minisvr_Callback gGetCallback = NULL;
static minisvr_Callback gSoapCallback = NULL;
static minisvr_Callback gGenaCallback = NULL;

static minisvr_LogCallback gLogCallback = NULL;
///////////////////////////////////

void minisvr_SetSoapHandler( minisvr_Callback callback )
{
    gSoapCallback = callback;
}

minisvr_Callback minsvr_GetSoapHandler()
{
    return gSoapCallback;
}

void minisvr_SetGenaHandler( minisvr_Callback callback )
{
    gGenaCallback = callback;
}

minisvr_Callback minisvr_GetGenaHandler()
{
    return gGenaCallback;
}

void minisvr_SetHttpGetHandler( minisvr_Callback callback )
{
    gGetCallback = callback;
}

minisvr_Callback minisvr_GetHttpGetHandler()
{
    return gGetCallback;
}

void minisvr_SetLogHandler( minisvr_LogCallback callback )
{
    gLogCallback = callback;
}

minisvr_LogCallback minisvr_GetLogHandler()
{
    return gLogCallback;
}

// throws MiniserverException.RCODE_INTERNAL_SERVER_ERROR
static void DispatchRequest( HttpMessage& request, int sockfd )
{
    minisvr_Callback callback;
    
    switch ( request.requestLine.method )
    {
    case UPNP_POST:
    case UPNP_MPOST:
        callback = gSoapCallback;
        break;
            
    case UPNP_NOTIFY:
    case UPNP_SUBSCRIBE:
    case UPNP_UNSUBSCRIBE:
        callback = gGenaCallback;
        break;
            
    default:
        // GET, HEAD and unknown methods handled by HTTP web server
        callback = gGetCallback;    
    }
    
    if ( callback == NULL )
    {
        MiniServerException e( "callback not defined or unknown method" );
        e.setErrorCode( RCODE_INTERNAL_SERVER_ERROR );
        throw e;
    }
    
    callback( request, sockfd );
}

static void HandleError( int errCode, int sockfd )
{
    xstring errMsg;
    int statusCode = -1;
    HttpMessage response;
    
    switch (errCode)
    {
    case RCODE_BAD_FORMAT:
        statusCode = HTTP_BAD_REQUEST;
        break;
        
    case RCODE_INTERNAL_SERVER_ERROR:
        statusCode = HTTP_INTERNAL_SERVER_ERROR;
        break;
                
    default:
        DBG( printf( "HandleError: unknown code %d\n", errCode ); )
        break;
    };

    if ( statusCode > 0 )
    {
        response.responseLine.setValue( statusCode );
        response.isRequest = false;
        http_SendMessage( sockfd, response );
    }   
    
    DBG( printf("http error: %s\n", http_GetCodeText(statusCode)); )
}

static void HandleRequest( void *args )
{
    int sockfd;
    HttpMessage request;
    int status;
    MiniServerException e( "HandleRequest(): " );
    
    sockfd = (int) args;
    
    try
    {
        status = http_RecvMessage( sockfd, request );
        switch ( status )
        {
            case 0:
                // ok
                break;
                
            case HTTP_E_BAD_MSG_FORMAT:
                e.setErrorCode( RCODE_BAD_FORMAT );
                throw e;
                break;
                
            case HTTP_E_OUT_OF_MEMORY:
                e.setErrorCode( RCODE_INTERNAL_SERVER_ERROR );
                throw e;
                break;
                
            case HTTP_E_TIMEDOUT:
                // nothing to do
                break;
                
            default:
                DBG( printf("HandleRequest(): got unknown status %d", status); )
                break;
        }
        
        // pass data to callback
        DispatchRequest( request, sockfd );
    }
    catch ( MiniServerException& e )
    {
        DBG( e.print(); )
        DBG( printf("error code = %d\n", e.getErrorCode()); )
        
        HandleError( e.getErrorCode(), sockfd );
        close( sockfd );
    }
    
    // note: callback responsible for closing socket
}


static void RunMiniServer( void* args )
{
    struct sockaddr_in clientAddr;
    int listenfd;

    listenfd = (int)args;   

    try
    {
        while ( true )
        {
            int connectfd;
            socklen_t clientLen;
            
            //DBG( printf( "Waiting...\n" ); )
            
            // get a client connection
            while ( true )
            {
                // stop server
                if ( gMServState == MSERV_STOPPING )
                {
                    throw -9;
                }
                
                connectfd = accept( listenfd, (sockaddr*) &clientAddr,
                        &clientLen );
                if ( connectfd > 0 )
                {
                    // valid connection
                    break;
                }
                if ( connectfd == -1 && errno == EINTR )
                {
                    // interrupted. -- stop?
                    if ( gMServState == MSERV_STOPPING )
                    {
                        throw -9;   // signal to stop
                    }
                    else
                    {
                        // ignore interruption
                        continue;
                    }
                }
                else
                {
                    xstring errStr =  "Error: RunMiniServer: accept(): ";
                    errStr = strerror( errno );
                    NetException e( errStr.c_str() );
                    e.setErrorCode( errno );
                    throw e;
                }
            }
            
            int sched_stat;
        
            sched_stat = tpool_Schedule( HandleRequest, (void*)connectfd );
                    
            if ( sched_stat < 0 )
            {
                HandleError( RCODE_INTERNAL_SERVER_ERROR, connectfd );
            }
        }
    }
    catch ( GenericException& e )
    {
        DBG( e.print(); )
    }
    catch ( int code )
    {
        if ( code == -9 )
        {
            // miniserver to be stopped
            assert( gMServState == MSERV_STOPPING );
            
            // free resources
            close( listenfd );
            
            gMServState = MSERV_IDLE;
        }
    }
}

static int GetListenQueueSize()
{
    return 10;
}

// return;
//  0 - ok
// -1 - check errno
// -2 - miniserver not idle
// -3 - can't start server thread
int minisvr_Start( unsigned short listen_port )
{
    struct sockaddr_in serverAddr;
    int listenfd = 0;
    int success;
    int retCode = 0;
    
    //DBG( printf("listen port: %d\n",listen_port); )

    try
    {   
        // idle --> running only
        if ( gMServState != MSERV_IDLE )
        {
            throw -2;
        }
        
        listenfd = socket( AF_INET, SOCK_STREAM, 0 );
        if ( listenfd <= 0 )
        {
            throw -1;   // error creating socket
        }
        
        bzero( &serverAddr, sizeof(serverAddr) );
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
        serverAddr.sin_port = htons( listen_port );
        
        success = bind( listenfd, (sockaddr*)&serverAddr,
            sizeof(serverAddr) );
        if ( success == -1 )
        {
            throw -1;   // bind failed
        }
    
        success = listen( listenfd, GetListenQueueSize() );
        if ( success == -1 )
        {
            throw -1; // listen failed
        }
    
        success = tpool_Schedule( RunMiniServer, (void *)listenfd );
        if ( success < 0 )
            throw -3;   // schedule failed

        gMServState = MSERV_RUNNING;    // OK
    }
    catch ( int code )
    {
        // error occured
        assert( code < 0 );
        
        if ( listenfd != 0 )
            close( listenfd );
            
        retCode = code;
    }
            
    return retCode;
}

int minsvr_Stop()
{
    if ( gMServState == MSERV_IDLE )
        return -2;
        
    gMServState = MSERV_STOPPING;
    // interrupt miniserver thread
    
    return 0;
}


#if 0

/////////////////////////////////////////////////////////////////////////
// **********#)))))))))))))))))))))###################************#####
// (*(*&(&(*&%#(&(*&%#(*%&##*************%&!{*&%#)*&%#*&%)#*@&%)#
// (*%&#)*#&)%(#*)(*#)(*@$)(#@*$)(*#@)$(*@#)(&%)@#&)($&*@)(#&$)(#@$)(

CREATE_NEW_EXCEPTION_TYPE( MiniServerReadException, BasicException, "MiniServerReadException" )

enum READ_EXCEPTION_CODE {
        RCODE_SUCCESS               =  0,
        RCODE_NETWORK_READ_ERROR    = -1,
        RCODE_MALFORMED_LINE        = -2,
        RCODE_LENGTH_NOT_SPECIFIED  = -3,
        RCODE_METHOD_NOT_ALLOWED    = -4,
        RCODE_INTERNAL_SERVER_ERROR = -5,
        RCODE_METHOD_NOT_IMPLEMENTED = -6,
        };
        
enum HTTP_COMMAND_TYPE { CMD_HTTP_GET,
        CMD_SOAP_POST, CMD_SOAP_MPOST,
        CMD_GENA_SUBSCRIBE, CMD_GENA_UNSUBSCRIBE, CMD_GENA_NOTIFY,
        CMD_HTTP_UNKNOWN,
        CMD_HTTP_MALFORMED };

// module vars

static MiniServerCallback gGetCallback = NULL;
static MiniServerCallback gSoapCallback = NULL;
static MiniServerCallback gGenaCallback = NULL;

//////////////

void SetHTTPGetCallback( MiniServerCallback callback )
{
    gGetCallback = callback;
}

MiniServerCallback GetHTTPGetCallback( void )
{
    return gGetCallback;
}

void SetSoapCallback( MiniServerCallback callback )
{
    gSoapCallback = callback;
}

MiniServerCallback GetSoapCallback( void )
{
    return gSoapCallback;
}

void SetGenaCallback( MiniServerCallback callback )
{
    gGenaCallback = callback;
}

MiniServerCallback GetGenaCallback( void )
{
    return gGenaCallback;
}


class NetReader1
{
public:
    NetReader1( int socketfd );
    virtual ~NetReader1();
    
    int getChar( char& c );
    int getLine( xstring& s, bool& newlineNotFound );
    int readData( void* buf, size_t bufferLen );
    
    int getMaxBufSize() const
        { return maxbufsize; }
    
private:
    bool bufferHasData() const
    { return offset < buflen; }
    
    ssize_t refillBuffer();
        
private:
    int sockfd;
    char data[5 + 1];
    int offset;
    int buflen;
    int maxbufsize;
};

NetReader1::NetReader1( int socketfd )
{
    sockfd = socketfd;
    offset = 0;
    maxbufsize = 5;
    buflen = 0;
    data[maxbufsize] = 0;
}

NetReader1::~NetReader1()
{

}

int NetReader1::getChar( char& c )
{
    int status;
    
    if ( !bufferHasData() )
    {
        status = refillBuffer();
        
        if ( status <= 0 )
            return status;
            
        if ( !bufferHasData() )
            return 0;
    }
        
    c = data[offset];
    offset++;
    
    return 1;   // length of data returned
}

int NetReader1::getLine( xstring& s, bool& newlineNotFound )
{
    int startOffset;
    char c;
    int status;
    bool crFound;
        
    startOffset = offset;
    
    s = "";
    
    newlineNotFound = false;
    crFound = false;
    while ( true )
    {
        status = getChar( c );
        
        if ( status == 0 )
        {
            // no more chars in stream
            newlineNotFound = true;
            return s.length();
        }
        
        if ( status < 0 )
        {
            // some kind of error
            return status;
        }
        
        s += c;
        
        if ( c == 0xA )
        {
            return s.length();
        }
        else if ( c == 0xD )            // CR
        {
            crFound = true;
        }
        else
        {
            // wanted to see LF after CR; error
            if ( crFound )
            {
                newlineNotFound = true;
                return s.length();
            }
        }
    }
    
    return 0;
}

// read data
int NetReader1::readData( void* buf, size_t bufferLen )
{
    int status;
    int copyLen;
    size_t dataLeft;    // size of data left in buffer
    
    if ( bufferLen <= 0 )
        return 0;
    
    // refill empty buffer  
    if ( !bufferHasData() )
    {
        status = refillBuffer();
        
        if ( status <= 0 )
            return status;
            
        if ( !bufferHasData() )
            return 0;
            
        dataLeft = buflen;
    }
    else
    {
        dataLeft = buflen - offset;
    }
    
    if ( bufferLen < dataLeft )
    {
        copyLen = bufferLen;
    }
    else
    {
        copyLen = dataLeft;
    }
    
    memcpy( buf, &data[offset], copyLen );
    
    offset += copyLen;
    
    return copyLen;
}

ssize_t NetReader1::refillBuffer()
{
    ssize_t numRead;
    
    // test version
    //numRead = DoRead( sockfd, data, maxbufsize );
    
    numRead = read( sockfd, data, maxbufsize );
    
    if ( numRead >= 0 )
    {
        buflen = numRead;
    }
    else
    {
        buflen = 0;
    }
    
    offset = 0;
        
    return numRead;
}


static void WriteNetData( const char* s, int sockfd )
{
    write( sockfd, s, strlen(s) );
    
}
    
// determines type of UPNP command from request line, ln
static HTTP_COMMAND_TYPE GetCommandType( const xstring& ln )
{
    // commands GET, POST, M-POST, SUBSCRIBE, UNSUBSCRIBE, NOTIFY
    
    xstring line = ln;
    int i;
    char c;
    
    char * getStr = "GET";
    char * postStr = "POST";
    char * mpostStr = "M-POST";
    char * subscribeStr = "SUBSCRIBE";
    char * unsubscribeStr = "UNSUBSCRIBE";
    char * notifyStr = "NOTIFY";
    char * pattern;
    HTTP_COMMAND_TYPE retCode=CMD_HTTP_UNKNOWN;
    
    try
    {
        line.toUppercase();
    
        c = line[0];
        
        switch (c)
        {
            case 'G':
                pattern = getStr;
                retCode = CMD_HTTP_GET;
                break;
                
            case 'P':
                pattern = postStr;
                retCode = CMD_SOAP_POST;
                break;
                
            case 'M':
                pattern = mpostStr;
                retCode = CMD_SOAP_MPOST;
                break;
                
            case 'S':
                pattern = subscribeStr;
                retCode = CMD_GENA_SUBSCRIBE;
                break;
                
            case 'U':
                pattern = unsubscribeStr;
                retCode = CMD_GENA_UNSUBSCRIBE;
                break;
                
            case 'N':
                pattern = notifyStr;
                retCode = CMD_GENA_UNSUBSCRIBE;
                break;
            
            default:
                // unknown method
                throw -1;
                
        }
        
        int patLength = strlen( pattern );
        
        for ( i = 1; i < patLength; i++ )
        {
            if ( line[i] != pattern[i] )
                throw -1;
        }
    }
    catch ( OutOfBoundsException& e )
    {
        return CMD_HTTP_UNKNOWN;
    }
    catch ( int parseCode )
    {
        if ( parseCode == -1 )
        {
            return CMD_HTTP_UNKNOWN;
        }
    }
    
    return retCode;
}

static int ParseContentLength( const xstring& textLine, bool& malformed )
{
    xstring line;
    xstring asciiNum;
    char *pattern = "CONTENT-LENGTH";
    int patlen = strlen( pattern );
    int i;
    int contentLength;
    
    malformed = false;
    
    contentLength = -1;
    line = textLine;
    line.toUppercase();
    
    if ( strncmp(line.c_str(), pattern, patlen) != 0 )
    {
        // unknown header
        return -1;
    }
    
    i = patlen;
    
    try
    {
        // skip whitespace
        while ( line[i] == ' ' || line [i] == '\t' )
        {
            i++;
        }
        
        // ":"
        if ( line[i] != ':' )
        {
            throw -1;
        }
        i++;
        
        char* invalidChar = NULL;
        
        contentLength = strtol( &line[i], &invalidChar, 10 );
        // anything other than crlf or whitespace after number is invalid
        if ( *invalidChar != '\0' )
        {
            // see if there is an invalid number
            while ( *invalidChar )
            {
                char c;
                
                c = *invalidChar;
                
                if ( !(c == ' ' || c == '\t' || c == '\r' || c == '\n') )
                {
                    // invalid char in number
                    throw -1;
                }
                
                invalidChar++;
            }
        }
    }
    catch ( OutOfBoundsException& e )
    {
        malformed = true;
        return -1;
    }
    catch ( int errCode )
    {
        if ( errCode == -1 )
        {
            malformed = true;
            return -1;
        }
    }
    
    return contentLength;   
}

static void ReadRequest( int sockfd, xstring& document,
    HTTP_COMMAND_TYPE& command )
{
    NetReader1 reader( sockfd );
    xstring reqLine;
    xstring line;
    bool newlineNotFound;
    int status;
    HTTP_COMMAND_TYPE cmd;
    int contentLength;
    const int BUFSIZE = 3;
    char buf[ BUFSIZE + 1 ];
    MiniServerReadException excep;
    
    document = "";
    
    // read request-line
    status = reader.getLine( reqLine, newlineNotFound );
    
    if ( status < 0 )
    {
        // read error
        excep.setErrorCode( RCODE_NETWORK_READ_ERROR );
        throw excep;
    }
    
    if ( newlineNotFound )
    {
        // format error
        excep.setErrorCode( RCODE_MALFORMED_LINE );
        throw excep;
    }
        
    cmd = GetCommandType( reqLine );
    
    if ( cmd == CMD_HTTP_UNKNOWN )
    {
        // unknown or unsupported cmd
        excep.setErrorCode( RCODE_METHOD_NOT_IMPLEMENTED );
        throw excep;
    }
    
    document += reqLine;
    
    contentLength = -1;     // init
    
    // read headers
    while ( true )
    {
        status = reader.getLine( line, newlineNotFound );
        
        if ( status < 0 )
        {
            // network error
            excep.setErrorCode( RCODE_NETWORK_READ_ERROR );
            throw excep;
        }   
        
        if ( newlineNotFound )
        {
            // bad format
            excep.setErrorCode( RCODE_MALFORMED_LINE );
            throw excep;
        }
        
        // get content-length if not obtained already
        if ( contentLength < 0 )
        {
            bool malformed;
            
            contentLength = ParseContentLength( line, malformed );
            
            if ( malformed )
            {
                excep.setErrorCode( RCODE_MALFORMED_LINE );
                throw excep;
            }
        }
        
        document += line;
        
        // done ?
        if ( line == "\n" || line == "\r\n" )
        {
            break;
        }
    }
    
    // must have body for POST and M-POST msgs
    if ( contentLength < 0 &&
         (cmd == CMD_SOAP_POST || cmd == CMD_SOAP_MPOST)
       )
    {
        // HTTP: length reqd
        excep.setErrorCode( RCODE_LENGTH_NOT_SPECIFIED );
        throw excep;
    }
    
    if ( contentLength > 0 )
    {
        int totalBytesRead = 0;
        
        // read body
        while ( true )
        {
            int bytesRead;
            
            bytesRead = reader.readData( buf, BUFSIZE );
            buf[ bytesRead ] = 0;   // null terminate string
            if ( bytesRead > 0 )
            {
                document.appendLimited( buf, bytesRead );
                totalBytesRead += bytesRead;
                
                if ( totalBytesRead >= contentLength )
                {
                    // done reading data
                    break;
                }
            }
            else if ( bytesRead == 0 )
            {
                // done
                break;
            }
            else
            {
                // error reading
                excep.setErrorCode( RCODE_NETWORK_READ_ERROR );
                throw excep;
            }
        }
    }
    
    command = cmd;
}


static void HandleError( int errCode, int sockfd )
{
    xstring errMsg;
    
    switch (errCode)
    {
    case RCODE_NETWORK_READ_ERROR:
        break;
        
    case RCODE_MALFORMED_LINE:
        errMsg = "400 Bad Request";
        break;
        
    case RCODE_LENGTH_NOT_SPECIFIED:
        errMsg = "411 Length Required";
        break;
        
    case RCODE_METHOD_NOT_ALLOWED:
        errMsg = "405 Method Not Allowed";
        break;

    case RCODE_INTERNAL_SERVER_ERROR:
        errMsg = "500 Internal Server Error";
        break;
                
    case RCODE_METHOD_NOT_IMPLEMENTED:
        errMsg = "511 Not Implemented";
        break;
        
    default:
        DBG( printf( "HandleError: unknown code %d\n", errCode ); )
        break;
    };
    
    if ( errMsg.length() == 0 )
        return;
        
    xstring msg;
    
    msg = "HTTP/1.1 ";
    msg += errMsg;
    msg += "\r\n\r\n";
    
    // send msg
    WriteNetData( msg.c_str(), sockfd );
    
    // dbg
    // sleep so that client does get connection reset
    //sleep( 3 );
    ///////
    
    // dbg
    DBG( printf( "http error: %s\n", msg.c_str() ); )
    ///////
}

static void MultiplexCommand( HTTP_COMMAND_TYPE cmd, const xstring& document,
        int sockfd )
{
    MiniServerCallback callback;
    
    switch ( cmd )
    {
    case CMD_SOAP_POST:
    case CMD_SOAP_MPOST:
        callback = gSoapCallback;
        break;
            
    case CMD_GENA_NOTIFY:
    case CMD_GENA_SUBSCRIBE:
    case CMD_GENA_UNSUBSCRIBE:
        callback = gGenaCallback;
        break;
            
    case CMD_HTTP_GET:
        callback = gGetCallback;
        break;
            
    default:
        callback = NULL;
    }
    
    if ( callback == NULL )
    {
        MiniServerReadException e( "callback not defined or unknown method" );
        e.setErrorCode( RCODE_METHOD_NOT_IMPLEMENTED );
        throw e;
    }
    
    callback( document.c_str(), sockfd );
}

static void HandleRequest( void *args )
{
    int sockfd;
    xstring document;
    HTTP_COMMAND_TYPE cmd;
    
    sockfd = (int) args;
    
    try
    {
        ReadRequest( sockfd, document, cmd );
        
        // pass data to callback
        MultiplexCommand( cmd, document, sockfd );
        
        //printf( "input document:\n%s\n", document.c_str() );
    }
    catch ( MiniServerReadException& e )
    {
        // dbg
        DBG( e.print(); )
        DBG( printf( "error code = %d\n", e.getErrorCode() ); )
        //////
        
        HandleError( e.getErrorCode(), sockfd );
        
        // destroy connection
        close( sockfd );
    }   
}


static void RunMiniServer( void* args )
{
    struct sockaddr_in clientAddr;
    int listenfd;

    listenfd = (int)args;   

    try
    {
        while ( true )
        {
            int connectfd;
            socklen_t clientLen;
            
            // dbg
            //printf( "Waiting...\n" );
            ///////
            
            // get a client request
            while ( true )
            {
                connectfd = accept( listenfd, (sockaddr*) &clientAddr,
                        &clientLen );
                if ( connectfd > 0 )
                {
                    // valid connection
                    break;
                }
                if ( connectfd == -1 && errno == EINTR )
                {
                    // ignore interruption
                    continue;
                }
                else
                {
                    xstring errStr =  "Error: RunMiniServer: accept(): ";
                    errStr = strerror( errno );
                    throw BasicException( errStr.c_str() );
                }
            }
            
            int sched_stat;
            
            sched_stat = tpool_Schedule( HandleRequest, (void*)connectfd );
            if ( sched_stat < 0 )
            {
                HandleError( RCODE_INTERNAL_SERVER_ERROR, connectfd );              
            }
                
                
            //HandleRequest( (void *)connectfd );
        }
    }
    catch ( GenericException& e )
    {
        e.print();
        free(StartMiniServer); //temporarily crashes if 
                               //miniserver doesn't start up
                               //needs a better way to detect this case
    }
}

int StartMiniServer( unsigned short listen_port )
{
    struct sockaddr_in serverAddr;
    int listenfd;
    int success;

    //printf("listen port: %d\n",listen_port);
    listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( listenfd <= 0 )
    {
        return -1;  // error creating socket
    }
        
    bzero( &serverAddr, sizeof(serverAddr) );
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
    serverAddr.sin_port = htons( listen_port );
        
    success = bind( listenfd, (sockaddr*)&serverAddr,
        sizeof(serverAddr) );
    if ( success == -1 )
    {
        return -1;  // bind failed
    }
    
    success = listen( listenfd, 10 );
    if ( success == -1 )
    {
        return -1; // listen failed
    }
    
    tpool_Schedule( RunMiniServer, (void *)listenfd );
    
    return 0;
}

////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

#endif
