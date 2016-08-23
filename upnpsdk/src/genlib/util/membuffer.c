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
//
// $Revision: 1.1.1.1 $
// $Date: 2001/06/15 00:22:16 $
//

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <genlib/util/membuffer.h>
#include "upnp.h"

// ================================================
// string
// ================================================
char* str_alloc( IN const char* str, IN size_t str_len )
{
	char *s;

	s = (char *)malloc( str_len + 1 );
	if ( s == NULL )
	{
		return NULL;	// no mem
	}

	memcpy( s, str, str_len );
	s[str_len] = '\0';

	return s;
}

// =================================================
// memptr 
// =================================================

//////////////////////////////////////////////////
int memptr_cmp( IN memptr* m, IN const char* s )
{
	int cmp;

	cmp = strncmp( m->buf, s, m->length );

	if ( cmp == 0 && m->length < strlen(s) )
	{
		// both strings equal for 'm->length' chars
		//  if m is shorter than s, then s is greater
		return -1;
	}

	return cmp;
}

//////////////////////////////////////////////////
int memptr_cmp_nocase( IN memptr* m, IN const char* s )
{
	int cmp;

#ifdef _WIN32
	cmp = _strnicmp( m->buf, s, m->length );
#else	// UNIX
	cmp = strncasecmp( m->buf, s, m->length );
#endif

	if ( cmp == 0 && m->length < strlen(s) )
	{
		// both strings equal for 'm->length' chars
		//  if m is shorter than s, then s is greater
		return -1;
	}
	
	return cmp;
}


// =================================================
// membuffer
// =================================================
//////////////////////////////////////////////////
static XINLINE void membuffer_initialize( INOUT membuffer* m )
{
	m->buf = NULL;
	m->length = 0;
	m->capacity = 0;
}

//////////////////////////////////////////////////
// Increases or decreases buffer cap so that at least
//   'new_length' bytes can be stored
//
// On error, m's fields do not change.
//
// returns:
//	 UPNP_E_SUCCESS
//	 UPNP_E_OUTOF_MEMORY
static int membuffer_set_size( INOUT membuffer* m, 
							   IN size_t new_length )
{
	size_t diff;
	size_t alloc_len;
	char *temp_buf;

	if ( new_length >= m->length )	// increase length
	{
		// need more mem?
		if ( new_length <= m->capacity )
		{
			return 0;	// have enough mem; done
		}

		diff = new_length - m->length;
		alloc_len = MAXVAL(m->size_inc, diff) + m->capacity;
	}
	else	// decrease length
	{
		assert( new_length <= m->length );

		// if diff is 0..m->size_inc, don't free
		if ( (m->capacity - new_length) <= m->size_inc )
		{
			return 0;
		}

		alloc_len = new_length + m->size_inc;
	}

	assert( alloc_len >= new_length );

	temp_buf = realloc( m->buf, alloc_len + 1 );
	if ( temp_buf == NULL )
	{
		// try smaller size
		alloc_len = new_length;
		temp_buf = realloc( m->buf, alloc_len + 1 );

		if ( temp_buf == NULL )
		{
			return UPNP_E_OUTOF_MEMORY;
		}
	}

	// save
	m->buf = temp_buf;
	m->capacity = alloc_len;
	return 0;
}


// ===============================================


//////////////////////////////////////////////////
void membuffer_init( INOUT membuffer* m )
{
	assert( m != NULL );

	m->size_inc = MEMBUF_DEF_SIZE_INC;
	membuffer_initialize( m );
}

//////////////////////////////////////////////////
void membuffer_destroy( INOUT membuffer* m )
{
	if ( m == NULL )
	{
		return;
	}

	free( m->buf );
	membuffer_init( m );	
}

//////////////////////////////////////////////////
int membuffer_assign( INOUT membuffer* m, IN const void* buf, 
					 IN size_t buf_len )
{
	int return_code;

	assert( m != NULL );

	// set value to null
	if ( buf == NULL )
	{
		membuffer_destroy( m );
		return 0;
	}

	// alloc mem
	return_code = membuffer_set_size( m, buf_len );
	if ( return_code != 0 )
	{
		return return_code;
	}
	
	// copy
	memcpy( m->buf, buf, buf_len );
	m->buf[buf_len] = 0;		// null-terminate

	m->length = buf_len;

	return 0;
}

//////////////////////////////////////////////////
int membuffer_assign_str( INOUT membuffer* m, IN const char* c_str )
{
	return membuffer_assign( m, c_str, strlen(c_str) );
}

//////////////////////////////////////////////////
int membuffer_append( INOUT membuffer* m, IN const void* buf,
					 IN size_t buf_len )
{
	assert( m != NULL );

	return membuffer_insert( m, buf, buf_len, m->length );
}

int membuffer_append_str( INOUT membuffer* m, IN const char* c_str )
{
	return membuffer_insert( m, c_str, strlen(c_str), m->length );
}

//////////////////////////////////////////////////
int membuffer_insert( INOUT membuffer* m, IN const void* buf, 
					 IN size_t buf_len, int index )
{
	int return_code;

	assert( m != NULL );

	if ( index < 0 || index > (int)m->length )
		return UPNP_E_OUTOF_BOUNDS;

	if ( buf == NULL || buf_len == 0 )
	{
		return 0;
	}

	// alloc mem
	return_code = membuffer_set_size( m, m->length + buf_len );
	if ( return_code != 0 )
	{
		return return_code;
	}

	// insert data
	
	// move data to right of insertion point
	memmove( m->buf + index + buf_len, m->buf + index, 
		m->length - index );
	memcpy( m->buf + index, buf, buf_len );
	m->length += buf_len;
	m->buf[m->length] = 0;		// null-terminate
	
	return 0;
}


//////////////////////////////////////////////////
void membuffer_delete( INOUT membuffer* m, IN int index, 
					  IN size_t num_bytes )
{
	int return_value;
	int new_length;
	size_t copy_len;

	assert( m != NULL );

	if ( m->length == 0 )
	{
		return;
	}

	assert( index >= 0 && index < (int)m->length );

	// shrink count if it goes beyond buffer
	if ( index + num_bytes > m->length )
	{
		num_bytes = m->length - (size_t)index;
		copy_len = 0;	// every thing at and after index purged
	}
	else
	{
		// calc num bytes after deleted string
		copy_len = m->length - (index + num_bytes);
	}

	memmove( m->buf + index, m->buf + index + num_bytes, copy_len );

	new_length = m->length - num_bytes;
	return_value =  membuffer_set_size( m, new_length );	// trim buffer
	assert( return_value == 0 );	// shrinking should always work

	// don't modify until buffer is set
	m->length = new_length;
	m->buf[new_length] = 0;
}

//////////////////////////////////////////////////
char* membuffer_detach( INOUT membuffer* m )
{
	char *buf;
	assert( m != NULL );

	buf = m->buf;

	// free all
	membuffer_initialize( m );

	return buf;
}

//////////////////////////////////////////////////
void membuffer_attach( INOUT membuffer* m, IN char* new_buf,
					   IN size_t buf_len )
{
	assert( m != NULL );

	membuffer_destroy( m );
	m->buf = new_buf;
	m->length = buf_len;
	m->capacity = buf_len;
}



