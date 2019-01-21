/*
* @file         hx_uart.c 
* @brief        ESP32双串口使用
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       hx-zsj 
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
* @par History:          
*               Ver0.0.1:
                     hx-zsj, 2018/08/06, 初始化版本\n 
*/

/* 
=============
头文件包含
=============
*/
#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

//TM1638模块引脚定义
#define DIO 18
#define CLK 19
#define STB 21

//共阴数码管显示代码0~9 A~F
uint8_t tab[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,  \
              0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71};

#define SMa 0x01
#define SMb 0x02
#define SMc 0x04
#define SMd 0x08
#define SMe 0x10
#define SMf 0x20
#define SMg 0x40

static uint8_t init_gpio_flag = 0;

//初始化IO
static void init_gpio()
{
    if(init_gpio_flag == 0){
    	printf("开始初始化gpio\n");
		gpio_config_t io_conf = {                    	
			.pin_bit_mask = (1<<18)|(1<<19)|(1<<21), 	       
			.mode = GPIO_MODE_INPUT_OUTPUT,         	
			.pull_down_en = 0,                     		 
			.pull_up_en = 1,                      		 
			.intr_type = GPIO_INTR_DISABLE,			
		};       
		gpio_config(&io_conf);
		init_gpio_flag++;
		printf("初始化gpio成功\n");
	}
}
//写数据函数
void TM1638_Write(uint8_t DATA)			
{
	uint8_t i = 0;
	for(i=0;i<8;i++)
	{
		//CLK=0;
        gpio_set_level(CLK,0);
		if(DATA&0X01)		//取字节的低位d0的布尔值，
			//DIO=1;			//gpio高电平
            gpio_set_level(DIO,1);
		else
			//DIO=0;			//gpio低电平
            gpio_set_level(DIO,0);
		DATA>>=1;
		//CLK=1;
        gpio_set_level(CLK,1);
	}
}
//读数据函数
uint8_t TM1638_Read(void)					
{
	uint8_t i;
	uint8_t temp=0;
	//DIO=1;	//设置为输入
	gpio_set_level(DIO,1);
    for(i=0;i<8;i++)
	{
		temp>>=1;
		//CLK=0;
		gpio_set_level(CLK,0);
        if(gpio_get_level(DIO))
			temp|=0x80;
		//CLK=1;
        gpio_set_level(CLK,1);
	}
	return temp;
}
//发送命令字
void Write_COM(uint8_t cmd)		
{
	//STB=0;
    gpio_set_level(STB,0);
	TM1638_Write(cmd);
	//STB=1;
    gpio_set_level(STB,1);
}
//读取键值
uint8_t Read_key(void)
{
	uint8_t c[4],i,key_value=0;
	//STB=0;
	gpio_set_level(STB,0);
    TM1638_Write(0x42);		           //读键扫数据 命令
	for(i=0;i<4;i++)		
		c[i]=TM1638_Read();
	//STB=1;					           //4个字节数据合成一个字节
	gpio_set_level(STB,1);
    for(i=0;i<4;i++)
		key_value|=c[i]<<i;
	for(i=0;i<8;i++)
		if((0x01<<i)==key_value)
			break;
	return i;		
}
//指定地址写入数据
void Write_DATA(uint8_t add,uint8_t DATA)		
{
	Write_COM(0x44);
	//STB=0;
	gpio_set_level(STB,0);
    TM1638_Write(0xc0|add);
	TM1638_Write(DATA);
	//STB=1;
    gpio_set_level(STB,1);
}
//控制全部LED函数，LED_flag表示各个LED状态
void Write_allLED(uint8_t LED_flag)					
{
	uint8_t i;
	for(i=0;i<8;i++)
		{
			if(LED_flag&(1<<i))
				//Write_DATA(2*i+1,3);
				Write_DATA(2*i+1,1);
			else
				Write_DATA(2*i+1,0);
		}
}

//TM1638初始化函数
void init_TM1638(void)
{
	uint8_t i;
	Write_COM(0x8b);       //亮度 (0x88-0x8f)8级亮度可调
	Write_COM(0x40);       //采用地址自动加1
	//STB=0;		           //
    gpio_set_level(STB,0);
	TM1638_Write(0xc0);    //设置起始地址

	for(i=0;i<16;i++)	   //传送16个字节的数据
		TM1638_Write(0x00);
	//STB=1;
    gpio_set_level(STB,1);
}

/*
===========================
函数声明
=========================== 
*/
void tm1638_test_task(void *pvParameter);

/*
 * 应用程序的函数入口
 * @param[in]   NULL
 * @retval      NULL              
 * @par         修改日志 
 *               Ver0.0.1:
                     hx-zsj, 2018/08/06, 初始化版本\n  
*/
void app_main()
{    
	xTaskCreatePinnedToCore(&tm1638_test_task,"tm1638_test", 5000, NULL, 5,NULL,1);
	printf("创建tm1603模块测试任务\n");
}
/*
* TM1638任务
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void tm1638_test_task(void *pvParameter)
{
    uint8_t num[8];		//各个数码管显示的值
    uint8_t i;
    uint8_t key;
    init_gpio();
	init_TM1638();	                           //初始化TM1638
	for(i=0;i<8;i++){
            Write_DATA(i<<1,SMg);    //初始化寄存器
        }
    for(i=0;i<8;i++){
            Write_allLED(1<<i);
            ets_delay_us(100000);
        }
    Write_allLED(0);
	while(1)
	{
		key=Read_key();                          //读按键值
        if(key<8)
		{   
            printf("按键%1u\n",key);
			num[key]++;
			while(Read_key()==key);		       //等待按键释放
			if(num[key]>15)
			num[key]=0;
			Write_DATA(key*2,tab[num[key]]);	//键值显示
			Write_allLED(1<<key);
		}
        vTaskDelay(100 / portTICK_PERIOD_MS);
	}
} 