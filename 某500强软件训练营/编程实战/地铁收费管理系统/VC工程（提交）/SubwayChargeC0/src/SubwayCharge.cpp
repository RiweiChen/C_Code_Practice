#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "api.h"
#include "SubwayCharge.h"

#include "malloc.h"
#include <stdarg.h>

/*其他全局变量定义考生根据需要补充*/

#define FILE_NAME   "SubwayCharge.txt"
#define MAX_FREE_STAY_TIME  30

#define MAX_CARD_NUM 10

HistoryInfoNode *pHead = NULL;
LogItem_ST arrayLogItem[10];
int log_num = 0 ;
//记录9张卡是否注销
int tag_delete[MAX_CARD_NUM] = {1,1,1,1,1,1,1,1,1,1};




/*****************************************************************************
 函 数 名  : main
 功能描述  : 主入口参数(考生无需更改)
 输入参数  : argc  程序启动时的参数个数
             argv  程序启动时的参数
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void main(int argc, char* argv[])
{
    /*启动Socket服务侦听5555端口(apiServerStart函数在lib库已实现)*/
    apiServerStart(argc, argv);
	//程序退出时，删除malloc 分别的全局指针头。
	RemoveList(pHead);
	//free(pHead);
    return;
}

/*****************************************************************************
 函 数 名  : opResetProc
 功能描述  : 考生需要实现的接口
             完成程序初始化,或程序功能复位操作
             程序启动自动调用该函数,r/R命令时自动调用该函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void opResetProc(void)
{
	//链表空间删除，注意表头的信息空间没有删除。
	if (NULL != pHead)
	{
		RemoveList(pHead);
	}

	pHead = CreateList();

	//
	log_num = 0;
	int i = 1;
	for(i = 1; i < MAX_CARD_NUM; i++)
	{
		tag_delete[i] = 1 ;
	}
	for(i = 1; i < MAX_CARD_NUM ; i++)
	{
		apiDeleteLog(i);
		
	}
}

/*****************************************************************************
 函 数 名  : opChargeProc
 功能描述  : 考生需要实现的接口
             完成请求扣费的功能(详见试题规格说明)
             c/C命令时自动调用该函数
 输入参数  : pstTravelInfo  单次乘车记录信息
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void opChargeProc(TravelInfo_ST* pstTravelInfo)
{
	//需要先判断卡是否存在
	int distance = 0;
	int charge = 0;
	RetCode_EN retcode = RET_OK ;
	ErrCode_EN error_id= E01;
	int ret_if = 0;
	//判断是否出栈比入栈早
	if(((pstTravelInfo->nInHour*60)+(pstTravelInfo->nInMinute)) >
		((pstTravelInfo->nOutHour*60)+(pstTravelInfo->nOutMinute)) )
	{
		error_id = E02;
		goto cleanup;
		
	}
	//查询线路是否存在
	
	ret_if = apiGetDistanceBetweenTwoStation(pstTravelInfo->sInStation, pstTravelInfo->sOutStation, &distance);
	if(RET_ERROR == ret_if)
	{
		apiPrintOpStatusInfo(I10, pstTravelInfo->nCardNo, pstTravelInfo->nCardMoney);
		charge = 0;
		retcode = RET_ERROR;
		goto writeLog;
	}

	if ( 0 ==tag_delete[pstTravelInfo->nCardNo])
	{
		error_id = E22;
		goto cleanup;
	}
	//下面只计算该交多少钱
	//同个站点的情况
	if(0 == strcmp(pstTravelInfo->sInStation,pstTravelInfo->sOutStation))
	{
		//小于逗留阈值 
		int time_diff = apiTimeDiff(pstTravelInfo->nOutHour,pstTravelInfo->nOutMinute,pstTravelInfo->nInHour,pstTravelInfo->nInMinute);
		//printf("%d\n",time_diff);
		if( time_diff <= MAX_FREE_STAY_TIME)
		{
			//单程卡
			if(CARDTYPE_A == pstTravelInfo->enCardType)
			{
				charge = pstTravelInfo->nCardMoney;
			}
			else
			{
				charge = 0 ;
			}
			
		}
		else
		{
			//单程卡
			if(CARDTYPE_A == pstTravelInfo->enCardType)
			{
				charge = (pstTravelInfo->nCardMoney >= 3) ?pstTravelInfo->nCardMoney : 3 ;
			}
			else
			{
				charge = 3;
			}
		}
	}
	//不同站点
	else
	{
		int base_price = 0;
		base_price = get_base_price(pstTravelInfo->sInStation,pstTravelInfo->sOutStation);
		switch (pstTravelInfo->enCardType)
		{
			case CARDTYPE_A:
				charge = (base_price >pstTravelInfo->nCardMoney) ? base_price : pstTravelInfo->nCardMoney;
				break;
			case CARDTYPE_B:
				switch (get_time_tpye(pstTravelInfo->nInHour,pstTravelInfo->nInMinute))
				{
					case 0:
						charge = base_price*9/10;
						break;
					case 1:
						charge = base_price;
						break;
					case 2:
						charge = base_price*5/10;
						
				}
				break;
			case CARDTYPE_C:
				switch (get_time_tpye(pstTravelInfo->nInHour,pstTravelInfo->nInMinute))
				{
					case 0:
						charge = base_price;
						break;
					case 1:
						charge = base_price;
						break;
					case 2:
						charge = base_price*5/10;
						
				}
				
		}
	}
	//printf("%d",charge);
	//余额够了
	if(charge <= (pstTravelInfo->nCardMoney))
	{
		
		if(CARDTYPE_A == pstTravelInfo->enCardType)
		{
			charge = pstTravelInfo->nCardMoney;
			apiPrintOpStatusInfo(I11, pstTravelInfo->nCardNo, 0);
		}
		else
		{
			if(((pstTravelInfo->nCardMoney)-charge) <20)
				apiPrintOpStatusInfo(I12, pstTravelInfo->nCardNo, (pstTravelInfo->nCardMoney)-charge);
			else
				apiPrintOpStatusInfo(I11, pstTravelInfo->nCardNo, (pstTravelInfo->nCardMoney)-charge);
			
		}
		//写历史纪录
		HistoryItem * ptrHisItem = (HistoryItem *)malloc(sizeof(HistoryItem));
		ptrHisItem->nCardNo = pstTravelInfo->nCardNo;
		ptrHisItem->enCardType = pstTravelInfo->enCardType;
		ptrHisItem->nInHour = pstTravelInfo->nInHour;
		ptrHisItem->nInMin = pstTravelInfo->nInMinute;
		strcpy(ptrHisItem->sInStation , pstTravelInfo->sInStation);
		ptrHisItem->nOutHour = pstTravelInfo->nOutHour;
		ptrHisItem->nOutMin = pstTravelInfo->nOutMinute;
		strcpy(ptrHisItem->sOutStation ,pstTravelInfo->sOutStation);
		ptrHisItem->nMoney = charge;

		printf("good to here 1 \n");
		PushBackNode(pHead, ptrHisItem);
		printf("good to here 2 \n");
		
		retcode = RET_OK;
		goto writeLog;
	}
	//余额不够
	else
	{
		apiPrintOpStatusInfo(I13,pstTravelInfo->nCardNo, pstTravelInfo->nCardMoney);
		charge = 0;
		retcode = RET_ERROR;
		goto writeLog;
		
	}
	

cleanup:
	apiPrintErrInfo(error_id);
	return ;
	
//写内存日志
writeLog:
	apiWriteLog(charge, pstTravelInfo, retcode);
	return;
}

/*****************************************************************************
 函 数 名  : opQueryLogProc
 功能描述  : 考生需要实现的接口
             完成查询乘车记录日志的功能(详见试题规格说明)
             q/Q命令时自动调用该函数
 输入参数  : pstQueryCond  日志查询条件
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void opQueryLogProc(QueryCond_ST* pstQueryCond)
{
	//log 是指内存
	ErrCode_EN error_id = E01;
	int logNum = apiGetLogNum();
	int startTime = pstQueryCond->nStartHour*60+pstQueryCond->nStartMinute;
	int endTime = pstQueryCond->nEndHour*60+pstQueryCond->nEndMinute;
	printf("%d\n",logNum);
	
	int delete_count = 0;
	int k = 0;
	for ( k = 1;k<MAX_CARD_NUM;k++)
	{
		if(0 == tag_delete[k])
			delete_count++;
	}
	if(9 == delete_count)
	{
		error_id = E22;
		goto cleanup;
	}
	if ( 0 ==tag_delete[pstQueryCond->nCardNo])
	{
		error_id = E22;
		goto cleanup;
	}
	
	if (0 == logNum)
	{
		error_id = E21 ;
		goto cleanup;
	}
	//时间错误。

	
	
	if(startTime > endTime )
	{
		error_id = E02 ;
		goto cleanup;
	}
	else
	{
		LogItem_ST * headLogAddr = apiGetLogAddr();
		int i = 0;
		int j = 0;
		int count = 0;
		//打印所有的卡号
		int tag_print_all = 0 ;
		if(0 == pstQueryCond->nCardNo )
		{
			tag_print_all = 1;
			int k = 0;
			for( k=1;k<MAX_CARD_NUM;k++)
			{
				for(i = 0;i<logNum;i++)
				{
					int outTime = ((headLogAddr+i)->nOutHour)*60 + (headLogAddr+i)->nOutMin;
					//加入log
					if(outTime>= startTime && outTime<= endTime && (((headLogAddr+i)->nCardNo == k)))
					{
						arrayLogItem[j].nCardNo = (headLogAddr+i)->nCardNo ;
						arrayLogItem[j].nInHour = (headLogAddr+i)->nInHour;
						arrayLogItem[j].nInMin = (headLogAddr+i)->nInMin;
						strcpy(arrayLogItem[j].sInStation , (headLogAddr+i)->sInStation);
						arrayLogItem[j].nOutHour = (headLogAddr+i)->nOutHour;
						arrayLogItem[j].nOutMin = (headLogAddr+i)->nOutMin ;
						strcpy(arrayLogItem[j].sOutStation , (headLogAddr+i)->sOutStation);
						arrayLogItem[j].nMoney = (headLogAddr+i)->nMoney;
						arrayLogItem[j].enOpFlg = (headLogAddr+i)->enOpFlg;
						count++;
						j++;
						
					}
				}
			}
		}
		else
		{
			for(i = 0;i<logNum;i++)
			{
				int outTime = ((headLogAddr+i)->nOutHour)*60 + (headLogAddr+i)->nOutMin;
				//加入log
				if(outTime>= startTime && outTime<= endTime && (((headLogAddr+i)->nCardNo == pstQueryCond->nCardNo)))
				{
					arrayLogItem[j].nCardNo = (headLogAddr+i)->nCardNo ;
					arrayLogItem[j].nInHour = (headLogAddr+i)->nInHour;
					arrayLogItem[j].nInMin = (headLogAddr+i)->nInMin;
					strcpy(arrayLogItem[j].sInStation , (headLogAddr+i)->sInStation);
					arrayLogItem[j].nOutHour = (headLogAddr+i)->nOutHour;
					arrayLogItem[j].nOutMin = (headLogAddr+i)->nOutMin ;
					strcpy(arrayLogItem[j].sOutStation , (headLogAddr+i)->sOutStation);
					arrayLogItem[j].nMoney = (headLogAddr+i)->nMoney;
					arrayLogItem[j].enOpFlg = (headLogAddr+i)->enOpFlg;
					count++;
					j++;
					
				}
			}
		}
		if(0 == count)
		{
			error_id = E21 ;
			goto cleanup;
		}
		else
		{
			apiPrintLog(arrayLogItem, count);
			return ;
		}
	}
	//日志信息可以用一个数组保存，因为它本身不大，才10个最多。
	
cleanup:
	apiPrintErrInfo(error_id);
	return ;
	
}

/*****************************************************************************
 函 数 名  : opQueryHistoryChargeListProc
 功能描述  : 考生需要实现的接口
             完成查询指定卡号的票卡消费历史记录功能(详见试题规格说明)
 输入参数  : iCardNo  待查询的票卡卡号
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void opQueryHistoryChargeListProc(int iCardNo)
{
	//历史，是指消费
	
	if(0 == iCardNo)
	{
		//查询所有
		HistoryInfoNode* ptr_node = pHead->pNext;
		if(NULL == ptr_node)
		{
			apiPrintErrInfo(E21);
			return ;
		}
		else
		{
			while(NULL != ptr_node)
			{
				apiPrintHistoryChargeList(&(ptr_node->data));
				ptr_node=ptr_node->pNext;
			}
		}
	}
	else
	{
		HistoryInfoNode* ptr_node = pHead;
		//单独判断每一个
		int count = 0;
		if(NULL == ptr_node)
		{
			apiPrintErrInfo(E21);
			return ;
		}
		if ( 0 ==tag_delete[iCardNo])
		{
			apiPrintErrInfo(E22);
			return;
		}
		
		while(NULL != ptr_node)
		{
			ptr_node = FindNodeByCardNo(ptr_node,iCardNo);
			if(NULL != ptr_node)
			{
				apiPrintHistoryChargeList(&(ptr_node->data));
				count++; 
			}
			//ptr_node->pNext;
		}
		
		printf("here count : %d\n",count);
		if(count == 0)
		{	
			//已经被删除
			if(0 == tag_delete[iCardNo])
				apiPrintErrInfo(E22);
			else
				apiPrintErrInfo(E21);
		}
		
		
	}
}

/*****************************************************************************
 函 数 名  : opDestroyCardProc
 功能描述  : 考生需要实现的接口
             完成注销指定卡号的票卡消费历史记录功能(详见试题规格说明)
 输入参数  : iCardNo  待注销的票卡卡号
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void opDestroyCardProc(int iCardNo)
{
	//删除所有
	if(0 ==iCardNo)
	{
		int i = 1;
		for ( i = 1; i < MAX_CARD_NUM; i++ )
		{
			int delete_count = 0;
			int k = 0;
			for ( k = 1;k<MAX_CARD_NUM;k++)
			{
				if(0 == tag_delete[k])
					delete_count++;
			}
			if(9 == delete_count)
			{
				apiPrintErrInfo(E22);
				return ;
			}
			//如果之前没有被删除过
			if(1 == tag_delete[i])
			{
				tag_delete[i] = 0 ;
				RemoveNodeByCardNo(pHead, i);
				apiDeleteLog(i);
				
			}
		}
		apiPrintOpStatusInfo(I22, 0, 0);
	}
	
	else 
	{
		//已经被注销过了
		if(0 == tag_delete[iCardNo])
		{
			apiPrintErrInfo(E22);
		}
		else
		{
			//历史消费信息需要删除
			tag_delete[iCardNo] = 0;
			RemoveNodeByCardNo(pHead, iCardNo);
			apiDeleteLog(iCardNo);
			apiPrintOpStatusInfo(I22, iCardNo, 0);
		}
	}
}



/*其他函数定义考生根据功能需要补充*/


