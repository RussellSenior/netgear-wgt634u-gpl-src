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
// $Revision: 1.1.1.2 $
// $Date: 2001/06/15 00:22:16 $
//

#ifndef GENLIB_UTIL_UTIL_H
#define GENLIB_UTIL_UTIL_H

// usually used to specify direction of parameters in functions
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif


#ifdef NO_DEBUG
#define DBG(x)
#else
#define DBG(x) x
#endif

// C++ specific
#ifdef __cplusplus

template <class T>
inline T MaxVal( T a, T b )
{
    return a > b ? a : b;
}

template <class T>
inline T MinVal( T a, T b )
{
    return a < b ? a : b;
}

#endif /* __cplusplus */

// boolean type in C
typedef char xboolean;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

///////////////////////////
// funcs

#ifdef __cplusplus
extern "C" {
#endif

void log_error( IN const char *fmt, ... );

#ifdef __cplusplus
} // extern C
#endif

//////////////////////////////////

// C specific
#ifndef __cplusplus

#ifdef _WIN32
#define		XINLINE __inline
#else
// GCC
#define		XINLINE inline
#endif // _WIN32

#endif // __cplusplus

#endif /* GENLIB_UTIL_UTIL_H */
