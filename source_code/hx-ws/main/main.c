/*
* @file         main.c 
* @brief        websocket server数据解析
* @details      源码来源，http://www.barth-dev.de/websockets-on-the-esp32
* @author       hx-zsj 
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
* @par History:          
*               Ver0.0.1:
                     hx-zsj, 2018/11/22, 初始化版本\n 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "WebSocket_Task.h"
#include <string.h>

//LED
#define LED    2
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<LED)

#define LED_ON()		gpio_set_level(LED, 0)
#define LED_OFF()		gpio_set_level(LED, 1)					


//WebSocket数据包队列
QueueHandle_t WebSocket_rx_queue;

/*
* websocket server数据解析 
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/11/22, 初始化版本\n 
*/
void task_process_WebSocket( void *pvParameters )
{
    (void)pvParameters;
    //WebSocket数据包
    WebSocket_frame_t __RX_frame;
    //create WebSocket RX 队列
    WebSocket_rx_queue = xQueueCreate(10,sizeof(WebSocket_frame_t));
    while (1)
    {
        //接收到WebSocket数据包
        if(xQueueReceive(WebSocket_rx_queue,&__RX_frame, 3*portTICK_PERIOD_MS)==pdTRUE)
        {
        	//打印下
        	printf("Websocket Data Length %d, Data: %.*s \r\n", __RX_frame.payload_length, __RX_frame.payload_length, __RX_frame.payload);
        	if(memcmp( __RX_frame.payload,"ON",2)==0)
            {
                LED_ON();
            }else if(memcmp( __RX_frame.payload,"OFF",3)==0)
            {
                LED_OFF();
            }else {
                //把接收到的数据回发
                WS_write_data(__RX_frame.payload, __RX_frame.payload_length);
            }
        	//free memory
			if (__RX_frame.payload != NULL)
				free(__RX_frame.payload);
        }
    }
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void app_main(void)
{
    //一如既往的配置
    nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = "stop",//自定义
            .password = "11111111111",//自定义
            .bssid_set = false
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );

    //配置led
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);


    //接收websocket数据任务：数据接收处理
    xTaskCreate(&task_process_WebSocket, "ws_process_rx", 2048, NULL, 5, NULL);

    //websocket server任务：建立server、等待连接、连接、数据接收打包
    xTaskCreate(&ws_server, "ws_server", 2048, NULL, 5, NULL);

}
