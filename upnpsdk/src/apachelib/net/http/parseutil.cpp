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

// $Revision: 1.1 $
// $Date: 2002/01/15 19:30:51 $
#include "../../inc/tools/config.h"
#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <genlib/net/netexception.h>
#include <genlib/net/http/tokenizer.h>
#include <genlib/net/http/parseutil.h>
#include <genlib/net/http/statuscodes.h>
#include <genlib/util/utilall.h>
#include <genlib/util/util.h>
#include <genlib/util/gmtdate.h>
#include <genlib/file/fileexceptions.h>
#include <genlib/util/memreader.h>


// HttpParseException
HttpParseException::HttpParseException( const char* s, int lineNumber )
    : BasicException("")
{
    if ( lineNumber != -1 )
    {
        char buf[100];
        
        sprintf( buf, "line %d: ", lineNumber );
        appendMessage( buf );
    }
    appendMessage( s );
}

//////
// callback used to read a HttpHeader value from a scanner
typedef HttpHeaderValue* (*ReadHttpValueCallback)
    ( Tokenizer& scanner );
    
typedef void (*AddValueToListCallback)
    ( HttpHeaderValueList& list, HttpHeaderValue* value );


// module vars ////////////////////

// static -- to do: determine os/version on the fly
static const char* gServerDesc = "Linux/6.0 UPnP/1.0 Intel UPnP/0.9";

static const char* gUserAgentDesc = "Intel UPnP/0.9";

///////////////////////////////////
    
// ******** CREATE callback functions ****
static HttpHeaderValue* CreateIdentifierValue()
{
    return new IdentifierValue;
}

static HttpHeaderValue* CreateIdentifierQValue()
{
    return new IdentifierQValue;
}

static HttpHeaderValue* CreateMediaRange()
{
    return new MediaRange;
}

static HttpHeaderValue* CreateLanguageTag()
{
    return new LanguageTag;
}

static HttpHeaderValue* CreateCacheDirective()
{
    return new CacheDirective;
}
// *********** end

struct SortedTableEntry
{
    char* name;
    int   id;
};

#define NUM_HEADERS 52

// table _must_ be sorted by header name
static SortedTableEntry HeaderNameTable[NUM_HEADERS] =
{
    { "ACCEPT",             HDR_ACCEPT },
    { "ACCEPT-CHARSET",     HDR_ACCEPT_CHARSET },
    { "ACCEPT-ENCODING",    HDR_ACCEPT_ENCODING },
    { "ACCEPT-LANGUAGE",    HDR_ACCEPT_LANGUAGE },
    { "ACCEPT-RANGES",      HDR_ACCEPT_RANGES },
    { "AGE",                HDR_AGE },
    { "ALLOW",              HDR_ALLOW },
    { "AUTHORIZATION",      HDR_AUTHORIZATION },
    
    { "CACHE-CONTROL",      HDR_CACHE_CONTROL },
    { "CALLBACK",           HDR_UPNP_CALLBACK },
    { "CONNECTION",         HDR_CONNECTION },
    { "CONTENT-ENCODING",   HDR_CONTENT_ENCODING },
    { "CONTENT-LANGUAGE",   HDR_CONTENT_LANGUAGE },
    { "CONTENT-LENGTH",     HDR_CONTENT_LENGTH },
    { "CONTENT-LOCATION",   HDR_CONTENT_LOCATION },
    { "CONTENT-MD5",        HDR_CONTENT_MD5 },
    { "CONTENT-TYPE",       HDR_CONTENT_TYPE },
    
    { "DATE",               HDR_DATE },
    
    { "ETAG",               HDR_ETAG },
    { "EXPECT",             HDR_EXPECT },
    { "EXPIRES",            HDR_EXPIRES },
    
    { "FROM",               HDR_FROM },
    
    { "HOST",               HDR_HOST },
    
    { "IF-MATCH",           HDR_IF_MATCH },
    { "IF-MODIFIED-SINCE",  HDR_IF_MODIFIED_SINCE },
    { "IF-NONE-MATCH",      HDR_IF_NONE_MATCH },
    { "IF-RANGE",           HDR_IF_RANGE },
    { "IF-UNMODIFIED-SINCE",HDR_IF_UNMODIFIED_SINCE },
    
    { "LAST-MODIFIED",      HDR_LAST_MODIFIED },
    { "LOCATION",           HDR_LOCATION },
    
    { "MAN",                HDR_UPNP_MAN },
    { "MAX-FORWARDS",       HDR_MAX_FORWARDS },
    
    { "NT",                 HDR_UPNP_NT },
    { "NTS",                HDR_UPNP_NTS },
    
    { "PRAGMA",             HDR_PRAGMA },
    { "PROXY-AUTHENTICATE", HDR_PROXY_AUTHENTICATE },
    { "PROXY-AUTHORIZATION",HDR_PROXY_AUTHORIZATION },
    
    { "RANGE",              HDR_RANGE },
    { "REFERER",            HDR_REFERER },
    
    { "SERVER",             HDR_SERVER },
    { "SID",                HDR_UPNP_SID },
    { "SOAPACTION",         HDR_UPNP_SOAPACTION },
    { "ST",                 HDR_UPNP_ST },
    
    { "TE",                 HDR_TE },
    { "TRAILER",            HDR_TRAILER },
    { "TRANSFER-ENCODING",  HDR_TRANSFER_ENCODING },
    
    { "USER-AGENT",         HDR_USER_AGENT },
    { "USN",                HDR_UPNP_USN },
    
    { "VARY",               HDR_VARY },
    { "VIA",                HDR_VIA },
    
    { "WARNING",            HDR_WARNING },
    { "WWW-AUTHENTICATE",   HDR_WWW_AUTHENTICATE },
};

// returns header ID; or -1 on error
int NameToID( const char* name,
    SortedTableEntry* table, int size, bool caseSensitive = true )
{
    int top, mid, bot;
    int cmp;
    
    top = 0;
    bot = size - 1;
    
    while ( top <= bot )
    {
        mid = (top + bot) / 2;
        if ( caseSensitive )
        {
            cmp = strcmp( name, table[mid].name );
        }
        else
        {
            cmp = strcasecmp( name, table[mid].name );
        }
        
        if ( cmp > 0 )
        {
            top = mid + 1;      // look below mid
        }
        else if ( cmp < 0 )
        {
            bot = mid - 1;      // look above mid
        }
        else    // cmp == 0
        {
            return table[mid].id;   // match
        }
    }
        
    return -1;  // header name not found
}

// returns textual representation of id in table;
//   NULL if id is invalid or missing from table
const char* IDToName( int id,
    SortedTableEntry* table, int size )
{
    if ( id < 0 )
        return NULL;
    
    for ( int i = 0; i < size; i++ )
    {
        if ( id == table[i].id )
        {
            return table[i].name;
        }
    }

    return NULL;
}


// skips all blank lines (lines with crlf + optional whitespace);
// removes leading whitespace from first non-blank line
static void SkipBlankLines( IN Tokenizer& scanner )
{
    Token *token;
    
    while ( true )
    {
        token = scanner.getToken();
        if ( !(token->tokType == Token::CRLF ||
               token->tokType == Token::WHITESPACE) )
        {
            // not whitespace or crlf; restore and done
            scanner.pushBack();
            return;
        }
    }
}

// reads and discards LWS
// returns true if matched; false if not matched LWS
bool SkipLWS( IN Tokenizer& scanner )
{
    Token* token;
    bool crlfMatch = true;
    
    // skip optional CRLF
    token = scanner.getToken();
    if ( token->tokType != Token::CRLF )
    {
        // not CRLF
        scanner.pushBack();
        crlfMatch = false;
    }
    
    // match whitespace
    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        // no match
        scanner.pushBack();
        
        // put back crlf as well, if read
        if ( crlfMatch )
            scanner.pushBack();

        return false;   // input does not match LWS
    }
    
    return true;    // match
}

// skips *LWS
void SkipOptionalLWS( IN Tokenizer& scanner )
{
    // skip LWS until no match
    while ( SkipLWS(scanner) )
    {
    }
}

static void SkipOptionalWhitespace( IN Tokenizer& scanner )
{
    Token* token;
    
    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
    }
}

// reads and discards a header from input stream
static void SkipHeader( IN Tokenizer& scanner )
{
    Token* token;
    
    // skip all lines that are continuation of the header
    while ( true )
    {
        // skip until eol
        do
        {
            token = scanner.getToken();
            
            // handle incomplete or bad input
            if ( token->tokType == Token::END_OF_STREAM )
            {
                scanner.pushBack();
                return;
            }
        } while ( token->tokType != Token::CRLF );
        
        // header continues or ends?
        token = scanner.getToken();
        if ( token->tokType != Token::WHITESPACE )
        {
            // possibly, new header starts here
            scanner.pushBack();
            break;
        }
    }
}

/*
// returns a string that matches the pattern
//
// pattern should not have Token::END_OF_STREAM,
// last pattern should be null, (0) and
//   patternSize excludes this null
static int MatchPattern( IN Tokenizer& scanner,
    IN char* pattern, IN int patternSize,
    OUT xstring& match )
{
    assert( pattern != NULL );
    assert( patternSize > 0 );

    match = "";

    scanner.getToken();
    for ( i = 0; i < patternSize; i++ )
    {
        token = scanner.getToken();
        if ( token.tokType != pattern[i] )
        {
            return -1;
        }
        match += token->s;
    }

    return 0;
}
*/

// reads header name from input
// call when scanning start of line
// header ::= headername : value
//
// throws ParseFailException if header identifier not found
static void ParseHeaderName( IN Tokenizer& scanner, OUT xstring& hdrName )
{
    Token *token;
    
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();     // put token back in stream
        // not an identifier
        throw HttpParseException(
            "ParseHeaderName()", scanner.getLineNum() );
    }
    
    hdrName = token->s;
}

