#include <string.h>
#include "stdio.h"
#include<stdlib.h>
#include "api.h"
#include "car.h"

/*
车辆的状态信息保存在全局变量中。
需要在系统初始化的时候，进行设置。
*/

//链表定义
typedef struct violation_record_node
{
	QueryResult data;
	struct violation_record_node * next ;
}violation_record_node;

/***************系统状态******************/

// 0 表示没有被初始化
int g_flag_init = 0;
violation_record_node * g_head = NULL;
int g_index = 1;

/***************车辆状态信息******************/
//剩余积分
int g_remain_score = MAX_SCORE_VALUE ;
//记录车辆违规的数目
int g_record_num = 0;
// 记录最近的操作时间
int g_last_op_time = 1;
// 0 表示没有被注销
int g_flag_delete = 0 ;

// 待缴违规记录，只记录指针节点即可。
violation_record_node * wait_for_pay_records[MAX_PECCANCY_RECORD_NUM]; 


//费用和积分信息,便于查找和修改
int g_Fees[5]={100,200,300,400,500};
int g_Scores[5] ={1,2,3,4,5};

/*****************************************************************************
 函 数 名  : delete_record_all
 功能描述  : 删除所有的记录信息，
             
 输入参数  : violation_record_node * head       :    记录链表的头指针
 输出参数  : 无
 返 回 值  : 无
 调用函数  : 无
 被调函数  : onInit

 修改历史      :
  1.日    期   : 2015年12月25日
    作    者   : 陈日伟
    修改内容   : 新生成函数

*****************************************************************************/

void delete_record_all(violation_record_node * head )
{
	violation_record_node * tmp = head;
	while(tmp != NULL)
	{
		violation_record_node * delete_node =tmp;
		tmp = tmp->next;
		free(delete_node);
	}
}


/*****************************************************************************
 函 数 名  : add_record
 功能描述  : 添加历史纪录到链表中
             
 输入参数  : violation_record_node * head       :    记录链表的头指针
 			 carInfo							: 	 违规记录信息
 输出参数  : 无
 返 回 值  : 无
 调用函数  : 无
 被调函数  : onRecord

 修改历史      :
  1.日    期   : 2015年12月25日
    作    者   : 陈日伟
    修改内容   : 新生成函数

*****************************************************************************/
int add_record(violation_record_node * head, CarSysInfo carInfo)
{
	//找到最后一个插入点 
	violation_record_node * tmp1 = head;
	violation_record_node * tmp2 = head;
	while(tmp1 != NULL)
	{
		tmp2 = tmp1;
		tmp1 = tmp1->next;
	}
	tmp1 = (violation_record_node *)malloc(sizeof(violation_record_node));
	tmp1->data.Fee = g_Fees[carInfo.Reason];
	tmp1->data.Index = g_index;
	tmp1->data.PayFlag = STAT_NO_PAY;
	//注意是否超出范围
	tmp1->data.Score = g_remain_score - g_Scores[carInfo.Reason];
	tmp1->data.stCarSysInfo.Reason = carInfo.Reason;
	tmp1->data.stCarSysInfo.Time = carInfo.Time;
	
	tmp1->next = NULL;
	if(tmp2 != NULL)
		tmp2->next = tmp1;
	if(head == NULL)
		g_head = tmp1;

	//更改状态
	wait_for_pay_records[g_record_num] = tmp1 ;
	g_record_num = g_record_num + 1;
	g_remain_score = g_remain_score - g_Scores[carInfo.Reason];
	g_last_op_time = carInfo.Time;
	if(g_remain_score <= 0)
	{
		g_flag_delete = 1;
	}
	return 0;
}


/*****************************************************************************
 函 数 名  : pay_one
 功能描述  : 从待缴费的列表中选择一个进行缴费，
             
 输入参数  : violation_record_node * wait_for_pay_records[]       :    保存待缴费的链表节点数组。
 			 int Money											  :    剩余可提交的钱
 输出参数  : 无
 返 回 值  : 
 			 -1	 : 表示失败，没有找到可以缴纳的记录
 			 > 0 : 表示缴纳成功，数值大小为已经缴纳的金额
 调用函数  : 无
 被调函数  : onPay

 修改历史      :
  1.日    期   : 2015年12月25日
    作    者   : 陈日伟
    修改内容   : 新生成函数

*****************************************************************************/
int pay_one(violation_record_node * wait_for_pay_records[], int Money)
{
	//查找第一个满足付款条件的节点
	int pay_day = 0;
	int pay_index = -1;
	int i = 0;
	for( i = 0; i < g_record_num; i++)
	{
		if(wait_for_pay_records[i]->data.Fee <= Money)
		{
			
			pay_day = wait_for_pay_records[i]->data.stCarSysInfo.Time;
			pay_index = i;
			break;
		}
		
	}
	if(pay_index < 0)
	{
		return -1;
	}
	//查找同一天的最大可缴纳值进行缴纳
	int max_pay = wait_for_pay_records[pay_index]->data.Fee;
	for( i = 0; i < g_record_num; i++)
	{
		if((wait_for_pay_records[i]->data.Fee <= Money) &&(pay_day == wait_for_pay_records[i]->data.stCarSysInfo.Time)
			&&(wait_for_pay_records[i]->data.Fee > max_pay ))
		{
			pay_index = i;
			max_pay = wait_for_pay_records[i]->data.Fee;
		}
		
	}

	wait_for_pay_records[pay_index]->data.PayFlag = STAT_HAVE_PAY ;
	for( i = pay_index+1; i < g_record_num; i++)
	{
		wait_for_pay_records[i-1] = wait_for_pay_records[i];
	}
	g_record_num--;
	return max_pay ;
}

