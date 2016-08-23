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
#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <genlib/util/utilall.h>
#include <genlib/meta/stream/charreader.h>
#include <genlib/net/http/tokenizer.h>

#define TOK_CHAR_TAB 0x9
#define TOK_CHAR_SPACE 0x20
#define TOK_CHAR_CR 0xD
#define TOK_CHAR_LF 0xA
    
Token::Token()
{
}

Token::~Token()
{
}

const char* Token::getTokenTypeStr() const
{
    static const char* Names[] =
    { "IDENTIFIER", "WHITESPACE", "CRLF", "SEPARATOR", "QUOTED_STRING" };
    
    const int NumItems = sizeof(Names) / sizeof(char*);
    
    if ( tokType < 0 || tokType >= NumItems )
        return "UNKNOWN TOKEN";
    else
        return Names[ tokType ];
}

// make it from: this <-> node to this <-> t <-> node   
void Token::insertAfterSelf( Token* t )
{
    assert( t != NULL );
    
    t->next = next;
    t->prev = this;
    
    next->prev = t;
    next = t;
}

// make it from: node<->this to node <-> t <-> this
void Token::insertBeforeSelf( Token* t )
{
    assert( t != NULL );
    
    t->prev = prev;
    t->next = this;
    
    prev->next = t;
    prev = t;
}

// remove self from linked list
void Token::unlinkSelf()
{
    prev->next = next;
    next->prev = prev;
}

//////////////////////////////////////////
// Tokenizer

static bool IsSeparator( char c )
{
    static const char *separators = " \t()<>@,;:\\\"/[]?={}";
    
    return strchr( separators, c ) != NULL;
}

static bool IsControlChar( char c )
{
    return ( (c >= 0 && c <= 31) || c == 127 );
}

//static bool IsHexDigit( char c )
//{
//  return ( (c >= '0' && c <= '0') ||
//           (c >= 'a' && c <= 'f') ||
//           (c >= 'A' && c <= 'F')
//          );
//}

// identifier = any char 0..127 which is not separator or ctrl char
static bool IsIdentifierChar( char c )
{
    return (c >= 32 && c <= 126) && (!IsSeparator(c));
}

// normal quoted string chars
static bool IsQdText( char c )
{
    switch (c)
    {
        case TOK_CHAR_CR:
        case TOK_CHAR_LF:
        case TOK_CHAR_TAB:
            return true;

        case '"':
            return false;
    }

    return !IsControlChar(c);
}

Tokenizer::Tokenizer( CharReader& r )
    : reader(r)
{
    head = new Token();
    tail = new Token();
    
    if ( head == NULL || tail == NULL )
    {
        throw OutOfMemoryException( "Tokenizer::Tokenizer()" );
    }
    
    // head is near oldest token; tail near newest
    // i.e., head points to first item in q; tail last
    // head <-> item1 <-> item2 <-> tail
    head->next = tail;
    head->prev = NULL;
    tail->next = NULL;
    tail->prev = head;
    
    current = head;
    
    listLen = 0;
    lineNum = 1;
}

Tokenizer::~Tokenizer()
{
    deleteUntilSizeEquals( 0 );
    
    delete head;
    delete tail;
}

// throws OutOfMemoryException
Token* Tokenizer::getToken()
{
    Token *t;
    
    if ( current->next == tail )
    {
        // no more items; read next one
        
        t = new Token();
        if ( t == NULL )
        {
            throw OutOfMemoryException( "Tokenizer::getToken()" );
        }
        
        readNextToken( *t );
        
        // insert at end of list
        tail->insertBeforeSelf( t );
        listLen++;
        
        current = t;    // point to current token
        
        // remove extra tokens saved for pushbacks
        //cleanupOldTokens();
        deleteUntilSizeEquals( MAX_PUSHBACKS );
    }
    else
    {
        t = current->next;
        current = current->next;    // next item
    }

    //DBG( printf("get_token: %s\n", t->s.c_str()); )

    return t;
}

void Tokenizer::pushBack()
{
    char errbuf[100];

    
    if ( current == head )
    {
        // error
        sprintf( errbuf, "Extra push back on line %d: Tokenizer::pushback()",
            lineNum );
        throw TokenizerException( errbuf );
    }
    else
    {
        // move back one token
        current = current->prev;

        // DBG( printf("push_back: %s\n", current->s.c_str()); )
    }
}

