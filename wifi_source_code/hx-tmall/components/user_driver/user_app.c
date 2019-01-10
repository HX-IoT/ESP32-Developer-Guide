/** 
* @file         user_app.c
* @brief        用户实现自己功能相关函数定义
* @details      定义用户所需的结构体变量以及局部全局变量定义
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/19, 初始化版本\n 
*/

/*
===========================
头文件包含
=========================== 
*/
#include "user_led.h"
#include "user_app.h"
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "user_tmall_genie.h"
/*
===========================
全局变量
=========================== 
*/

/* 填充需要驱动rgb通道参数 */
static ledc_channel_config_t gs_m_ledc_channel_config[] = {
    {.channel = R_CHANNEL,
     .duty = TARGET_DUTY,
     .gpio_num = R_PIN_NUM,
     .speed_mode = LEDC_HIGH_SPEED_MODE,
     .timer_sel = LEDC_TIMER_0},
    {.channel = G_CHANNEL,
     .duty = TARGET_DUTY,
     .gpio_num = G_PIN_NUM,
     .speed_mode = LEDC_HIGH_SPEED_MODE,
     .timer_sel = LEDC_TIMER_0},
    {.channel = B_CHANNEL,
     .duty = TARGET_DUTY,
     .gpio_num = B_PIN_NUM,
     .speed_mode = LEDC_HIGH_SPEED_MODE,
     .timer_sel = LEDC_TIMER_0},
};
/* 填充驱动RGB相关的时间参数 */
static ledc_timer_config_t gs_p_m_ledc_timer_config = {
    .duty_resolution = LEDC_TIMER_15_BIT,
    .freq_hz = 1000,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .timer_num = LEDC_TIMER_0};


/** 
 * 用户的rgb驱动初始化函数
 * @param[in]   null 
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
 */
static void user_app_rgb_init(void)
{
  esp_err_t err_code;
  err_code = rgb_init(&gs_p_m_ledc_timer_config, gs_m_ledc_channel_config, 3);
  ESP_LOGI("user_app_rgb_init", "rgb_init is %d\n", err_code);

  ledc_set_duty(LEDC_HIGH_SPEED_MODE,gs_m_ledc_channel_config[0].channel,0);
  ledc_set_duty(LEDC_HIGH_SPEED_MODE,gs_m_ledc_channel_config[1].channel,0);
  ledc_set_duty(LEDC_HIGH_SPEED_MODE,gs_m_ledc_channel_config[2].channel,0);
  // esp_err_t = r_fade_start();
  // ESP_LOGI("user_app_rgb_init", "r_fade_start is %d\n", r_fade_start());
}

/** 
 * tcp连接任务处理函数
 * @param[in]   pvParameters: 表示任务所携带的参数 
 * @retval      null 
 *              其他:          失败 
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/08/12, 初始化版本\n 
 */
static void tcp_connect_task(void *pvParameters)
{  
  big_iot_cloud_connect(BIG_IOT_URL,BIG_IOT_PORT);  
  vTaskDelete(NULL);
}
/** 
 * wifi事件处理函数
 * @param[in]   ctx     :表示传入的事件类型对应所携带的参数
 * @param[in]   event   :表示传入的事件类型
 * @retval      
 *              ESP_OK  : succeed
 *              其他    :失败 
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/06/04, 初始化版本\n 
 */
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  switch (event->event_id)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI("event_handler","\nSYSTEM_EVENT_STA_GOT_IP\n");
    int err_code = xTaskCreate(tcp_connect_task,
                               "tcp_connect_task",
                               1024 * 8,
                               NULL,
                               3,
                               NULL);
    if(err_code != pdPASS)                            
    {
      ESP_LOGI("event_handler", "tcp_connect_task create failure,reason is %d\n", err_code);
    }
    break;
  case SYSTEM_EVENT_STA_CONNECTED:
    ESP_LOGI("event_handler","\nSYSTEM_EVENT_STA_CONNECTED\n");
    break;
  default:
    break;
  }
  return ESP_OK;
}

/** 
 * 用户的wifi初始化
 * @param[in]   NULL
 * @retval      NULL              
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/06/04, 初始化版本\n 
 */
static void user_wifi_init(void)
{
  esp_err_t err_code;
  err_code = nvs_flash_init();  
  ESP_LOGI("user_wifi_init","\nnvs_flash_init is %d\n",err_code);

  tcpip_adapter_init();
  err_code = esp_event_loop_init(event_handler, NULL);  
  ESP_LOGI("user_wifi_init","\nesp_event_loop_init is %d\n",err_code);
  
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  err_code = esp_wifi_init(&cfg);  
  ESP_LOGI("event_handler","\nesp_wifi_init is %d\n",err_code);

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = "你们的wifi账号",
          .password = "你们的wifi密码",
      },
  };
  err_code = esp_wifi_set_mode(WIFI_MODE_STA);
  ESP_LOGI("user_wifi_init","\nesp_wifi_set_mode is %d\n",err_code);

  err_code = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  ESP_LOGI("user_wifi_init","\nesp_wifi_set_config is %d\n",err_code);  

  err_code = esp_wifi_start();
  ESP_LOGI("user_wifi_init","\nesp_wifi_start is %d\n",err_code);  
  
  err_code = esp_wifi_connect();  
  ESP_LOGI("user_wifi_init","\n esp_wifi_connect is %d\n",err_code);
}

/** 
 * 上电初始化任务,初始化按键,rgb等外设驱动
 * @param[in]   null 
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/26, 初始化版本\n 
 */
void startup_initialization_task(void *pvParameters)
{
  user_app_rgb_init();
  user_wifi_init();
  /* 初始化完成,删除这个任务 */
  vTaskDelete(NULL);
}
