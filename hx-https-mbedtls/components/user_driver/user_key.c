/** 
* @file         user_key.c 
* @brief        按键相关的处理函数定义
* @details      定义日常经常使用到按键相关函数,例如单击,双击以及N击的处理等函数
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
*/

/*
===========================
头文件包含
=========================== 
*/
#include "user_key.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sys/time.h"
/*
===========================
全局变量
=========================== 
*/
/* 用于存放各个不同的按键动作所需要的时间参数 */
static key_time_params_t gs_m_key_time_params;
/* 用于存放传进来的短按和长按的回调函数以及消抖时间 */
static key_param_t gs_m_key_param;
/* gpio事件队列句柄 */
static xQueueHandle gpio_evt_queue = NULL;
/* 存放key对应的GPIO口进入中断时的电平 */
static uint64_t gs_key_state = 0;
/* 存放key对应的GPIO口电平发生转移时，对应的IO口置１ */
static uint64_t gs_key_transition = 0;
/* 存放传进来的按键个数 */
static uint8_t gs_key_counts;
/* 存放传进来的按键设置参数 */
static key_config_t *gs_m_key_config = NULL;
/*
===========================
函数定义
=========================== 
*/

/** 
 * 消抖之后按键的具体处理函数
 * @param[in]   pin_no       :表示哪组按键
 * @param[in]   key_action   :表示按键的状态
 * @retval      NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void short_pressed_cb(uint8_t pin_no, uint8_t key_action)
{
  int64_t current_time;
  static int64_t last_time = 0;
  struct timeval system_time;
  key_config_t *s_m_key_config = (gs_m_key_config + pin_no);
  /* 获取此时的时间 */
  gettimeofday(&system_time, NULL);
  /* 转换成ms */
  current_time = system_time.tv_sec * 1000 + (system_time.tv_usec / 1000);
  switch (key_action)
  {
  case APP_KEY_PUSH:
    esp_timer_stop(gs_m_key_time_params.short_press_time_handle);
    esp_timer_start_once(gs_m_key_time_params.long_press_time_handle, s_m_key_config->long_pressed_time);
    ESP_LOGI("short_pressed_cb", "APP_KEY_PUSH\n");
    break;
  case APP_KEY_RELEASE:
    ESP_LOGI("short_pressed_cb", "APP_KEY_RELEASE\n");
    esp_timer_stop(gs_m_key_time_params.long_press_time_handle);
    /* 如果按键的按下与释放的时间小于MULTI_PRESSED_TIMER则认为是整个短按按键动作完成 */
    // ESP_LOGI("short_pressed_cb", "current_time - last_time is %lld\n", current_time - last_time);
    if ((current_time - last_time) < MULTI_PRESSED_TIMER)
    {
      s_m_key_config->short_pressed_counts++;      
      esp_timer_start_once(gs_m_key_time_params.short_press_time_handle, SHORT_PRESS_DELAY_CHECK);
    }
    else
    {
      s_m_key_config->short_pressed_counts = 0;
    }
    break;
  }
  /* 处理完之后,将处理之前的值就是上一次的值了 */
  last_time = current_time;
}