int Tokenizer::read( OUT void* buffer, IN int bufsize )
{
    assert( buffer != NULL );

    if ( bufsize <= 0 )
        return 0;
        
    int addedLen = 0;
    Token* saveCurrent = current;
    Token* token;
    char* buf = (char *)buffer;
        
    // add data from any pending tokens
    while ( current->next != tail )
    {
        int slen;
        
        token = current;
        slen = token->s.length();
        
        if ( addedLen + slen > bufsize )
        {
            // buffer full
            return addedLen;
        }
        
        // append buf
        memcpy( &buf[addedLen], token->s.c_str(), slen );
        addedLen += slen;

        current = current->next;    // next token
    }

    // read rest of data unparsed from stream
    if ( addedLen < bufsize && !reader.endOfStream() )
    {
        int streamRead;

        streamRead = reader.read( &buf[addedLen], bufsize - addedLen );
        if ( streamRead < 0 )
        {
            current = saveCurrent;  // restore tokenizer state
            return -1;
        }
        addedLen += streamRead;
    }
    
    return addedLen;
}

bool Tokenizer::endOfData() const
{
    // true if no token data, and no data in stream
    if ( (current->next == tail) && (reader.endOfStream()) )
        return true;
        
    return false;
}

void Tokenizer::deleteUntilSizeEquals( int newSize )
{
    // precond:
    // current now points to last item
    // listLen = num used up tokens in list
    
    Token* t;
    
    while ( listLen > newSize )
    {
        // delete oldest token
        t = head->next;
        t->unlinkSelf();
        delete t;
        
        listLen--;
    }
}
    
void Tokenizer::readNextToken( Token& tok )
{
    char c;
    char errbuf[100];
    
    // any data left in stream?
    if ( reader.endOfStream() )
    {
        tok.tokType = Token::END_OF_STREAM;
        return;
    }   
    
    c = reader.getChar();
    if ( IsIdentifierChar(c) )
    {
        // identifier
        tok.tokType = Token::IDENTIFIER;
        tok.s = c;
        
        while ( !reader.endOfStream() )
        {
            c = reader.getChar();
            if ( !IsIdentifierChar(c) )
            {
                reader.pushBack();
                break;
            }
            tok.s += c;
        }
    }
    else if ( c == ' ' || c == TOK_CHAR_TAB )
    {
        // space or TAB
        tok.tokType = Token::WHITESPACE;
        tok.s = c;
        
        while ( !reader.endOfStream() )
        {
            // add rest of whitespace, if any
            c = reader.getChar();
            if ( !(c == ' ' || c == TOK_CHAR_TAB) )
            {
                reader.pushBack();  // return unprocessed char
                break;
            }
            tok.s += c;
        }
    }
    else if ( c == TOK_CHAR_CR )
    {
        // crlf
        if ( reader.endOfStream() || reader.getChar() != TOK_CHAR_LF )
        {
            sprintf( errbuf, "line %d: Expected LF after CR", lineNum );
            throw TokenizerException( errbuf );
        }
        tok.tokType = Token::CRLF;
        tok.s = "\r\n";
        lineNum++;      // now in next line
    }
    else if ( c == TOK_CHAR_LF )
    {
	tok.tokType = Token::CRLF;
	tok.s = "\r\n";
	lineNum++;
    }
    else if ( c == '"' )
    {
        // quoted string
        tok.tokType = Token::QUOTED_STRING;
        tok.s = c;
        bool gotEndQuote = false;
        
        while ( !reader.endOfStream() )
        {
            c = reader.getChar();
            
            if ( c == '"' )
            {
                // end of string
                tok.s += c;
                gotEndQuote = true;
                break;
            }
            
            if ( c == '\\' )
            {
                // add \char
                if ( reader.endOfStream() )
                {
                    break;
                }
                c = reader.getChar();
                
                // accept ctrl, ascii text
                // note: deviation from spec, don't add NULL
                if ( c >= 1 && (unsigned char)c < 127 )
                {
                    tok.s += c;
                }
                else
                {
                    // unacceptable char in string
                    sprintf( errbuf,
                        "line %d: unknown char %c (ascii %d)",
                        lineNum, c, int(c) );
                    throw TokenizerException( errbuf );
                }
            }
            else if ( IsQdText(c) )
            {
                tok.s += c;
            }
            else
            {
                // unacceptable char in string
                sprintf( errbuf,
                    "line %d: unknown char %c (ascii %d)",
                    lineNum, c, int(c) );
                throw TokenizerException( errbuf );
            }
        }
        
        if ( !gotEndQuote )
        {
            sprintf( errbuf, "line %d: no end quote", lineNum );
            throw TokenizerException( errbuf );
        }
    }
    else if ( IsControlChar(c) )
    {
        // control char
        tok.tokType = Token::CTRL;
        tok.s = c;
        tok.c = c;
    }
    else if ( IsSeparator(c) )
    {
        // separator
        tok.tokType = Token::SEPARATOR;
        tok.s = c;
        tok.c = c;
    }
    else
    {
        sprintf( errbuf, "line %d: unknown char %c (ascii %d)",
            lineNum, c, int(c) );
        throw TokenizerException( errbuf );
    }
}

#endif
#endif
