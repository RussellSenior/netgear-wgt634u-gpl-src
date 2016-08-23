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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <genlib/tpool/tpool.h>
#include <genlib/util/utilall.h>
#include <genlib/util/util.h>
#include <genlib/util/xdlist.h>
#include <genlib/util/miscexceptions.h>
#include <genlib/util/dbllist.h>
#include "../../../inc/tools/config.h"

#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>
#define EVENT_TIMEDOUT 		-2
#define EVENT_TERMINATE		-3


//typedef xdlistNode<PoolQueueItem> ThreadPoolNode;
typedef dblListNode ThreadPoolNode;

struct ThreadArg
{
    unsigned *timeoutSecs;
    ThreadPoolQueue* q;
    
    // pool data
    pthread_mutex_t* poolMutex;
    pthread_cond_t* condVariable;
    pthread_cond_t* zeroCountCondVariable;
    unsigned* numThreads;
    bool* die;
};

// returns
//	 0: success
//	 EVENT_TIMEDOUT
//	 EVENT_TERMINATE
int GetNextItemInQueue( ThreadArg* arg,
    OUT PoolQueueItem& callback )
{
    unsigned timeoutSecs = *arg->timeoutSecs;
    int code = 0;
    int retCode;
    struct timespec timesleep;

    ThreadPoolQueue* q;
   
    timeval now;
    timespec timeout;
    timeval current_time;
    
    timesleep.tv_sec=0;
    timesleep.tv_nsec=10000000;

    // wait until item becomes available
    if ( timeoutSecs < 0 )
    {
   	q = arg->q;
	pthread_mutex_lock( arg->poolMutex );

	while ( q->length() == 0 && *arg->die == false )
        {
            pthread_cond_wait( arg->condVariable,
                arg->poolMutex );
        }
    }
    else
    {
   	q = arg->q;
        timeout.tv_sec = now.tv_sec + timeoutSecs;
        timeout.tv_nsec = now.tv_usec * 1000;
	pthread_mutex_lock( arg->poolMutex );
	
	while ((q->length() == 0)&&( *arg->die == false))
	{
        	if(arg->condVariable->__c_lock.__spinlock != 0)
		{
			--(arg->condVariable->__c_lock.__spinlock);
			code=0;
			break;	
		}
		gettimeofday(&current_time,NULL);
                if (current_time.tv_sec>=timeout.tv_sec)
		{
			code=ETIMEDOUT;
			break;
		}
		
		pthread_mutex_unlock( arg->poolMutex );
		nanosleep(&timesleep, NULL);
		pthread_mutex_lock( arg->poolMutex );
	}
    }
    
    DBG( pthread_t tempThread = pthread_self(); )
    
    if ( *arg->die == true )
    {
        retCode = EVENT_TERMINATE;
        
        DBG(
            UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
                "thread %ld: got terminate msg\n", tempThread); )
    }
    else if ( code == ETIMEDOUT )
    {
        retCode = EVENT_TIMEDOUT;
        DBG(
            UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
                "thread %ld: got timeout msg\n", tempThread); )
    }
    else
    {
        ThreadPoolNode* node;
        PoolQueueItem* callbackptr;
        
        assert( q->length() > 0 );
        
        node = q->getFirstItem();
        callbackptr = (PoolQueueItem*) node->data;
        callback = *callbackptr;
        q->remove( node );

        // broadcast zero count condition of queue
        if ( arg->q->length() == 0 )
        {
            //DBG( printf("thread pool q len = 0 broadcast\n"); )
            pthread_cond_broadcast( arg->zeroCountCondVariable );
        }
        
        retCode = 0;
    }
    
    pthread_mutex_unlock( arg->poolMutex );	
    
    return retCode;
}

static void* ThreadCallback( void* the_arg )
{
    ThreadArg* arg = (ThreadArg *)the_arg;
    int retCode;
    PoolQueueItem callback;
    
    // DBG
    //unsigned threadNum;
    
    //{
    //}
    ///////////////
    
    DBG(
        UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
            "thread %ld: started...\n", pthread_self()); )
    //DBG( int countxxx = 0; )
    
    while ( true )
    {
        // pull out next callback from queue
        DBG(
            UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
                "thread %ld: waiting for item\n", pthread_self()); )
        retCode = GetNextItemInQueue( arg, callback );
        
        if ( retCode == EVENT_TIMEDOUT || retCode == EVENT_TERMINATE )
        {
            //DBG( printf("thread got signal %d\n", retCode); )
            break;		// done with thread
        }
        
        // invoke callback
        callback.func( callback.arg );
    }
    
    // decrement active thread count
    pthread_mutex_lock( arg->poolMutex );
    
    *arg->numThreads = *arg->numThreads - 1;
    
    pthread_mutex_unlock( arg->poolMutex );

    delete arg;

    pthread_exit( NULL );
    return NULL;
}


///////////////////
// ThreadPool

