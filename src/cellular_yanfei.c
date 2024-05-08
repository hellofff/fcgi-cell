#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <easylogger/elog.h>
#include <pthread.h>

#include "cellular_serial.h"
#include "cellular_yanfei.h"

#define DEVICE_SERIAL "/dev/ttyUSB2"
#define SELFTEST_INTERVAL_MAX	100
#define PING_INTERVAL_MAX		3
#define MAX_COMMAND_LENGTH 100

CELLUALR_T cellular_info = {0};
APN_T apn_info ={0};

YANFEI_UPLINE_E yanfei_upline_e = YANFEI_UPLINE_AT;//yanfei上线流程

char cmodel[100] = {0};

// 函数声明
int isUsb0Online(char *interface);
float getAverageLatency(char *output);
int yanfei_upline_at(void);
int yanfei_upline_ate0(void);
int yanfei_upline_ati(void);
int yanfei_upline_cgreg(void);
//int yanfei_upline_gsn(void);
int yanfei_upline_cpin(void);
int yanfei_upline_gtrndis(void);
int yanfei_upline_csq(void);
int yanfei_upline_cgreg(void);
int yanfei_upline_cops(void);
int yanfei_upline_apn(void);
void *yanfei_upline_ppp();

int get_apn(char *data);
int yanfei_apn_set(void);




/*
    查询流程
*/
void yanfei_run(void)
{	
	pthread_t net_test;
    pthread_create(&net_test, NULL,yanfei_upline_ppp, NULL);
	
    
	bool flag = true;
	while(flag){
		switch (yanfei_upline_e)
		{
			case YANFEI_UPLINE_AT:
				yanfei_upline_at();
				break;
			case YANFEI_UPLINE_ATE0:
				yanfei_upline_ate0();
				break;
			case YANFEI_UPLINE_ATI:
				yanfei_upline_ati();
				break;  		  
			case YANFEI_UPLINE_CPIN:
				yanfei_upline_cpin();
				break;
			case YANFEI_UPLINE_GTRNDIS:
				yanfei_upline_gtrndis();
				break;
			case YANFEI_UPLINE_CSQ:
				yanfei_upline_csq();
				break;
			case YANFEI_UPLINE_CGREG:
				yanfei_upline_cgreg();
				break;
			case YANFEI_UPLINE_COPS:
				yanfei_upline_cops();
				break;
			case YANFEI_UPLINE_APN:
				yanfei_upline_apn();
				break;	
			case YANFEI_UPLINE_END:
				flag = false;
				break;	

			default:break;
		}

	}yanfei_upline_e = YANFEI_UPLINE_AT;
	
	pthread_join(net_test, NULL);
}   

static void yanfei_uart_init(void)
{
	int ret = Module_SerialPort_Init(DEVICE_SERIAL);
    if(ret){
		log_d("yanfei init %s fail = %d",DEVICE_SERIAL,ret);
	}
}

static void yanfei_uart_close(void)
{
    Module_SerialPort_Close();
}

static void yanfei_uart_send(char *pSend)
{
    Module_SerialPort_SendStr(pSend);
	log_i(pSend);
}

static int yanfei_uart_recive(char *pRecv)
{
	int len = Module_SerialPort_Recive(pRecv, sizeof(pRecv));
	log_i(pRecv);
    return len;
}

/************************************************************************/
/*
	AT
	让AT多发送几次，保证模组AT串口正常
*/
int yanfei_upline_at(void)
{
	char buf[100] = {0};
	static uint8_t at_max_cnt = 0;

	yanfei_uart_init();
	for(int i=0;i<YANFEI_AT_COUNT;i++){
		yanfei_uart_send("AT\r\n");
		usleep(10000);
		int len = yanfei_uart_recive(buf);
		if(len > 0){
			if (strstr(buf, "OK") != 0){
				if(at_max_cnt++ >= 3){
					at_max_cnt = 0;
					yanfei_upline_e = YANFEI_UPLINE_ATE0;
				}else{
					yanfei_upline_e = YANFEI_UPLINE_AT;
				}
				yanfei_uart_close();
				return 0;
			}
		}
		sleep(YANFEI_AT_TIMEOUT);
	}
	yanfei_uart_close();
	return 1;
}

