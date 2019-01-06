/*
* @file         hx_sc.c 
* @brief        ESP32的smartconfig操作
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       红旭团队 
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
* @par History:          
*               Ver0.0.1:
                     hx-zsj, 2018/08/10, 初始化版本\n 
*/

/* 
=============
头文件包含
=============
*/
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "cJSON.h"


/*
===========================
宏定义
=========================== 
*/
#define false		0
#define true		1
#define errno		(*__errno())

//http组包宏，获取天气的http接口参数
#define WEB_SERVER          "api.thinkpage.cn"              
#define WEB_PORT            "80"
#define WEB_URL             "/v3/weather/now.json?key="
#define host 		        "api.thinkpage.cn"
#define APIKEY		        "g3egns3yk2ahzb0p"       
#define city		        "suzhou"
#define language	        "en"
/*
===========================
全局变量定义
=========================== 
*/
//http请求包
static const char *REQUEST = "GET "WEB_URL""APIKEY"&location="city"&language="language" HTTP/1.1\r\n"
    "Host: "WEB_SERVER"\r\n"
    "Connection: close\r\n"
    "\r\n";

//wifi链接成功事件
static EventGroupHandle_t wifi_event_group;
//天气解析结构体
typedef struct 
{
    char cit[20];
    char weather_text[20];
    char weather_code[2];
    char temperatur[3];
}weather_info;

weather_info weathe;

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "sc";
static const char *TAG1 = "u_event";
static const char *HTTP_TAG = "http_task";


void smartconfig_example_task(void *parm);
void http_get_task(void *pvParameters);

/*
* wifi事件
* @param[in]   event  		       :事件
* @retval      esp_err_t           :错误类型
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/10, 初始化版本\n 
*/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG1, "SYSTEM_EVENT_STA_START");
        //创建smartconfig任务
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG1, "SYSTEM_EVENT_STA_GOT_IP");
        //sta链接成功，set事件组
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG1, "SYSTEM_EVENT_STA_DISCONNECTED");
        //断线重连
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

/*smartconfig事件回调
* @param[in]   status  		       :事件状态
* @retval      void                 :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/10, 初始化版本\n 
*/
static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status)
    {
    case SC_STATUS_WAIT:                    //等待配网
        ESP_LOGI(TAG, "SC_STATUS_WAIT");
        break;
    case SC_STATUS_FIND_CHANNEL:            //扫描信道
        ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
        break;
    case SC_STATUS_GETTING_SSID_PSWD:       //获取到ssid和密码
        ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
        break;
    case SC_STATUS_LINK:                    //连接获取的ssid和密码
        ESP_LOGI(TAG, "SC_STATUS_LINK");
        wifi_config_t *wifi_config = pdata;
        //打印账号密码
        ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
        //断开默认的
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        //设置获取的ap和密码到寄存器
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
        //连接获取的ssid和密码
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SC_STATUS_LINK_OVER: //连接上配置后，结束
        ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
        //
        if (pdata != NULL)
        {
            uint8_t phone_ip[4] = {0};
            memcpy(phone_ip, (uint8_t *)pdata, 4);
            ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
        }
        //发送sc结束事件
        xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
        break;
    default:
        break;
    }
}
/*smartconfig任务
* @param[in]   void  		       :wu
* @retval      void                 :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/10, 初始化版本\n 
*/
void smartconfig_example_task(void *parm)
{
    EventBits_t uxBits;
    //使用ESP-TOUCH配置
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    //开始sc
    ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
    while (1)
    {
        //死等事件组：CONNECTED_BIT | ESPTOUCH_DONE_BIT
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        
        //sc结束
        if (uxBits & ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            
            xTaskCreate(http_get_task, "http_get_task", 4096, NULL, 3, NULL);
            vTaskDelete(NULL);
            
        }
        //连上ap
        if (uxBits & CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
    }
}

/*解析json数据 只处理 解析 城市 天气 天气代码  温度  其他的自行扩展
* @param[in]   text  		       :json字符串
* @retval      void                 :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/10, 初始化版本\n 
*/
void cjson_to_struct_info(char *text)
{
    cJSON *root,*psub;
    cJSON *arrayItem;
    //截取有效json
    char *index=strchr(text,'{');
    strcpy(text,index);

    root = cJSON_Parse(text);
    
    if(root!=NULL)
    {
        psub = cJSON_GetObjectItem(root, "results");
        arrayItem = cJSON_GetArrayItem(psub,0);

        cJSON *locat = cJSON_GetObjectItem(arrayItem, "location");
        cJSON *now = cJSON_GetObjectItem(arrayItem, "now");
        if((locat!=NULL)&&(now!=NULL))
        {
            psub=cJSON_GetObjectItem(locat,"name");
            sprintf(weathe.cit,"%s",psub->valuestring);
            ESP_LOGI(HTTP_TAG,"city:%s",weathe.cit);

            psub=cJSON_GetObjectItem(now,"text");
            sprintf(weathe.weather_text,"%s",psub->valuestring);
            ESP_LOGI(HTTP_TAG,"weather:%s",weathe.weather_text);
            
            psub=cJSON_GetObjectItem(now,"code");
            sprintf(weathe.weather_code,"%s",psub->valuestring);
            //ESP_LOGI(HTTP_TAG,"%s",weathe.weather_code);

            psub=cJSON_GetObjectItem(now,"temperature");
            sprintf(weathe.temperatur,"%s",psub->valuestring);
            ESP_LOGI(HTTP_TAG,"temperatur:%s",weathe.temperatur);

            //ESP_LOGI(HTTP_TAG,"--->city %s,weather %s,temperature %s<---\r\n",weathe.cit,weathe.weather_text,weathe.temperatur);
        }
    }
    //ESP_LOGI(HTTP_TAG,"%s 222",__func__);
    cJSON_Delete(root);
}

/*http任务
* @param[in]   pvParameters  		:无
* @retval      void                 :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/10, 初始化版本\n 
*/
 void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[1024];
    char mid_buf[1024];
    int index;
    while(1) {
        
        //DNS域名解析
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
        if(err != 0 || res == NULL) {
            ESP_LOGE(HTTP_TAG, "DNS lookup failed err=%d res=%p\r\n", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        //打印获取的IP
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        //ESP_LOGI(HTTP_TAG, "DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

        //新建socket
        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(HTTP_TAG, "... Failed to allocate socket.\r\n");
            close(s);
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        //连接ip
        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(HTTP_TAG, "... socket connect failed errno=%d\r\n", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        freeaddrinfo(res);
        //发送http包
        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(HTTP_TAG, "... socket send failed\r\n");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        //清缓存
        memset(mid_buf,0,sizeof(mid_buf));
        //获取http应答包
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            strcat(mid_buf,recv_buf);
        } while(r > 0);
        //json解析
        cjson_to_struct_info(mid_buf);
        //关闭socket，http是短连接
        close(s);

        //延时一会
        for(int countdown = 10; countdown >= 0; countdown--) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    //事件组
    wifi_event_group = xEventGroupCreate();
    //注册wifi事件
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    //wifi设置:默认设置，等待sc配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //sta模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //启动wifi
    ESP_ERROR_CHECK(esp_wifi_start());
}