/*************************************************
Function      : CreateList
Description   : 创建链表头节点,只在需要的时候添加，第一个的时候� 
				整个链表的头节点不论是否存在数据，都是存在的。
Return        : 链表的头指针
*************************************************/
HistoryInfoNode* CreateList(void)
{
    
    pHead = (HistoryInfoNode *)malloc(sizeof(HistoryInfoNode));
    if (NULL == pHead)
    {
        apiPrintErrInfo(E99);
        return NULL;
    }

    pHead->data.nCardNo = 0;
    pHead->pNext = NULL;

    return pHead;
}

/*************************************************
Function      : FindNodeByCardNo
Description   : 根据卡号，查找某个节点
Input         : pHead 链表的头节点指针
                要查找的卡号
Return        : 正确:返回指定节点的指针
                失败:返回空指针
*************************************************/
HistoryInfoNode* FindNodeByCardNo(HistoryInfoNode *pHead, int iCradNo)
{
    HistoryInfoNode *pNode = NULL;

    if ((NULL == pHead) || (iCradNo < 0))
    {
        //apiPrintErrInfo(E99);
        return NULL;
    }

    pNode = pHead->pNext;
	//pNode = pHead;
    while ((NULL != pNode))
    {
    	printf("card no is : %d\n",pNode->data.nCardNo);
        if (pNode->data.nCardNo == iCradNo)
        {
            break;
        }
        pNode = pNode->pNext;
    }

    return pNode;
}