/*
	ATE0
*/
int yanfei_upline_ate0(void)
{
	char buf[100] = {0};
	yanfei_uart_init();
	for(int i=0;i<YANFEI_ATE0_COUNT;i++){
		yanfei_uart_send("ATE1\r\n");
		usleep(10000);
		int len = yanfei_uart_recive(buf);
		if(len > 0){
			if (strstr(buf, "OK") != 0){
				yanfei_upline_e = YANFEI_UPLINE_ATI;
				//log_i("ATE0 status :%d",yanfei_upline_e);
				yanfei_uart_close();
				return 0;
			}
		}
		sleep(YANFEI_ATE0_TIMEOUT);
	}
	yanfei_uart_close();
	 

	return 1;
}

/*
	ATI
*/
int yanfei_upline_ati(void)
{
	char buf[1024] = {0};
	yanfei_uart_init();
	for(int i=0;i<YANFEI_ATI_COUNT;i++){
		yanfei_uart_send("ATI\r\n");
		usleep(10000);
		int len = yanfei_uart_recive(buf);
		log_i("buf:%s",buf);
		if(len > 0){
			if (strstr(buf, "OK") != 0){
				char *line;
				char *lines[5]; // 假设文本最多有5行

				int i = 0;
				line = strtok(buf, "\n");
				while (line != NULL && i < 5) {
					lines[i++] = line;
					line = strtok(NULL, "\n");
				}
				char *thirdLine = lines[2];
				// 去掉末尾的换行符
				size_t length = strcspn(thirdLine, "\n");
				thirdLine[length-1] = '\0';
				strcpy(cellular_info.cmodel,thirdLine);
				log_i("模组型号为：%s",cellular_info.cmodel);
				yanfei_upline_e = YANFEI_UPLINE_CPIN;
				yanfei_uart_close();
				return 0; 
			} else {  
					log_i("String 'Model:' not found.\n");
					yanfei_upline_e = YANFEI_UPLINE_CPIN;
					yanfei_uart_close();
				return 0;  
			} 
		}
		sleep(YANFEI_ATI_TIMEOUT);
	}
	yanfei_uart_close();
	 

	return 1;
}


/*
	CPIN
*/
int yanfei_upline_cpin(void)
{
	char buf[100] = {0};

	yanfei_uart_init();
	for(int i=0;i<YANFEI_CPIN_COUNT;i++){
		yanfei_uart_send("AT+CPIN?\r\n");
		usleep(20000);
		int len = yanfei_uart_recive(buf);
		if(len > 0){
			if (strstr(buf, "READY") != 0){
				strcpy(cellular_info.sim,"READY");
				log_i("SIM卡状态为：%s",cellular_info.sim);
				yanfei_upline_e = YANFEI_UPLINE_GTRNDIS;
				yanfei_uart_close();
				return 0;
			}else{
				strcpy(cellular_info.sim,buf-1);
				log_i("SIM卡状态异常为：%s\r\n",cellular_info.sim);
				yanfei_upline_e = YANFEI_UPLINE_GTRNDIS;
				yanfei_uart_close();
				return 0;
			}
		}
		sleep(YANFEI_CPIN_TIMEOUT);
	}
	yanfei_uart_close();
	return 1;
}

/*
	GTRNDIS
*/
int yanfei_upline_gtrndis(void)
{
	char buf[1000] = {0};

	yanfei_uart_init();
	for(int i=0;i<YANFEI_CPIN_COUNT;i++){
		yanfei_uart_send("AT+CGPADDR\r\n");
		usleep(20000);
		int len = yanfei_uart_recive(buf);
		if(len > 0){
			if (strstr(buf, "OK") != 0){
				const char *gt_start = strchr(buf, '"');
				const char *gt_end = strchr(gt_start + 1, '"');
				size_t gt_Len = gt_end - gt_start-1;
				char buf_gt[gt_Len + 1];
				strncpy(buf_gt,gt_start+1,gt_Len);
				buf_gt[gt_Len] = '\0';
				strcpy(cellular_info.interface_ip,buf_gt);
				log_i("IP地址为：%s\r\n",cellular_info.interface_ip);
				yanfei_upline_e = YANFEI_UPLINE_CSQ;
				yanfei_uart_close();
				return 0;
			}else{
				log_i("IP地址检测异常为：%s\r\n",buf);
				yanfei_upline_e = YANFEI_UPLINE_CSQ;
				yanfei_uart_close();
				return 0;
			}
		}
		sleep(YANFEI_GTRNDIS_TIMEOUT);
	}
	yanfei_uart_close();
	 

	return 1;
}







