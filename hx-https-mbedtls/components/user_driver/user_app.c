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
#include "user_key.h"
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
#include "user_http_s.h"
/*
===========================
全局变量
=========================== 
*/
/* 填充需要配置的按键个数以及对应的相关参数 */
static key_config_t gs_m_key_config[BOARD_BUTTON_COUNT] =
    {
        {BOARD_BUTTON, APP_KEY_ACTIVE_LOW, 0, LONG_PRESSED_TIMER},
};
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
 * https的get请求任务，用于处理并解析北上广深的天气预报的json数据
 * @param[in]   pvParameters     :表示传进任务的值，此任务传进来的是城市的选择
 * @retval      null            
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/06/29, 初始化版本\n 
 */
static void https_request_by_get_task(void *pvParameters)
{
  switch (*((uint8_t *)pvParameters))
  {
  case 1:
    https_request_by_GET(HTTPS_URL_BJ);
    break;
  case 2:
    https_request_by_GET(HTTPS_URL_SH);
    break;
  case 3:
    https_request_by_GET(HTTPS_URL_GZ);
    break;
  case 4:
    https_request_by_GET(HTTPS_URL_SZ);
    break;
  default:
    break;
  }
  vTaskDelete(NULL);
}

/** 
 * 用户的短按处理函数
 * @param[in]   key_num                 :短按按键对应GPIO口
 * @param[in]   short_pressed_counts    :短按按键对应GPIO口按下的次数,这里用不上
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void short_pressed_cb(uint8_t key_num, uint8_t *short_pressed_counts)
{
  static uint8_t s_sigle_click_num = 0;
  static uint8_t s_city_select = 0;
  switch (key_num)
  {
  case BOARD_BUTTON:
    switch (*short_pressed_counts)
    {
    case 1:
      ESP_LOGI("short_pressed_cb", "first press!!!\n");
      switch (s_sigle_click_num)
      {
      case 0:
        r_fade_start();       
        s_sigle_click_num++;        
        break;
      case 1:
        r_fade_stop();
        g_fade_start();
        s_sigle_click_num++;
        break;
      case 2:
        g_fade_stop();
        b_fade_start();
        s_sigle_click_num++;
        break;
      case 3:
        b_fade_stop();
        r_fade_start();
        s_sigle_click_num = 1;
        break;
      }
      s_city_select++;
      if(s_city_select ==5)
      {
        s_city_select = 1;
      }
      /* 创建https的get请求的任务，用于处理接收到的天气预报json数据 */
      int err_code = xTaskCreate(https_request_by_get_task,
                                 "https_request_by_get_task",
                                 1024 * 8,
                                 &s_city_select,
                                 3,
                                 NULL);
      if (err_code != pdPASS)
      {
        ESP_LOGI("event_handler", "https_request_by_get_task create failure,reason is %d\n", err_code);
      }
      break;
    case 2:
      ESP_LOGI("short_pressed_cb", "double press!!!\n");
      break;
    case 3:
      ESP_LOGI("short_pressed_cb", "trible press!!!\n");
      break;
    case 4:
      ESP_LOGI("short_pressed_cb", "quatary press!!!\n");
      break;
      // case ....:
      // break;
    }
    *short_pressed_counts = 0;
    break;

  default:
    break;
  }
}

/** 
 * 用户的长按处理函数
 * @param[in]   key_num                 :短按按键对应GPIO口
 * @param[in]   long_pressed_counts     :按键对应GPIO口按下的次数,这里用不上
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void long_pressed_cb(uint8_t key_num, uint8_t *long_pressed_counts)
{
  switch (key_num)
  {
  case BOARD_BUTTON:
    ESP_LOGI("long_pressed_cb", "long press!!!\n");
    break;
  default:
    break;
  }
}

/** 
 * 用户的按键初始化函数
 * @param[in]   null 
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void user_app_key_init(void)
{
  int32_t err_code;
  err_code = user_key_init(gs_m_key_config, BOARD_BUTTON_COUNT, DECOUNE_TIMER, long_pressed_cb, short_pressed_cb);
  ESP_LOGI("user_app_key_init", "user_key_init is %d\n", err_code);
}

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
  /* 默认上电不亮灯 */
  // ESP_LOGI("user_app_rgb_init", "r_fade_start is %d\n", r_fade_start());
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
    /* 1.连接上AP并获取得到IP地址
       2.初始化按键防止还没有获取得到IP地址
       3.此时还可以按下按键获取北上广深的天气预报 */
    user_app_key_init();
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
  ESP_ERROR_CHECK(nvs_flash_init());
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  wifi_config_t wifi_config = {
      .sta = {
          .ssid = "stop",
          .password = "11111111111",
      },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());
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
  // user_app_key_init();
  user_app_rgb_init();
  user_wifi_init();
  /* 初始化完成,删除这个任务 */
  vTaskDelete(NULL);
}
