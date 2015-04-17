#include "AisleManage.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "MyClientSocket.h"
#include "Gpr.h"

#define INVAILD_SLAVE_ADDR		65535

//----------------------------------------DECLARATION FUNCIONT---------------------------------//

static void 	SendFwToSlave(int aisle, int pos, unsigned char *pData);
static void 	GReqResponsePro(int aisle, int pos, unsigned char *pData);
static void     PReqResponsePro(int aisle, int pos, unsigned char *pData);
static int		GetFw(AisleInfo *pInfo, unsigned char type);		
static int 		UploadDataToServer(const char *pFileName, int address, int type, int len, unsigned char *pData, int pos);
static int 		GetAislePositionOnTab(int aisle);

//------------------------------------DECLARATION FUNCIONT END-------------------------------//

//--------------------------------------DECLARATION VARIABLE--------------------------------//


/*
Description			: to store fw information
Default value		: /.
The scope of value	: /.
First used			: AisleManageInit()
*/
FWInformation g_FWInfo = {0};

/*
Description			: aisles.
Default value		: /.
The scope of value	: /.
First used			: ProtocolEventInit()
*/
static AisleInfo g_AisleInfo[USER_COM_SIZE];

//----------------------------------DECLARATION VARIABLE END --------------------------------//

/***********************************************************************
**Function Name	: AisleManageInit
**Description	: initialize some variable.
**Parameter		: none.
**Return		: none.
***********************************************************************/
void AisleManageInit()
{
	unsigned int slave_sum = 0;
	int slave_addr = 0;
	char slaves_addr_conf[38] = {0};
	int i = 0;
	int n = 0;
	FILE *fp = NULL;
	
	SaveTmpData((void*)0);
	
	//--- get aisle configration information ---//	
	for (i = 0; i < USER_COM_SIZE; ++i)
	{
		memset(g_AisleInfo[i].m_SlavesAddrTab, 0xff, (MAX_SLAVE_SUM*SLAVE_ADDR_LEN));
		
		sprintf(slaves_addr_conf, "%s%.2d", SLAVES_ADDR_CONF, i);
		
		fp = fopen(slaves_addr_conf, "r");
		if (NULL == fp)
		{
			printf("%s:get slaves address failed(%s)!\n", __FUNCTION__, slaves_addr_conf);
			exit(1);
		}
		
		//--- manage slave address ---//
		while(!feof(fp))
		{
			if (0 >= fscanf(fp, "%d\n", &slave_addr))
			{
				printf("slave address is empty!\n");
				break;
			}
			
			if (INVAILD_SLAVE_ADDR == slave_addr)
			{
				printf("a invaild slave addr!\n");
				continue;
			}
			
			g_AisleInfo[i].m_SlavesAddrTab[n][0] = slave_addr >> 8;
			g_AisleInfo[i].m_SlavesAddrTab[n][1] = slave_addr;
		
			L_DEBUG("%.5d\n",slave_addr);	
			n++;
			slave_sum++;
		}		
		
		g_AisleInfo[i].m_Flag = NULL_DATA_FLAG;
		g_AisleInfo[i].m_FwCount = -1;
		g_AisleInfo[i].m_Aisle = g_UartFDS[i];
		g_AisleInfo[i].m_SlaveSum = slave_sum;
		g_AisleInfo[i].m_CurSlavePosition = 0;
		
		L_DEBUG("slave_sum[%d] = %d\n", i, g_AisleInfo[i].m_SlaveSum);
		
		fclose(fp);
	} 
	
}