/*************************************************
Function      : PushBackNode
Description   : 向链表中尾部插入某个节点
Input         : pHead        链表的头节点指针
                pCardInfo    消费记录信息
Output        : 无
Return        : 正确:返回头节点指针
                失败:返回空指针
*************************************************/
HistoryInfoNode* PushBackNode(HistoryInfoNode *pHead, HistoryItem *pCardInfo)
{
    HistoryInfoNode* pNode      = NULL;
    HistoryInfoNode* pNewNode   = NULL;

    if ((NULL == pHead) || (NULL == pCardInfo))
    {
        //apiPrintErrInfo(E99);
        return NULL;
    }

    pNode = pHead;
    while (NULL != pNode->pNext)
    {
        pNode = pNode->pNext;
    }
    pNewNode = (HistoryInfoNode *)malloc(sizeof(HistoryInfoNode));
    if (NULL == pNewNode)
    {
        //apiPrintErrInfo(E99);
        return NULL;
    }

    pNode->pNext     = pNewNode;
    pNewNode->pNext = NULL;
	
    memcpy(&(pNewNode->data), pCardInfo, sizeof(LogItem_ST));

    return pHead;
}

/*************************************************
Function      : RemoveNodeByCardNo
Description   : 从链表中删除指定卡号的记录
Input         : pHead       链表的头节点指针
                iCradNo     待删除的节点的卡号
Return        : 正确:返回链表头节点的指针
                失败:返回空指针
*************************************************/
HistoryInfoNode* RemoveNodeByCardNo(HistoryInfoNode *pHead, int iCradNo)
{
    HistoryInfoNode* pNode      = NULL;
    HistoryInfoNode* pDelNode   = NULL;

    if ((NULL == pHead) || (iCradNo < 0))
    {
        //apiPrintErrInfo(E99);
        return NULL;
    }

    pNode = pHead;
    while (NULL != pNode->pNext)
    {
        if (pNode->pNext->data.nCardNo == iCradNo)
        {
            break;
        }

        pNode = pNode->pNext;
    }

    pDelNode = pNode->pNext;
    if (NULL == pDelNode)
    {
        //apiPrintErrInfo(E99);
        return NULL;
    }

    pNode->pNext = pDelNode->pNext;
    free(pDelNode);

    pDelNode = NULL;

    return pHead;
}


