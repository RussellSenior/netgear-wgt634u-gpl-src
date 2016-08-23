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

#include "../../inc/tools/config.h"
#if EXCLUDE_WEB_SERVER == 0
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "upnp.h"
#include "upnpdom/domCif.h"
#include <genlib/util/xstring.h>
#include <genlib/net/http/parseutil.h>
#include <genlib/net/http/server.h>
#include <genlib/http_client/http_client.h>
#include "urlconfig/urlconfig.h"

// returns "ip_addr:port"
static void addrToString( IN const sockaddr_in* addr,
    OUT xstring& ipaddr_port )
{
    char buf[100];

    sprintf( buf, "%s:%d", inet_ntoa(addr->sin_addr),
        ntohs(addr->sin_port) );
    ipaddr_port = buf;
}

// urlBaseOut = URLBASE = http://1.1.1.1/foo/bar
// rootPath = /foo/bar
// returns:
//   UPNP_E_SUCCESS
//   UPNP_E_OUTOF_MEMORY
//   UPNP_E_INVALID_URL
//   UPNP_E_INVALID_DESC
static int config_url( INOUT Upnp_Document doc, IN const sockaddr_in* addr,
    OUT xstring& rootPath, OUT xstring& ipPortString )
{
    Upnp_NodeList baseList;
    bool addNew = false;
    Upnp_Element element = NULL;
    Upnp_Element newElement = NULL;
    Upnp_Text textNode = NULL;
    Upnp_DOMException excep;
    Upnp_Node rootNode = NULL;
    char* urlBaseStr = "URLBase";
    int retCode;
    xstring urlStr;
    xstring ipStr;
    Upnp_DOMString domStr;
    uri_type uri;

    try
    {
        addrToString( addr, ipStr );
        ipPortString = ipStr;

        baseList = UpnpDocument_getElementsByTagName( doc, urlBaseStr );
        if ( baseList == NULL )
        {
            // urlbase not found -- create new one
            addNew = true;
            element = UpnpDocument_createElement( doc, urlBaseStr, &excep );
            if ( excep != NO_ERR )
            {
                element = NULL;
                throw UPNP_E_OUTOF_MEMORY;
            }

            urlStr = "http://";
            urlStr += ipStr;
            urlStr += '/';

            rootPath = "/";

            rootNode = UpnpDocument_getFirstChild( doc );
            if ( rootNode == NULL )
            {
                DBG(
                    UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                        "config_url(): didn't get root node\n"); )
                throw UPNP_E_INVALID_DESC;
            }

            newElement = UpnpNode_appendChild( rootNode, element, &excep );
            if ( excep != NO_ERR )
            {
                throw UPNP_E_OUTOF_MEMORY;
            }

            textNode = UpnpDocument_createTextNode( doc,
                 (char *)urlStr.c_str() );
            if ( textNode == NULL )
            {
                throw UPNP_E_OUTOF_MEMORY;
            }

            UpnpElement_appendChild( newElement, textNode, &excep );
            if ( excep != NO_ERR )
            {
                throw UPNP_E_OUTOF_MEMORY;
            }

            UpnpElement_free( element );
            UpnpNode_free( textNode );
        }
        else
        {
            // urlbase found
            addNew = false;

            element = UpnpNodeList_item( baseList, 0 );
            assert( element != NULL );


            textNode = UpnpNode_getFirstChild( element );
            if ( textNode == NULL )
            {
                //DBG( printf("config_url(): didn't get text node\n"); )
                throw UPNP_E_INVALID_DESC;
            }

            domStr = UpnpNode_getNodeValue( textNode, &excep );
            if ( excep != NO_ERR )
            {
                throw UPNP_E_INVALID_URL;
            }

            int len;

            len = parse_uri( domStr, strlen(domStr), &uri );
            if ( len < 0 || uri.type != ABSOLUTE )
            {
                throw UPNP_E_INVALID_URL;
            }

            urlStr.copyLimited( uri.scheme.buff, uri.scheme.size );
            urlStr += "://";
            urlStr += ipStr;

            rootPath = "";

            // add leading '/' if missing from rel path
            if ( (uri.pathquery.size > 0 && uri.pathquery.buff[0] != '/') ||
                 (uri.pathquery.size == 0)
               )
            {
                urlStr += '/';
                rootPath += '/';
            }

            urlStr.appendLimited( uri.pathquery.buff, uri.pathquery.size );
            rootPath.appendLimited( uri.pathquery.buff, uri.pathquery.size );

            UpnpNode_setNodeValue( textNode, (char *)urlStr.c_str(),
                &excep );
            if ( excep != NO_ERR )
            {
                throw UPNP_E_OUTOF_MEMORY;
            }
        }

        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "urlbase: %s\n", urlStr.c_str()); )

        retCode = UPNP_E_SUCCESS;
    }
    catch ( int excepCode )
    {
        if ( addNew )
        {
            if ( element != NULL )
                UpnpElement_free( element );

            if ( textNode != NULL )
                UpnpNode_free( element );

            if ( newElement != NULL )
                UpnpNode_removeChild( rootNode, newElement, &excep );
        }

        retCode = excepCode;
    }

    return retCode;
}