/***********************************************************************
**Function Name	: GetFw
**Description	: get a fw and send to slave.
**Parameter		: pInfo - in.
				: type - in.
**Return		: ?.
***********************************************************************/
static int	GetFw(AisleInfo *pInfo, unsigned char type)
{
	FILE *p_read_fw = 0;
	int real_read = 0;
	int read_len = 0;
	int address = 0;
	char fw_path[34] = {0};
	unsigned char fw_buff[AVG_SECTION_FW_SIZE + 1] = {0};
		
	//--- part fw data ---//
	sprintf(fw_path, "%s0.%s.fw", FW_0_PATH, g_FWInfo.m_Version);
	
	L_DEBUG("fw_path = %s\n", fw_path);
	
	p_read_fw = fopen(fw_path, "rb");
	
	if (NULL == p_read_fw)
	{
		printf("open file failed!\n");
		return -1;
	}
	
	read_len = (pInfo->m_FwCount + 1) < g_FWInfo.m_SectionSum ? AVG_SECTION_FW_SIZE : g_FWInfo.m_LastSectionSize;

	fseek(p_read_fw, (AVG_SECTION_FW_SIZE * pInfo->m_FwCount), SEEK_SET);

	real_read = fread(fw_buff, 1, read_len, p_read_fw);

	if (read_len != real_read)
	{
		printf("%dth read %s error!\n",pInfo->m_FwCount+1, fw_path);
		fclose(p_read_fw);
		return -2;
	}

	fclose(p_read_fw);
	
	//--- send fw ---//
	L_DEBUG("send fw = %d byte\n", read_len);
	
	address = pInfo->m_SlavesAddrTab[pInfo->m_CurSlavePosition][0];
	address <<= 8;
	address |= pInfo->m_SlavesAddrTab[pInfo->m_CurSlavePosition][0];
	
	SendResponse(pInfo->m_Aisle, address, type, 'G', fw_buff, read_len);
	
	return 0;
}

/***********************************************************************
**Function Name	: GetAisleTabPosition
**Description	: get aisle position in g_AisleInfo.
**Parameter		: pData - in.
**Return		: -1 - not exist aisle in g_AisleInfo, i - position.
***********************************************************************/
static int GetAislePositionOnTab(int aisle)
{
	int i = 0;

	for (i = 0; i < USER_COM_SIZE; ++i)
	{
		if (g_AisleInfo[i].m_Aisle == aisle)
		{
			return i;
		}
	}

	return -1;
}

/***********************************************************************
**Function Name	: SendFwToSlave
**Description	: send a fw to slave.
**Parameter		: aisle - in.
				: pos - in.
				: pData - alert data.
**Return		: none.
***********************************************************************/
static void SendFwToSlave(int aisle, int pos, unsigned char *pData)
{
	unsigned char *p = NULL;
	AisleInfo *p_info = NULL;
	int res = 0;
	int type = 0;

	if (NULL == pData)
	{
		return;
	}
	
	p = pData;
	
	p_info = &g_AisleInfo[pos];
	
	type = (int)p[3];
				
	L_DEBUG("slave(%.5d) get %d type request by %d aisle!\n", ((int)(p[1] << 8) | p[2]), type, p_info->m_Aisle);
		
	res = 1; //-- default correct type --//
	
	switch (type)
	{
		case NEXT_FW_G_REQ:	//-- send next fw --//
			p_info->m_FwCount = (-1 == p_info->m_FwCount) ? 0 : p_info->m_FwCount++;					
			GetFw(p_info, 4);
			L_DEBUG("--------------------------sent %d th section\n",p_info->m_FwCount+1);
			break;
		case CUR_FW_G_REQ: //-- send cur fw --//
			L_DEBUG("--------------------------sent %d th failed ,send againt\n",p_info->m_FwCount+1);
			GetFw(p_info, 5);					
			break;
		case STOP_UPDATE_SLAVE_G_REQ: //-- stop sending fw --//
			if (0 == p[4])
			{
				p_info->m_FwCount++;
				L_DEBUG("--------------------------send end! total %d sector\n",p_info->m_FwCount);				
			}	
			else
			{
				L_DEBUG("--------------------------%.5d is latest version fw\n",((int)(p[3] << 8) | p[4]));
				p_info->m_FwCount = g_FWInfo.m_SectionSum;				
			}
			break;
		default:
			res = -1;	//-- error type --//
			break;
	}
	
	if (1 == res)
	{		
		p_info->m_Flag |= PRO_DATA_OK_FLAG;
	}	
}

