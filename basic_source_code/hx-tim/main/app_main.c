#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "esp_timer.h"

#define LED_R_IO 		2

//定时器句柄
esp_timer_handle_t fw_timer_handle = 0;

void fw_timer_cb(void *arg) 
{
	//获取时间戳
	int64_t tick = esp_timer_get_time();
	printf("timer cnt = %lld \r\n", tick);

	if (tick > 50000000) //50秒结束
	{
		//定时器暂停、删除
		esp_timer_stop(fw_timer_handle);
		esp_timer_delete(fw_timer_handle);
		printf("timer stop and delete!!! \r\n");
		//重启
		esp_restart();
	}

	gpio_set_level(LED_R_IO, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(LED_R_IO, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);

}

void app_main() {
	//选择IO
    gpio_pad_select_gpio(LED_R_IO);
    //设置IO为输出
    gpio_set_direction(LED_R_IO, GPIO_MODE_OUTPUT);

	//定时器结构体初始化
	esp_timer_create_args_t fw_timer = 
	{ 
		.callback = &fw_timer_cb, 	//回调函数
		.arg = NULL, 				//参数
		.name = "fw_timer" 			//定时器名称
	};

	//定时器创建、启动
	esp_err_t err = esp_timer_create(&fw_timer, &fw_timer_handle);
	err = esp_timer_start_periodic(fw_timer_handle, 1000 * 1000);//1秒回调
	if(err == ESP_OK)
	{
		printf("fw timer cteate and start ok!\r\n");
	}


}

