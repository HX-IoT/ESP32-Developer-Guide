/*
* @file         tcp_bsp.c 
* @brief        wifi连接事件处理和socket收发数据处理
* @details      在官方源码的基础是适当修改调整，并增加注释
* @author       hx-zsj 
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
* @par History:          
*               Ver0.0.1:
                     hx-zsj, 2018/08/08, 初始化版本\n 
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
#include "tcp_bsp.h"

/*
===========================
全局变量定义
=========================== 
*/
EventGroupHandle_t tcp_event_group;                     //wifi建立成功信号量
//socket
static int server_socket = 0;                           //服务器socket
static struct sockaddr_in server_addr;                  //server地址
static struct sockaddr_in client_addr;                  //client地址
static unsigned int socklen = sizeof(client_addr);      //地址长度
static int connect_socket = 0;                          //连接socket
bool g_rxtx_need_restart = false;                       //异常后，重新连接标记

// int g_total_data = 0;



// #if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO

// int g_total_pack = 0;
// int g_send_success = 0;
// int g_send_fail = 0;
// int g_delay_classify[5] = {0};

// #endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/


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
        xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:    //STA模式-连接成功
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:       //STA模式-获取IP
        ESP_LOGI(TAG, "got ip:%s\n",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:  //AP模式-有STA连接成功
        //作为ap，有sta连接
        ESP_LOGI(TAG, "station:" MACSTR " join,AID=%d\n",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED://AP模式-有STA断线
        ESP_LOGI(TAG, "station:" MACSTR "leave,AID=%d\n",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        //重新建立server
        g_rxtx_need_restart = true;
        xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
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
        len = recv(connect_socket, databuff, sizeof(databuff), 0);
        g_rxtx_need_restart = false;
        if (len > 0)
        {
            //g_total_data += len;
            //打印接收到的数组
            ESP_LOGI(TAG, "recvData: %s", databuff);
            //接收数据回发
            send(connect_socket, databuff, strlen(databuff), 0);
            //sendto(connect_socket, databuff , sizeof(databuff), 0, (struct sockaddr *) &remote_addr,sizeof(remote_addr));
        }
        else
        {
            //打印错误信息
            show_socket_error_reason("recv_data", connect_socket);
            //服务器故障，标记重连
            g_rxtx_need_restart = true;
            
#if TCP_SERVER_CLIENT_OPTION
            //服务器接收异常，不用break后close socket,因为有其他client
            //break;
            vTaskDelete(NULL);
#else
            //client
            break;
#endif
        }
    }
    close_socket();
    //标记重连
    g_rxtx_need_restart = true;
    vTaskDelete(NULL);
}

/*
* 建立tcp server
* @param[in]   isCreatServer  	    :首次true，下次false
* @retval      void                 :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
esp_err_t create_tcp_server(bool isCreatServer)
{
    //首次建立server
    if (isCreatServer)
    {
        ESP_LOGI(TAG, "server socket....,port=%d", TCP_PORT);
        //新建socket
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            show_socket_error_reason("create_server", server_socket);
            //新建失败后，关闭新建的socket，等待下次新建
            close(server_socket);
            return ESP_FAIL;
        }
        //配置新建server socket参数
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TCP_PORT);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //bind:地址的绑定
        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            show_socket_error_reason("bind_server", server_socket);
            //bind失败后，关闭新建的socket，等待下次新建
            close(server_socket);
            return ESP_FAIL;
        }
    }
    //listen，下次时，直接监听
    if (listen(server_socket, 5) < 0)
    {
        show_socket_error_reason("listen_server", server_socket);
        //listen失败后，关闭新建的socket，等待下次新建
        close(server_socket);
        return ESP_FAIL;
    }
    //accept，搜寻全连接队列
    connect_socket = accept(server_socket, (struct sockaddr *)&client_addr, &socklen);
    if (connect_socket < 0)
    {
        show_socket_error_reason("accept_server", connect_socket);
        //accept失败后，关闭新建的socket，等待下次新建
        close(server_socket);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "tcp connection established!");
    return ESP_OK;
}


/*
* 建立tcp client
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*               Ver0.0.12:
                    hx-zsj, 2018/08/09, 增加close socket\n 
*/
esp_err_t create_tcp_client()
{

    ESP_LOGI(TAG, "will connect gateway ssid : %s port:%d",
             TCP_SERVER_ADRESS, TCP_PORT);
    //新建socket
    connect_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect_socket < 0)
    {
        //打印报错信息
        show_socket_error_reason("create client", connect_socket);
        //新建失败后，关闭新建的socket，等待下次新建
        close(connect_socket);
        return ESP_FAIL;
    }
    //配置连接服务器信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_ADRESS);
    ESP_LOGI(TAG, "connectting server...");
    //连接服务器
    if (connect(connect_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        //打印报错信息
        show_socket_error_reason("client connect", connect_socket);
        ESP_LOGE(TAG, "connect failed!");
        //连接失败后，关闭之前新建的socket，等待下次新建
        close(connect_socket);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "connect success!");
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
    tcp_event_group = xEventGroupCreate();
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
* WIFI作为AP的初始化
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void wifi_init_softap()
{
    tcp_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = SOFT_AP_SSID,
            .password = SOFT_AP_PAS,
            .ssid_len = 0,
            .max_connection = SOFT_AP_MAX_CONNECT,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };
    if (strlen(SOFT_AP_PAS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP set finish,SSID:%s password:%s \n",
             wifi_config.ap.ssid, wifi_config.ap.password);
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
        ESP_LOGE(TAG, "socket error code:%s", strerror(err));
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
#if EXAMPLE_ESP_TCP_MODE_SERVER
    ESP_LOGD(TAG, "check server_socket");
    ret = get_socket_error_code(server_socket);
    if (ret != 0)
    {
        ESP_LOGW(TAG, "server socket error %d %s", ret, strerror(ret));
    }
    if (ret == ECONNRESET)
    {
        return ret;
    }
#endif
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
    return 0;
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
    close(server_socket);
}
