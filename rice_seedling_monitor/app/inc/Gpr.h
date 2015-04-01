#ifndef _GPR_H_
#define _GPR_H_

//----------------------Define macro for-------------------//

#define DATA_LEN	2*1024

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//
//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int SendGReq(int aisle, unsigned short address, unsigned char type, unsigned char param);
extern int SendPReq(int aisle, unsigned short address, unsigned char type, unsigned char *pData, unsigned short dataLen);
extern int SendResponse(int aisle, unsigned short address, unsigned char type, unsigned char gpr_type, unsigned char  *pData, unsigned short dataLen);
extern int IsGpr(unsigned char *pGpr);

//---------------------------end---------------------------//

#endif	//--_GPR_H_--//