/***********************************************************************
**Function Name	: GReqResponsePros
**Description	: process get request response.
**Parameter		: aisle - in.
				: pos - in.
				: pData - alert data.
**Return		: none.
***********************************************************************/
static void GReqResponsePro(int aisle, int pos, unsigned char *pData)
{	
	int data_len = 0;
	int data_type = 0;
	int address = 0;

	data_len = (int)pData[5];
	data_len <<= 8;
	data_len |= (int)pData[6];	
	
	data_type = (int)pData[3];
	
	address = ((int)(pData[1] << 8) | pData[2]);
	
	L_DEBUG("get %d type data response from %.5d by %d aisle\n", data_type, address, aisle);
	
	g_AisleInfo[pos].m_Flag |= REC_DATA_FLAG;
	
	if (0 <= data_len)
	{	
		UploadDataToServer(SLAVES_DATA_BACKUP, address, data_type, data_len, &pData[7], pos);
	}
	
	g_AisleInfo[pos].m_Flag |= PRO_DATA_OK_FLAG;
}

/***********************************************************************
**Function Name	: PReqResponsePro
**Description	: process post request response.
**Parameter		: aisle - in.
				: pos - in.
				: pData - alert data.
**Return		: none.
***********************************************************************/
static void PReqResponsePro(int aisle, int pos, unsigned char *pData)
{
	int data_len = 0;
	unsigned char *p = NULL;
	
	/*
	p = &g_RemoteData.m_PData[2];
	data_len = (int)(g_RemoteData.m_DataLen - 2);
	
	L_DEBUG("post %d type data response from %.5d by %d aisle", pData[3], ((int)(pData[1] << 8) | pData[2]), aisle);
	
	g_AisleInfo[pos].m_Flag |= REC_DATA_FLAG;
	
	if (1 == pData[7]) //-- 1 - modify failed,0 - ok --//
	{	
		SendPReq(aisle, ((int)(pData[1] << 8) | pData[2]), pData[3], p, data_len);
		
		return;
	}
	
	g_AisleInfo[pos].m_Flag |= PRO_DATA_OK_FLAG;
	*/
}

/***********************************************************************
**Function Name	: UploadDataToServer
**Description	: upload data to server.
**Parameter		: pFileName - file name.
				: address - in.
				: type - in.
				: len - in.
				: pData - data.
				: pos - in.
**Return		: 0 - ok, ohter - failed.
***********************************************************************/
static int UploadDataToServer(const char *pFileName, int address, int type, int len, unsigned char *pData, int pos)
{
	int i = 0;
	int tmp = 0;
	unsigned char upload_buff[UPLOAD_SER_SIZE] = {0};
	unsigned char str[200] = {0};
	
	sprintf(upload_buff, "{\"type\":%d,\"midAddress\":\"%s\",\"address\":\"%.5d\",\"data\":",type, g_MyLocalID, address);
	
	switch (type)
	{                
		case 0:
			{
				int total_rotates = 0;
				double work_speed = 0;
				
				total_rotates = CONV_TO_INT(pData[0], pData[1], pData[2], pData[3]);
				
				work_speed = (double)pData[4] + ((double)pData[5] / 100);	
				                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
				tmp = strlen(upload_buff);
				
				sprintf(&upload_buff[tmp], "[%d,%.1f]}",total_rotates, work_speed);		
				
			}
			break;
		default:
			break;
	}
	
	tmp = strlen(upload_buff);
	
	L_DEBUG("json(%d):%s\n", tmp, upload_buff);
	
	//--- send to server ---//
	tmp = SendDataToServer(upload_buff, tmp);
	
	{
		
		for (i = 1; i < 8; ++i)
		{
		
			if (i != address)
			{
				sprintf(str, "{\"type\":0,\"midAddress\":\"0000000000\",\"address\":\"%.5d\",\"data\":[25,5]}",i);	
			
				tmp = strlen(str);
			
				tmp = SendDataToServer(str, tmp);
			}
		}
	}
	
	if (0 != tmp)
	{
		BackupAsciiData(pFileName, upload_buff);
	}
	else
	{
		L_DEBUG("data upload server ok!\n");
	}
	
	//tmp = strlen(upload_buff);
	//memcpy(&upload_buff[tmp], "</br>", 5);
	//SaveTmpData(upload_buff);	
	
	return tmp;
}


