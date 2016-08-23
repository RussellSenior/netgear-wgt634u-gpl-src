#include "genlib/timer_thread/timer_thread.h"
#include "syslog.h"
timer_thread_struct GLOBAL_TIMER_THREAD;

void free_upnp_timeout(upnp_timeout *event)
{
  if (event)
  { 
    if (event->Event)
      free(event->Event);
    free(event);
  }
}


void TimerThread(void * input)
{ 
  time_t next_event_time;
  time_t current_time;
  timer_event * current_event;
  struct timespec timeout;
  int retcode;

  struct timespec timesleep;
  timer_thread_struct * timer=(timer_thread_struct *) input;
  
  timesleep.tv_sec=0;
  timesleep.tv_nsec=10000000;
  
  while (1)
    { 
      
      if ( (pthread_mutex_lock(&timer->mutex)!=0)
	   || (timer->shutdown))
	{ 
	  pthread_mutex_unlock(&timer->mutex);
	  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER THREAD SHUT DOWN"));
	  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",pthread_self()));
	  return;
	}
      
      time(&current_time);
      if ((current_event=timer->eventQ)!=NULL)
	{
	  next_event_time=current_event->time;
	}
      else next_event_time=-1;
      //make callback if time has expired
      
      if ( (next_event_time!=-1) && (current_time>=next_event_time))
	{ //remove from the Q
	  timer->eventQ=timer->eventQ->next;
	  pthread_mutex_unlock(&timer->mutex);
	  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"SCHEDULING TIMER EVENT INTO THREAD POOL"));
	  tpool_Schedule( current_event->callback, current_event->argument); 
	  free(current_event);
	  continue;
	}
      
      if (next_event_time!=-1)
	{
	  timeout.tv_sec=next_event_time;
	  timeout.tv_nsec=0;
	  retcode=0;
	  
	  while ( (timer->newEvent==0) && (retcode!=ETIMEDOUT))
	  { 
		if (timer->newEventCond.__c_lock.__spinlock != 0)
		{
			--(timer->newEventCond.__c_lock.__spinlock);
			retcode=0;
			break;
		}
		time(&current_time);
		if (current_time>=timeout.tv_sec)
		{
			retcode=ETIMEDOUT;
			break;
		}
		pthread_mutex_unlock(&timer->mutex);
		nanosleep(&timesleep, NULL);
		pthread_mutex_lock(&timer->mutex);
	  }

	  if (timer->shutdown)
	    {
	      pthread_mutex_unlock(&timer->mutex);
	      DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER THREAD SHUT DOWN"));
	      DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",pthread_self()));
	      return;
	    }
	  if (timer->newEvent==1)
	    timer->newEvent=0;
	  pthread_mutex_unlock(&timer->mutex);
	  continue;
	  
	}
      else
	{
	  while ((timer->newEvent==0))
	    {
	      retcode=pthread_cond_wait(&timer->newEventCond,
					     &timer->mutex);
	    }
	 
	  if (timer->shutdown)
	    {
	      //shutdown     
	      pthread_mutex_unlock(&timer->mutex);
	      DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER_THREAD SHUTDOWN"));
	      DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",pthread_self()));
	      return;
	    }
	  timer->newEvent=0;
	  pthread_mutex_unlock(&timer->mutex);
	  continue;
	}
    }
  
}

int InitTimerThread(timer_thread_struct * timer)
{
  pthread_mutex_init(&timer->mutex, NULL);

  pthread_mutex_lock(&timer->mutex);

  pthread_cond_init(&timer->newEventCond,NULL);
  
  timer->eventQ=NULL;
  timer->newEvent=0;
  timer->currentEventId=0;
  timer->shutdown=0;
  
  pthread_mutex_unlock(&timer->mutex);
  //schedule timer_thread
  if (tpool_Schedule( TimerThread, timer)!=0)
    return UPNP_E_INIT_FAILED;
  
  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER THREAD INITIALIZED"));
  return UPNP_E_SUCCESS;
}

