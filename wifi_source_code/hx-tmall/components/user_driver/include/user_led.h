/** 
* @file         user_led.h 
* @brief        驱动RGB灯的相关函数声明
* @details      声明驱动RGB所需要的一些宏定义和函数
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
*/
#ifndef USER_LED_H_
#define USER_LED_H_

/*
===========================
头文件包含
=========================== 
*/
#include "driver/ledc.h"
/*
===========================
宏定义
=========================== 
*/
#define R_IDX                         0
#define G_IDX                         1
#define B_IDX                         2
#define TARGET_DUTY                   32767                 ///<2^duty_resolution-1,此处duty_resolution为15bit
#define MAX_FADE_TIME_MS              2000                  ///<整个周期为5秒
/*
===========================
函数声明
=========================== 
*/
/** 
 * 板载rgb灯初始化
 * @param[in]   ledc_timer_config       :填充驱动rgb所需要的定时器相关参数
 * @param[in]   ledc_channel_config     :填充rgb对应的gpio以及选择相对应的PWM口驱动等参数
 * @param[in]   rgb_led_counts          :rgb灯的个数,即对应控制的rgb灯需要多少个gpio口
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
 */
esp_err_t rgb_init(ledc_timer_config_t *timer_config, ledc_channel_config_t *channel_config, uint8_t rgb_led_counts);

/** 
 * 开启rgb灯中的r渐变
 * @param[in]   ledc_timer_config       :填充驱动rgb所需要的定时器相关参数
 * @param[in]   ledc_channel_config     :填充rgb对应的gpio以及选择相对应的PWM口驱动等参数
 * @param[in]   rgb_led_counts          :rgb灯的个数,即对应控制的rgb灯需要多少个gpio口
 * @retval      
 *              - ESP_OK
 *              - 其他错误                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
 */
int r_fade_start(void);

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
void r_fade_stop(void);


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
int g_fade_start(void);

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
void g_fade_stop(void);

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
int b_fade_start(void);

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
void b_fade_stop(void);

#endif/* USER_LED_H_ */