/***********************************************************************
**Function Name	: ProcAisleData
**Description	: process data from aisle(uart).
**Parameter		: aisle - in
				: pData - protocol data.
                : len - the length of uart data.
**Return		: none.
***********************************************************************/
void ProcAisleData(int aisle, unsigned char *pData, unsigned int len)
{
	unsigned char *p = NULL;
	int pos = 0;
    int slave_position = 0;
	int res = 0;
	int type = 0;

	if (NULL == pData)
    {
        return;
    }
    
	p = pData;
	
	while (len)
	{
		pos = GetAislePositionOnTab(aisle);
		slave_position = g_AisleInfo[pos].m_CurSlavePosition;		
		type = p[3];
		
		if (0 <= IsGpr(p) && 0 == memcmp(g_AisleInfo[pos].m_SlavesAddrTab[slave_position], &p[1], SLAVE_ADDR_LEN))
		{
			if ('G' == p[0]) //-- get request --//
			{
				{
					switch (type)
					{
						case NEXT_FW_G_REQ:
						case CUR_FW_G_REQ:
						case STOP_UPDATE_SLAVE_G_REQ:
							SendFwToSlave(aisle, pos, p);
							break;
						default:
							break;
					}
				}
			}
			else if ('P' == p[0]) //-- post request --//
			{
				{
					//-----//
				}
			}
			else if ('R' == p[0]) //-- response --//
			{
				{			
					if ('G' == p[4]) //-- get request response --//
					{
						GReqResponsePro(aisle, pos, p);
					}
					else //-- post request response --//
					{
						PReqResponsePro(aisle, pos, p);						
					}
				}
			}
			else
			{
				
			}
			
			len = 1;
		}
		
		p++;
		len--;
	}	
}

/***********************************************************************
**Function Name	: IsFWUpdateSuccess
**Description	: check whether update is successful by aisle.
**Parameter		: aisle - in
**Return		: 1 - updating, 0 - update successful, -1 - error aisle.
***********************************************************************/
int IsFWUpdateSuccess(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);

	if (-1 == res)
	{
		printf("error aisle\n");
		return -1;
	}

	return (g_AisleInfo[res].m_FwCount < g_FWInfo.m_SectionSum ? 1 : 0);
}

/***********************************************************************
**Function Name	: ClearFwCount
**Description	: clear fw count by aisle
**Parameter		: aisle - in
**Return		: none.
***********************************************************************/
void ClearFwCount(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("error aisle\n");
		
		return;
	}
	
	g_AisleInfo[res].m_FwCount = -1;
}

/***********************************************************************
**Function Name	: GetCurFwCount
**Description	: get fw count by aisle
**Parameter		: aisle - in
**Return		: -1 - failed, count.
***********************************************************************/
int GetCurFwCount(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("error aisle\n");
		
		return;
	}	
	
	return g_AisleInfo[res].m_FwCount;
}

/***********************************************************************
**Function Name	: SetAisleFlag
**Description	: set aisle flag.
**Parameter		: aisle - in.
				: flag - in.
**Return		: none.
***********************************************************************/
void SetAisleFlag(int aisle, unsigned char flag)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("error aisle\n");
		return;
	}
	
	if (NULL_DATA_FLAG == flag)
	{
		g_AisleInfo[res].m_Flag = NULL_DATA_FLAG;
	}
	else
	{
		g_AisleInfo[res].m_Flag |= flag;
	}
}