/*
	CSQ
*/
int yanfei_upline_csq(void)
{
	char buf[100] = {0};

	yanfei_uart_init();
	for(int i=0;i<YANFEI_CSQ_COUNT;i++){
		yanfei_uart_send("AT+CSQ\r\n");
		usleep(20000);
		int len = yanfei_uart_recive(buf);
		
		if(len > 0){
			if (strstr(buf, "OK") != 0){
				// 查找"CSQ:"的位置  
				const char *csqStart = strstr(buf, "CSQ:"); 
				// 如果找到了"CSQ:"  
				if (csqStart != NULL) {  
					csqStart += strlen("CSQ:");
        			const char *csqEnd = strchr(csqStart, ',');  
					// 分配内存存储"csq"的值
					size_t csqLen = csqEnd - csqStart;
					char yanfei_csq[csqLen + 1];
					strncpy(yanfei_csq, csqStart, csqLen);
					yanfei_csq[csqLen] = '\0';
					strcpy(cellular_info.signal,yanfei_csq);
					log_i("当前信号值为：%s",cellular_info.signal);

					yanfei_upline_e = YANFEI_UPLINE_CGREG;
					yanfei_uart_close();
				return 0;
				} else {  
					log_i("String 'CSQ:' not found.\n");
					yanfei_upline_e = YANFEI_UPLINE_CGREG;
					yanfei_uart_close();
				return 0;  
				} 
			}
		}
		sleep(YANFEI_CSQ_TIMEOUT);
	}
	yanfei_uart_close();
	 

	return 1;
}

/*
	CGREG
*/
int yanfei_upline_cgreg(void)
{
	char buf[100] = {0};

	yanfei_uart_init();
	for(int i=0;i<YANFEI_CGREG_COUNT;i++){
		yanfei_uart_send("AT+C5GREG?\r\n");
		usleep(30000);
		int len = yanfei_uart_recive(buf);
		if(len > 0){
			if (strstr(buf, "+C5GREG: 1,1") != 0){
				strcpy(cellular_info.regis,"ON");
				log_i("当前注册状态为：%s",cellular_info.regis);
				yanfei_upline_e = YANFEI_UPLINE_COPS;
				yanfei_uart_close();
				return 0;
			}else{
				strcpy(cellular_info.regis,"OFF");
				log_i("当前注册状态为：%s",cellular_info.regis);
				yanfei_upline_e = YANFEI_UPLINE_COPS;
				yanfei_uart_close();
				return 0;
			}
		}
		sleep(YANFEI_CGREG_TIMEOUT);
	}
	yanfei_uart_close();
	 

	return 1;
}

