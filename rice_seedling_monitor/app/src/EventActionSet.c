#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "EventActionSet.h"
#include "AsyncEvents.h"
#include "MyPublicFunction.h"
#include "AisleManage.h"
#include "Gpr.h"

//----------------------Define macro for-------------------//

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			:
Default value		: 0x00
The scope of value	: 0x01 - 0 aisle evt flag.
					: 0x02 - 1 aisle evt flag.
First used			:
*/
char g_EvtOkFlag = 0x00;

/*
Description			: connect server parameter.
Default value		: /
The scope of value	: /
First used			: AppInit();
*/
CNetParameter g_CParam = {0};

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void 	UploadBackData(void);

//---------------------------end---------------------------//

/***********************************************************************
**Function Name	: UploadBackData
**Description	: upload back data to server.
**Parameter		: none.
**Return		: none.
***********************************************************************/
static void UploadBackData(void)
{
	FILE *fp = NULL;
	unsigned char upload_buf[UPLOAD_SER_SIZE] = {0};
	TIME start;
	
	L_DEBUG("start a connection!\n");
				
	if(!ConnectServer(3, g_CParam))
	{
		fp = fopen(SLAVES_DATA_BACKUP, "r");
		if (NULL != fp)
		{
			do
			{
				fscanf(fp, "%s\n", upload_buf);
	
				if (0 != upload_buf[0])
				{
					SendDataToServer(upload_buf, strlen(upload_buf));
				}
	
			}while(!feof(fp));

			fclose(fp);
	
			remove(SLAVES_DATA_BACKUP);				
		}	
	}	
}

/***********************************************************************
**Function Name	: GetSlaveBaseInfo
**Description	: send get request.
**Parameters	: arg - in.
**Return		: 0 - ok, other value - error.
***********************************************************************/
void GetSlaveBaseInfo(int arg)
{
	EventParams param = (*(EventParams*)arg); 	
	int slave_sum = 0;
	int counter = 0;
	int address = 0;
	int res = 0;
	int send_req_counter = 4;
	TIME start;
	
	UploadBackData();
	
	slave_sum = GetSlaveSumOnAisle(param.m_Aisle);

	while (counter < slave_sum)
	{						
		address = GetSlaveAddrByPos(counter, param.m_Aisle);
	
		SendGReq(param.m_Aisle, (unsigned short)address, param.m_DataType, param.m_Param);
	
		L_DEBUG("%s:get a %d type request to %.5d\n", __FUNCTION__, param.m_DataType, address);
	
		GET_SYS_CURRENT_TIME(start);
	
		while (send_req_counter)
		{
			if (NULL_DATA_FLAG == GetAisleFlag(param.m_Aisle))
			{
				res = IS_TIMEOUT(start, (3*1000));
				if (res)
				{
					printf("%s:receive %.5d get req(%d) response timeout!\n", __FUNCTION__, address, param.m_DataType);
					//sprintf(g_TmpLog, "receive %.5d get req(%d) response timeout!</br>",((int)(address[0] << 8) | address[1]), param.m_DataType);
					//SaveTmpData(g_TmpLog);
			
					SendGReq(param.m_Aisle, (unsigned short)address, param.m_DataType, param.m_Param);
					GET_SYS_CURRENT_TIME(start);
					send_req_counter--;									
				}
			}
			else
			{
				while (!(PRO_DATA_OK_FLAG & GetAisleFlag(param.m_Aisle)))
				{
					Delay_ms(5);
				}
			
				break;
			}
		} //--- end of while (send_req_counter) ---//
	
		counter++;
		send_req_counter = 4;
	
		SetCurSlavePositionOnTab(param.m_Aisle, counter);
		SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
	
	} //--- end of while (counter < slave_sum) --//

	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);

	L_DEBUG("%s:send %d type request ok on %d aisle\n", __FUNCTION__, param.m_DataType, param.m_Aisle);	
	
	g_EvtOkFlag = (g_EvtOkFlag & !(0x01)); 	
	
	LogoutClient();
}