// skips the ':' after header name and the all whitespace
//  surrounding it
// precond: cursor pointing after headerName
static void SkipColonLWS( IN Tokenizer& scanner )
{
    Token *token;

    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->s != ':' )
    {
        scanner.pushBack();
        throw HttpParseException( "SkipColonLWS(): expecting colon",
            scanner.getLineNum() );
    }
    
    SkipOptionalLWS( scanner ); 
}

// header ::= name : value
// returns value of a header
// called when currToken is pointing to a non-whitespace token
//   after colon
// NOTE: value can be blank; "" (len = 0)
static void ParseHeaderValue( IN Tokenizer& scanner, OUT xstring& value )
{
    Token* token;

    value = "";

    //SkipOptionalLWS( scanner ); // no leading whitespace

    // precond: next token is not whitespace

    while ( true )
    {
        // add all str till eol or end of stream
        while ( true )
        {
            token = scanner.getToken();
            if ( token->tokType == Token::END_OF_STREAM )
            {
                scanner.pushBack();
                throw HttpParseException(
                    "ParseHeaderValue(): unexpected end" );
            }
            if ( token->tokType == Token::CRLF )
            {
                break;  // end of value?
            }
            value += token->s;
        }

        token = scanner.getToken();
            
        // header continued on new line?
        if ( token->tokType != Token::WHITESPACE )
        {
            // no; header done
            scanner.pushBack();
            scanner.pushBack(); // return CRLF too
            break;
        }
        else
        {
            // header value continued
            value += ' ';
        }
    }

    // precond:
    // value is either empty "", or has one at least one
    //   non-whitespace char; also, no whitespace on left

    if ( value.length() > 0 )
    {
        const char *start_ptr, *eptr;
        int sublen;

        start_ptr = value.c_str();
        eptr = start_ptr + value.length() - 1;  // start at last char
        while ( *eptr == ' ' || *eptr == '\t' )
        {
            eptr--;
        }

        sublen = eptr - start_ptr + 1;

        if ( value.length() != sublen )
        {
            // trim right whitespace
            value.deleteSubstring( eptr - start_ptr + 1,
                value.length() - sublen );
        }
    }
    else
    {
        value = "";
    }
}

#ifdef no_such_deff
// ************************************************
// no special parsing performed for value
//  throws:
//    ParseNoMatchException: if error reading header name
//    ParseNoMatchException(PARSERR_COLON_NOT_FOUND) :
//          if colon following parse is not found
//         & scanner points non-colon token
static void ParseSimpleHeader( IN Tokenizer& scanner,
    OUT xstring& name, OUT xstring& value )
{
    // get header name
    ParseHeaderValue( scanner, name );
    
    // skip colon
    SkipOptionalLWS( scanner );
    
    Token *token = scanner.getToken();
    if ( token->s != ":" )
    {
        scanner.pushBack();
        
        HttpParseException e( "ParseSimpleHeader()", scanner.getLineNum() );
        e.setErrorCode( PARSERR_COLON_NOT_FOUND );
        throw e;
    }
    
    SkipOptionalLWS( scanner );
    
    // get header value
    ParseHeaderValue( scanner, name );
}


// Reads a request line
// throws ParseNoMatchException : code PARSERR_BAD_REQUEST_LINE
//    if request line is unacceptable
static void ParseRequestLine( IN Tokenizer& scanner,
    OUT xstring& method, OUT xstring& uri, OUT xstring& httpVers )
{
    Token* token;
    ParseNoMatchException e( "ParseSimpleHeader()" );
    e.setErrorCode( PARSERR_BAD_REQUEST_LINE );
    
    // get method
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
        throw e;
    }
    method = token->s;
    
    // skip spaces
    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
        throw e;
    }
    
    // get uri
    int count = 0;
    uri = "";

    while ( true )
    {
        token = scanner.getToken();
        if ( token->tokType == Token::CRLF ||
             token->tokType == Token::WHITESPACE )
        {
            if ( count == 0 )
            {
                // no uri; bad req line
                scanner.pushBack();
                throw e;
            }
            else
            {
                break;
            }
        }
        else
        {
            count++;
            uri += token->s;
        }
    }
}

// *************************************
#endif /* no_such_deff */


// reads comma separated list header values
// precond:
// scanner points after the header name;
// the colon and any whitespace around it has also been skipped
// minItems: at least this many items should be in list
// maxItems: no more than this many items should be in list
//   -1 means maxItems = infinity
// --------
// adds HttpHeaderValues to the list, 0..infin items could be
//  added
// throws HttpParseException.PARSERR_BAD_LISTCOUNT if
//  item count is not in range [minItems, maxItems]
// throws HttpParseException.PARSERR_BAD_COMMALIST if
//  list does not have a correct comma-separated format
static void ReadCommaSeparatedList( IN Tokenizer& scanner,
    OUT HttpHeaderValueList& list,
    IN int minItems, IN int maxItems,
    IN bool qIsUsed,
    IN CreateNewValueCallback createCallback,
    IN AddValueToListCallback addCallback = NULL )
{
    Token* token;
    HttpHeaderValue* value = NULL;
    int itemCount = 0;

    try
    {   
        // at this point, all LWS has been skipped;
        // empty list is indicated by a CRLF
        token = scanner.getToken();
        if ( token->tokType == Token::CRLF )
        {
            scanner.pushBack();
            throw -2;       // end of list
        }
        
        scanner.pushBack(); // return item for value to parse
    
        while ( true )
        {
            // create new item
            value = createCallback();
            if ( value == NULL )
            {
                throw OutOfMemoryException( "ReadCommaSeparatedList()" );
            }
            
            // allow q to be read
            if ( qIsUsed )
            {
                HttpQHeaderValue* qvalue;
                
                qvalue = (HttpQHeaderValue*) value;
                qvalue->qIsUsed = true;
            }
            
            // read list item
            value->load( scanner );
            
            if ( addCallback == NULL )
            {
                list.addAfterTail( value );
            }
            else
            {
                addCallback( list, value );
            }
            itemCount++;
    
            // skip comma and whitespace
            //
            
            SkipOptionalLWS( scanner );
        
            token = scanner.getToken();
            if ( token->s == "," )
            {
                // ok
            }
            else if ( token->tokType == Token::CRLF )
            {
                scanner.pushBack();
                throw -2;   // end of list
            }
            else
            {
                scanner.pushBack();
                throw -3;   // unexpected element
            }
    
            SkipOptionalLWS( scanner );
        }
    }
    catch ( int code )
    {
        
        if ( code == -2 )
        {
            HttpParseException e("ReadCommaSeparatedList():end of list",
                scanner.getLineNum() );
            e.setErrorCode( PARSERR_BAD_LISTCOUNT );
            
            // end of list
            if ( itemCount < minItems )
            {
                throw e;    // too few items
            }
            
            // maxItems == -1 means no limit
            if ( maxItems >= 0 && itemCount > maxItems )
            {
                throw e;    // too many items
            }
        }
        else if ( code == -3 )
        {
            // unexpected element where comma should've been
            HttpParseException e("ReadCommaSeparatedList(): comma expected",
                scanner.getLineNum() );
            e.setErrorCode( PARSERR_BAD_COMMALIST );
            throw e;
        }
    }
    catch ( HttpParseException& )
    {
       if( value ) 
	 delete value; 
    }

}

////////////////////
// returns integer number 0..INT_MAX from scanner
// throws ParseFailException.PARSERR_BAD_NUMBER if number is
//  invalid or negative; the bad token is put back in the scanner
//
static int loadNum( Tokenizer& scanner, int base )
{
    Token *token;
    char *endptr;
    int num = 0;

    token = scanner.getToken();

    errno = 0;  // set this to explicitly avoid bugs
    num = strtol( token->s.c_str(), &endptr, base );

    if ( *endptr != '\0' || num < 0 )
    {
        xstring msg("loadNum(): bad number " );
        msg += token->s;
        scanner.pushBack();
        throw HttpParseException( msg.c_str(), scanner.getLineNum() );
    }
    if ( (num == LONG_MIN || num == LONG_MAX) && (errno == ERANGE) )
    {
        xstring msg("loadNum(): out of range '" );
        msg += token->s;
        msg += "'";
        scanner.pushBack();
        throw HttpParseException( msg.c_str(), scanner.getLineNum() );
    }
    
    return num;
}

void NumberToString( int num, xstring& s )
{
    char buf[50];
    
    sprintf( buf, "%d", num );
    s += buf;
}


static void LoadDateTime( Tokenizer& scanner, tm& datetime )
{
    xstring rawValue;
    int numCharsParsed;
    int success;

    ParseHeaderValue( scanner, rawValue );
    
    const char* cstr = rawValue.c_str();
    
    success = ParseDateTime( cstr, &datetime,
        &numCharsParsed );

    if ( success == -1 )
    {
        throw HttpParseException( "LoadDateTime() bad date/time",
            scanner.getLineNum() );
    }   
        
    // skip all whitespace after date/time
    const char* s2 = &cstr[numCharsParsed];
    while ( *s2 == ' ' || *s2 == '\t' )
    {
        s2++;
    }
    
    if ( *s2 != '\0' )
    {
        throw HttpParseException( "LoadDateTime() bad trailer",
            scanner.getLineNum() );
    }   
}


static void LoadUri( IN Tokenizer& scanner, OUT uri_type& uri,
    OUT xstring& uriStr )
{
    Token *token;

    uriStr = "";
    while ( true )
    {
        token = scanner.getToken();
        if ( token->tokType == Token::IDENTIFIER ||
             token->tokType == Token::SEPARATOR ||
             token->tokType == Token::QUOTED_STRING )
        {
            uriStr += token->s;
        }
        else
        {
            scanner.pushBack();
            break;
        }
    }


    if ( uriStr.length() == 0 )
    {
        throw HttpParseException( "LoadUri(): no uri",
            scanner.getLineNum() );
    }

    int len;

    len = parse_uri( (char*)(uriStr.c_str()), uriStr.length(),
        &uri );

    if ( len < 0 )
    {
        throw HttpParseException( "LoadUri(): bad uri",
            scanner.getLineNum() );
    }
}

