#ifndef _ASYNC_EVENTS_H_
#define _ASYNC_EVENTS_H_

//----------------------Define macro for-------------------//

#define MAX_ASYNC_EVT_SUM	5

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

typedef void (*EvtAction)(int); 

typedef struct _EventParams
{
	int				m_Aisle;
	int 			m_DataType;
	unsigned char 	m_Param;
}EventParams;

typedef struct _AsyncEvent
{
	EventParams		m_Params;	
	EvtAction		m_Action;
}AsyncEvent;

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int 		AsyncEventsInit(void);
extern int 		AddAsyncEvent(AsyncEvent evt);

//---------------------------end---------------------------//

#endif	//--_ASYNC_EVENTS_H_--//
