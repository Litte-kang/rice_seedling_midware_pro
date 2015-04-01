#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "AsyncEvents.h"

//----------------------Define macro for-------------------//

#define READ_ASYNC_CMD_THREAD_STARTING	1	
#define READ_ASYNC_CMD_THREAD_STARTED	0

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: async event queue.
Default value		: IDLE_ASYNC_EVT.
The scope of value	: /
First used			: AsyncEventsInit
*/
static AsyncEvent g_AsyncEvents[MAX_ASYNC_EVT_SUM];

/*
Description			: the number of async event,it also is a flag.
Default value		: READ_ASYNC_CMD_THREAD_STARTING.
The scope of value	: 0 ~ MAX_ASYNC_EVT_SUM.
First used			: /
*/
static unsigned char g_CurAsyncEvtsSum = READ_ASYNC_CMD_THREAD_STARTING;

/*
Description			: position where read async cmd from queue.
Default value		: 0.
The scope of value	: 0 ~ (MAX_ASYNC_EVT_SUM - 1).
First used			: /
*/
static unsigned char g_CurReadPos = 0;

/*
Description			: position where write async cmd to queue.
Default value		: 0.
The scope of value	: 0 ~ (MAX_ASYNC_EVT_SUM - 1).
First used			: /
*/
static unsigned g_CurWritePos = 0;

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void* 	ReadAsyncEvtsThrd(void *pArg);
static void		MyDelay_ms(unsigned int xms);

//---------------------------end---------------------------//


/***********************************************************************
**Function Name	: AsyncEventsInit
**Description	: initialize async event queue,create a thread to read a event.
**Parameters	: none.
**Return		: 0 - initialize ok, other value - failed.
***********************************************************************/
int AsyncEventsInit()
{
	int res = 0;
	pthread_t thread;
	pthread_attr_t attr;
	void *thrd_ret = NULL;

	res = pthread_attr_init(&attr);
	if (0 != res)
	{
		printf("%s:create thread attribute failed!\n",__FUNCTION__);
		return -2;
	}

	res = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if (0 != res)
	{
		printf("%s:bind attribute failed!\n", __FUNCTION__);
	}
	
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (0 != res)
	{
		printf("%s:setting attribute failed!\n",__FUNCTION__);
		return -3;
	}

	res = pthread_create(&thread, &attr, ReadAsyncEvtsThrd, (void*)0);
	if (0 != res)
	{
		printf("%s:create connect \"ReadAsyncCmdsThrd\" failed!\n",__FUNCTION__);
		return -4;
	}
	
	pthread_attr_destroy(&attr);	
	
	while (READ_ASYNC_CMD_THREAD_STARTING == g_CurAsyncEvtsSum)
	{
		MyDelay_ms(2);
	}
	
	return 0;
}

/***********************************************************************
**Function Name	: MyDelay_ms
**Description	: delay ? ms,but it is not exact.
**Parameters	: xms - in.
**Return		: none.
***********************************************************************/
static void MyDelay_ms(unsigned int xms)
{
	struct timeval delay;
	
	delay.tv_sec = 0;
	delay.tv_usec = xms * 1000;
	
	select(0, NULL, NULL, NULL, &delay);
}

/***********************************************************************
**Function Name	: ReadAsyncEvtsThrd
**Description	: read async event from async event queue.
**Parameters	: pArg - in.
**Return		: none.
***********************************************************************/
static void* ReadAsyncEvtsThrd(void *pArg)
{
	AsyncEvent evt;
	
	g_CurAsyncEvtsSum = READ_ASYNC_CMD_THREAD_STARTED;
	
	printf("%s\n", __FUNCTION__);
	
	while (1)
	{
		if (0 < g_CurAsyncEvtsSum)
		{			
			evt = g_AsyncEvents[g_CurReadPos];
			
			evt.m_Action((int)&evt.m_Params);
			
			g_CurReadPos++;
			g_CurAsyncEvtsSum--;
			
			if (MAX_ASYNC_EVT_SUM <= g_CurReadPos)
			{
				g_CurReadPos = 0;
			}	
		}

		MyDelay_ms(50);		
	}
		
	pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: AddAsyncEvent
**Description	: add a async event.
**Parameters	: evt - in.
**Return		: 0 - add ok, -1 - failed.
***********************************************************************/
int AddAsyncEvent(AsyncEvent evt)
{
	if (MAX_ASYNC_EVT_SUM > g_CurAsyncEvtsSum)
	{
		g_AsyncEvents[g_CurWritePos] = evt;
		
		g_CurWritePos++;
		g_CurAsyncEvtsSum++;
			
		if (MAX_ASYNC_EVT_SUM <= g_CurWritePos)
		{
			g_CurWritePos = 0;
		}
		
		return 0;
	}
	
	printf("%s: async cmd queue fulled!\n", __FUNCTION__);
	
	return -1;
}