static void ParseMajorMinorNumbers( const char *s,
    int& majorVers, int& minorVers )
{
    int major, minor;
    char* endptr;
    char* s2;

    try
    {
        errno = 0;
        major = strtol( s, &endptr, 10 );
        if ( (major < 0) ||
             ((major == LONG_MAX || major == LONG_MIN) && (errno == ERANGE))
            )
        {
            throw -1;
        }

        if ( *endptr != '.' )
        {
            throw -1;
        }

        s2 = endptr + 1;
        errno = 0;
        minor = strtol( s2, &endptr, 10 );
        if ( (minor < 0) ||
             (*endptr != 0) ||
             ((minor == LONG_MAX || minor == LONG_MIN) && (errno == ERANGE))
            )
        {
            throw -1;
        }

        majorVers = major;
        minorVers = minor;
    }
    catch ( int code )
    {
        if ( code == -1 )
        {
            HttpParseException e("ParseMajorMinorNumbers(): "
                "bad http version: " );
            e.appendMessage( s );
            throw e;
        }
    }
}

static void ParseHttpVersion( IN Tokenizer& scanner,
    OUT int& majorVers, OUT int& minorVers )
{
    Token* token;
    HttpParseException eVers( "ParseHttpVersion(): bad http version" );
    
    token = scanner.getToken();
    if ( token->s.compareNoCase("HTTP") != 0 )
    {
        throw eVers;
    }
    
    token = scanner.getToken();
    
    // skip optional whitespace
    if ( token->tokType == Token::WHITESPACE )
    {
        token = scanner.getToken();
    }
    
    if ( token->s != '/' )
    {
        throw eVers;
    }

    token = scanner.getToken();
    if ( token->tokType == Token::WHITESPACE )
    {
        // skip optional whitespace and read version
        token = scanner.getToken();
    }

    ParseMajorMinorNumbers( token->s.c_str(), majorVers, minorVers );


    /* ***************** old code ************
    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
    }
    
    majorVers = loadNum( scanner, 10 );
    
    token = scanner.getToken();
    if ( token->tokType == Token::WHITESPACE )
    {
        token = scanner.getToken();
    }
    
    if ( token->s != '.' )
    {
        throw eVers;
    }
    
    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
    }
    
    minorVers = loadNum( scanner, 10 );
    ******************************** old code end ***** */
}

static void PrintHttpVersion( int major, int minor, xstring& s )
{
    s += "HTTP/";
    NumberToString( major, s );
    s += '.';
    NumberToString( minor, s );
}

static void HeaderValueListToString( IN HttpHeaderValueList& list,
    INOUT xstring& s )
{
    HttpHeaderValueNode* node;
    HttpHeaderValue* value;
    
    node = list.getFirstItem();
    for ( int i = 0; i < list.length(); i++ )
    {
        value = (HttpHeaderValue *) node->data;
        value->toString( s );
        node = list.next( node );
    }
}



//static void QToString( float q, INOUT xstring& s )
//{
//  char buf[50];
    
//  sprintf( buf, ";q=%1.3f", q );
//  s += buf;
//}


// match: ( 0 ['.' 0*3DIGIT] ) | ( '1' 0*3('0') )
// returns 0.0 to 1.0 on success;
// throws HttpParseExecption
static float ParseQValue( const char *s )
{
    char c;
    int i;
    char dot;
    int count;
    char *endptr;
    float value = 0.0;
    bool partial;
    
    i = 0;
    partial = true;
    
    try
    {
        c = s[i++];
        if ( !(c == '0' || c == '1') )
        {
            throw -1;
        }
        
        dot = s[i++];
        if ( dot == '.' )
        {
            if ( c == '1' )
            {
                // match 3 zeros
                for ( count = 0; count < 3; count++ )
                {
                    c = s[i++];
                    if ( c == '\0' )
                    {
                        partial = false;
                        break;  // done; success
                    }
                    if ( c != '0' )
                    {
                        throw -1;
                    }
                }
            }
            else
            {
                // c == 0, match digits
                for ( count = 0; count < 3; count++ )
                {
                    c = s[i++];
                    if ( c == 0 )
                    {
                        partial = false;
                        break;
                    }
                    if ( !(c >= '0' && c <= '9') )
                    {
                        throw -1;
                    }
                }
            }
            // all chars must be processed
            if ( partial && s[i] != '\0' )
            {
                throw -1;
            }
        }
        else if ( dot != '\0' )
        {
            throw -1;
        }
        
        value = (float)strtod( s, &endptr );
    }
    catch ( int code )
    {
        if ( code == -1 )
        {
            xstring msg("ParseQValue(): ");
            msg += s;
            throw HttpParseException( msg.c_str() );
        }
    }
    
    return value;
}

// match: q = floatValue
static float LoadQValue( Tokenizer& scanner )
{
    Token* token;
    float q;
    
    token = scanner.getToken();
    if ( token->s != 'q' )
    {
        scanner.pushBack();
        throw HttpParseException( "LoadQValue(): 'q' expected",
            scanner.getLineNum() );
    }
    
    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->s != '=' )
    {
        scanner.pushBack();
        throw HttpParseException( "LoadQValue(): '=' expected",
            scanner.getLineNum() );
    }
    
    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
        throw HttpParseException( "LoadQValue(): number expected",
            scanner.getLineNum() );
    }
    
    // ident to
    q = ParseQValue( token->s.c_str() );
    if ( q < 0 )
    {
        throw HttpParseException( "LoadQValue(): invalid value",
            scanner.getLineNum() );
    }
    
    return q;
}

static void PrintCommaSeparatedList( HttpHeaderValueList& list,
    xstring& s )
{
    bool first = true;
    HttpHeaderValueNode* node;
    
    node = list.getFirstItem();
    while ( node != NULL )
    {
        if ( !first )
        {
            s += ", ";
        }
        else
        {
            first = false;
        }
        
        HttpHeaderValue* data;
        data = (HttpHeaderValue *) node->data;
        data->toString( s );
        node = list.next( node );
    }
}

// can't include ctype.h because of a xerces bug
static bool IsAlpha( char c )
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// returns true if the tag matches language tag syntax
//  (1*8ALPHA * ("-" 1*8ALPHA)) | *
static bool ValidLanguageTag( const char* tag )
{
    int i;
    char c;
    int count;
    
    if ( strcmp(tag, "*") == 0 )
    {
        return true;
    }
    
    i = 0;
    while ( true )
    {
        // match 1..8 alpha
        for ( count = 0; count < 8; count++ )
        {
            c = tag[i++];
            if ( !IsAlpha(c) )
            {
                if ( count == 0 )
                {
                    return false;   // no alpha found
                }
                i--;    // put back char
                break;  // okay; valid alpha str
            }
        }
        
        c = tag[i++];
        
        if ( c == '\0' )
        {
            return true;    // end of valid tag
        }
        
        if ( c != '-' )
        {
            return false;   // expected '-'
        }
    }
    
    return false;       // this line never executes
}


// parses a header from input stream; returns header type and
//  and value
// on success: headerID > 0  and returnValue != NULL
//             headerID == -1 and returnValue = UnknownHeader
// on failure: returnValue == NULL
static HttpHeaderValue* ParseHeader( IN Tokenizer& scanner,
    OUT int& headerID )
{
    int id;
    Token* token;
    xstring name;

    // read header name
    ParseHeaderName( scanner, name );

    // map name to id
    id = NameToID( name.c_str(), HeaderNameTable, NUM_HEADERS, false );
    if ( id < 0 )
    {
        // header type currently not known; process raw

        id = HDR_UNKNOWN;

        scanner.pushBack(); // return unknown header name
    }

    // skip past colon;
    if ( id != HDR_UNKNOWN )
    {
        SkipColonLWS( scanner );
    }
    // now pointing at first non-whitespace token after colon

    HttpHeaderValue *header = NULL;
        
    switch ( id )
    {
        case HDR_ACCEPT:
            header = new CommaSeparatedList( true, CreateMediaRange );
            break;
            
        case HDR_ACCEPT_CHARSET:
        case HDR_ACCEPT_ENCODING:
            header = new CommaSeparatedList( true, CreateIdentifierQValue, 1 );
            break;
            
        case HDR_ACCEPT_LANGUAGE:
            header = new CommaSeparatedList( true, CreateLanguageTag, 1 );
            break;

        case HDR_AGE:
        case HDR_CONTENT_LENGTH:
        case HDR_MAX_FORWARDS:
            header = new HttpNumber;
            break;          
            
        case HDR_ALLOW:
        case HDR_CONNECTION:
        case HDR_CONTENT_ENCODING:
        case HDR_TRANSFER_ENCODING:
            header = new CommaSeparatedList( false, CreateIdentifierValue );
            break;
            
        case HDR_CACHE_CONTROL:
            header = new CommaSeparatedList( false, CreateCacheDirective );
            break;
            
        case HDR_CONTENT_LANGUAGE:
            header = new CommaSeparatedList( false, CreateLanguageTag );
            break;
            
        case HDR_CONTENT_LOCATION:
        case HDR_LOCATION:
            header = new UriType;
            break;
            
        case HDR_CONTENT_TYPE:
            header = new MediaRange;
            break;
            
        case HDR_DATE:
        case HDR_EXPIRES:
        case HDR_IF_MODIFIED_SINCE:
        case HDR_IF_UNMODIFIED_SINCE:
        case HDR_LAST_MODIFIED:
            header = new HttpDateValue;
            break;
            
        case HDR_HOST:
            header = new HostPortValue;
            break;
            
        case HDR_RETRY_AFTER:
            header = new HttpDateOrSeconds;
            break;
        
		/*	--trimming down parser
        case HDR_UPNP_NTS:
            header = new NTSType;
            break;
		*/

        case HDR_UNKNOWN:
            header = new UnknownHeader;
            break;

        default:
            header = new RawHeaderValue;
    }
    
    if ( header == NULL )
    {
        throw OutOfMemoryException( "ParseHeader()" );
    }

    try
    {
        header->load( scanner );

        // skip whitespace after value
        SkipOptionalLWS( scanner );

        // end of header -- crlf
        token = scanner.getToken();
        if ( token->tokType != Token::CRLF )
        {
            throw HttpParseException();
        }
    }
    catch ( HttpParseException& /* e */ )
    {
            SkipHeader( scanner );  // bad header -- ignore it

            id = -2;
            delete header;
            header = NULL;
    }

    headerID = id;
        
    return header;
}

