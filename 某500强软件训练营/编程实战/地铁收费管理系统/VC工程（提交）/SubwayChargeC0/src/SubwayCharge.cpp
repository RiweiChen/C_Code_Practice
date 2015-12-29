#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "api.h"
#include "SubwayCharge.h"

#include "malloc.h"
#include <stdarg.h>

/*ÆäËûÈ«¾Ö±äÁ¿¶¨Òå¿¼Éú¸ù¾İĞèÒª²¹³ä*/

#define FILE_NAME   "SubwayCharge.txt"
#define MAX_FREE_STAY_TIME  30

#define MAX_CARD_NUM 10

HistoryInfoNode *pHead = NULL;
LogItem_ST arrayLogItem[10];
int log_num = 0 ;
//¼ÇÂ¼9ÕÅ¿¨ÊÇ·ñ×¢Ïú
int tag_delete[MAX_CARD_NUM] = {1,1,1,1,1,1,1,1,1,1};




/*****************************************************************************
 º¯ Êı Ãû  : main
 ¹¦ÄÜÃèÊö  : Ö÷Èë¿Ú²ÎÊı(¿¼ÉúÎŞĞè¸ü¸Ä)
 ÊäÈë²ÎÊı  : argc  ³ÌĞòÆô¶¯Ê±µÄ²ÎÊı¸öÊı
             argv  ³ÌĞòÆô¶¯Ê±µÄ²ÎÊı
 Êä³ö²ÎÊı  : ÎŞ
 ·µ »Ø Öµ  : ÎŞ
*****************************************************************************/
void main(int argc, char* argv[])
{
    /*Æô¶¯Socket·şÎñÕìÌı5555¶Ë¿Ú(apiServerStartº¯ÊıÔÚlib¿âÒÑÊµÏÖ)*/
    apiServerStart(argc, argv);
	//³ÌĞòÍË³öÊ±£¬É¾³ımalloc ·Ö±ğµÄÈ«¾ÖÖ¸ÕëÍ·¡£
	RemoveList(pHead);
	//free(pHead);
    return;
}

