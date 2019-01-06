/*
* @file         hx_pwm.c 
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
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"


#define LED_R_IO 		2
#define LED_G_IO 		18
#define LED_B_IO 		19
#define LEDC_MAX_DUTY         	(8191)	//2的13次方-1(13位PWM)
#define LEDC_FADE_TIME    		(1000)	//渐变时间(ms)

//ledc配置结构体
ledc_channel_config_t 	g_ledc_ch_R,g_ledc_ch_G,g_ledc_ch_B;
	
/*
* void ledc_init(void):定时器0用在PWM模式，输出3通道的LEDC信号
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void ledc_init(void)
{
	//定时器配置结构体
	ledc_timer_config_t 	ledc_timer;
	//定时器配置->timer0
	ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; //PWM分辨率
	ledc_timer.freq_hz = 5000;                      //频率
	ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;  	//速度
	ledc_timer.timer_num = LEDC_TIMER_0;           	// 选择定时器
	ledc_timer_config(&ledc_timer);					//设置定时器PWM模式
	//PWM通道0配置->IO2->红色灯
	g_ledc_ch_R.channel    = LEDC_CHANNEL_0;		//PWM通道
	g_ledc_ch_R.duty       = 0;						//占空比
	g_ledc_ch_R.gpio_num   = LED_R_IO;						//IO映射
	g_ledc_ch_R.speed_mode = LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_R.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_R);				//配置PWM
	//PWM通道1配置->IO18->绿色灯
	g_ledc_ch_G.channel    = LEDC_CHANNEL_1;		//PWM通道
	g_ledc_ch_G.duty       = 0;						//占空比
	g_ledc_ch_G.gpio_num   = LED_G_IO;				//IO映射
	g_ledc_ch_G.speed_mode = LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_G.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_G);				//配置PWM
	//PWM通道2配置->IO19->蓝色灯
	g_ledc_ch_B.channel    = LEDC_CHANNEL_2;		//PWM通道
	g_ledc_ch_B.duty       = 0;						//占空比
	g_ledc_ch_B.gpio_num   =LED_B_IO ;				//IO映射
	g_ledc_ch_B.speed_mode = LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_B.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_B);				//配置PWM
	//使能ledc渐变功能
	ledc_fade_func_install(0);
}

void app_main()
{    
	ledc_init();//ledc初始化
    while(1)
	{
		//ledc 红灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_R.speed_mode,
                    g_ledc_ch_R.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_R.speed_mode,
				g_ledc_ch_R.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 绿灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_G.speed_mode,
                    g_ledc_ch_G.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_G.speed_mode,
				g_ledc_ch_G.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 蓝灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_B.speed_mode,
                    g_ledc_ch_B.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_B.speed_mode,
				g_ledc_ch_B.channel, 
				LEDC_FADE_NO_WAIT);
		//延时LEDC_FADE_TIME，给LEDC控制时间
        vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
		

		//ledc 红灯 渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_R.speed_mode,
                    g_ledc_ch_R.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_R.speed_mode,
				g_ledc_ch_R.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 绿灯渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_G.speed_mode,
                    g_ledc_ch_G.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_G.speed_mode,
				g_ledc_ch_G.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 蓝灯渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_B.speed_mode,
                    g_ledc_ch_B.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_B.speed_mode,
				g_ledc_ch_B.channel, 
				LEDC_FADE_NO_WAIT);
		//延时LEDC_FADE_TIME，给LEDC控制时间
        vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
		
	}
}