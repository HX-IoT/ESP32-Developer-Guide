/** 
* @file         user_app_main.c 
* @brief        用户应用程序入口
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/04, 初始化版本\n 
*/

/* =============
头文件包含
 =============*/
#include <stdio.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_event.h"

/** 
 * wifi事件处理函数
 * @param[in]   ctx     :表示传入的事件类型对应所携带的参数
 * @param[in]   event   :表示传入的事件类型
 * @retval      ESP_OK  : succeed
 *              其他    :失败 
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/04, 初始化版本\n 
 */
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_AP_START:
        printf("\nwifi_softap_start\n");
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        printf("\nwifi_softap_connectted\n");
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        printf("\nwifi_softap_disconnectted\n");
        break;
    default:
        break;
    }
    return ESP_OK;
}

/** 
 * 应用程序的函数入口
 * @param[in]   NULL
 * @retval      NULL              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/04, 初始化版本\n 
 */
void app_main()
{    
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Helon_test",
            .ssid_len = 0,
            /* 最多只能被4个station同时连接,这里设置为只能被一个station连接 */
            .max_connection = 1,
            .password = "20180604",
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());    
}
