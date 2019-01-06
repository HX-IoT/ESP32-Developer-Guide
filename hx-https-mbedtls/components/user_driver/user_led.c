/** 
* @file         user_led.c 
* @brief        RGB灯的相关函数定义
* @details      定义相关驱动RGB的PWM函数
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
*/

/*
===========================
头文件包含
=========================== 
*/
#include "driver/ledc.h"
#include "user_led.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
/*
===========================
全局变量定义
=========================== 
*/
static ledc_timer_config_t *gs_m_lec_timer_config = NULL;
static ledc_channel_config_t *gs_m_ledc_channel_config = NULL;
static TaskHandle_t gs_m_r_fase_handle,gs_m_g_fase_handle,gs_m_b_fase_handle;
/*
===========================
函数定义
=========================== 
*/

/** 
 * 板载rgb灯初始化
 * @param[in]   timer_config       :填充驱动rgb所需要的定时器相关参数
 * @param[in]   channel_config     :填充rgb对应的gpio以及选择相对应的PWM口驱动等参数
 * @param[in]   rgb_led_counts          :rgb灯的个数,即对应控制的rgb灯需要多少个gpio口
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
 */
esp_err_t rgb_init(ledc_timer_config_t *timer_config, ledc_channel_config_t *channel_config, uint8_t rgb_led_counts)
{
  esp_err_t err_code;
  if ((timer_config == NULL) || (channel_config == NULL) || (rgb_led_counts == 0))
  {
    ESP_LOGI("rgb_init", "ERR_INVALID_ARG is %d\n", ESP_ERR_INVALID_ARG);
    err_code = ESP_ERR_INVALID_ARG;
    return err_code;
  }
  /* 保存用户填充的时间和通道参数 */
  gs_m_lec_timer_config = timer_config;
  gs_m_ledc_channel_config = channel_config;
  /* 设置驱动RGB的PWM相关参数 */
  err_code = ledc_timer_config(timer_config);
  if(err_code != ESP_OK)
  {
    ESP_LOGI("rgb_init", "ledc_timer_config fail,reason is %d\n", err_code);
    return err_code;
  }
  /* 根据rgb的个数配置 */
  for (uint8_t i = 0; i < rgb_led_counts; i++)
  {
    ledc_channel_config_t *s_ledc_channel_config = (ledc_channel_config_t *)(channel_config + i);
    err_code = ledc_channel_config(s_ledc_channel_config);
    if (err_code != ESP_OK)
    {
      ESP_LOGI("rgb_init", "ledc_channel_config fail,reason is %d\n", err_code);
      return err_code;
    }
    err_code = ledc_stop(s_ledc_channel_config->speed_mode,
                         s_ledc_channel_config->channel,
                         1);
    if (err_code != ESP_OK)
    {
      ESP_LOGI("rgb_init", "ledc_stop fail,reason is %d\n", err_code);
      return err_code;
    }
  }
  /* 初始化fade服务 */
  err_code = ledc_fade_func_install(0);
  if(err_code != ESP_OK)
  {
    ESP_LOGI("rgb_init", "ledc_fade_func_install fail,reason is %d\n", err_code);
    return err_code;
  }
  return err_code;
}


/** 
 * rgb灯中的r渐变任务
 * @param[in]   null
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
 */
static void r_fade_task(void *pvParameters)
{  
  for (;;)
  {
    ledc_set_fade_with_time(gs_m_ledc_channel_config[R_IDX].speed_mode,
                            gs_m_ledc_channel_config[R_IDX].channel,
                            0,
                            MAX_FADE_TIME_MS);
    ledc_fade_start(gs_m_ledc_channel_config[R_IDX].speed_mode,
                    gs_m_ledc_channel_config[R_IDX].channel,
                    LEDC_FADE_NO_WAIT);
    vTaskDelay(MAX_FADE_TIME_MS / portTICK_PERIOD_MS);

    ledc_set_fade_with_time(gs_m_ledc_channel_config[R_IDX].speed_mode,
                            gs_m_ledc_channel_config[R_IDX].channel,
                            TARGET_DUTY,
                            MAX_FADE_TIME_MS);
    ledc_fade_start(gs_m_ledc_channel_config[R_IDX].speed_mode,
                    gs_m_ledc_channel_config[R_IDX].channel,
                    LEDC_FADE_NO_WAIT);
    vTaskDelay(MAX_FADE_TIME_MS / portTICK_PERIOD_MS);    
  }
}

/** 
 * rgb灯中的g渐变任务
 * @param[in]   null
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/25, 初始化版本\n 
 */
static void g_fade_task(void *pvParameters)
{  
  for (;;)
  {
    ledc_set_fade_with_time(gs_m_ledc_channel_config[G_IDX].speed_mode,
                            gs_m_ledc_channel_config[G_IDX].channel,
                            0,
                            MAX_FADE_TIME_MS);
    ledc_fade_start(gs_m_ledc_channel_config[G_IDX].speed_mode,
                    gs_m_ledc_channel_config[G_IDX].channel,
                    LEDC_FADE_NO_WAIT);
    vTaskDelay(MAX_FADE_TIME_MS / portTICK_PERIOD_MS);

    ledc_set_fade_with_time(gs_m_ledc_channel_config[G_IDX].speed_mode,
                            gs_m_ledc_channel_config[G_IDX].channel,
                            TARGET_DUTY,
                            MAX_FADE_TIME_MS);
    ledc_fade_start(gs_m_ledc_channel_config[G_IDX].speed_mode,
                    gs_m_ledc_channel_config[G_IDX].channel,
                    LEDC_FADE_NO_WAIT);
    vTaskDelay(MAX_FADE_TIME_MS / portTICK_PERIOD_MS);    
  }
}

