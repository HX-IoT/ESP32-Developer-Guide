/*
* @file         udp_bsp.c 
* @brief        wifi连接事件处理和socket收发数据处理
* @details      在官方源码的基础是适当修改调整，并增加注释
* @author       hx-zsj 
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
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "udp_bsp.h"

/*
===========================
全局变量定义
=========================== 
*/
EventGroupHandle_t udp_event_group;                     //wifi建立成功信号量

struct sockaddr_in client_addr;                  //client地址
static unsigned int socklen = sizeof(client_addr);      //地址长度
int connect_socket=0;                          //连接socket
/*
===========================
函数声明
=========================== 
*/

/*
* wifi 事件
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:        //STA模式-开始连接
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED: //STA模式-断线
        esp_wifi_connect();
        xEventGroupClearBits(udp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:    //STA模式-连接成功
        xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:       //STA模式-获取IP
        ESP_LOGI(TAG, "got ip:%s\n",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}



/*
* 接收数据任务
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void recv_data(void *pvParameters)
{
    int len = 0;            //长度
    char databuff[1024];    //缓存
    while (1)
    {
        //清空缓存
        memset(databuff, 0x00, sizeof(databuff));

        //读取接收数据
		len = recvfrom(connect_socket, databuff, sizeof(databuff), 0,
				(struct sockaddr *) &client_addr, &socklen);
        if (len > 0)
        {
            //打印接收到的数组
            ESP_LOGI(TAG, "UDP Client recvData: %s", databuff);
            //接收数据回发
            sendto(connect_socket, databuff, strlen(databuff), 0,
			            (struct sockaddr *) &client_addr, sizeof(client_addr));
        }
        else
        {
            //打印错误信息
            show_socket_error_reason("UDP Client recv_data", connect_socket);
            break;
        }
    }
    close_socket();

    vTaskDelete(NULL);
}
/*
* 建立udp client
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*               Ver0.0.12:
                    hx-zsj, 2018/08/09, 增加close socket\n 
*/
esp_err_t create_udp_client()
{

    ESP_LOGI(TAG, "will connect gateway ssid : %s port:%d",
             UDP_ADRESS, UDP_PORT);
    //新建socket
    connect_socket = socket(AF_INET, SOCK_DGRAM, 0);                         /*参数和TCP不同*/
    if (connect_socket < 0)
    {
        //打印报错信息
        show_socket_error_reason("create client", connect_socket);
        //新建失败后，关闭新建的socket，等待下次新建
        close(connect_socket);
        return ESP_FAIL;
    }
    //配置连接服务器信息
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(UDP_PORT);
    client_addr.sin_addr.s_addr = inet_addr(UDP_ADRESS);
    

    int len = 0;            //长度
    char databuff[1024] = "Hello Server,Please ack!!";    //缓存
    //测试udp server,返回发送成功的长度
	len = sendto(connect_socket, databuff, 1024, 0, (struct sockaddr *) &client_addr,
			sizeof(client_addr));
	if (len > 0) {
		ESP_LOGI(TAG, "Transfer data to %s:%u,ssucceed\n",
				inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	} else {
        show_socket_error_reason("recv_data", connect_socket);
		close(connect_socket);
		return ESP_FAIL;
	}
    return ESP_OK;
}


/*
* WIFI作为STA的初始化
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void wifi_init_sta()
{
    udp_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = GATEWAY_SSID,           //STA账号
            .password = GATEWAY_PAS},       //STA密码
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s \n",
             GATEWAY_SSID, GATEWAY_PAS);
}

/*
* 获取socket错误代码
* @param[in]   socket  		       :socket编号
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
int get_socket_error_code(int socket)
{
    int result;
    u32_t optlen = sizeof(int);
    int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen);
    if (err == -1)
    {
        //WSAGetLastError();
        ESP_LOGE(TAG, "socket error code:%d", err);
        // ESP_LOGE(TAG, "socket error code:%s", strerror(err));
        return -1;
    }
    return result;
}

/*
* 获取socket错误原因
* @param[in]   socket  		       :socket编号
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
int show_socket_error_reason(const char *str, int socket)
{
    int err = get_socket_error_code(socket);

    if (err != 0)
    {
        ESP_LOGW(TAG, "%s socket error reason %d %s", str, err, strerror(err));
    }

    return err;
}
/*
* 检查socket
* @param[in]   socket  		       :socket编号
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
int check_working_socket()
{
    int ret;

    ESP_LOGD(TAG, "check connect_socket");
    ret = get_socket_error_code(connect_socket);
    if (ret != 0)
    {
        ESP_LOGW(TAG, "connect socket error %d %s", ret, strerror(ret));
    }
    if (ret != 0)
    {
        return ret;
    }
    return ret;
}
/*
* 关闭socket
* @param[in]   socket  		       :socket编号
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void close_socket()
{
    close(connect_socket);
}

