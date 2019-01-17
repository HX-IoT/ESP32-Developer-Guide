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
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

//UART1
#define RX1_BUF_SIZE 		(1024)
#define TX1_BUF_SIZE 		(512)
#define TXD1_PIN 			(GPIO_NUM_5)
#define RXD1_PIN 			(GPIO_NUM_4)

//UART2
#define RX2_BUF_SIZE 		(1024)
#define TX2_BUF_SIZE 		(512)
#define TXD2_PIN 			(GPIO_NUM_12)
#define RXD2_PIN 			(GPIO_NUM_13)



/*
===========================
全局变量定义
=========================== 
*/
	
/*
===========================
函数声明
=========================== 
*/
void uart1_rx_task();
void uart2_rx_task();

/*
* esp32双路串口配置
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void uart_init(void)
{
	//串口配置结构体
	uart_config_t uart1_config,uart2_config;
	//串口参数配置->uart1
	uart1_config.baud_rate = 115200;					//波特率
	uart1_config.data_bits = UART_DATA_8_BITS;			//数据位
	uart1_config.parity = UART_PARITY_DISABLE;			//校验位
	uart1_config.stop_bits = UART_STOP_BITS_1;			//停止位
	uart1_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;	//硬件流控
	uart_param_config(UART_NUM_1, &uart1_config);		//设置串口
	//IO映射-> T:IO4  R:IO5
	uart_set_pin(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	//注册串口服务即使能+设置缓存区大小
	uart_driver_install(UART_NUM_1, RX1_BUF_SIZE * 2, TX1_BUF_SIZE * 2, 0, NULL, 0);

	//串口参数配置->uart2
	uart2_config.baud_rate = 115200;					//波特率
	uart2_config.data_bits = UART_DATA_8_BITS;			//数据位
	uart2_config.parity = UART_PARITY_DISABLE;			//校验位
	uart2_config.stop_bits = UART_STOP_BITS_1;			//停止位
	uart2_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;	//硬件流控
	uart_param_config(UART_NUM_2, &uart2_config);		//设置串口
	//IO映射-> T:IO12  R:IO13
	uart_set_pin(UART_NUM_2, TXD2_PIN, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	//注册串口服务即使能+设置缓存区大小
	uart_driver_install(UART_NUM_2, RX2_BUF_SIZE * 2, TX2_BUF_SIZE * 2, 0, NULL, 0);


}


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
	//串口初始化
	uart_init();
	//创建串口1接收任务
	xTaskCreate(uart1_rx_task, "uart1_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
	//创建串口2接收任务
	xTaskCreate(uart2_rx_task, "uart2_rx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
	//串口1数据发送测试		
	uart_write_bytes(UART_NUM_1, "uart1 test OK ", strlen("uart1 test OK "));
	//串口2数据发送测试		
	uart_write_bytes(UART_NUM_2, "uart2 test OK ", strlen("uart2 test OK "));
}
/*
* 串口1接收任务
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void uart1_rx_task()
{
    uint8_t* data = (uint8_t*) malloc(RX1_BUF_SIZE+1);
    while (1) {
        //获取串口1接收的数据
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX1_BUF_SIZE, 10 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
			//将接收到的数据发出去
			uart_write_bytes(UART_NUM_1, (char *)data, rxBytes);
        }
    }
    free(data);
}

/*
* 串口2接收任务
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void uart2_rx_task()
{
    uint8_t* data = (uint8_t*) malloc(RX2_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX2_BUF_SIZE, 10 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
			//将接收到的数据发出去
			uart_write_bytes(UART_NUM_2, (char *)data, rxBytes);
        }
    }
    free(data);
}


