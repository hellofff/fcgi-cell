/*
    cellular信息查询
*/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <easylogger/elog.h>
#include <fcgi2/fcgi_stdio.h>
#include <cjson/cJSON.h>
#include <sqlite3.h>
#include "cellular_yanfei.h"
#define TSN_DB  "/opt/fnaiot/tsn-manage/tsn.db"

extern APN_T apn_info;
static void get_data_handler(FCGX_Stream *in,FCGX_Stream *out,FCGX_ParamArray envp);
static void post_data_handler(FCGX_Stream *in,FCGX_Stream *out,FCGX_ParamArray envp);


/*
    cellular信息查询程序
*/
void fcgi_cell(void)
{
    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;

    while(FCGX_Accept(&in, &out, &err, &envp) >= 0)
    {
        char *pmoethod = FCGX_GetParam("REQUEST_METHOD", envp);
        if(pmoethod){
            if(strcmp(pmoethod, "POST") == 0){
                post_data_handler(in,out,envp);
                log_i("pmoethod is :%s",pmoethod);
            // }else if(strcmp(pmoethod, "GET") == 0){
            //     get_data_handler(in,out,envp);
            }else{
                log_i("cannot handle moethod:%s",pmoethod);
            }
        }
    }
    // 关闭FastCGI库
    //FCGX_Finish();

}
/************************************************************************/
/*
    从json字符串中解析method
*/
static bool jsonparse_method(char *pjson,char *method)
{
    bool res = true;
    cJSON *jsonPara = NULL;
    char *pStr = NULL;
    cJSON *jsonRoot = cJSON_Parse(pjson);
    if(jsonRoot != NULL){
        jsonPara = cJSON_GetObjectItem(jsonRoot, "method");
        if(jsonPara != NULL){
            pStr = jsonPara->valuestring;
            if(pStr != NULL){
                if(0 != strcmp(pStr,method)){
                    res = false;
                    goto PARSE_EXIT;
                }
            }else{
                res = false;
                goto PARSE_EXIT;
            }
        }else{
            res = false;
            goto PARSE_EXIT;
        }
    }else{
        res = false;
    }

PARSE_EXIT:

    cJSON_Delete(jsonRoot);
    return res;
}

/************************************************************************/
/*
    从json字符串中解析apn
*/
static bool jsonparse_apn(char *pjson,char *method)
{
    bool res = true;
    cJSON *jsonPara = NULL;
    char *pStr = NULL;
    cJSON *jsonRoot = cJSON_Parse(pjson);
    if(jsonRoot != NULL){
        jsonPara = cJSON_GetObjectItem(jsonRoot, "method");
        if(jsonPara != NULL){
            pStr = jsonPara->valuestring;
            if(pStr != NULL){
                if(0 != strcmp(pStr,method)){
                    res = false;
                    goto PARSE_EXIT;
                }
            }else{
                res = false;
                goto PARSE_EXIT;
            }
        }else{
            res = false;
            goto PARSE_EXIT;
        }
        
        jsonPara = cJSON_GetObjectItem(jsonRoot, "apn");
        if(jsonPara != NULL){
            pStr = jsonPara->valuestring;
            if(pStr != NULL){
                memcpy(apn_info.apn,pStr,strlen(pStr));
            }else{
                res = false;
                goto PARSE_EXIT;
            }
        }else{
            res = false;
            goto PARSE_EXIT;
        }
       
    }else{
        res = false;
    }

PARSE_EXIT:

    cJSON_Delete(jsonRoot);
    return res;
}




/*
    应答apn下发,处理POST
*/
static void post_data_handler(FCGX_Stream *in,FCGX_Stream *out,FCGX_ParamArray envp)
{   
    
    int len = 0;
    bool apn_results = true;
    int result;

    char *pLenstr = FCGX_GetParam("CONTENT_LENGTH",envp);
    if(pLenstr != NULL){
        len = strtol(pLenstr, NULL, 10);
    }else{
        apn_results = false;
    }
    if(len > 0){
        char *data = malloc(len);
        int ch = FCGX_GetStr(data,len,in);
        if(ch >= 0){
            log_i("recv post %d:\r\n%s",len,data);
            if(true == jsonparse_apn(data,"apn")){//提取apn配置
                result = yanfei_apn_set();
                char *getReply = malloc(1000);
                sprintf(getReply, "{\"method\":\"apn\",\"result\":\"%d\"}",result);
                FCGX_FPrintF(out, "Content-type: text/html\r\n\r\n");
                FCGX_FPrintF(out, getReply);
                free(getReply);
            }else if(true == jsonparse_method(data,"cellular")){
                yanfei_run();
                char *getReply = malloc(1000);
                if(getReply != NULL){
                    
                    sprintf(getReply, 
                  // "{\"method\":\"cellular\", \"params\":{\"cmodel\":\"%s\", \"interface_name\":\"usb0\", \"sim\":\"%s\", \"regis\":\"%s\", \"interface_ip\":\"%s\", \"apn\":\"%s\",  \"apn_order\":\"%s\", \"apn_ip_type\":\"%s\", \"apn_ip_addr\":\"%s\",\"oper\":\"%s\", \"act\":\"%s\", \"signal\":\"%sDBm\", \"online\":\"%s\", \"latency\":\"%sms\"}}",
                    //cellular_info.cmodel, cellular_info.sim, cellular_info.regis, cellular_info.interface_ip, cellular_info.apn, cellular_info.order, cellular_info.ip_type, cellular_info.ip_addr, cellular_info.oper,cellular_info.act,cellular_info.signal,cellular_info.online,cellular_info.latency);
                    "{\"method\":\"cellular\", \"params\":{\"cmodel\":\"%s\", \"interface_name\":\"usb0\", \"sim\":\"%s\", \"regis\":\"%s\", \"interface_ip\":\"%s\", \"apn\":\"%s\", \"oper\":\"%s\", \"act\":\"%s\", \"signal\":\"%s DBm\", \"online\":\"%s\", \"latency\":\"%sms\"}}",
                    cellular_info.cmodel, cellular_info.sim, cellular_info.regis, cellular_info.interface_ip, cellular_info.apn,cellular_info.oper,cellular_info.act,cellular_info.signal,cellular_info.online,cellular_info.latency);
                    FCGX_FPrintF(out, "Content-type: text/html\r\n\r\n");
                    FCGX_FPrintF(out, getReply); 
                }
                free(getReply);
            }else {
                apn_results = false;
                FCGX_FPrintF(out, "Content-type: text/html\r\n\r\n");
                FCGX_FPrintF(out, "{\"error\":\"Invalid Method \"}");
            }
        }else{
            apn_results = false;
            FCGX_FPrintF(out, "Content-type: text/html\r\n\r\n");
            FCGX_FPrintF(out,  "{\"error\":\"method NULL \"}");
        }
        free(data);
    }else{
        apn_results = false;
        FCGX_FPrintF(out, "Content-type: text/html\r\n\r\n");
        FCGX_FPrintF(out, "{\"error\":\"Invalid Data\"}");
    }
    // char *postReply = malloc(100);
    // if(postReply != NULL){
    //     sprintf(postReply,"{\"method\":\"password\",\"results\":%d}",apn_results);
    //     FCGX_FPrintF(out, "Content-type: text/html\r\n\r\n");
    //     FCGX_FPrintF(out, postReply);
    // }
    // free(postReply);
}