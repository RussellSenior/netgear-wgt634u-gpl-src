#ifndef _TIMER_THREAD_
#define _TIMER_THREAD_

#include "upnp.h"
#include <time.h>
#include <errno.h>
#include "genlib/tpool/scheduler.h"
#include <malloc.h>
#include "tools/config.h"


#include <pthread.h>
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else 
#define EXTERN_C 
#endif

typedef struct UPNP_TIMEOUT {
  int EventType;
  int handle;
  int eventId;
  void *Event;
} upnp_timeout;


typedef struct TIMER_EVENT {
  time_t time;
  ScheduleFunc callback;
  void * argument;
  int eventId;
  struct TIMER_EVENT * next;
} timer_event;

typedef struct TIMER_THREAD_STRUCT {
  pthread_mutex_t mutex;
  pthread_cond_t newEventCond; 
  int newEvent;
  int shutdown;
  int currentEventId;
  timer_event * eventQ;
} timer_thread_struct;


extern timer_thread_struct GLOBAL_TIMER_THREAD;

EXTERN_C int InitTimerThread(timer_thread_struct * timer);

EXTERN_C int StopTimerThread(timer_thread_struct * timer);

EXTERN_C int ScheduleTimerEvent(int TimeOut, 
				ScheduleFunc callback,
				void * argument,
				timer_thread_struct * timer,
				int * eventId);

EXTERN_C int RemoveTimerEvent(int eventId, void **argument, timer_thread_struct *timer);

EXTERN_C void free_upnp_timeout(upnp_timeout *event);

#endif