// precond: scanner pointing to the line after the start line
static void ParseHeaders( IN Tokenizer& scanner,
    OUT HttpHeaderList& list )
{
    Token *token;
    int id;
    HttpHeaderValue* headerValue;
    HttpHeader* header;
    
    while ( true )
    {
        token = scanner.getToken();
        if ( token->tokType == Token::CRLF )
        {
            // end of headers
            break;
        }
        else if ( token->tokType == Token::END_OF_STREAM )
        {
            // bad format
            scanner.pushBack();
            throw HttpParseException( "ParseHeaders(): unexpected end of msg" );
            break;
        }
        else
        {
            // token was probably start of a header
            scanner.pushBack();
        }
    
        headerValue = ParseHeader( scanner, id );
        if ( headerValue != NULL )
        {
            header = new HttpHeader;
            if ( header == NULL )
            {
                throw OutOfMemoryException( "ParseHeaders()" );
            }

            header->type = id;
            header->value = headerValue;
            
            list.addAfterTail( header );
        }
    }
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

// HttpHeaderQValue
void HttpQHeaderValue::toString( xstring& s )
{
    if ( qIsUsed )
    {
        char buf[100];
        sprintf( buf, ";q=%1.3f", q );
        s += buf;
    }
}

void HttpQHeaderValue::loadOptionalQValue( IN Tokenizer& scanner )
{
    q = 1.0;    // default value
    
    // try getting q
    try
    {
        Token* token;
        
        if ( qIsUsed )
        {
            SkipOptionalLWS( scanner );
        
            token = scanner.getToken();
            if ( token->s != ';' )
            {
                scanner.pushBack();
                //DBG( printf("HttpQHeaderValue: did not find ;\n"); )
                throw 1;
            }
            SkipOptionalLWS( scanner );
            q = LoadQValue( scanner );
        }
    }
    catch ( int /* code */ )
    {
        // no q value
        //DBG( printf("HttpQHeaderValue: did not find semicolon\n"); )
    }
}

// HttpNumber
void HttpNumber::toString( xstring& s )
{
    char buf[50];
    
    sprintf( buf, "%d", num );
    s += buf;
}

void HttpNumber::load( Tokenizer& scanner )
{
    num = loadNum( scanner, 10 );   
}

// HttpHexNumber
void HttpHexNumber::toString( xstring& s )
{
    char buf[50];
    
    sprintf( buf, "%X", num );
    s += buf;
}

void HttpHexNumber::load( Tokenizer& scanner )
{
    num = loadNum( scanner, 16 );
}

// MediaExtension
void MediaExtension::toString( xstring& s )
{
    s += ';';
    s += name;
    s += "=";
    s += value;
}

// match: ext = value
void MediaExtension::load( Tokenizer& scanner )
{
    Token* token;
    HttpParseException e("MediaExtension::load()",
        scanner.getLineNum() );
    
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        throw e;
    }
    name = token->s;
    
    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->s != "=" )
    {
        throw e;
    }
    
    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER &&
         token->tokType != Token::QUOTED_STRING )
    {
        throw e;
    }
    value = token->s;
}

// MediaParam
void MediaParam::load( Tokenizer& scanner )
{
    Token* token;
    
    token = scanner.getToken();
    if ( token->s != ';' )
    {
        scanner.pushBack();
        throw HttpParseException( "MediaParam::load(): ; missing",
            scanner.getLineNum() );
    }
    
    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->s == 'q' )
    {
        scanner.pushBack();
        q = LoadQValue( scanner );
    }
    
    SkipOptionalLWS( scanner );
    
    while ( true )
    {
        MediaExtension* ext;
        
        token = scanner.getToken();
        if ( token->s != ';' )
        {
            scanner.pushBack();
            break;  // no more extensions
        }
        
        ext = new MediaExtension();
        if ( ext == NULL )
        {
            throw OutOfMemoryException( "MediaParam::load()" );
        }

        SkipOptionalLWS( scanner );

        ext->load( scanner );
        
        extList.addAfterTail( ext );
    }
}

void MediaParam::toString( xstring& s )
{
    HttpQHeaderValue::toString( s );    // print q
    
    HttpHeaderValue* value;
    HttpHeaderValueNode* node;
    
    node = extList.getFirstItem();
    while ( node != NULL )
    {
        value = (HttpHeaderValue *)node->data;
        value->toString( s );
        
        node = extList.next( node );
    }
}

// MediaRange
void MediaRange::toString( xstring& s )
{
    s += type;
    s += '/';
    s += subtype;

    mparam.toString( s );
}

void MediaRange::load( Tokenizer& scanner )
{
    Token* token;
    
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
        throw HttpParseException( "MediaRange::load(): type expected",
            scanner.getLineNum() );
    }
    type = token->s;

    SkipOptionalLWS( scanner );

    token = scanner.getToken();
    if ( token->s != '/' )
    {
        scanner.pushBack();
        throw HttpParseException( "MediaRange::load(): / expected",
            scanner.getLineNum() );
    }

    SkipOptionalLWS( scanner );

    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER ||
         (type == '*' && token->s != '*')
        )
    {
        scanner.pushBack();
        throw HttpParseException( "MediaRange::load(): subtype expected",
            scanner.getLineNum() );
    }
    subtype = token->s;

    mparam.load( scanner );
}

// returns: 0 same, -1 this < r, 1 this > r, -2 unrelated
// returns true if 'this' range has higher precedence than r
int MediaRange::compare( const MediaRange& r ) const
{
    if ( mparam.q > r.mparam.q )
    {
        return 1;
    }
    else if ( mparam.q < r.mparam.q )
    {
        return -1;
    }
    else    // equal
    {
        bool t = false, rt = false;
        
        t = (type == '*');
        rt = (type == '*');
        
        if ( t && !rt ) return -1;
        else if ( !t && rt ) return 1;
        else if ( type == r.type )
        {
            if ( subtype == r.subtype )
            {
                return 0;
            }
            else if ( subtype == '*' && r.subtype != '*' )
            {
                return 1;
            }
            else if ( subtype != '*' && r.subtype == '*' )
            {
                return -1;
            }
            else
            {
                return 0;
            }
        }
        else if ( type != r.type )
        {
            return -2;
        }
    }
    
    return -2;
}

// MediaRangeList
void MediaRangeList::toString( xstring& s )
{
    HeaderValueListToString( mediaList, s );
}

/*
static void ReadCommaSeperatedList( IN Tokenizer& scanner,
    OUT HttpHeaderValueList& list,
    IN int minItems, IN int maxItems,
    IN ReadHttpValueCallback callback,
    IN AddValueToListCallback addCallback = NULL )
*/


// add media range in a sorted manner
static void AddMediaRangeToList( HttpHeaderValueList& list,
    HttpHeaderValue *value )
{
    MediaRange* b = (MediaRange*) value;
    MediaRange* a;
    HttpHeaderValueNode* node;
    int inserted = false;
    int comp = 0;
    
    node = list.getFirstItem();
    while ( node != NULL )
    {
        a = (MediaRange*) node->data;
        comp = a->compare( *b );
        if ( comp == 1 || comp == 0 )
        {
            list.addBefore( node, b );
            inserted = true;
            break;
        }
        else if ( comp == -1 )
        {
            list.addAfter( node, b );
            inserted = true;
            break;
        }

        node = list.next( node );       
        
    }
    
    if ( !inserted )
    {
        list.addAfterTail( value );
    }
}

void MediaRangeList::load( Tokenizer& scanner )
{
    ReadCommaSeparatedList( scanner, mediaList,
        0, -1, true, CreateMediaRange, AddMediaRangeToList );
}


// CommaSeparatedList
void CommaSeparatedList::toString( xstring& s )
{
    PrintCommaSeparatedList( valueList, s );
}

// add value sorted list; sorted descending 'q'
static void AddValueToSortedListCallback( HttpHeaderValueList& list,
    HttpHeaderValue *value )
{
    HttpQHeaderValue *a, *b;
    HttpHeaderValueNode* node;
    bool inserted = false;
    
    b = (HttpQHeaderValue *)value;
    
    node = list.getFirstItem();
    while ( node != NULL )
    {
        a = (HttpQHeaderValue*) node->data;
        if ( b->q > a->q )
        {
            list.addBefore( node, b );
            inserted = true;
            break;
        }

        node = list.next( node );       
    }
    
    if ( !inserted )
    {
        list.addAfterTail( value );
    }
}


void CommaSeparatedList::load( Tokenizer& scanner )
{
    AddValueToListCallback addCallback = NULL;
    
    if ( qUsed )
    {
        addCallback = AddValueToSortedListCallback;
    }
    
    ReadCommaSeparatedList( scanner, valueList,
        minItems, maxItems, qUsed, createCallback, addCallback );
}

// IdentifierValue
void IdentifierValue::toString( xstring& s )
{
    s += value;
}

void IdentifierValue::load( Tokenizer& scanner )
{
    Token* token;
    
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
        throw HttpParseException(
            "IdentifierValue::load(): identifier expected",
            scanner.getLineNum() );
    }
    
    value = token->s;
}

// IdentifierQValue
void IdentifierQValue::toString( xstring& s )
{
    s += value;
    HttpQHeaderValue::toString( s );    // print q
}

