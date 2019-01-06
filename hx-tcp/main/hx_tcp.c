/*
* @file         hx_tcp.c 
* @brief        ESP32搭建tcp server和client
* @details      在官方源码的基础是适当修改调整，并增加注释
* @author       hx-zsj 
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
* @par History:          
*               Ver0.0.1:
                    hx-zsj, 2018/08/08, 初始化版本\n 
*               Ver0.0.2:
                    hx-zsj, 2018/08/09, 增加close socket，防止连接不上服务器多次后，就再也连不上\n 
*/

/* 
=============
头文件包含
=============
*/
#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "tcp_bsp.h"
	
/*
===========================
函数定义
=========================== 
*/

/*
* 任务：建立TCP连接并从TCP接收数据
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
static void tcp_connect(void *pvParameters)
{
    while (1)
    {
        g_rxtx_need_restart = false;
        //等待WIFI连接信号量，死等
        xEventGroupWaitBits(tcp_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "start tcp connected");

        TaskHandle_t tx_rx_task = NULL;
#if TCP_SERVER_CLIENT_OPTION
        //延时3S准备建立server
        vTaskDelay(3000 / portTICK_RATE_MS);
        ESP_LOGI(TAG, "create tcp server");
        //建立server
        int socket_ret = create_tcp_server(true);
#else
        //延时3S准备建立clien
        vTaskDelay(3000 / portTICK_RATE_MS);
        ESP_LOGI(TAG, "create tcp Client");
        //建立client
        int socket_ret = create_tcp_client();
#endif
        if (socket_ret == ESP_FAIL)
        {
            //建立失败
            ESP_LOGI(TAG, "create tcp socket error,stop...");
            continue;
        }
        else
        {
            //建立成功
            ESP_LOGI(TAG, "create tcp socket succeed...");            
            //建立tcp接收数据任务
            if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task))
            {
                //建立失败
                ESP_LOGI(TAG, "Recv task create fail!");
            }
            else
            {
                //建立成功
                ESP_LOGI(TAG, "Recv task create succeed!");
            }

        }


        while (1)
        {

            vTaskDelay(3000 / portTICK_RATE_MS);

#if TCP_SERVER_CLIENT_OPTION
            //重新建立server，流程和上面一样
            if (g_rxtx_need_restart)
            {
                ESP_LOGI(TAG, "tcp server error,some client leave,restart...");
                //建立server
                if (ESP_FAIL != create_tcp_server(false))
                {
                    if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task))
                    {
                        ESP_LOGE(TAG, "tcp server Recv task create fail!");
                    }
                    else
                    {
                        ESP_LOGI(TAG, "tcp server Recv task create succeed!");
                        //重新建立完成，清除标记
                        g_rxtx_need_restart = false;
                    }
                }
            }
#else
            //重新建立client，流程和上面一样
            if (g_rxtx_need_restart)
            {
                vTaskDelay(3000 / portTICK_RATE_MS);
                ESP_LOGI(TAG, "reStart create tcp client...");
                //建立client
                int socket_ret = create_tcp_client();

                if (socket_ret == ESP_FAIL)
                {
                    ESP_LOGE(TAG, "reStart create tcp socket error,stop...");
                    continue;
                }
                else
                {
                    ESP_LOGI(TAG, "reStart create tcp socket succeed...");
                    //重新建立完成，清除标记
                    g_rxtx_need_restart = false;
                    //建立tcp接收数据任务
                    if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task))
                    {
                        ESP_LOGE(TAG, "reStart Recv task create fail!");
                    }
                    else
                    {
                        ESP_LOGI(TAG, "reStart Recv task create succeed!");
                    }
                }
                
                
            }
#endif
        }
    }

    vTaskDelete(NULL);
}


/*
* 主函数
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/08, 初始化版本\n 
*/
void app_main(void)
{
    //初始化flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

#if TCP_SERVER_CLIENT_OPTION
    //server，建立ap
    wifi_init_softap();
#else
    //client，建立sta
    wifi_init_sta();
#endif
    //新建一个tcp连接任务
    xTaskCreate(&tcp_connect, "tcp_connect", 4096, NULL, 5, NULL);
}