/** 
 * rgb灯中的b渐变任务
 * @param[in]   null
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/25, 初始化版本\n 
 */
static void b_fade_task(void *pvParameters)
{  
  for (;;)
  {
    ledc_set_fade_with_time(gs_m_ledc_channel_config[B_IDX].speed_mode,
                            gs_m_ledc_channel_config[B_IDX].channel,
                            0,
                            MAX_FADE_TIME_MS);
    ledc_fade_start(gs_m_ledc_channel_config[B_IDX].speed_mode,
                    gs_m_ledc_channel_config[B_IDX].channel,
                    LEDC_FADE_NO_WAIT);
    vTaskDelay(MAX_FADE_TIME_MS / portTICK_PERIOD_MS);

    ledc_set_fade_with_time(gs_m_ledc_channel_config[B_IDX].speed_mode,
                            gs_m_ledc_channel_config[B_IDX].channel,
                            TARGET_DUTY,
                            MAX_FADE_TIME_MS);
    ledc_fade_start(gs_m_ledc_channel_config[B_IDX].speed_mode,
                    gs_m_ledc_channel_config[B_IDX].channel,
                    LEDC_FADE_NO_WAIT);
    vTaskDelay(MAX_FADE_TIME_MS / portTICK_PERIOD_MS);    
  }
}
/** 
 * 开启rgb灯中的r渐变
 * @param[in]   null
 * @retval      
 *              - pdPASS
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
 */
int r_fade_start(void)
{
  int err_code;
  err_code = xTaskCreate(r_fade_task, "r_fade_task", 256 * 4, NULL, 3, &gs_m_r_fase_handle);
  if(err_code != pdPASS)
  {
    ESP_LOGI("r_fade_start", "r_fade_task create fail,reason is %d\n", err_code);
    return err_code;
  }
  return err_code;
}

/** 
 * 开启rgb灯中的g渐变
 * @param[in]   null
 * @retval      
 *              - pdPASS
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/25, 初始化版本\n 
 */
int g_fade_start(void)
{
  int err_code;
  err_code = xTaskCreate(g_fade_task, "g_fade_task", 256 * 4, NULL, 3, &gs_m_g_fase_handle);
  if(err_code != pdPASS)
  {
    ESP_LOGI("g_fade_start", "g_fade_task create fail,reason is %d\n", err_code);
    return err_code;
  }
  return err_code;
}

/** 
 * 开启rgb灯中的b渐变
 * @param[in]   null
 * @retval      
 *              - pdPASS
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/25, 初始化版本\n 
 */
int b_fade_start(void)
{
  int err_code;
  err_code = xTaskCreate(b_fade_task, "b_fade_task", 256 * 4, NULL, 3, &gs_m_b_fase_handle);
  if(err_code != pdPASS)
  {
    ESP_LOGI("b_fade_task", "b_fade_task create fail,reason is %d\n", err_code);
    return err_code;
  }
  return err_code;
}


/** 
 * 关闭rgb灯中的r渐变,并将r的亮度设为0
 * @param[in]   null
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
 */
void r_fade_stop(void)
{
  esp_err_t err_code;
  vTaskDelete(gs_m_r_fase_handle);
  err_code = ledc_stop(gs_m_ledc_channel_config[R_IDX].speed_mode,
                       gs_m_ledc_channel_config[R_IDX].channel,
                       1);
  if (err_code != ESP_OK)
  {
    ESP_LOGI("r_fade_stop", "ledc_stop fail,reason is %d\n", err_code);    
  }
}

/** 
 * 关闭rgb灯中的g渐变,并将g的亮度设为0
 * @param[in]   null
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/25, 初始化版本\n 
 */
void g_fade_stop(void)
{
  esp_err_t err_code;
  vTaskDelete(gs_m_g_fase_handle);
  err_code = ledc_stop(gs_m_ledc_channel_config[G_IDX].speed_mode,
                       gs_m_ledc_channel_config[G_IDX].channel,
                       1);
  if (err_code != ESP_OK)
  {
    ESP_LOGI("g_fade_stop", "ledc_stop fail,reason is %d\n", err_code);    
  }
}

/** 
 * 关闭rgb灯中的b渐变,并将b的亮度设为0
 * @param[in]   null
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/25, 初始化版本\n 
 */
void b_fade_stop(void)
{
  esp_err_t err_code;
  vTaskDelete(gs_m_b_fase_handle);
  err_code = ledc_stop(gs_m_ledc_channel_config[B_IDX].speed_mode,
                       gs_m_ledc_channel_config[B_IDX].channel,
                       1);
  if (err_code != ESP_OK)
  {
    ESP_LOGI("g_fade_stop", "ledc_stop fail,reason is %d\n", err_code);  
  }
}