void IdentifierQValue::load( Tokenizer& scanner )
{
    Token *token;
    
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
        throw HttpParseException(
            "IdentifierQValue::load(): identifer expected",
            scanner.getLineNum() );
    }
    value = token->s;
    
    if ( qIsUsed )
    {
        loadOptionalQValue( scanner );
    }
}


// LanguageTag : public HttpHeaderValue
void LanguageTag::toString( xstring& s )
{
    s += lang;
    if ( qIsUsed )
    {
        HttpQHeaderValue::toString( s );
    }
}

void LanguageTag::load( Tokenizer& scanner )
{
    Token* token;
    
    token = scanner.getToken();
    if ( !ValidLanguageTag(token->s.c_str()) )
    {
        scanner.pushBack();
        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "invalid lang = %s\n", token->s.c_str()); )
        throw HttpParseException ( "LanguageTag::load() invalid language tag" );
    }

    if ( qIsUsed )
    {
        loadOptionalQValue( scanner );
    }
    
    lang = token->s;    
}

bool LanguageTag::setTag( const char* newLangTag )
{
    if ( !ValidLanguageTag(newLangTag) )
    {
        return false;
    }
    lang = newLangTag;
    return true;
}


// CacheDirective
void CacheDirective::toString( xstring& s )
{
    switch ( type )
    {
        case NO_CACHE:
            s += "no-cache";
            break;
            
        case NO_CACHE_FIELDS:
            s += "no-cache=";
            assert( fields.length() > 0 );
            s += fields;
            break;
            
        case NO_STORE:
            s += "no-store";
            break;
            
        case MAX_AGE:
            s += "max-age = ";
            assert( secondsValid );
            deltaSeconds.toString( s );
            break;
            
        case MAX_STALE:
            s += "max-stale";
            if ( secondsValid )
            {
                s += " = ";
                deltaSeconds.toString( s );
            }
            break;
            
        case MIN_FRESH:
            s += "min-fresh = ";
            assert( secondsValid );
            deltaSeconds.toString( s );
            break;
            
        case NO_TRANSFORM:
            s += "no-transform";
            break;
            
        case ONLY_IF_CACHED:
            s += "only-if-cached";
            break;
            
        case PUBLIC:
            s += "public";
            break;
            
        case PRIVATE:
            s += "private";
            if ( fields.length() > 0 )
            {
                s += " = ";
                s += fields;
            }
            break;
            
        case MUST_REVALIDATE:
            s += "must-revalidate";
            break;
            
        case PROXY_REVALIDATE:
            s += "proxy-revalidate";
            break;
            
        case S_MAXAGE:
            s += "max age = ";
            assert( secondsValid );
            deltaSeconds.toString( s );
            break;
            
        case EXTENSION:
            s += extensionName;
            s += " = ";
            if ( extType == IDENTIFIER )
            {
                s += extensionValue;
            }
            else if ( extType == QUOTED_STRING )
            {
                s += '"';
                s += extensionValue;
                s += '"';
            }
            else
            {
                assert( 0 );    // unknown type
            }
            break;
        
        case UNKNOWN:
        default:
            assert( 0 );
            break;
    }
}


void CacheDirective::load( Tokenizer& scanner )
{
    Token* token;

    // init
    secondsValid = false;
    fields = "";
    deltaSeconds.num = 0;
    extensionName = "";
    extensionValue = "";


    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
        throw HttpParseException( "CacheDirective::load() expected identifier",
            scanner.getLineNum() );
    }
    
    if ( token->s == "no-cache" )
    {
        bool fieldsRead;
        
        fieldsRead = readFields( scanner );
        if ( fieldsRead )
        {
            type = NO_CACHE_FIELDS;
        }
        else
        {
            type = NO_CACHE;
        }
    }
    else if ( token->s == "no-store" )
    {
        type = NO_STORE;
    }
    else if ( token->s == "max-age" )
    {
        type = MAX_AGE;
        readDeltaSeconds( scanner, false );
    }
    else if ( token->s == "max-stale" )
    {
        type = MAX_STALE;
        readDeltaSeconds( scanner, true );
    }
    else if ( token->s == "min-fresh" )
    {
        type = MIN_FRESH;
        readDeltaSeconds( scanner, false );
    }
    else if ( token->s == "no-transform" )
    {
        type = NO_TRANSFORM;
    }
    else if ( token->s == "only-if-cached" )
    {
        type = ONLY_IF_CACHED;
    }
    else if ( token->s == "public" )
    {
        type = PUBLIC;
    }
    else if ( token->s == "private" )
    {
        type = PRIVATE;
        readFields( scanner );
    }
    else if ( token->s == "must-revalidate" )
    {
        type = MUST_REVALIDATE;
    }
    else if ( token->s == "proxy-revalidate" )
    {
        type = PROXY_REVALIDATE;
    }
    else if ( token->s == "s-maxage" )
    {
        type = S_MAXAGE;
        readDeltaSeconds( scanner, false );
    }
    else
    {
        type = EXTENSION;
        extensionName = token->s;
        readExtension( scanner );
    }
}

// returns true if any fields were read
bool CacheDirective::readFields( Tokenizer& scanner )
{
    Token* token;

    SkipOptionalLWS( scanner );
    token = scanner.getToken();
    if ( token->s == '=' )
    {
        // match: '=' 1#fields
            
        SkipOptionalLWS( scanner );
        token = scanner.getToken();
        if ( token->tokType != Token::QUOTED_STRING )
        {
            scanner.pushBack();
            throw HttpParseException(
                "CacheDirective::load() expected quoted fields",
                scanner.getLineNum() );
        }

        fields = token->s;

        return true;

        /* ********** old code *************
        token = scanner.getToken();
        if ( token->s != '"' )
        {
            scanner.pushBack();
            throw HttpParseException(
                "CacheDirective::readFields() expected \" quote",
                scanner.getLineNum() );
        }
        SkipOptionalLWS( scanner );
        
        fields.load( scanner );
            
        SkipOptionalLWS( scanner );
        token = scanner.getToken();
        if ( token->s != '"' )
        {
            scanner.pushBack();
            throw HttpParseException(
                "CacheDirective:load() expected close quote",
                scanner.getLineNum() );
        }

        return true;
         ****************** old code end *******************/
    }
    else
    {
        scanner.pushBack();
        return false;
    }
}

void CacheDirective::readDeltaSeconds( Tokenizer& scanner, bool optional )
{
    Token* token;
    
    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->s == '=' )
    {
        SkipOptionalLWS( scanner );
        deltaSeconds.load( scanner );
        secondsValid = true;
    }
    else
    {
        scanner.pushBack();
        if ( !optional )
        {
            throw HttpParseException(
                "CacheDirective::readDeltaSeconds() expected =",
                scanner.getLineNum() );
        }
    }
}

void CacheDirective::readExtension( Tokenizer& scanner )
{
    Token* token;
    
    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->s != '=' )
    {
        scanner.pushBack();
        throw HttpParseException(
            "CacheDirective::readExtension() expected =",
            scanner.getLineNum() );
    }
    
    SkipOptionalLWS( scanner );
    
    token = scanner.getToken();
    if ( token->tokType == Token::IDENTIFIER )
    {
        extType = IDENTIFIER;
        extensionValue = token->s;
    }
    else if ( token->tokType == Token::QUOTED_STRING )
    {
        extType = QUOTED_STRING;
        extensionValue = token->s;
    }
    else
    {
        scanner.pushBack();
        throw HttpParseException(
            "CacheDirective::readExtension() expected ident or quotedstring",
            scanner.getLineNum() );
    }
}

//////////////////////////////////
// HttpDateValue
void HttpDateValue::load( Tokenizer& scanner )
{
    LoadDateTime( scanner, gmtDateTime );
}

void HttpDateValue::toString( xstring& s )
{
    char *str;
    
    str = DateToString( &gmtDateTime );
    s += str;
    free( str );
}


//////////////////////////////////
// HttpDateOrSeconds
void HttpDateOrSeconds::load( Tokenizer& scanner )
{
    isSeconds = true;
    
    // is it seconds?
    try
    {
        seconds = loadNum( scanner, 10 );
    }
    catch ( HttpParseException& e )
    {
        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "httpexception: %s\n", e.getMessage()); )
        isSeconds = false;
    }
    
    // then date/time maybe?
    if ( !isSeconds )
    {
        LoadDateTime( scanner, gmtDateTime );
    }
}

void HttpDateOrSeconds::toString( xstring& s )
{
    if ( isSeconds )
    {
        NumberToString( seconds, s );
    }   
    else
    {
        char *str;
        str = DateToString( &gmtDateTime );
        s += str;
        free( str );
    }
}

//////////////////////////////////
// HostPortValue
void HostPortValue::load( Tokenizer& scanner )
{
    Token* token;
    
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
        throw HttpParseException( "HostPortValue::load()",
            scanner.getLineNum() );
    }
    
    xstring hport = token->s;
    int len;
    
    token = scanner.getToken();
    if ( token->s == ':' )
    {
        hport += ':';
        token = scanner.getToken();
        if ( token->tokType != Token::IDENTIFIER )
        {
            scanner.pushBack();
            throw HttpParseException( "HostPortValue::load()",
                scanner.getLineNum() );
        }
        hport += token->s;
    }
    else
    {
        scanner.pushBack();
    }
    
    len = parse_hostport( (char *)(hport.c_str()), hport.length(),
        &hostport );
    if ( len < 0 )
    {
        throw HttpParseException( "HostPortValue::load()",
            scanner.getLineNum() );
    }
    tempBuf = hport;
}

void HostPortValue::toString( xstring& s )
{
    s += tempBuf;
}

bool HostPortValue::setHostPort( const char* hostName, unsigned short port )
{
    xstring s;
    char buf[50];
    int len;
    hostport_type hport;
    
    s = hostName;
    sprintf( buf, ":%d", port );
    s += buf;
    
    len = parse_hostport( (char *)(s.c_str()), s.length(), &hport );
    if ( len < 0 )
    {
        return false;
    }

    tempBuf = s;

    hostport = hport;
    return true;
}

