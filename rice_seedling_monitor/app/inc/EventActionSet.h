#ifndef _EVENT_ACTION_SET_H_
#define _EVENT_ACTION_SET_H_

#include "MyClientSocket.h"

//----------------------Define macro for-------------------//



//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

/*
Description			:
Default value		:
The scope of value	: 0x01 - 0 aisle evt flag.
					: 0x02 - 1 aisle evt flag.
First used			:
*/
extern char g_EvtOkFlag;

/*
Description			: connect server parameter.
Default value		: /
The scope of value	: /
First used			: AppInit();
*/
extern CNetParameter g_CParam;

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern void 		GetSlaveBaseInfo(int arg);

//---------------------------end---------------------------//

#endif	//--_EVENT_ACTION_SET_H_--//
