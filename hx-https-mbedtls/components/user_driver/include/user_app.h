/** 
* @file         user_app.h 
* @brief        用户实现自己功能相关声明
* @details      声明用户所需的结构体或者宏定义,以及函数声明
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/19, 初始化版本\n 
*/
#ifndef USER_APP_H_
#define USER_APP_H_


/*
===========================
宏定义
=========================== 
*/
#define R_CHANNEL           LEDC_CHANNEL_0
#define G_CHANNEL           LEDC_CHANNEL_1
#define B_CHANNEL           LEDC_CHANNEL_2

#define R_PIN_NUM           (2)
#define G_PIN_NUM           (18)
#define B_PIN_NUM           (19)

/** 
 * 用户的按键初始化函数
 * @param[in]   null 
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
// void user_app_key_init(void);

/** 
 * 用户的rgb驱动初始化函数
 * @param[in]   null 
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/24, 初始化版本\n 
 */
// void user_app_rgb_init(void);


/** 
 * 上电初始化任务,初始化按键,rgb等外设驱动
 * @param[in]   null 
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/26, 初始化版本\n 
 */
void startup_initialization_task(void *pvParameters);
#endif/* USER_APP_H_ */