ThreadPool::ThreadPool()
{
    numThreads = 0;
    maxThreads = DEF_MAX_THREADS;
    lingerTime = DEF_LINGER_TIME;
    allDie = false;
    
    int success;
    
    //DBG( printf("thread pool constructor\n"); )
    
    success = pthread_mutex_init( &mutex, NULL );
    if ( success == -1 )
    {
        DBG(
            UpnpPrintf(
                UPNP_CRITICAL, TPOOL, __FILE__, __LINE__,
                    "thread pool: error creating mutex\n"); )
        throw OutOfMemoryException( "Mutex creation error in thread pool" );
    }
    
    success = pthread_cond_init( &condVariable, NULL );
    if ( success == -1 )
    {
        DBG(
            UpnpPrintf(
                UPNP_CRITICAL, TPOOL, __FILE__, __LINE__,
                    "thread pool: error creating cond var\n"); )
        throw OutOfMemoryException( "Thread Pool: error creating cond variable" );
    }
    
    success = pthread_cond_init( &zeroCountCondVariable, NULL );
    if ( success == -1 )
    {
        DBG(
            UpnpPrintf(
                UPNP_CRITICAL, TPOOL, __FILE__, __LINE__,
                    "thread pool: error creating zero cond var\n"); )
        throw OutOfMemoryException( "Thread Pool: error creating count condition variable" );
    }
}

ThreadPool::~ThreadPool()
{
    // signal all threads to die
    pthread_mutex_lock( &mutex );
    allDie = true;
    pthread_cond_broadcast( &condVariable );
    pthread_mutex_unlock( &mutex );
    
    DBG(
        UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
            "thread pool destructor: sent die msg\n"); )
    
    // wait till all threads are done
    while ( true )
    {
        if ( numThreads == 0 )
        {
            break;
        }

        //DBG( printf("thread destructor waiting: pending = %d, running = %d\n", getNumJobsPending(), getNumThreadsRunning()); )

        // send signal again, if missed previously
        pthread_cond_broadcast( &condVariable );
        sleep( 1 );
    }

    // signal no more threads to run
    pthread_mutex_lock( &mutex );
    pthread_cond_broadcast( &zeroCountCondVariable );
    pthread_mutex_unlock( &mutex );

    int code;
        
    // destroy zeroCountCondVariable
    while ( true )
    {
        code = pthread_cond_destroy( &zeroCountCondVariable );
        if ( code == 0 )
        {
            break;
        }
            
        sleep( 1 );
    }
    
    code = pthread_cond_destroy( &condVariable );
    assert( code == 0 );
    
    code = pthread_mutex_destroy( &mutex );
    assert( code == 0 );
}

/////////
// queues function f to be executed in a worker thread
// input:
//   f: function to be executed
//   arg: argument to passed to the function
// returns:
//   0 if success; -1 if not enuf mem; -2 if input func in NULL
int ThreadPool::schedule( ScheduleFunc f, void* arg )
{
    int retCode = 0;
    
    if ( f == NULL )
        return -2;
        
    pthread_mutex_lock( &mutex );
    
    try
    {
        PoolQueueItem* job;

        job = (PoolQueueItem*)malloc( sizeof(PoolQueueItem) );
        if ( job == NULL )
        {
            throw OutOfMemoryException( "ThreadPool::schedule()" );
        }

        job->func = f;
        job->arg = arg;
        
        q.addAfterTail( job );
        
        // signal a waiting thread
        pthread_cond_signal( &condVariable );
        
        // generate a thread if not too many threads
        if ( numThreads < maxThreads )
        {
            ThreadArg* t_arg = new ThreadArg;
            if ( t_arg == NULL )
            {
                throw -2;	// optional error - out of mem
            }	
            
            t_arg->timeoutSecs = &lingerTime;
            t_arg->q = &q;
            t_arg->poolMutex = &mutex;
            t_arg->condVariable = &condVariable;
            t_arg->zeroCountCondVariable = &zeroCountCondVariable;
            t_arg->numThreads = &numThreads;
            t_arg->die = &allDie;
        
            pthread_t thread;
            int code;
            
            // start a new thread
            code = pthread_create( &thread, NULL, ThreadCallback,
                t_arg );
                
            if ( code == 0 )
            {
                numThreads++;
                code = pthread_detach( thread );
                assert( code == 0 );
            }	
            
            if ( numThreads == 0 )
            {
                throw -2;
            }
        }
    }
    catch ( OutOfMemoryException& e )
    {
        retCode = -1;
    }
    catch ( int code )
    {
        if ( code == -2 )
        {
            // couldn't start new thread
            if ( numThreads == 0 )
            {
                retCode = -1;
            }
        }
    }
    
    pthread_mutex_unlock( &mutex );
    
    return retCode;
}

void ThreadPool::setMaxThreads( unsigned max )
{
    maxThreads = max;
}

unsigned ThreadPool::getMaxThreads()
{
    return maxThreads;
}

unsigned ThreadPool::getNumJobsPending()
{
    return q.length();
}

unsigned ThreadPool::getNumThreadsRunning()
{
    return numThreads;
}

unsigned ThreadPool::getLingerTime()
{
    return lingerTime;
}

void ThreadPool::setLingerTime( unsigned seconds )
{
    if ( seconds > MAX_LINGER_TIME )
    {
        lingerTime = MAX_LINGER_TIME;
    }
    else
    {
        lingerTime = seconds;
    }
}

// returns if num jobs in q == 0 or die signal has been given
void ThreadPool::waitForZeroJobs()
{
    sleep( 1 );
    
    pthread_mutex_lock( &mutex );
    // wait until num jobs == 0
    if ( q.length() != 0 && allDie == false )
    {
        pthread_cond_wait( &zeroCountCondVariable, &mutex );
    }
    pthread_mutex_unlock( &mutex );
    //DBG( printf("waitForZeroJobs(): done\n"); )
}