/*
	COPS
*/
int yanfei_upline_cops(void)
{
	char buf[100] = {0};

	yanfei_uart_init();
	for(int i=0;i<YANFEI_COPS_COUNT;i++){
		yanfei_uart_send("AT+COPS?\r\n");
		usleep(30000);
		int len = yanfei_uart_recive(buf);
		if(len > 0){
			if (strstr(buf, "+COPS: ") != 0){
		
				const char *copStart = strchr(buf,'"'); 
                const char *copEnd = strrchr(buf, '"');  
				const char *actStart = strrchr(buf,',');
				const char *actEnd = strchr(buf,'\0');
				if (copStart != NULL) {    
					// 分配内存存储"cop"的值
					size_t copLen = copEnd - copStart-1;
					char yanfei_cop[copLen + 1];
					strncpy(yanfei_cop, copStart+1, copLen);
					yanfei_cop[copLen] = '\0';
					strcpy(cellular_info.oper,yanfei_cop);
					log_i("当前运营商为：%s",cellular_info.oper);

					//分配内存存储"act"
					size_t actLen = actEnd - actStart - 1;
					char yanfei_act[actLen + 1];
					strncpy(yanfei_act,actStart+1,actLen);
					yanfei_act[actLen] = '\0';
					int act = atoi(yanfei_act);
					switch (act) {
						case 0:
							strcpy(cellular_info.act, "GSM");
							break;
						case 1:
							strcpy(cellular_info.act, "GSM Compact");
							break;
						case 2:
							strcpy(cellular_info.act, "UTRAN");
							break;
						case 3:
							strcpy(cellular_info.act, "GSM w/EGPRS");
							break;
						case 4:
							strcpy(cellular_info.act, "UTRAN w/HSDPA");
							break;
						case 5:
							strcpy(cellular_info.act, "UTRAN w/HSUPA");
							break;
						case 6:
							strcpy(cellular_info.act, "UTRAN w/HSDPA and HSUPA");
							break;
						case 7:
							strcpy(cellular_info.act, "E-UTRAN");
							break;
						case 8:
							strcpy(cellular_info.act, "EC-GSM-IoT");
							break;
						case 9:
							strcpy(cellular_info.act, "E-UTRAN (NB-S1 mode)");
							break;
						case 10:
							strcpy(cellular_info.act, "E-UTRA connected to a 5GCN");
							break;
						case 11:
							strcpy(cellular_info.act, "NR connected to a 5GCN");
							break;
						case 12:
							strcpy(cellular_info.act, "NG-RAN");
							break;
						case 13:
							strcpy(cellular_info.act, "E-UTRA-NR dual connectivity");
							break;
						default:
							printf("Unknown Network Mode\n");
							break; // 可以根据实际情况选择如何处理未知的网络制式
				    }
					log_i("当前网络制式为:%s",cellular_info.act);

					yanfei_upline_e = YANFEI_UPLINE_APN;
					yanfei_uart_close();
				return 0;
				} else {  
					log_i("String 'COPS:' not found.\n");
					yanfei_upline_e = YANFEI_UPLINE_APN;
					yanfei_uart_close();
				return 0;  
				} 
			}
		}
		sleep(YANFEI_COPS_TIMEOUT);
	}
	yanfei_uart_close();
	 

	return 1;
}

/*
	apn
*/
int yanfei_upline_apn(void)
{
	char buf[1024] = {0};

	yanfei_uart_init();
	for(int i=0;i<YANFEI_APN_COUNT;i++){
		yanfei_uart_send("AT+CGDCONT?\r\n");
		usleep(50000);
		int len = yanfei_uart_recive(buf);
		if(len > 0){
			if (strstr(buf, "CGDCONT: ") != 0){
				get_apn(buf);
				//yanfei_upline_e = YANFEI_UPLINE_PPP;
				yanfei_upline_e = YANFEI_UPLINE_END;
				yanfei_uart_close();
				return 0;
				} else {  
					log_i("String 'APN:' not found.\n");
					yanfei_upline_e = YANFEI_UPLINE_END;
					//yanfei_upline_e = YANFEI_UPLINE_PPP;
					yanfei_uart_close();
				return 0;  
				} 
			}
		}
		sleep(YANFEI_COPS_TIMEOUT);
	
	yanfei_uart_close();
	 

	return 1;
}