/*************************************************
Function      : RemoveList
Description   : 删除整个链表。
Input         : pHead 链表的头节点指针
Return        : 正确:RET_OK
                失败:RET_ERROR
*************************************************/
int RemoveList(HistoryInfoNode *pHead)
{
    HistoryInfoNode *pNode  = NULL;
    HistoryInfoNode *pb     = NULL;

    if (NULL == pHead)
    {
        apiPrintErrInfo(E99);
        return RET_ERROR;
    }

    pNode = pHead;

    pb = pNode->pNext;
    if (NULL == pb)
    {
        free(pNode);
    }
    else
    {
        while (NULL != pb)
        {
            free(pNode);
            pNode = pb;
            pb = pb->pNext;
        }

        free(pNode);
    }

    pNode = NULL;
    return RET_OK;
}

//写入文件,前提是需要写入的文本数据缓存已经准备好了。
void WriteToFile(char * pstLogBuff)
{
    unsigned int    uiStrLen    = 0;
    FILE            *fp         = NULL;

    //创建之后打开读取
    fp = fopen(FILE_NAME, "w");
    if (NULL == fp)
    {
        apiPrintErrInfo(E99);
        return;
    }

    uiStrLen = fwrite(pstLogBuff, 1, strlen(pstLogBuff), fp);
    if (uiStrLen != strlen(pstLogBuff))
    {
        apiPrintErrInfo(E99);
        fclose(fp);
        return;
    }

    fclose(fp);

    return;
}




//get base price
int get_base_price(char *intStation ,char * outStation )
{
	int distance = 0;
	int base_price = 0 ; 
	apiGetDistanceBetweenTwoStation(intStation, outStation,&distance);
	if (distance <= 3)
		base_price = 2;
	else if(distance <= 5)
		base_price = 3;
	else if(distance <= 10)
		base_price = 4;
	else if (distance > 10)
		base_price = 5;
	else
		base_price = -1;

	return base_price;
		
}

/********************************
//判断进站的时间段
//0，正常时间
//1，忙时
//2，闲时
//-1，错误
*/
int get_time_tpye(int inHour, int InMinute)
{
	int type = 0;
	int totalMinute = inHour*60+InMinute;
	
	if((totalMinute>=(7*60))&&(totalMinute<(9*60)))
		type = 1 ;
	if((totalMinute>=(16*60+30))&&(totalMinute<(18*60+30)))
		type = 1;
	if((totalMinute>=(10*60))&&(totalMinute<(11*60)))
		type = 2;
	if((totalMinute>=(15*60))&&(totalMinute<(16*60)))
		type = 2;

	return type;
	
}