int RemoveTimerEvent(int eventId, void **argument, timer_thread_struct *timer)
{
  timer_event * current_event=NULL;
  timer_event * previous=NULL;
  int found=0;
  
  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,
		     "TRYING TO REMOVE EVENT : %d\n",eventId));
  if (eventId!=-1)
  {
    pthread_mutex_lock(&timer->mutex);
    current_event=timer->eventQ;
    while (current_event)
      {
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TRYING TO REMOVE EVENT : %d\n",eventId));
	if (current_event->eventId==eventId)
	  {
	    found=1;
	    break;
	  }
	previous=current_event;
	current_event=current_event->next;
      }
    if (found)
      {
	(*argument)=current_event->argument;
	if (previous)
	  {
	    previous->next=current_event->next;
	  }
	else
	  {
	    timer->eventQ=current_event->next;
	  }
	free(current_event);
	pthread_cond_signal(&timer->newEventCond);
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"REMOVING TIMER EVENT"));
      }
    else
      (*argument)=NULL;
    pthread_mutex_unlock(&timer->mutex);
  }
  
  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER EVENT REMOVED"));
  return found;
}

int StopTimerThread(timer_thread_struct * timer)
{
  timer_event * current_event;

  pthread_mutex_lock(&timer->mutex);
  
  timer->shutdown=1;
  
  while (timer->eventQ)
    {
      current_event=timer->eventQ;
      timer->eventQ=timer->eventQ->next;
      pthread_mutex_unlock(&timer->mutex);
      
      current_event->callback(current_event->argument);
      free(current_event);

      pthread_mutex_lock(&timer->mutex);
    }
 
  //signal timer thread to stop
  timer->newEvent=1;
  pthread_cond_signal(&timer->newEventCond);
  pthread_mutex_unlock(&timer->mutex);
  pthread_mutex_destroy(&timer->mutex);
  pthread_cond_destroy(&timer->newEventCond);
  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"STOP TIMER FINISHED"));
  return UPNP_E_SUCCESS;
}

int ScheduleTimerEvent(int TimeOut, ScheduleFunc callback, void * argument,
		       timer_thread_struct *timer, int * eventId)
{
  timer_event *new_event;
  timer_event *prev=NULL;
  timer_event *finger=NULL;
  time_t current_time;
  time_t expireTime;
  //get time
  time(&current_time);
  expireTime=current_time+TimeOut;

  new_event=(timer_event*) malloc(sizeof(timer_event));
  if (new_event==NULL)
    return UPNP_E_OUTOF_MEMORY;

  new_event->time=expireTime;
  new_event->callback=callback;
  new_event->argument=argument;
  new_event->next=NULL;
  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TRYING TO GET TIMER MUTEX"));
  pthread_mutex_lock(&timer->mutex);
  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"GOT TIMER MUTEX"));
  if (timer->shutdown)
    {
      free(new_event);
      pthread_mutex_unlock(&timer->mutex);
      return UPNP_E_INVALID_PARAM;
    }
  new_event->eventId=timer->currentEventId++;

  //check for overflow
  if (timer->currentEventId<0)
    timer->currentEventId=0;

  finger=timer->eventQ;
  
  while ( (finger) && (finger->time<new_event->time) )
    {
      prev=finger;
      finger=finger->next;
    }

  new_event->next=finger;

  if (prev)
    {
      prev->next=new_event;
    }
  else
    { 
      timer->eventQ=new_event;
    }
   
  timer->newEvent=1;
  (*eventId)=new_event->eventId;
  pthread_cond_signal(&timer->newEventCond);
  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"ADDING TIMER EVENT TO TIMER Q"));
  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER EVENT ID %d\n",new_event->eventId));
  pthread_mutex_unlock(&timer->mutex);		 
  return UPNP_E_SUCCESS;
}