/***********************************************************************
**Function Name	: SetCurSlavePositionOnTab
**Description	: set current slave table position by aisle.
**Parameter		: aisle - in.
				: position - in.
**Return		: none.
***********************************************************************/
void SetCurSlavePositionOnTab(int aisle, unsigned int position)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("SetCurCommuInfoTabPosition:error aisle\n");
		return;
	}
	
	g_AisleInfo[res].m_CurSlavePosition = position;	
}

/***********************************************************************
**Function Name	: GetCurSlavePositionOnTab
**Description	: get current slaver table position.
**Parameter		: aisle - in.
**Return		: none.
***********************************************************************/
unsigned int GetCurSlavePositionOnTab(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("GetCurCommuInfoTabPosition:error aisle\n");
		return -1;
	}
	
	return g_AisleInfo[res].m_CurSlavePosition;		
}

/***********************************************************************
**Function Name	: GetSlaveSumOnAisle
**Description	: get the number of slave on a aisle.
**Parameter		: aisle - in.
**Return		: none.
***********************************************************************/
unsigned int GetSlaveSumOnAisle(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("GetCommuInfoTabSlaverSum:error aisle\n");
		return -1;
	}
	
	return g_AisleInfo[res].m_SlaveSum;	
}

/***********************************************************************
**Function Name	: GetAisleFlag
**Description	: get aisle flag.
**Parameter		: aisle - in.
**Return		: aisle flag.
***********************************************************************/
unsigned char GetAisleFlag(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("IsRecCommuDat:error aisle\n");
		return -1;
	}
	
	return g_AisleInfo[res].m_Flag;		
}

/***********************************************************************
**Function Name	: GetSlavePositionOnTab
**Description	: find position of slave addr in g_AisleInfo[i].m_SlavesAddrTab.
**Parameter		: addr - in.
				: pPos - position.
				: aisle - in.
**Return		: -1 - failed, 0 - success
***********************************************************************/
int GetSlavePositionOnTab(int addr, int *pPos ,int aisle)
{
	int i = 0;
	int pos = 0;
	unsigned char tmp_addr[SLAVE_ADDR_LEN] = {0};
	
	if (NULL == pPos)
	{
		return -1;
	}
	
	tmp_addr[0] = (unsigned char)(addr >> 8);
	tmp_addr[1] = (unsigned char)addr;
	
	pos = GetAislePositionOnTab(aisle);
	
	for (i = 0; i < g_AisleInfo[pos].m_SlaveSum; ++i)
	{
		if (0 == memcmp(g_AisleInfo[pos].m_SlavesAddrTab[i], tmp_addr, SLAVE_ADDR_LEN))
		{
			*pPos = i;
			return 0;
		}
	}
	
	return -1;
}

/***********************************************************************
**Function Name	: SaveTmpData
**Description	: save 1000 data.
**Parameter		: pData - in.
**Return		: none.
***********************************************************************/
void SaveTmpData(unsigned char *pData)
{
	FILE *fp = NULL;
	static short counter = 1000;
	
	counter++;
		
	if (1000 >= counter)
	{
		BackupAsciiData("/tmp/tmp.log", pData);
	}
	else
	{
		fp = fopen("/tmp/tmp.log", "w");
		fprintf(fp, "{\"tmp.log\"}</br>\n");
		fclose(fp);
		counter = 0;
	}
}

/***********************************************************************
**Function Name	: GetSlaveAddrByPos
**Description	: get a slave address by position.
**Parameter		: pos - in.
				: ailse - in.
**Return		: slave address.
***********************************************************************/
int	GetSlaveAddrByPos(int pos, int aisle)
{
	int n = 0;
	int address = 0;
	
	
	n = GetAislePositionOnTab(aisle);
	
	address = (int)g_AisleInfo[n].m_SlavesAddrTab[pos][0];
	address <<= 8;
	address |= (int)g_AisleInfo[n].m_SlavesAddrTab[pos][1];
	
	return address;
}