/*****************************************************************************
 º¯ Êı Ãû  : opResetProc
 ¹¦ÄÜÃèÊö  : ¿¼ÉúĞèÒªÊµÏÖµÄ½Ó¿Ú
             Íê³É³ÌĞò³õÊ¼»¯,»ò³ÌĞò¹¦ÄÜ¸´Î»²Ù×÷
             ³ÌĞòÆô¶¯×Ô¶¯µ÷ÓÃ¸Ãº¯Êı,r/RÃüÁîÊ±×Ô¶¯µ÷ÓÃ¸Ãº¯Êı
 ÊäÈë²ÎÊı  : ÎŞ
 Êä³ö²ÎÊı  : ÎŞ
 ·µ »Ø Öµ  : ÎŞ
*****************************************************************************/
void opResetProc(void)
{
	//Á´±í¿Õ¼äÉ¾³ı£¬×¢Òâ±íÍ·µÄĞÅÏ¢¿Õ¼äÃ»ÓĞÉ¾³ı¡£
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
 º¯ Êı Ãû  : opChargeProc
 ¹¦ÄÜÃèÊö  : ¿¼ÉúĞèÒªÊµÏÖµÄ½Ó¿Ú
             Íê³ÉÇëÇó¿Û·ÑµÄ¹¦ÄÜ(Ïê¼ûÊÔÌâ¹æ¸ñËµÃ÷)
             c/CÃüÁîÊ±×Ô¶¯µ÷ÓÃ¸Ãº¯Êı
 ÊäÈë²ÎÊı  : pstTravelInfo  µ¥´Î³Ë³µ¼ÇÂ¼ĞÅÏ¢
 Êä³ö²ÎÊı  : ÎŞ
 ·µ »Ø Öµ  : ÎŞ
*****************************************************************************/
void opChargeProc(TravelInfo_ST* pstTravelInfo)
{
	//ĞèÒªÏÈÅĞ¶Ï¿¨ÊÇ·ñ´æÔÚ
	int distance = 0;
	int charge = 0;
	RetCode_EN retcode = RET_OK ;
	ErrCode_EN error_id= E01;
	int ret_if = 0;
	//ÅĞ¶ÏÊÇ·ñ³öÕ»±ÈÈëÕ»Ôç
	if(((pstTravelInfo->nInHour*60)+(pstTravelInfo->nInMinute)) >
		((pstTravelInfo->nOutHour*60)+(pstTravelInfo->nOutMinute)) )
	{
		error_id = E02;
		goto cleanup;
		
	}
	//²éÑ¯ÏßÂ·ÊÇ·ñ´æÔÚ
	
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
	//ÏÂÃæÖ»¼ÆËã¸Ã½»¶àÉÙÇ®
	//Í¬¸öÕ¾µãµÄÇé¿ö
	if(0 == strcmp(pstTravelInfo->sInStation,pstTravelInfo->sOutStation))
	{
		//Ğ¡ÓÚ¶ºÁôãĞÖµ 
		int time_diff = apiTimeDiff(pstTravelInfo->nOutHour,pstTravelInfo->nOutMinute,pstTravelInfo->nInHour,pstTravelInfo->nInMinute);
		//printf("%d\n",time_diff);
		if( time_diff <= MAX_FREE_STAY_TIME)
		{
			//µ¥³Ì¿¨
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
			//µ¥³Ì¿¨
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
	//²»Í¬Õ¾µã
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
	//Óà¶î¹»ÁË
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
		//Ğ´ÀúÊ·¼ÍÂ¼
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
	//Óà¶î²»¹»
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
	
//Ğ´ÄÚ´æÈÕÖ¾
writeLog:
	apiWriteLog(charge, pstTravelInfo, retcode);
	return;
}

/*****************************************************************************
 º¯ Êı Ãû  : opQueryLogProc
 ¹¦ÄÜÃèÊö  : ¿¼ÉúĞèÒªÊµÏÖµÄ½Ó¿Ú
             Íê³É²éÑ¯³Ë³µ¼ÇÂ¼ÈÕÖ¾µÄ¹¦ÄÜ(Ïê¼ûÊÔÌâ¹æ¸ñËµÃ÷)
             q/QÃüÁîÊ±×Ô¶¯µ÷ÓÃ¸Ãº¯Êı
 ÊäÈë²ÎÊı  : pstQueryCond  ÈÕÖ¾²éÑ¯Ìõ¼ş
 Êä³ö²ÎÊı  : ÎŞ
 ·µ »Ø Öµ  : ÎŞ
*****************************************************************************/
void opQueryLogProc(QueryCond_ST* pstQueryCond)
{
	//log ÊÇÖ¸ÄÚ´æ
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
	//Ê±¼ä´íÎó¡£

	
	
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
		//´òÓ¡ËùÓĞµÄ¿¨ºÅ
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
					//¼ÓÈëlog
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
				//¼ÓÈëlog
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
	//ÈÕÖ¾ĞÅÏ¢¿ÉÒÔÓÃÒ»¸öÊı×é±£´æ£¬ÒòÎªËü±¾Éí²»´ó£¬²Å10¸ö×î¶à¡£
	
cleanup:
	apiPrintErrInfo(error_id);
	return ;
	
}

/*****************************************************************************
 º¯ Êı Ãû  : opQueryHistoryChargeListProc
 ¹¦ÄÜÃèÊö  : ¿¼ÉúĞèÒªÊµÏÖµÄ½Ó¿Ú
             Íê³É²éÑ¯Ö¸¶¨¿¨ºÅµÄÆ±¿¨Ïû·ÑÀúÊ·¼ÇÂ¼¹¦ÄÜ(Ïê¼ûÊÔÌâ¹æ¸ñËµÃ÷)
 ÊäÈë²ÎÊı  : iCardNo  ´ı²éÑ¯µÄÆ±¿¨¿¨ºÅ
 Êä³ö²ÎÊı  : ÎŞ
 ·µ »Ø Öµ  : ÎŞ
*****************************************************************************/
void opQueryHistoryChargeListProc(int iCardNo)
{
	//ÀúÊ·£¬ÊÇÖ¸Ïû·Ñ
	
	if(0 == iCardNo)
	{
		//²éÑ¯ËùÓĞ
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
		//µ¥¶ÀÅĞ¶ÏÃ¿Ò»¸ö
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
			//ÒÑ¾­±»É¾³ı
			if(0 == tag_delete[iCardNo])
				apiPrintErrInfo(E22);
			else
				apiPrintErrInfo(E21);
		}
		
		
	}
}

/*****************************************************************************
 º¯ Êı Ãû  : opDestroyCardProc
 ¹¦ÄÜÃèÊö  : ¿¼ÉúĞèÒªÊµÏÖµÄ½Ó¿Ú
             Íê³É×¢ÏúÖ¸¶¨¿¨ºÅµÄÆ±¿¨Ïû·ÑÀúÊ·¼ÇÂ¼¹¦ÄÜ(Ïê¼ûÊÔÌâ¹æ¸ñËµÃ÷)
 ÊäÈë²ÎÊı  : iCardNo  ´ı×¢ÏúµÄÆ±¿¨¿¨ºÅ
 Êä³ö²ÎÊı  : ÎŞ
 ·µ »Ø Öµ  : ÎŞ
*****************************************************************************/
void opDestroyCardProc(int iCardNo)
{
	//É¾³ıËùÓĞ
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
			//Èç¹ûÖ®Ç°Ã»ÓĞ±»É¾³ı¹ı
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
		//ÒÑ¾­±»×¢Ïú¹ıÁË
		if(0 == tag_delete[iCardNo])
		{
			apiPrintErrInfo(E22);
		}
		else
		{
			//ÀúÊ·Ïû·ÑĞÅÏ¢ĞèÒªÉ¾³ı
			tag_delete[iCardNo] = 0;
			RemoveNodeByCardNo(pHead, iCardNo);
			apiDeleteLog(iCardNo);
			apiPrintOpStatusInfo(I22, iCardNo, 0);
		}
	}
}



/*ÆäËûº¯Êı¶¨Òå¿¼Éú¸ù¾İ¹¦ÄÜĞèÒª²¹³ä*/


/*************************************************
Function      : CreateList
Description   : ´´½¨Á´±íÍ·½Úµã,Ö»ÔÚĞèÒªµÄÊ±ºòÌí¼Ó£¬µÚÒ»¸öµÄÊ±ºòå 
				Õû¸öÁ´±íµÄÍ·½Úµã²»ÂÛÊÇ·ñ´æÔÚÊı¾İ£¬¶¼ÊÇ´æÔÚµÄ¡£
Return        : Á´±íµÄÍ·Ö¸Õë
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
Description   : ¸ù¾İ¿¨ºÅ£¬²éÕÒÄ³¸ö½Úµã
Input         : pHead Á´±íµÄÍ·½ÚµãÖ¸Õë
                Òª²éÕÒµÄ¿¨ºÅ
Return        : ÕıÈ·:·µ»ØÖ¸¶¨½ÚµãµÄÖ¸Õë
                Ê§°Ü:·µ»Ø¿ÕÖ¸Õë
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
Description   : ÏòÁ´±íÖĞÎ²²¿²åÈëÄ³¸ö½Úµã
Input         : pHead        Á´±íµÄÍ·½ÚµãÖ¸Õë
                pCardInfo    Ïû·Ñ¼ÇÂ¼ĞÅÏ¢
Output        : ÎŞ
Return        : ÕıÈ·:·µ»ØÍ·½ÚµãÖ¸Õë
                Ê§°Ü:·µ»Ø¿ÕÖ¸Õë
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
Description   : ´ÓÁ´±íÖĞÉ¾³ıÖ¸¶¨¿¨ºÅµÄ¼ÇÂ¼
Input         : pHead       Á´±íµÄÍ·½ÚµãÖ¸Õë
                iCradNo     ´ıÉ¾³ıµÄ½ÚµãµÄ¿¨ºÅ
Return        : ÕıÈ·:·µ»ØÁ´±íÍ·½ÚµãµÄÖ¸Õë
                Ê§°Ü:·µ»Ø¿ÕÖ¸Õë
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
Description   : É¾³ıÕû¸öÁ´±í¡£
Input         : pHead Á´±íµÄÍ·½ÚµãÖ¸Õë
Return        : ÕıÈ·:RET_OK
                Ê§°Ü:RET_ERROR
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

//Ğ´ÈëÎÄ¼ş,Ç°ÌáÊÇĞèÒªĞ´ÈëµÄÎÄ±¾Êı¾İ»º´æÒÑ¾­×¼±¸ºÃÁË¡£
void WriteToFile(char * pstLogBuff)
{
    unsigned int    uiStrLen    = 0;
    FILE            *fp         = NULL;

    //´´½¨Ö®ºó´ò¿ª¶ÁÈ¡
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
//ÅĞ¶Ï½øÕ¾µÄÊ±¼ä¶Î
//0£¬Õı³£Ê±¼ä
//1£¬Ã¦Ê±
//2£¬ÏĞÊ±
//-1£¬´íÎó
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