void HostPortValue::getHostPort( sockaddr_in* addr )
{
    *addr = hostport.IPv4address;
}

///////////
// UriType
void UriType::load( Tokenizer& scanner )
{
    // uri is composed of identifiers and separators
    LoadUri( scanner, uri, tempBuf );
}


void UriType::toString( xstring& s )
{
    s += tempBuf;
}

bool UriType::setUri( const char* newUriValue )
{
    xstring uriStr;
    int len;
    uri_type temp;
    
    if ( newUriValue == NULL )
    {
        return false;
    }
    
    uriStr = newUriValue;
    
    len = parse_uri( (char*)(uriStr.c_str()), uriStr.length(),
        &temp );
    if ( len < 0 )
    {
        return false;
    }
    
    uri = temp;

    tempBuf = uriStr;

    return true;
}

const char* UriType::getUri() const
{
    return tempBuf.c_str();
}

int UriType::getIPAddress( OUT sockaddr_in& address )
{
    if ( uri.type != ABSOLUTE )
    {
        return -1;
    }

    memcpy( &address, &uri.hostport.IPv4address, sizeof(sockaddr_in) );
    return 0;
}

/* -------------xxxxxxxxxxxxxxxxxxxxxxxx
/////////////////
// NTSType
void NTSType::load( Tokenizer& scanner )
{
    xstring strVal;

    ParseHeaderValue( scanner, strVal );

    if ( strVal == "upnp:propchange" )
        value = UPNP_PROPCHANGE;
    else if ( strVal == "ssdp:alive" )
        value = SSDP_ALIVE;
    else if ( strVal == "ssdp:byebye" )
        value = SSDP_BYEBYE;
    else
    {
        DBG( printf("unknown nts: %s\n", strVal.c_str()); )
        throw HttpParseException( "NTSType::load() unknown NTS",
            scanner.getLineNum() );
    }
        
}

void NTSType::toString( xstring& s )
{
    if ( value == UPNP_PROPCHANGE )
        s += "upnp:propchange";
    else if ( value == SSDP_ALIVE )
        s += "ssdp:alive";
    else if ( value == SSDP_BYEBYE )
        s += "ssdp:byebye";
    else
        assert( 0 );    // invalid NTS
}

  ----------------xxxxxxxxxxxxxxxxxxxxxxxxx */

///////////////////////////////
// RawHeaderValue
void RawHeaderValue::load( Tokenizer& scanner )
{
    ParseHeaderValue( scanner, value );     
}

void RawHeaderValue::toString( xstring& s )
{
    s += value;
}

////////////////////////////
// UnknownHeader
void UnknownHeader::load( Tokenizer& scanner )
{
    ParseHeaderName( scanner, name );
    SkipColonLWS( scanner );
    ParseHeaderValue( scanner, value );
}

void UnknownHeader::toString( xstring& s )
{
    s += name;
    s += ": ";
    s += value;
}

//////////////////////////////
// HttpHeader
HttpHeader::HttpHeader()
{
    value = NULL;
}

HttpHeader::~HttpHeader()
{
    delete value;
}

void HttpHeader::toString( xstring& s )
{
    if ( type != HDR_UNKNOWN )
    {
        const char *name = IDToName( type, HeaderNameTable, NUM_HEADERS );
        s += name;
        s += ": ";
    }
    value->toString( s );
    s += "\r\n";
}


////////////////////////////
// HttpRequestLine

#define NUM_METHODS 8

static SortedTableEntry HttpMethodTable[NUM_METHODS] =
{
    { "GET",        HTTP_GET },
    { "HEAD",       HTTP_HEAD },
    { "M-POST",     UPNP_MPOST },
    { "M-SEARCH",   UPNP_MSEARCH },
    { "NOTIFY",     UPNP_NOTIFY },
    { "POST",       UPNP_POST },
    { "SUBSCRIBE",  UPNP_SUBSCRIBE },
    { "UNSUBSCRIBE",UPNP_UNSUBSCRIBE },
};

void HttpRequestLine::load( Tokenizer& scanner )
{
    Token* token;

    SkipBlankLines( scanner );
        
    // method **********
    token = scanner.getToken();
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
        throw HttpParseException( "HttpRequestLine::load() ident expected",
            scanner.getLineNum() );
    }

    method = (UpnpMethodType) NameToID( token->s.c_str(), HttpMethodTable,
        NUM_METHODS );
    if ( method == -1 )
    {
        HttpParseException e( "HttpRequestLine::load() unknown method",
            scanner.getLineNum() );
        e.setErrorCode( PARSERR_UNKNOWN_METHOD );
    }
    
    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
        throw HttpParseException( "HttpRequestLine::load() space 1",
            scanner.getLineNum() );
    }   

    // url ***********  
    token = scanner.getToken();
    if ( token->s == '*' )
    {
        pathIsStar = true;  
    }
    else
    {
        // get url
        scanner.pushBack();
        uri.load( scanner );
        pathIsStar = false;
    }
    
    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
        throw HttpParseException( "HttpRequestLine::load() space 2",
            scanner.getLineNum() );
    }
    
    // http version ***********
    ParseHttpVersion( scanner, majorVersion, minorVersion );
    
    token = scanner.getToken();
    if ( token->tokType == Token::WHITESPACE )
    {
        token = scanner.getToken();
    }
    
    if ( token->tokType != Token::CRLF )
    {
        scanner.pushBack();
        throw HttpParseException( "RequestLine::load() bad data after vers",
            scanner.getLineNum() );
    }
}

void HttpRequestLine::toString( xstring& s )
{
    const char* methodStr = IDToName( method, HttpMethodTable,
        NUM_METHODS );
        
    assert( methodStr != NULL );
    
    s += methodStr;
    s += ' ';
    
    // print uri
    if ( pathIsStar )
    {
        s += '*';
    }
    else
    {
        uri.toString( s );
    }

    s += ' ';

    // print vers
    PrintHttpVersion( majorVersion, minorVersion, s );
    
    s += "\r\n";
}

/////////////////////////////////
// HttpResponseLine


int HttpResponseLine::setValue( int statCode, int majorVers,
    int minorVers )
{
    int retVal;
    
    statusCode = statCode;
    majorVersion = majorVers;
    minorVersion = minorVers;
    
    const char *description = http_GetCodeText( statusCode );
    if ( description == NULL )
    {
        reason = "";
        retVal = -1;
    }
    else
    {
        reason = description;
        retVal = 0;
    }
    
    return retVal;
}

void HttpResponseLine::load( Tokenizer& scanner )
{
    Token* token;
    
    SkipBlankLines( scanner );

    ParseHttpVersion( scanner, majorVersion, minorVersion );
    
    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
        throw HttpParseException( "HttpResponseLine::load() space 1",
            scanner.getLineNum() );
    }
    
    statusCode = loadNum( scanner, 10 );

    token = scanner.getToken();
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
        throw HttpParseException( "HttpResponseLine::load() space 2",
            scanner.getLineNum() );
    }

    // reason phrase
        
    reason = "";
    while ( true )
    {
        token = scanner.getToken();
        
        if (token->tokType == Token::IDENTIFIER ||
            token->tokType == Token::SEPARATOR ||
            token->tokType == Token::WHITESPACE
            )
        {
            reason += token->s;
        }
        else
        {
            break;
        }
    }
    
    if ( token->tokType != Token::CRLF )
    {
        throw HttpParseException( "HttpResponseLine::load() no crlf",
            scanner.getLineNum() );
    }
}

void HttpResponseLine::toString( xstring& s )
{
    PrintHttpVersion( majorVersion, minorVersion, s );

    s += ' ';
    NumberToString( statusCode, s );
    s += ' ';
    s += reason;
    s += "\r\n";
}



///////////////////////
// HttpEntity
HttpEntity::HttpEntity()
{
    init();
    type = TEXT;
}

HttpEntity::HttpEntity( const char* file_name )
{
    init();
    type = FILENAME;
    filename = file_name;
}

void HttpEntity::init()
{
    type = TEXT;
    entity = NULL;
    entitySize = 0;
    appendState = IDLE;
    sizeInc = 20;
    allocLen = 0;
}

HttpEntity::~HttpEntity()
{
    if ( appendState == APPENDING )
        appendDone();
        
    if ( type == TEXT && entity != NULL )
        free( entity );
}

// throws OutOfMemoryException
// numChars = number of chars to read from reader
// return 0: OK
//        -ve: network error
//        return val < numChars : unable to read more data
#if 0
// *************************************************
int HttpEntity::load( CharReader* reader, unsigned numChars )
{
    char buf[1024];
    int numLeft = (int)numChars;
    int readSize = 0;
    int numRead;
    
    if ( entity != NULL )
    {
        free( entity );
    }
    
    entity = (char *) malloc( 1 );  // 1 char for null terminator
    entitySize = 0;
    
    while ( numLeft > 0 )
    {
        readSize = MinVal<int>( numLeft, sizeof(buf) );
        numRead = reader->read( buf, readSize );
        
        if ( numRead < 0 )
        {
            return numRead;
        }
        else if ( numRead == 0 )
        {
            return entitySize;
        }
        else
        {
            entity = (char *)realloc( entity, entitySize + numRead );
            if ( entity == NULL )
            {
                throw OutOfMemoryException( "HttpEntity::load()" );
            }
            
            memcpy( &entity[entitySize], buf, numRead );
            entitySize += numRead;
        }
        numLeft -= numRead;
    }
    
    entity[ entitySize ] = 0;
    
    return 0;
}
// **********************XXXXXXXXXXXXXXXXXXXXXX ******* RIP
#endif



HttpEntity::EntityType HttpEntity::getType() const
{
    return type;
}

const char* HttpEntity::getFileName() const
{
    return filename.c_str();
}

