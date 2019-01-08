/*
* @file         hx_key.c 
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
#define KEY_IO          34
unsigned char led_r_status = 0;
unsigned char led_g_status = 0;
unsigned char key_status[2];
void key_read(void)
{
    if(gpio_get_level(KEY_IO)==0)//按键按下
    {
        //等待松手，最傻的办法
        while(gpio_get_level(KEY_IO)==0);
        if (led_r_status==1)
        {
            led_r_status = 0;
            gpio_set_level(LED_R_IO, 1);//不亮
        }
        else
        {
            led_r_status = 1;
            gpio_set_level(LED_R_IO, 0);//亮
        }

    }
}
void key_read1(void)
{
    //按键识别
    if(gpio_get_level(KEY_IO)==0){
        key_status[0] = 0;
    }
    else{
       key_status[0] = 1; 
    }
    if(key_status[0]!=key_status[1]) {
        key_status[1] = key_status[0];
        if(key_status[1]==0){//按键按下
            if (led_r_status==1){
                led_r_status = 0;
                gpio_set_level(LED_R_IO, 1);//不亮
            }else{
                led_r_status = 1;
                gpio_set_level(LED_R_IO, 0);//亮
            }
        }else{//按键松手
            if (led_g_status==1){
                led_g_status = 0;
                gpio_set_level(LED_G_IO, 1);//不亮
            }else{
                led_g_status = 1;
                gpio_set_level(LED_G_IO, 0);//亮
            }
        }
    }
}
void app_main()
{
	//选择IO
    gpio_pad_select_gpio(LED_R_IO);
    gpio_pad_select_gpio(LED_G_IO);
    gpio_pad_select_gpio(KEY_IO);
    //设置灯IO为输出
    gpio_set_direction(LED_R_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_R_IO, 1);//不亮
    gpio_set_direction(LED_G_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_G_IO, 1);//不亮
    //设置按键IO输入
    gpio_set_direction(KEY_IO, GPIO_MODE_INPUT);
    
    while(1) {
        //key_read();
        key_read1();
    }
}
