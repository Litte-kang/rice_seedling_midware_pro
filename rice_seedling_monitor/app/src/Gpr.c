#include "Gpr.h"
#include "uart_api.h"
#include "stdio.h"
#include "string.h"
#include "MyPublicFunction.h"

//----------------------Define macro for-------------------//

#define MEMCPY	memcpy
#define SEND	write

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			:
Default value		:
The scope of value	:
First used			:
*/

//---------------------------end--------------------------//


//------------Declaration static function for--------------//
//---------------------------end---------------------------//

/***********************************************************************
**Function Name	: SendGReq
**Description	: send get request.
**Parameters	: aisle - in.
				: address - in.
				: type - request type.
				: param - in.
**Return		: 0 - ok, other value - error.
***********************************************************************/
int SendGReq(int aisle, unsigned short address, unsigned char type, unsigned char param)
{
	unsigned char req[6] = {0};
	
	req[0] = 'G';
	req[1] = (unsigned char)(address >> 8);
	req[2] = (unsigned char)address;
	req[3] = type;
	req[4] = param;
	req[5] = 'E';
	
	//--- send ---//
	if (6 != SEND(aisle, req, 6))
	{
		return -1;
	}
	
	return 0;
}

/***********************************************************************
**Function Name	: SendPReq
**Description	: send post request(post request is contain data).
**Parameters	: aisle - in
				: address - in.
				: type - request type.
				: pData - data.
				: dataLen - in.
**Return		: 0 - ok, other value - error.
***********************************************************************/
int SendPReq(int aisle, unsigned short address, unsigned char type, unsigned char *pData, unsigned short dataLen)
{
	unsigned char req[DATA_LEN] = {0};
	int crc_code = 0;
	
	if (NULL == pData)
	{
		return -1;
	}

	req[0] = 'P';
	req[1] = (unsigned char)(address >> 8);
	req[2] = (unsigned char)address;
	req[3] = type;
	req[4] = 'E';
	req[5] = (unsigned char)(dataLen >> 8);
	req[6] = (unsigned char)dataLen;

	MEMCPY(&req[7], pData, dataLen);

	crc_code = CreateCRC16CheckCode_1(pData, (unsigned int)dataLen);

	req[7 + dataLen] = (unsigned char)(crc_code >> 8);
	req[8 + dataLen] = (unsigned char)crc_code;

	req[9 + dataLen] = 'E';

	//--- send ---//
	if ((10 + dataLen) == SEND(aisle, req, (10 + dataLen)))
	{
		return -1;
	}

	return 0;
}

/***********************************************************************
**Function Name	: SendResponse
**Description	: send a response.
**Parameters	: aisle - in.
				: address - in.
				: type - request type.
				: gprType - in.
				: pData - response data.
				: dataLen - in.
**Return		: 0 - ok, other value - error.
***********************************************************************/
int SendResponse(int aisle, unsigned short address, unsigned char type, unsigned char gprType, unsigned char  *pData, unsigned short dataLen)
{
	unsigned char res[DATA_LEN] = {0};
	int crc_code = 0;
	
	if (NULL == pData)
	{
		return -1;
	}

	res[0] = 'R';
	res[1] = (unsigned char)(address >> 8);
	res[2] = (unsigned char)address;
	res[3] = type;
	res[4] = gprType;
	res[5] = (unsigned char)(dataLen >> 8);
	res[6] = (unsigned char)dataLen;

	MEMCPY(&res[7], pData, dataLen);

	crc_code = CreateCRC16CheckCode_1(pData, (unsigned int)dataLen);

	res[7 + dataLen] = (unsigned char)(crc_code >> 8);
	res[8 + dataLen] = (unsigned char)crc_code;

	res[9 + dataLen] = 'E';

	//--- send ---//
	if ((10 + dataLen) == SEND(aisle, res, (10 + dataLen)))
	{
		return -1;
	}

	return 0;
}

/***********************************************************************
**Function Name	: IsGpr
**Description	: check data 
**Parameters	: pGpr - in.
**Return		: gpr type, other value - error.
***********************************************************************/
int IsGpr(unsigned char *pGpr)
{
	int ret = 0;
	int data_len = 0;
	int crc_code = 0;

	if (NULL == pGpr)
	{
		return -1;
	}

	switch (pGpr[0])
	{
		case 'G':
			{
				if ('E' == pGpr[5])
				{
					return (int)pGpr[0];
				}

				return -3;
			}
		case 'P':
		case 'R':
			{
				if ('G' == pGpr[4] || 'P' == pGpr[4] || 'E' == pGpr[4])
				{
					data_len = (int)pGpr[5];
					data_len <<= 8;
					data_len |= (int)pGpr[6];
					
					if ('E' == pGpr[9 + data_len])
					{
						crc_code = (int)pGpr[7 + data_len];
						crc_code <<= 8;
						crc_code |= (int)pGpr[8 + data_len];
						
						if (crc_code == (0x0000ffff & CreateCRC16CheckCode_1(&pGpr[7], (unsigned int)data_len)))
						{
							return (int)pGpr[0];
						}
					}
				}

				return -3;
			}
		default:
			return -2;

	}

	return 0;
}