// throws FileNotFoundException, OutOfMemoryException
void HttpEntity::appendInit()
{
    assert( appendState == IDLE );
    
    if ( type == FILENAME )
    {
        fp = fopen( filename.c_str(), "wb" );
        if ( fp == NULL )
        {
            throw FileNotFoundException( filename.c_str() );
        }
    }
    else if ( type == TEXT )
    {
        entity = (char *)malloc( 1 );   // one char for null terminator
        if ( entity == NULL )
        {
            throw OutOfMemoryException( "HttpEntity::appendInit()" );
        }
    }

    appendState = APPENDING;    
}

void HttpEntity::appendDone()
{
    assert( appendState == APPENDING );
    
    if ( type == FILENAME )
    {
        fclose( fp );
    }
    else if ( type == TEXT )
    {
        if ( entity != NULL )
        {
            free( entity );
        }
        entity = NULL;
        entitySize = 0;
        allocLen = 0;
    }
    
    appendState = DONE;
}

// return codes:
// 0 : success
// -1: std error; check errno
// -2: not enough memory
// -3: file write error
int HttpEntity::append( const char* data, unsigned datalen )
{
    try
    {
        if ( appendState == IDLE )
        {
            appendInit();
        }
    
        if ( type == FILENAME )
        {
            size_t numWritten;
            
            numWritten = fwrite( data, datalen, sizeof(char), fp );
            if ( numWritten != sizeof(char) )
                throw -3;   // file write error
        }
        else if ( type == TEXT )
        {
            increaseSizeBy( datalen );
            
            memcpy( &entity[entitySize], data, datalen );
            entitySize += datalen;
            entity[entitySize] = 0; // null terminate
        }
    }
    catch ( FileNotFoundException& /* e */ )
    {
        return -1;
    }
    catch ( OutOfMemoryException& /* me */ )
    {
        return -2;
    }
    catch ( int code )
    {
        return code;
    }
    
    return 0;
}

int HttpEntity::getEntityLen() const
{
    return entitySize;
}

const void* HttpEntity::getEntity()
{
    if ( type == TEXT || type == TEXT_PTR )
    {
        return entity;
    }
    else
    {
        return NULL;
    }
}

unsigned HttpEntity::getSizeIncrement() const
{
    return sizeInc;
}

void HttpEntity::setSizeIncrement( unsigned incSize )
{
    sizeInc = incSize;
}

void* HttpEntity::detachTextEntity()
{
    void *entityBuffer;

    assert( type == TEXT );

    entityBuffer = entity;
    init();     // clean slate
    return entityBuffer;
}

void HttpEntity::attachTextEntity( const void* textEntity, int entityLen )
{
    if ( appendState == APPENDING )
    {
        appendDone();
    }

    entity = (char *)textEntity;
    type = TEXT;
    entitySize = entityLen;
    allocLen = entityLen;
    appendState = APPENDING;
}

void HttpEntity::setTextPtrEntity( const void* textEntity,
    int entityLen )
{
    if ( appendState == APPENDING )
    {
        appendDone();
    }

    entity = (char *)textEntity;
    type = TEXT_PTR;
    entitySize = entityLen;
    allocLen = entityLen;
    appendState = IDLE;
}

void HttpEntity::increaseSizeBy( unsigned sizeDelta )
{
    unsigned inc;
    char *temp;

    if ( allocLen >= (int)(entitySize + sizeDelta) )
        return;
        
    inc = MaxVal( sizeInc, sizeDelta );
    temp = (char *) realloc( entity, allocLen + inc + 1 );
    if ( temp == NULL )
    {
        // try smaller size
        if ( sizeInc > sizeDelta )
        {
            inc = sizeDelta;
            temp = (char *)realloc( entity, allocLen + inc + 1 );
        }
        
        if ( temp == NULL )
            throw OutOfMemoryException( "HttpEntity::increaseSizeBy()" );
    }   

    entity = temp;
    allocLen += inc;

    //DBG( printf( "entitySize = %d, incDelta = %u, allocLen = %d\n",entitySize, sizeDelta, allocLen ); )
    assert( (int)(entitySize + sizeDelta) <= allocLen );
}



/////////////////////////////
// HttpMessage

HttpMessage::~HttpMessage()
{
   dblListNode * ListNode;
   HttpHeader  * header;

   ListNode = headers.getFirstItem();
   while(ListNode !=NULL)
   {
       header = (HttpHeader*)ListNode->data;
       delete header->value;
       ListNode = headers.next(ListNode);
   }
}



void HttpMessage::loadRequest( Tokenizer& scanner, CharReader* reader )
{
    isRequest = true;
    requestLine.load( scanner );
    loadRestOfMessage( scanner, reader, HTTP_UNKNOWN_METHOD );
}

void HttpMessage::loadResponse( Tokenizer& scanner, CharReader* reader,
    UpnpMethodType requestMethod )
{
    isRequest = false;
    responseLine.load( scanner );
    loadRestOfMessage( scanner, reader, requestMethod );    
}

int HttpMessage::loadRequest( const char* request )
{
    int code = -1;
    
    try
    {
        MemReader reader( request );
        Tokenizer scanner( reader );
        loadRequest( scanner, &reader );
        code = 0;
    }
    catch ( HttpParseException& /* e */ )
    {
        code = HTTP_E_BAD_MSG_FORMAT;
    }
    catch ( OutOfMemoryException& /* e */ )
    {
        code = HTTP_E_OUT_OF_MEMORY;
    }
    catch ( TimeoutException& /* e */ )
    {
        code = HTTP_E_TIMEDOUT;
    }
    catch ( NetException& /* e */ )
    {
        code = -1;
    }
    
    return code;
}

int HttpMessage::loadResponse( const char* response,
        UpnpMethodType requestMethod )
{
    int code = -1;
    
    try
    {
        MemReader reader( response );
        Tokenizer scanner( reader );
        loadResponse( scanner, &reader, requestMethod );
        code = 0;
    }
    catch ( HttpParseException& /* e */ )
    {
        code = HTTP_E_BAD_MSG_FORMAT;
    }
    catch ( OutOfMemoryException& /* e */ )
    {
        code = HTTP_E_OUT_OF_MEMORY;
    }
    catch ( TimeoutException& /* e */ )
    {
        code = HTTP_E_TIMEDOUT;
    }
    catch ( NetException& /* e */ )
    {
        code = -1;
    }
    
    return code;
}

    
int HttpMessage::headerCount() const
{
    return headers.length();
}

void HttpMessage::addHeader( int headerType, HttpHeaderValue* value )
{
    HttpHeader *header;

    header = new HttpHeader;
    if ( header == NULL )
    {
        throw OutOfMemoryException( "HttpMessage::addHeader()" );
    }

    header->type = headerType;
    header->value = value;
    headers.addAfterTail( header );
}

void HttpMessage::deleteHeader( HttpHeaderNode* headerNode )
{
    headers.remove( headerNode );
}

HttpHeaderValue* HttpMessage::getHeaderValue( int headerType )
{
    HttpHeaderNode* node;
    HttpHeaderValue* value = NULL;
    HttpHeader* header;
    
    node = findHeader( headerType );
    if ( node == NULL )
        return NULL;
    else
    {
        header = (HttpHeader *)node->data;
        value = header->value;
    }
    
    return value;
}   

HttpHeaderNode* HttpMessage::findHeader( int headerType )
{
    HttpHeaderNode* node;
    
    node = getFirstHeader();
    while ( node != NULL )
    {
        HttpHeader *header;

        header = (HttpHeader *)node->data;
        if ( header->type == headerType )
        {
            return node;
        }
        node = getNextHeader( node );
    }
    
    return NULL;
}

HttpHeaderNode* HttpMessage::getFirstHeader()
{
    return headers.getFirstItem();
}

HttpHeaderNode* HttpMessage::getNextHeader( HttpHeaderNode* headerNode )
{
    return headers.next( headerNode );
}

void HttpMessage::startLineAndHeadersToString( xstring& s )
{
    if ( isRequest )
    {
        requestLine.toString( s );
    }
    else
    {
        responseLine.toString( s );
    }
    
    HttpHeaderNode* node;
    
    node = getFirstHeader();
    while ( node != NULL )
    {
        HttpHeader *header;

        header = (HttpHeader *)node->data;
        header->toString( s );
        node = getNextHeader( node );
    }
    
    s += "\r\n";
}

// throws OutOfMemoryException
void HttpMessage::addRawHeader( int headerType, const char* value )
{
    RawHeaderValue* rawValue = new RawHeaderValue;
    if ( rawValue == NULL )
    {
        throw OutOfMemoryException( "HttpMessage::addRawHeader()" );
    }
    
    rawValue->value = value;
    
    addHeader( headerType, rawValue );
}
    
// throws OutOfMemoryException
void HttpMessage::addContentTypeHeader( const char* type, const char* subtype )
{
    MediaRange *contentType = new MediaRange;
    if ( contentType == NULL )
    {
        throw OutOfMemoryException( "HttpMessage::addContentTypeHeader()" );
    }
    
    contentType->type = type;
    contentType->subtype = subtype;
    contentType->mparam.qIsUsed = false;
    
    addHeader( HDR_CONTENT_TYPE, contentType );
}


// throws OutOfMemoryException
void HttpMessage::addServerHeader()
{
    addRawHeader( HDR_SERVER, gServerDesc );
}

// throws OutOfMemoryException
void HttpMessage::addUserAgentHeader()
{
    addRawHeader( HDR_USER_AGENT, gUserAgentDesc );
}

