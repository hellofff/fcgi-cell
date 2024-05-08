#ifndef __CELLULAR_YANFEI_H
#define __CELLULAR_YANFEI_H

/*AT发送超时定义*/
#define YANFEI_AT_COUNT      2
#define YANFEI_AT_TIMEOUT    1
#define YANFEI_ATE0_COUNT      1
#define YANFEI_ATE0_TIMEOUT    1
#define YANFEI_ATI_COUNT      3
#define YANFEI_ATI_TIMEOUT    1

#define YANFEI_GSN_COUNT      3
#define YANFEI_GSN_TIMEOUT    1

#define YANFEI_CPIN_COUNT      3
#define YANFEI_CPIN_TIMEOUT    1
#define YANFEI_GTRNDIS_COUNT      3
#define YANFEI_GTRNDIS_TIMEOUT    1
#define YANFEI_CSQ_COUNT        3
#define YANFEI_CSQ_TIMEOUT      1
#define YANFEI_CGREG_COUNT       5
#define YANFEI_CGREG_TIMEOUT     3
#define YANFEI_COPS_COUNT         1
#define YANFEI_COPS_TIMEOUT       1
#define YANFEI_PPP_COUNT       5
#define YANFEI_PPP_TIMEOUT     3
#define YANFEI_APN_COUNT       3
#define YANFEI_APN_TIMEOUT     1
#define YANFEI_PING_COUNT       3
#define YANFEI_PING_TIMEOUT     5

/*
    yanfei查询流程
*/
typedef enum
{
    YANFEI_UPLINE_IDLE = 0,

    YANFEI_UPLINE_AT,              //测试串口是否通
    YANFEI_UPLINE_ATE0,            //关闭回显
    YANFEI_UPLINE_ATI,             //模组参数
 //   YANFEI_UPLINE_GSN,             //IMEI
    YANFEI_UPLINE_CPIN,            //SIM卡
    YANFEI_UPLINE_GTRNDIS,           //IP
    YANFEI_UPLINE_CSQ,           //信号质量
    YANFEI_UPLINE_CGREG,           //查询入网状态
    YANFEI_UPLINE_COPS,           //查询运营商
    YANFEI_UPLINE_APN,            //查看apn
    //YANFEI_UPLINE_PPP,           //网络测试
    YANFEI_UPLINE_END,              //end
}YANFEI_UPLINE_E;    
extern YANFEI_UPLINE_E yanfei_upline_e;



/*
    模组数据
*/
typedef struct
{
    char cmodel[20];
    char sim[20];
    char interface_ip[20];
    char regis[20];
    char order[20];
    char ip_type[20];
    char apn[20];
    char ip_addr[64];
    char oper[20];
    char act[32];
    char signal[20];
    char online[20];
    char latency[20];
}CELLUALR_T;

extern CELLUALR_T cellular_info;


typedef struct
{
    char order[20];
    char ip_type[20];
    char apn[20];
    char ip_addr[20];
}APN_T;

extern APN_T apn_info;


void yanfei_run(void);
int yanfei_apn_set(void);
#endif
