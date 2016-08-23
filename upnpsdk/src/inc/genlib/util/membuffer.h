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

#ifndef GENLIB_UTIL_MEMBUFFER_H
#define GENLIB_UTIL_MEMBUFFER_H


#include <stdlib.h>
#include <genlib/util/util.h>

#define MINVAL( a, b ) ( (a) < (b) ? (a) : (b) )
#define MAXVAL( a, b ) ( (a) > (b) ? (a) : (b) )

// pointer to a chunk of memory
typedef struct // memptr
{
	char	*buf;			// start of memory (read/write)
	size_t	length;			// length of memory (read-only)
} memptr;


// maintains a block of dynamically allocated memory
// note: Total length/capacity should not exceed MAX_INT
typedef struct // membuffer
{
	char	*buf;			// mem buffer; must not write 
							//   beyond buf[length-1] (read/write)
	size_t	length;			// length of buffer (read-only)
	size_t	capacity;		// total allocated memory (read-only)
	size_t	size_inc;		// used to increase size; MUST be > 0; (read/write)

	// default value of size_inc
	#define MEMBUF_DEF_SIZE_INC		5
} membuffer;

//--------------------------------------------------
//////////////// functions /////////////////////////
//--------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//////////////////////////////////////////////////
// malloc()s a string and stores 'str' in it. To
// destroy returned string, call free().
//
// returns:
//    NULL - not enough memory
//    a copy of 'str'
char* str_alloc( IN const char* str, IN size_t str_len );

//////////////////////////////////////////////////
// case-sensitive comparison of a memptr and a C-string
// parameters:
//   m: is a valid memory buffer
//	 s: a C-string
// returns:
//   0: if m == s
//  <0: if m < s
//  >0: if m > s
int memptr_cmp( IN memptr* m, IN const char* s );

//////////////////////////////////////////////////
// similar to memptr_cmp, except that comparison
//   is not case sensitive
int memptr_cmp_nocase( IN memptr* m, IN const char* s );

//////////////////////////////////////////////////
// initializes m->buf to NULL, length=0
void membuffer_init( INOUT membuffer* m );

//////////////////////////////////////////////////
// destroys the buffer; fields of 'm' will no longer
//   be valid
void membuffer_destroy( INOUT membuffer* m );

//////////////////////////////////////////////////
// sets m->buf to buf
//
// returns:
//	 UPNP_E_SUCCESS
//	 UPNP_E_OUTOF_MEMORY
int membuffer_assign( INOUT membuffer* m, IN const void* buf, 
					 IN size_t buf_len );

//////////////////////////////////////////////////
// Similar to membuffer_assign(), except that it appends a C string
int membuffer_assign_str( INOUT membuffer* m, IN const char* c_str );

//////////////////////////////////////////////////
// appends m->buf with given data
//
// returns:
//	 UPNP_E_SUCCESS
//	 UPNP_E_OUTOF_MEMORY
int membuffer_append( INOUT membuffer* m, IN const void* buf,
					 IN size_t buf_len );

//////////////////////////////////////////////////
// Similar to membuffer_append(), except that it appends a C string
int membuffer_append_str( INOUT membuffer* m, IN const char* c_str );


//////////////////////////////////////////////////
// inserts 'buf' into m->mem at the given index
//
// buf:   block of memory to insert into buffer.
//        If this is NULL, m->buf becomes NULL
// index: position to insert 'buf' in the membuffer.
//        valid values = 0..m->length
//
// returns:
//	 UPNP_E_SUCCESS
//	 UPNP_E_OUTOF_MEMORY
int membuffer_insert( INOUT membuffer* m, IN const void* buf, 
					 IN size_t buf_len, int index );


//////////////////////////////////////////////////
// deletes 'num_bytes' bytes from m->buf starting from
//	 'index' position
void membuffer_delete( INOUT membuffer* m, IN int index, 
					 IN size_t num_bytes );

//////////////////////////////////////////////////
// detaches current buffer and returns it. The caller
//	 must free the returned buffer using free().
// After this call, length becomes 0.
//
// returns: a pointer to the current buffer
char* membuffer_detach( INOUT membuffer* m );

//////////////////////////////////////////////////
// frees current buffer and sets its value to 'new_buf'.
// 'new_buf' must be allocted using malloc or realloc so
// that it can be freed using free()
void membuffer_attach( INOUT membuffer* m, IN char* new_buf,
					   IN size_t buf_len );


#ifdef __cplusplus
}		// extern "C"
#endif	// __cplusplus

#endif // GENLIB_UTIL_H