// returns
//   UPNP_E_OUTOF_MEMORY
static int convert_to_http_entity( IN Upnp_Document doc,
    OUT HttpEntity** entityOut )
{
    int retCode = UPNP_E_SUCCESS;
    char *docStr = NULL;
    HttpEntity* entity;

    *entityOut = NULL;

    if ( (entity = new HttpEntity) == NULL )
    {
        return UPNP_E_OUTOF_MEMORY;
    }

    entity->type = HttpEntity::TEXT;

    try
    {
        docStr = UpnpNewPrintDocument( doc );
        if ( docStr == NULL )
        {
            throw UPNP_E_OUTOF_MEMORY;
        }

        //DBG( printf("doc:>>>>>\n%s<<<<<\n", docStr); )

        entity->append( docStr, strlen(docStr) );

        *entityOut = entity;
    }
    catch ( int excepCode )
    {
        retCode = excepCode;
    }
    catch ( OutOfMemoryException& /* e */ )
    {
        retCode = UPNP_E_OUTOF_MEMORY;
    }

    if ( docStr != NULL )
    {
        free( docStr );
    }

    if ( retCode != UPNP_E_SUCCESS )
    {
        delete entity;
    }

    return retCode;
}

// throws OutOfMemoryException
static void calc_descURL( IN const char* ipPortStr, IN const char* alias,
    OUT xstring& descURL )
{
    assert( ipPortStr != NULL && strlen(ipPortStr) > 0 );
    assert( alias != NULL   && strlen(alias) >  0 );

    descURL = "http://";
    descURL += ipPortStr;
    descURL += alias;

    DBG(
        UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
            "desc url: %s\n", descURL.c_str()); )
}

// determine alias based urlbase's root path
// note: rootpath is at least '/'
static int calc_alias( IN const char* alias,
    IN const char* rootPath, OUT xstring& newAlias )
{
    const char *aliasPtr;
    int code;

    assert( rootPath != NULL && strlen(rootPath) > 0 );
    assert( alias != NULL && strlen(alias) > 0 );

    newAlias = rootPath;

    // add / suffix if missing
    if ( newAlias[newAlias.length() - 1] != '/' )
    {
        newAlias += '/';
    }

    if ( alias[0] == '/' )
    {
        aliasPtr = alias + 1;
    }
    else
    {
        aliasPtr = alias;
    }

    newAlias += aliasPtr;

    char *tempStr = newAlias.detach();
    code = remove_dots( tempStr, strlen(tempStr) );
    if ( code != UPNP_E_SUCCESS )
    {
        free( tempStr );
        return code;
    }

    newAlias = tempStr;
    free( tempStr );

    return UPNP_E_SUCCESS;
}

// returns:
//   UPNP_E_SUCCESS
//   UPNP_E_OUTOF_MEMORY
//   UPNP_E_INVALID_URL
//   UPNP_E_INVALID_DESC
int Configure_Urlbase( INOUT Upnp_Document doc,
    IN const struct sockaddr_in* serverAddr,
    IN const char* alias, OUT char** actual_alias,
    OUT char** docURL )
{
    HttpEntity* entity = NULL;
    int code;
    int retCode = UPNP_E_SUCCESS;
    xstring descURL;
    xstring actualAlias;
    xstring rootPath;
    xstring newAlias;
    xstring ipPortStr;

    try
    {
        code = config_url( doc, serverAddr, rootPath, ipPortStr );
        if ( code != UPNP_E_SUCCESS )
        {
            DBG(
                UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                    "config_url error code = %d\n", code); )
            throw code;
        }

        code = convert_to_http_entity( doc, &entity );
        if ( code != UPNP_E_SUCCESS )
        {
            throw code;
        }

        code = calc_alias( alias, rootPath.c_str(), newAlias );
        if ( code != UPNP_E_SUCCESS )
        {
            throw code;
        }

        code = http_AddAlias( newAlias.c_str(), entity, actualAlias );
        if ( code != 0 )
        {
            throw UPNP_E_OUTOF_MEMORY;
        }

        calc_descURL( ipPortStr.c_str(), actualAlias.c_str(), descURL );

        *actual_alias = actualAlias.detach();
        *docURL = descURL.detach();
    }
    catch ( int excepCode )
    {
        if ( entity != NULL )
        {
            delete entity;
        }

        retCode = excepCode;
    }
    catch ( OutOfMemoryException& /* e */ )
    {
        if ( entity != NULL )
        {
            delete entity;
        }

        retCode = UPNP_E_OUTOF_MEMORY;
    }

    return retCode;
}
#endif