/** 
 * 计时150ms的时间处理
 * @param[in]   arg   :"esp_short_press_timer"按键完全动作计时器传进来的值,此处传进来的按键短按的次数以及对应的按键
 * @retval      NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void short_press_time_out_handle(void *arg)
{
  key_config_t *s_key_config = (key_config_t *)arg;  
  gs_m_key_param.short_press_callback(s_key_config->key_number,&(s_key_config->short_pressed_counts));  
}

/** 
 * 计时长按时间处理
 * @param[in]   arg   :"esp_long_press_timer"按键长按时计时器传进来的值,此处传进来的按键短按的次数以及对应的按键
 * @retval      NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void long_press_time_out_handle(void *arg)
{
  key_config_t *s_key_config = (key_config_t *)arg;  
  gs_m_key_param.long_press_callback(s_key_config->key_number,&(s_key_config->short_pressed_counts)); 
}
/** 
 * 消抖之后的按键处理函数,判断是哪个GPIO口以及对应的电平
 * @param[in]   arg   :"esp_decounce_timer"消抖定时器传进来的值,此处没有传进值来
 * @retval      NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void after_key_decounce_cb(void *arg)
{
  /* 轮询判断是哪个GPIO */
  for (uint8_t i = 0; i < gs_key_counts; i++)
  {
    key_config_t *local_key_config = (gs_m_key_config + i);
    uint64_t key_mask = 1ULL << (local_key_config->key_number);
    /* 消抖之后,如果找到对应的GPIO的电平发生转移,则再判断电平是否与进入中断时一致 */
    if(gs_key_transition & key_mask)
    {
      /* 将对应的gpio电平转移标志位清0 */
      gs_key_transition &= ~key_mask;
      bool key_level = gpio_get_level(local_key_config->key_number);
      /* 如果消抖之后的电平与进来中断时的电平是一致的,可以认为按键稳定下来了,且是稳定了 */
      if ((gs_key_state & (1ULL << local_key_config->key_number)) ==
          (((uint64_t)key_level) << local_key_config->key_number))
      {
        short_pressed_cb(i, key_level);
      }
    }
  }
}

 /** 
 * 按键中断处理函数
 * @param[in]   arg :中断触发时传来的值是,触发的GPIO口
 * @retval      NULL                      
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void gpio_intr_handler(void *arg)
{
  /* 获取触发中断的gpio口 */  
  uint32_t key_num = (uint32_t) arg;
  /* 从中断处理函数中发出消息到队列 */
  xQueueSendFromISR(gpio_evt_queue,&key_num,NULL);
}
/** 
 * 读取gpio中断发送过来的队列消息
 * @param[in]   arg :中断触发时传来的值是,触发的GPIO口
 * @retval      NULL                      
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
static void gpio_intr_task_thread(void *arg)
{
  uint32_t key_num;
  uint64_t key_mask;
  for(;;)
  {
    /* 不断从队列中查询是否有消息 */
    if(xQueueReceive(gpio_evt_queue,&key_num,portMAX_DELAY))
    {
      key_mask = 1ULL << key_num;
      /* 判断引起中断的GPIO口是不是已经发生过电平转移,如果发生了则不处理 */
      if (!(gs_key_transition & key_mask))
      {              
        switch (gpio_get_level(key_num))
        {
        case 0:
          gs_key_state &= ~(key_mask);
          break;
        case 1:
          gs_key_state |= (key_mask);
          break;
        }
        gs_key_transition |= key_mask;
        /* 开启消抖计时定时器 */
        esp_timer_start_once(gs_m_key_time_params.key_decounce_time_handle, gs_m_key_param.decounce_time);
      }         
    }
  }
}

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
                      user_key_function_callback_t short_pressed_cb)
{  
  esp_err_t err_code = ESP_OK;
  if (key_config == NULL)
  {
    return -1;
  }
  if (short_pressed_cb == NULL)
  {
    return -2;
  }
  if (!key_counts)
  {
    return -3;
  }
  /* 保存传进来的按键相关设置参数 */
  gs_m_key_config = key_config;
  gs_key_counts = key_counts;
  /* 设置gpio中断服务 */
  gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
  /* 不断填充按键参数值 */
  for (uint8_t i = 0; i < key_counts; i++)
  {    
    /* 配置按键参数 */
    gpio_config_t m_gpio_config =
        {
            /* 此处一定要这样写,不然就会变成32bit */
            .pin_bit_mask = ((uint64_t)(((uint64_t)1) << ((key_config + i)->key_number))),
            .mode = GPIO_MODE_INPUT,
            /* 默认是不开启中断的 */
            .intr_type = GPIO_INTR_DISABLE,
        };
    /* 根据高低电平有效,来判断当前的按键是上升沿有效还是下降沿有效 */
    switch ((key_config + i)->active_state)
    {
    case APP_KEY_ACTIVE_LOW:
      m_gpio_config.pull_up_en = GPIO_PULLUP_ENABLE;
      m_gpio_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      break;
    case APP_KEY_ACTIVE_HIGH:
      m_gpio_config.pull_up_en = GPIO_PULLUP_DISABLE;
      m_gpio_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
      break;
    }
    /* 设置对应的gpio参数 */
    err_code = gpio_config(&m_gpio_config);
    if (err_code != ESP_OK)
    {
      ESP_LOGI("user_key_int","gpio_config is %d\n", err_code);
      return err_code;
    }
    /* 注册中调回调函数,并将触发中断的GPIO口传进中断处理函数 */
    gpio_isr_handler_add((key_config + i)->key_number, gpio_intr_handler, (void *)((key_config + i)->key_number));
  }    
  /* 依次打开对应IO的中断 */  
  for (uint8_t i = 0; i < key_counts;i++)
  {
    err_code = gpio_set_intr_type((key_config+i)->key_number,GPIO_INTR_ANYEDGE);
    if(err_code != ESP_OK)
    {
      ESP_LOGI("user_key_init","gpio_set_intr_type is %d\n",err_code);
      return err_code;
    }
  }  
  /* 填充按键处理相关的参数 */
  gs_m_key_param.decounce_time = decoune_timer;
  gs_m_key_param.long_press_callback = long_pressed_cb;
  gs_m_key_param.short_press_callback = short_pressed_cb;

  /* 创建一个长度为10,大小为4字节的队列 */
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  /* 创建一个名为"gpio_intr_task_thread"的任务,
  注意任务堆栈设大点,否则会在任务处理函数中复位,
  因为那里有打印输出函数,需要占用较大的空间 */
  err_code = xTaskCreate(gpio_intr_task_thread, "gpio_intr_task_thread", 256*8, NULL, 10, NULL);
  if(err_code != pdPASS)
  {
    ESP_LOGI("user_key_init","xTaskCreate is %d\n",err_code);
    return err_code;
  }
  /* 填充消抖定时器所需要的相关参数并创建消抖计时函数 */
  esp_timer_create_args_t esp_decounce_timer_args = 
  {
    .callback         = after_key_decounce_cb,
    .arg              = NULL,
    .dispatch_method  = ESP_TIMER_TASK,
    .name             = "esp_decounce_timer",
  };
  err_code = esp_timer_create(&esp_decounce_timer_args,&gs_m_key_time_params.key_decounce_time_handle);
  if (err_code != ESP_OK)
  {
    ESP_LOGI("user_key_init", "esp_decounce_timer is %d\n", err_code);
    return err_code;
  }
  /* 填充按键按下并释放之后延时一段时间判断是否有新的按键动作按下所需要的相关参数并创建对应的计时函数 */
  esp_timer_create_args_t esp_short_press_timer_args = 
  {
    .callback         = short_press_time_out_handle,
    .arg              = key_config,
    .dispatch_method  = ESP_TIMER_TASK,
    .name             = "esp_short_press_timer",
  };
  err_code = esp_timer_create(&esp_short_press_timer_args, &gs_m_key_time_params.short_press_time_handle);
  if (err_code != ESP_OK)
  {
    ESP_LOGI("user_key_init", "esp_short_press_timer is %d\n", err_code);
    return err_code;
  }
  /* 填充按键长按所需要的时间相关参数并创建对应的计时函数 */
  esp_timer_create_args_t esp_long_press_timer_args = 
  {
    .callback         = long_press_time_out_handle,
    .arg              = key_config,
    .dispatch_method  = ESP_TIMER_TASK,
    .name             = "esp_long_press_timer",
  };
  return esp_timer_create(&esp_long_press_timer_args, &gs_m_key_time_params.long_press_time_handle);
}


