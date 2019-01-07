/** 
* @file         user_key.h 
* @brief        按键相关的处理函数声明
* @details      定义按键相关的宏以及结构体,同时对应按键相关函数地声明
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
*/
#ifndef UER_KEY_H_
#define UER_KEY_H_

/*
===========================
头文件包含
=========================== 
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"  

/*
===========================
宏定义
=========================== 
*/
#define BOARD_BUTTON_COUNT                  1
#define BOARD_BUTTON                        GPIO_NUM_34
#define BOARD_BUTTON_NUM                    GPIO_ID_PIN(GPIO_NUM_34)
#define DECOUNE_TIMER                       10*1000                                 ///< 10mS
#define SHORT_PRESS_DELAY_CHECK             150*1000                                ///< 150ms
#define LONG_PRESSED_TIMER                  5000*1000                               ///< 5000mS
#define MULTI_PRESSED_TIMER                 250                                     ///< 250mS,,表示前一个按键释放与后一个按键按下的时间小于等于此值说明是多击
#define APP_KEY_PUSH                        0                                       ///< 表示按键按下
#define APP_KEY_RELEASE                     1                                       ///< 表示按键释放
#define APP_KEY_ACTIVE_HIGH                 1                                       ///< 按键按下是高电平有效
#define APP_KEY_ACTIVE_LOW                  0                                       ///< 按键按下是低电平有效
/*
===========================
枚举变量声明
=========================== 
*/

/* 全局枚举声明 */
enum
{
  BUTTON_PRESSED_AT_STARTUP = 0x00,
  BUTTON_PRESSED_SHORT = 0x01,
  BUTTON_PRESSED_LONG = 0x02,
  BUTTON_IDLE = 0x03,
};

/* 定义了一个别名为user_key_function_callback以及user_key_handler_t的函数指针 */
typedef void (*user_key_function_callback_t)(uint8_t pin_no, uint8_t *key_short_press_counts);
typedef void (*user_key_decounce_handler_t)(uint8_t pin_no, uint8_t key_action);

/* 定义一个配置按键参数的结构体 */
typedef struct key_config
{
  uint8_t key_number;                                                     ///< 按键对应的GPIO口
  uint8_t active_state;                                                   ///< 指定按键按下是高电平有效,还是低电平有效
  uint8_t short_pressed_counts;                                           ///< 保存按键短按的次数,用于多击的判断
  uint32_t long_pressed_time;                                             ///< 按键长按时间,单位us
} key_config_t;

/* 定义一个按键相关参数的结构体 */
typedef struct key_param
{
  uint32_t decounce_time;                                                 ///< 按键消抖的时间,单位us
  user_key_function_callback_t long_press_callback;                       ///< 长按的回调函数
  user_key_function_callback_t short_press_callback;                      ///< 短按的回调函数,包括单击/双击/多击等处理
  // struct key_param *next;                                              ///< 保留用于未来,使用链表创建多个按键的单击,双击以及多击ETS_GPIO_INTR_DISABLE
} key_param_t;

/* 定义一个按键计时相关参数的结构体 */
typedef struct key_time_params
{
  esp_timer_handle_t short_press_time_handle;                             ///< 用于计时按键按下并释放之后多少ms内,没有再次按下按键则说明整个按键动作完成
  esp_timer_handle_t key_decounce_time_handle;                            ///< 用于按键消抖的计时
  esp_timer_handle_t long_press_time_handle;                              ///< 用于按键长时的计时
} key_time_params_t;

/*
===========================
函数声明
=========================== 
*/

/** 
 * 按键初始化
 * @param[in]   key_config          :不同按键的参数的配置
 * @param[in]   key_counts          :按键的个数
 * @param[in]   decoune_timer       :消抖的时长,单位是ms
 * @param[in]   long_pressed_cb     :长按时的回调处理函数
 * @param[in]   short_pressed_cb    :短按以及多击的回调处理函数
 * @retval      -1                  :按键参数的配置为空
 *              -2                  :短按的回调处理函数为空,但是长按的回调可以为空,因为有的按键并不需要长按功能
 *              -3                  :按键的个数为0
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
int32_t user_key_init(key_config_t *key_config,
                      uint8_t key_counts,
                      uint16_t decoune_timer,
                      user_key_function_callback_t long_pressed_cb,
                      user_key_function_callback_t short_pressed_cb);

#endif/* UER_KEY_H_ */