/*
	提取apn
*/
int  get_apn(char *data)
{
	const char *order_start = strchr(data, ':');  // 寻找冒号的位置
	const char *apn_start = strchr(data, '"');  // 寻找第一个双引号的位置
    if (order_start != NULL) {
        const char *order_end = strchr(order_start, ',');  // 寻找逗号的位置
		size_t apn_order_Len = order_end - order_start-1;
		char buf_apn_order[apn_order_Len + 1];
		strncpy(buf_apn_order,order_start+1,apn_order_Len);
		buf_apn_order[apn_order_Len] = '\0';
		strcpy(cellular_info.order,buf_apn_order);
		log_i("APN的cid为：%s",cellular_info.order);
    }else{
		log_i("apn reutrn error");
		return 0;
	}
	for (int i = 0; i < 3; ++i) {
        if (apn_start == NULL) {
            break;  // 如果找不到足够的双引号，退出循环
        }

        const char *apn_end = strchr(apn_start + 1, '"');  // 寻找下一个双引号的位置
		size_t apn_Len = apn_end - apn_start-1;
		char buf_apn[apn_Len + 1];
		strncpy(buf_apn,apn_start+1,apn_Len);
		buf_apn[apn_Len] = '\0';
	

		int index = i % 3; // 判断当前是第几次循环

       
		switch (index) {
			case 0:
				strcpy(cellular_info.ip_type,buf_apn);
				log_i("APN 的类型是：%s",cellular_info.ip_type);
				break;
			case 1:
				strcpy(cellular_info.apn,buf_apn);
				log_i("APN 是：%s",cellular_info.apn);
				break;
			case 2:
				strcpy(cellular_info.ip_addr,buf_apn);
				log_i("APN 的IP地址是：%s",cellular_info.ip_addr);
				break;
		}
        apn_start = strchr(apn_end + 1, '\"');  // 寻找下一组双引号的位置
    }
    return 0;


}


/***************************************************************/


void *yanfei_upline_ppp(void){
    char interface[] = "usb0";
    int online = isUsb0Online(interface);

    if (online) {
        // 获取平均延迟
        char command[MAX_COMMAND_LENGTH];
        snprintf(command, sizeof(command), "ping -c 1 www.baidu.com  -I usb0| grep rtt");

        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            perror("Error executing ping command");
        }

        char output[256];
		char str_average[256];
        while (fgets(output, sizeof(output), fp) != NULL) {
            float averageLatency = getAverageLatency(output);
			sprintf(str_average, "%f", averageLatency);
			strcpy(cellular_info.latency,str_average);
			log_i("Average Latency: %s ms\n", cellular_info.latency);
			if(cellular_info.latency != NULL)
			{
				strcpy(cellular_info.online,"yes");
            	log_i("%s is %s\n", interface,cellular_info.online);
			}else{
				strcpy(cellular_info.online,"no");
        		log_i("%s is %s\n", interface,cellular_info.online);
			}			           
        }
        pclose(fp);
    } else {
		strcpy(cellular_info.online,"no");
        log_i("%s is %s\n", interface,cellular_info.online);
    }

    //return (yanfei_upline_e = YANFEI_UPLINE_END);
	pthread_exit(NULL);
}

// 判断网卡是否在线
int isUsb0Online(char *interface) {
    char command[MAX_COMMAND_LENGTH];
    snprintf(command, sizeof(command), "ifconfig %s | grep UP", interface);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error executing ifconfig command");
        exit(EXIT_FAILURE);
    }

    char output[256];
    fgets(output, sizeof(output), fp);

    pclose(fp);

    return (strstr(output, "UP") != NULL);
	
}

// 从ping命令的输出中提取平均延迟
float getAverageLatency(char *output) {
    float averageLatency = 0.0;
    char *token = strtok(output, "/");
    
    // 找到第5个标记，即平均延迟
    for (int i = 0; i < 4; i++) {
        token = strtok(NULL, "/");
        if (token == NULL) {
            fprintf(stderr, "Error parsing ping output\n");
            exit(EXIT_FAILURE);
        }

        if (i == 3) {
            averageLatency = atof(token);
        }
    }

    return averageLatency;
}


/*
    apn配置
*/

int yanfei_apn_set(void)
{
	char buf[100] = {0};
	char command[100];
	yanfei_uart_init();
	for(int i=0;i<YANFEI_APN_COUNT;i++){

		//拼接apn配置指令
		// snprintf(command, sizeof(command), "AT+CGDCONT=%s,\"%s\",\"%s\",\"%s\"\r\n",
        //      apn_info.order, apn_info.ip_type, apn_info.apn, apn_info.ip_addr);
		snprintf(command, sizeof(command), "AT+CGDCONT=1,\"IP\",\"%s\"\r\n",apn_info.apn);
		yanfei_uart_send(command);
		int len = yanfei_uart_recive(buf);
		if(len > 0){
			if (strstr(buf, "OK") != 0){
				//yanfei_run();
				yanfei_uart_close();
				return 0; 
			}
		}
		sleep(YANFEI_APN_COUNT);
	}
	yanfei_uart_close();
	return 1;
}
