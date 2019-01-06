/*
* @file         hx_led.c 
* @brief        用户应用程序入口
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       hx-zsj 
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
* @par History:          
*               Ver0.0.1:
                     hx-zsj, 2018/08/06, 初始化版本\n 
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define LED_R_IO 		2
#define LED_G_IO 		18
#define LED_B_IO 		19

void app_main()
{
	//选择IO
    gpio_pad_select_gpio(LED_R_IO);
    gpio_pad_select_gpio(LED_G_IO);
    gpio_pad_select_gpio(LED_B_IO);
    //设置IO为输出
    gpio_set_direction(LED_R_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_G_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_B_IO, GPIO_MODE_OUTPUT);
    while(1) {
        //只点亮红灯
        gpio_set_level(LED_R_IO, 0);
        gpio_set_level(LED_G_IO, 1);
        gpio_set_level(LED_B_IO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        //只点亮绿灯
        gpio_set_level(LED_R_IO, 1);
        gpio_set_level(LED_G_IO, 0);
        gpio_set_level(LED_B_IO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        //只点亮蓝灯
        gpio_set_level(LED_R_IO, 1);
        gpio_set_level(LED_G_IO, 1);
        gpio_set_level(LED_B_IO, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