/*****************************************************************************
 函 数 名  : get_sum_wait_for_pay
 功能描述  : 获取待缴纳金额的总数
             
 输入参数  : violation_record_node * wait_for_pay_records[]       :    保存待缴费的链表节点数组。
 输出参数  : result
 返 回 值  : 
 			 0	 : 表示无记录
 			 > 0 : 表示代缴款总额
 调用函数  : 无
 被调函数  : onPay

 修改历史      :
  1.日    期   : 2015年12月25日
    作    者   : 陈日伟
    修改内容   : 新生成函数

*****************************************************************************/

int get_sum_wait_for_pay(violation_record_node * wait_for_pay_records[])
{
	int result = 0;
	int i = 0;
	for( i = 0; i < g_record_num; i++)
	{
		result =result + wait_for_pay_records[i]->data.Fee;
	}
	return result ;
}

void main(int argc, char* argv[])
{
    /* 启动Socket服务侦听5555端口(sapi_server_start函数在lib库已实现)。
     * lib库已实现从Socket接收到字符串后的命令分发处理；
     * 考生只需要实现分发后的各命令即可。
    */
    api_server_start(argc, argv);
	// 此处不会执行到，注意不要在此处添加代码
}

/*****************************************************************************
 函 数 名  : opInit
 功能描述  : 考生需要实现的接口
             完成“系统初始化”操作
             命令实例：i
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2010年1月21日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void opInit(void)
{
	//系统状态初始化
	g_flag_init = 1;
	
	//删除已有的数据
	delete_record_all(g_head);
	g_head = NULL;
	g_index = 1;

	g_remain_score = MAX_SCORE_VALUE ;
	g_record_num = 0;
	g_last_op_time = 1;
	g_flag_delete = 0 ;
	api_print_result(S001);
	return ;
	
}

/*****************************************************************************
 函 数 名  : opRecord
 功能描述  : 考生需要实现的接口
             完成“录入违规记录”操作
             命令实例：r 0-1
 输入参数  : int Peccancy  :    违规类型
             int Days      :    时间
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2010年1月21日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void opRecord(int PeccancyType, int Days)
{
	//查找有没有初始化过
	if(0 == g_flag_init)
	{
		api_print_result(E001);
		return ;
	}
	//是否被吊销
	if(g_flag_delete == 1)
	{
		api_print_result(E004);
		return ;
	}
	//记录已满
	if(g_record_num == MAX_PECCANCY_RECORD_NUM)
	{
		api_print_result(E009);
		return ;
	}
	//时间不在范围之内
	if(Days < 1 || Days > MAX_DAYS_VALUE)
	{
		api_print_result(E002);
		return ;
	}
	//时间比上一次的还早
	if(Days < g_last_op_time)
	{
		api_print_result(E008);
		return ;
	}
	//类型错误
	if(PeccancyType<0 || PeccancyType >4)
	{
		api_print_result(E003);
		return ;
	}
	
	CarSysInfo carInfo;
	carInfo.Reason = PeccancyType;
	carInfo.Time = Days;

	if(0 == add_record(g_head, carInfo))
	{
		g_index = g_index + 1;
		api_print_result(S002);
		return ;
	}

}

/*****************************************************************************
 函 数 名  : opQuery
 功能描述  : 考生需要实现的接口
             完成查询违规记录操作
             命令实例：q
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2010年1月21日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void opQuery(void)
{
	//查找有没有初始化过
	if(0 == g_flag_init)
	{
		api_print_result(E001);
		return ;
	}
	//是否被吊销
	if(1 == g_flag_delete)
	{
		api_print_result(E004);
		return ;
	}
	//没有记录
	if(g_head == NULL)
	{
		api_print_result(E010);
		return ;
	}
	else
	{
		violation_record_node * tmp1 = g_head;
		while(tmp1!=NULL)
		{
			api_print_query_info(&(tmp1->data));
			tmp1=tmp1->next;
		}
		return ;
	}
}

/*****************************************************************************
 函 数 名  : opPay
 功能描述  : 考生需要实现的接口
             完成缴纳违规罚款操作
             命令实例：p 100
 输入参数  : int  Money       :    缴纳金额
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2010年1月21日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void opPay(int Money)
{
	//查找有没有初始化过
	if(0 == g_flag_init)
	{
		api_print_result(E001);
		return ;
	}
	//是否被吊销
	if(1 == g_flag_delete)
	{
		api_print_result(E004);
		return ;
	}
	//是否有待缴纳的记录
	if(0 == g_record_num)
	{
		api_print_result(E007);
		return ;
	}
	//判断金额是否在范围之内
	if(Money < 1 || Money > MAX_MONEY_VALUE)
	{
		api_print_result(E005);
		return ;
	}
	// 记录当前缴纳成功的笔数
	int pay_num = -1;
	int pay_money = 0;
	int money_remain = Money;
	do
	{
		money_remain = money_remain - pay_money;
		pay_money =pay_one(wait_for_pay_records, money_remain);
		pay_num++;
	}while(pay_money != -1);
	
	if(pay_num < 1)
	{
		api_print_result(E006);
		return ;
	}
	else
	{
		PayResult result ;
		result.Fee = get_sum_wait_for_pay(wait_for_pay_records);
		result.PeccancyNo = g_record_num;
		result.Score = g_remain_score;
		result.ReturnMoney = money_remain ;
		api_print_pay_info(&result);
		return ;
	}
	
}
