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

// $Revision: 1.1.1.1 $
// $Date: 2001/06/15 00:21:11 $

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <genlib/util/util.h>
#include <genlib/util/miscexceptions.h>
#include <genlib/util/dbllist.h>

// if itemDestructorPtr is NULL, free() is used
dblList::dblList( FreeDblItem itemDestructorPtr,
         CompareDblItems comparatorPtr)
{
    destroyItem = itemDestructorPtr;
    compareItems = comparatorPtr;

    head = new dblListNode;
    tail = new dblListNode;
    if ( head == NULL || tail == NULL )
    {
        throw OutOfMemoryException( "dblList::dblList()" );
    }

    head->next = tail;
    head->prev = NULL;
    tail->prev = head;
    tail->next = NULL;

    len = 0;
}

dblList::~dblList()
{
    removeAll();
    delete head;
    delete tail;
}

void dblList::addAfterTail( void* item )
{
    addBefore( tail, item );
}

void dblList::addBeforeHead( void* item )
{
    addAfter( head, item );
}

void dblList::addAfter( dblListNode* node, void* item )
{
    assert( node != NULL );

    dblListNode* newNode = new dblListNode;
    if ( node == NULL )
    {
        throw OutOfMemoryException( "dblList::addAfter()" );
    }
    newNode->data = item;

    insertAfter( node, newNode );
}

void dblList::addBefore( dblListNode* node, void* item )
{
    assert( node != NULL );

    dblListNode* newNode = new dblListNode;
    if ( node == NULL )
    {
        throw OutOfMemoryException( "dblList::addBefore()" );
    }
    newNode->data = item;

    insertBefore( node, newNode );
}

void dblList::insertAfter( dblListNode* node, dblListNode* newNode )
{
    assert( node != NULL );
    assert( newNode != NULL );

    newNode->next = node->next;
    newNode->prev = node;

    node->next->prev = newNode;
    node->next = newNode;

    len++;
}

void dblList::insertBefore( dblListNode* node, dblListNode* newNode )
{
    assert( node != NULL );
    assert( newNode != NULL );

    newNode->next = node;
    newNode->prev = node->prev;

    node->prev->next = newNode;
    node->prev = newNode;

    len++;
}

void* dblList::getHead()
{
    if ( len == 0 )
    {
        throw OutOfBoundsException( "dblList::getHead()" );
    }

    return head->next->data;
}

void* dblList::getTail()
{
    if ( len == 0 )
    {
        throw OutOfBoundsException( "dblList::getTail()" );
    }

    return tail->prev->data;
}

int dblList::length() const
{
    return len;
}

dblListNode* dblList::getFirstItem()
{
    return head->next == tail ? NULL : head->next;
}

dblListNode* dblList::getLastItem()
{
    return tail->prev == head ? NULL : tail->prev;
}

dblListNode* dblList::next( dblListNode* node )
{
    if ( len == 0 || node == NULL )
    {
        throw OutOfBoundsException( "dblList::next()" );
    }
    return node->next == tail ? NULL : node->next;
}

dblListNode* dblList::prev( dblListNode* node )
{
    if ( len == 0 || node == NULL )
    {
        throw OutOfBoundsException( "dblList::prev()" );
    }
    return node->prev == head ? NULL : node->prev;
}

void dblList::remove( dblListNode* node, bool freeNodeMemory)
{
    assert( node != NULL );

    // rewire to detach node
    node->prev->next = node->next;
    node->next->prev = node->prev;

    len--;

    node->next = NULL;
    node->prev = NULL;

    if ( freeNodeMemory )
    {
        if ( destroyItem != NULL )
        {
            destroyItem( node->data );
        }
        else
        {
            free( node->data );     // default
        }
        delete node;
    }
}

void dblList::removeAll( bool freeNodeMemory)
{
    while ( head->next != tail )
    {
        remove( head->next, freeNodeMemory );
    }
}

dblListNode* dblList::find( void* item )
{
    dblListNode* node;

    node = getFirstItem();
    while ( node != NULL )
    {
        if ( compareItems == NULL )
        {
            if ( node->data == item )
            {
                return node;
            }
        }
        else if ( compareItems(node->data, item) == true )
        {
            return node;
        }
        node = next( node );
    }

    return NULL;
}