/* *********************** old code *********************

// throws OutOfMemoryException
void HttpMessage::addContentLengthHeader( unsigned length )
{
    HttpNumber* number = new HttpNumber;
    if ( number == NULL )
    {
        throw OutOfMemoryException( "HttpMessage::addContentLengthHeader()" );
    }

    number->num = (int)length;
    addHeader( HDR_CONTENT_LENGTH, number );
}

// throws OutOfMemoryException
void HttpMessage::addDateHeader()
{
    HttpDateValue *dateVal = new HttpDateValue;
    if ( dateVal == NULL )
    {
        throw OutOfMemoryException( "HttpMessage::addDateHeader()" );
    }
    
    time_t currTime;
    tm *tptr;
    
    // get GMT
    time( &currTime );
    tptr = gmtime( &currTime );
    dateVal->gmtDateTime = *tptr;
    
    addHeader( HDR_DATE, dateVal );
}

void HttpMessage::addLastModifiedHeader( time_t last_mod )
{
    HttpDateValue* value = new HttpDateValue;
    if ( value == NULL )
    {
        throw OutOfMemoryException("HttpMessage::addLastModifiedHeader()");
    }

    struct tm *mod_ptr;

    mod_ptr = gmtime( &last_mod );
    memcpy( &value->gmtDateTime, mod_ptr, sizeof(struct tm) );

    addHeader( HDR_LAST_MODIFIED, value );
}


// throws OutOfMemoryException
void HttpMessage::addAllowHeader( const char *methods[], int numMethods )
{
    CommaSeparatedList *identList = new CommaSeparatedList( false, NULL );
    if ( identList == NULL )
    {
        throw OutOfMemoryException( "HttpMessage::addAllowHeader()" );
    }   
    
    try
    {
        for ( int i = 0; i < numMethods; i++ )
        {
            IdentifierValue* value;
        
            value = new IdentifierValue;
            if ( value == NULL )
            {
                throw OutOfMemoryException( "HttpMessage::addAllowHeader()" );
            }
            
            value->value = methods[i];
        }
        addHeader( HDR_ALLOW, identList );
    }
    catch ( OutOfMemoryException& e )
    {
        delete identList;
        throw;
    }
}
*/


void HttpMessage::addDateTypeHeader( int headerID, const time_t& t )
{
    HttpDateValue* value = new HttpDateValue;
    if ( value == NULL )
    {
        throw OutOfMemoryException("HttpMessage::addDateTypeHeader()");
    }

    struct tm *mod_ptr;

    mod_ptr = gmtime( &t );
    memcpy( &value->gmtDateTime, mod_ptr, sizeof(struct tm) );

    addHeader( headerID, value );
}

void HttpMessage::addNumTypeHeader( int headerID, int num )
{
    HttpNumber* number = new HttpNumber;
    if ( number == NULL )
    {
        throw OutOfMemoryException( "HttpMessage::addNumTypeHeader()" );
    }

    number->num = num;
    addHeader( headerID, number );
}

void HttpMessage::addIdentListHeader( int headerID,
    const char* idents[], int numIdents )
{
    CommaSeparatedList *identList = new CommaSeparatedList( false, NULL );
    if ( identList == NULL )
    {
        throw OutOfMemoryException( "HttpMessage::addIdentListHeader()" );
    }

    try
    {
        for ( int i = 0; i < numIdents; i++ )
        {
            IdentifierValue* value;

            value = new IdentifierValue;
            if ( value == NULL )
            {
                throw OutOfMemoryException( "HttpMessage::addIdentListHeader()" );
            }

            value->value = idents[i];

            identList->valueList.addAfterTail( value );
        }
        addHeader( headerID, identList );
    }
    catch ( OutOfMemoryException& /* e */ )
    {
        delete identList;
        throw;
    }
}


void HttpMessage::loadRestOfMessage( Tokenizer& scanner,
    CharReader* reader, UpnpMethodType requestMethod )
{
    int length;
    int bodyType;
    int code;

    ParseHeaders( scanner, headers );
    
    // determine length if possible
    bodyType = messageBodyLen( requestMethod, length );

    if ( bodyType == -1 )
    {
        // no body
        entity.type = HttpEntity::EMPTY;
    }
    else if ( bodyType == 1 )
    {
        // transfer encoding
        readEncodedEntity( scanner );
    }
    else if ( bodyType == 2 )
    {
        // use content length
        code = readEntityUsingLength( scanner, length );
        if ( code < 0 )
        {
            HttpParseException e( "HttpMessage::loadRestOfMessage(): "
                "incomplete body or error reading data\n" );
            e.setErrorCode( PARSERR_INCOMPLETE_ENTITY );
            throw e;
        }
    }
    else if ( bodyType == 3 )
    {
        // use connection close (or stream ending)
        readEntityUntilClose( scanner );
    }
    else
    {
        assert( 0 );    // internal error -- unknown body type
    }
}

// if message is response, requestMethod is method used for request
//   else ignored
// return value:
//              = -1, no body or ignore body
//              =  1, transfer encoding
//              =  2, content-length (length=content length)
//              =  3, connection close
int HttpMessage::messageBodyLen( IN UpnpMethodType requestMethod,
    OUT int& length )
{
    int responseCode = -1;
    int readMethod = -1;
    
    length = -1;
    
    try
    {
        if ( !isRequest )
            responseCode = responseLine.statusCode;

        // std http rules for determining content length
    
        // * no body for 1xx, 204, 304 and head
        //    get, subscribe, unsubscribe
        if ( isRequest )
        {
            if ( requestLine.method == HTTP_HEAD ||
                 requestLine.method == HTTP_GET ||
                 requestLine.method == UPNP_SUBSCRIBE ||
                 requestLine.method == UPNP_UNSUBSCRIBE ||
                 requestLine.method == UPNP_MSEARCH
                )
            {
                throw -1;
            }
        }
        else    // response
        {
            if ( responseCode == 204 ||
                 responseCode == 304 ||
                (responseCode >= 100 && responseCode <= 199)
                )
            {
                throw -1;
            }
        
            // request was HEAD
            if ( requestMethod == HTTP_HEAD )
                throw -1;
        }
    
        // * transfer-encoding -- used to indicate chunked data
        HttpHeaderNode* node;
        node = findHeader( HDR_TRANSFER_ENCODING );
        if ( node != NULL )
        {
            // read method to use transfer encoding
            throw 1;
        }

        // * content length present?
        length = getContentLength();
        if ( length >= 0 )
        {
            throw 2;    // content-length
        }
    
        // * multi-part/byteranges not supported (yet)
    
        // * length determined by connection closing
        if ( isRequest )
        {
            throw -1;   // invalid for requests
        }
    
        // read method = connection closing
        throw 3;
    }
    catch ( int code )
    {
        readMethod = code;
    }
    
    return readMethod;
}

// returns -1 if length not found
int HttpMessage::getContentLength()
{
    HttpNumber* lengthHdr;
    
    lengthHdr = (HttpNumber*) getHeaderValue( HDR_CONTENT_LENGTH );
    if ( lengthHdr == NULL )
        return -1;
    else
        return lengthHdr->num;
}

void HttpMessage::readEntityUntilClose( Tokenizer& scanner )
{
    const int BUFSIZE = 2 * 1024;
    int numRead;
    char buf[BUFSIZE];
    
    while ( !scanner.endOfData() )
    {
        numRead = scanner.read( buf, BUFSIZE );
        if ( numRead == -1 )
        {
            // abort reading -- we now have a partial entity
            break;
        }
        entity.append( buf, numRead );
    }
}

// returns -1 on error
// 0 on success
int HttpMessage::readEntityUsingLength( Tokenizer& scanner,
    int length )
{
    const int BUFSIZE = 2 * 1024;
    int numRead;
    int numLeft = length;
    int readLen;
    char buf[BUFSIZE + 1];  // extra byte for null terminator
        
    while ( !scanner.endOfData() && numLeft > 0 )
    {
        readLen = MinVal( BUFSIZE, numLeft );

        numRead = scanner.read( buf, readLen );
        if ( numRead < 0 )
        {
            // abort reading -- we now have a partial entity
            return -1;
        }
        entity.append( buf, numRead );
        numLeft -= numRead;
    }

    if ( numLeft != 0 )
    {
        return -1;      // partial entity
    }

    return 0;   
}

void HttpMessage::readEncodedEntity( Tokenizer& scanner )
{
    int chunkSize;
    Token* token;
    
    try
    {
        while ( true )
        {
            chunkSize = loadNum( scanner, 16 );
        
            // parse note: using whitespace as separator instead
            //   of LWS because this could treat chunkdata as header
            //   tokens (spec is vague)
        
            SkipOptionalWhitespace( scanner );
            token = scanner.getToken();
        
            // read optional extensions (and discard them)
            while ( true )
            {
                if ( token->tokType == Token::CRLF )
                {
                    break;  // end of chunk header
                }
                if ( token->s != ';' )
                {
                    throw -1;   // bad entity
                }

                SkipOptionalWhitespace( scanner );

                token = scanner.getToken();

                if ( token->tokType != Token::IDENTIFIER )
                {
                    throw -1;
                }

                SkipOptionalWhitespace( scanner );

                token = scanner.getToken();

                if ( token->s != '=' )
                {
                    continue;   // could be start of header
                }

                SkipOptionalWhitespace( scanner );

                token = scanner.getToken();

                if ( !(token->tokType == Token::IDENTIFIER ||
                       token->tokType == Token::QUOTED_STRING) )
                {
                    throw -1;
                }

                SkipOptionalWhitespace( scanner );

                token = scanner.getToken();
            }
        
            // read chunk data
            
            
            if ( chunkSize == 0 )
            {
                break;  // last chunk
            }
            
            readEntityUsingLength( scanner, chunkSize );

            // crlf after chunk data
            token = scanner.getToken();
            if ( token->tokType != Token::CRLF )
            {
                scanner.pushBack();
                throw HttpParseException( "HttpMessage::readEncodedEntity(): "
                    "bad chunk data\r\n", scanner.getLineNum() );
            }
        } // while
        
        // read entity headers
        ParseHeaders( scanner, headers );
    }
    catch ( int code )
    {
        if ( code == -1 )
        {
            HttpParseException e( "HttpMessage::readEncodedEntity() bad entity",
                scanner.getLineNum() );
            e.setErrorCode( PARSERR_BAD_ENTITY );
            throw e;
        }
    }
}

#endif
#endif
