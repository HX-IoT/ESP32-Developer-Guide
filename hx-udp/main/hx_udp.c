/*
* @file         hx_udp.c 
* @brief        ESP32搭建udp server和client
* @details      基于tcp教程基础上修改udp
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
#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "udp_bsp.h"
	
/*
===========================
函数定义
=========================== 
*/

/*
* 任务：建立UDP连接并从UDP接收数据
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/10, 初始化版本\n 
*/
static void udp_connect(void *pvParameters)
{
    //等待WIFI连接成功事件，死等
    xEventGroupWaitBits(udp_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "start udp connected");

    //延时3S准备建立clien
    vTaskDelay(3000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "create udp Client");
    //建立client
    int socket_ret = create_udp_client();
    if (socket_ret == ESP_FAIL)
    {
        //建立失败
        ESP_LOGI(TAG, "create udp socket error,stop...");
        vTaskDelete(NULL);
    }
    else
    {
        //建立成功
        ESP_LOGI(TAG, "create udp socket succeed...");            
        //建立tcp接收数据任务
        if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, NULL))
        {
            //建立失败
            ESP_LOGI(TAG, "Recv task create fail!");
            vTaskDelete(NULL);
        }
        else
        {
            //建立成功
            ESP_LOGI(TAG, "Recv task create succeed!");
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

#if UDP_SERVER_CLIENT_OPTION
    //server，建立ap
    wifi_init_softap();
#else
    //client，建立sta
    wifi_init_sta();
#endif
    //新建一个udp连接任务
    xTaskCreate(&udp_connect, "udp_connect", 4096, NULL, 5, NULL);
}
