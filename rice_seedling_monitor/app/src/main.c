#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include "MyClientSocket.h"
#include "MyPublicFunction.h"
#include "Gpr.h"
#include "AsyncEvents.h"
#include "uart_api.h"
#include "EventActionSet.h"

//----------------------Define macro for-------------------//

#define THREAD_SUM	2
#define ALARM_TIME	5

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: 
Default value		: /
The scope of value	: /
First used			: 
*/

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void		AppInit();
static void*	Thrds(void *pArg);
static void		IdleTask();
static void		RecUartData(int aisle);
static void		SendUartData(int aisle);
static void 	TimerCallback(int SigNum);

//---------------------------end---------------------------//

int main()
{
	
	AppInit();
	
	return 0;
}

/***********************************************************************
**Function Name	:
**Description	:
**Parameters	:
**Return		:
***********************************************************************/
static void AppInit()
{
	pthread_t thread[THREAD_SUM];
	void *thread_ret = NULL;
#if (COM_TYPE == GNR_COM)
	int target_com[USER_COM_SIZE] = {TARGET_COM_PORT0};
#else
	int target_com[USER_COM_SIZE] = {HOST_COM_PORT0}; 
#endif	
	int i = 0;
	int res = 0;
	int thrd_flag[THREAD_SUM] = {0,100};
	FILE *fp = NULL;
	
	for (i = 0; i < USER_COM_SIZE; ++i)
	{	
		res = open_port(TARGET_COM_PORT0);
	
		if (0 >= res)
		{
			printf("%s:open port failed!\n",__FUNCTION__);
			return;
		}
	
		g_UartFDS[i] = res;
		
		L_DEBUG("uart fd = %d\n",g_UartFDS[i]);
	
		set_com_config(res, 9600, 8, 'N', 1);
	}
	
	AisleManageInit();
	
	AsyncEventsInit();
	
	//--- read middleware id ---//
	{
		fp = fopen(MID_ID_PATH, "r");
		fscanf(fp, "%s", g_MyLocalID);
		fclose(fp);	
		
		L_DEBUG("MID_ID = %s\n",g_MyLocalID);
	}	
	//--- end ---//
	
	{
		char ip_addr[16] = {0};
		int ip = 0;

		fp = fopen(SER_IP_ADDR, "r");
		fscanf(fp, "%s", ip_addr);
		fclose(fp);
		
		memcpy(g_CParam.m_IPAddr, ip_addr, strlen(ip_addr));
		g_CParam.m_Port = 8000;

		printf("server ip %s\n", g_CParam.m_IPAddr);			
	}
	
	{
		struct sigaction sa;
		struct sigaction sig;

		sa.sa_handler = SIG_IGN;
		sig.sa_handler = TimerCallback;

		sigemptyset(&sig.sa_mask);
		sig.sa_flags = 0;

		sigaction(SIGALRM, &sa, 0);
		sigaction(SIGALRM, &sig, 0);

		alarm(30);		
	}
	
	for (i = 0; i < THREAD_SUM; ++i)
	{
		res = pthread_create(&thread[i], NULL, Thrds, (void*)thrd_flag[i]);
		
		if (0 != res)
		{
			printf("%s:create %d thread faild\n", __FUNCTION__, thrd_flag[i]);
			return;
		}
		
	}
	
	for (i = 0; i < THREAD_SUM; ++i)
	{
		res = pthread_join(thread[i], thread_ret);
		
		if (0 != res)
		{
			printf("%s:destroy %d thread faild\n", __FUNCTION__, thrd_flag[i]);
			return;
		}
		
		L_DEBUG("%d = destroy ok!\n", i);
	}
	
}

static void* Thrds(void *pArg)
{
	int flag = (int)pArg;
	
	L_DEBUG("thread num = %d\n", flag);
	
	switch (flag)
	{
		case 0:
			RecUartData(g_UartFDS[0]);
			break;
		case 100:
			IdleTask();
			break;
		default:
			break;
	}
	
	pthread_exit(NULL);
}

static void RecUartData(int aisle)
{
	int max_fd = 0;
	int res = 0;
	int real_read = 0;
	fd_set in_set;
	fd_set tmp_set;
	unsigned char buff[BUFFER_SIZE];
	
	max_fd = aisle + 1;
	
	FD_ZERO(&in_set);
	FD_SET(aisle, &in_set);
	
	if (!FD_ISSET(aisle, &in_set))
	{
		printf("%s:set fd error\n", __FUNCTION__);
		return;
	}
	
	L_DEBUG("recieve %d uart data thread!\n", aisle);
	
	while (1)
	{
		tmp_set = in_set;
		
		res = select(max_fd, &tmp_set, NULL, NULL, NULL);
		
		if (0 < res)
		{
			if (FD_ISSET(aisle, &tmp_set))
			{
				{
					printf("----------------------------------data from %d aisle\n",aisle);
					memset(buff, 0, BUFFER_SIZE);
					real_read = read(aisle, buff, BUFFER_SIZE);

					{
					    int j = 0;
					    
					    for (; j < real_read; ++j)
					    {
					        L_DEBUG("0x%.2x,",buff[j]);
					    }
					    L_DEBUG("\n");
					}
   
                    if (0 < real_read)
                    {
                       ProcAisleData(aisle, buff, real_read);
                    }//-- end of if (0 < real_read) --//
				}
			}
		}
		else
		{
			printf("%s:select %d error\n", __FUNCTION__, aisle);
		}
	}
}

static void TimerCallback(int SigNum)
{
	AsyncEvent evt;
	
	if (SIGALRM == SigNum)
	{
		if (!(0x01 & g_EvtOkFlag))
		{
			g_EvtOkFlag |= 0x01;
			
			evt.m_Params.m_Aisle = g_UartFDS[0];
			evt.m_Params.m_DataType = 0;
			evt.m_Params.m_Param = 0;
			evt.m_Action = GetSlaveBaseInfo;
			
			AddAsyncEvent(evt);
		}
	}
	
	alarm(ALARM_TIME);
}

static void	IdleTask()
{
	unsigned char str[200] = "{\"type\":0,\"midAddress\":\"0000000000\",\"address\":\"00004\",\"data\":[25,5]}";
	int tmp = 0;
	
	tmp = strlen(str);
	
	while (1)
	{
#if 0
		ConnectServer(1, g_CParam);
		
		SendDataToServer(str, tmp);
		
		LogoutClient();
#endif		
		sleep(5);
	}
}

