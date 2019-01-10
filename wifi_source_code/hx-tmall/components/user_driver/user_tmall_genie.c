/** 
* @file         user_tmall_genie.c
* @brief        定义接收以及处理天猫精灵命令的相关函数
* @details      获取得到天猫精灵的控制命令并控制
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/08/12, 初始化版本\n 
*/

/*
===========================
头文件包含
=========================== 
*/
#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/md5.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "cJSON.h"
#include "user_tmall_genie.h"
#include "esp_log.h"
#include "os.h"
#include "esp_wifi.h"

#include <freertos/FreeRTOS.h>
#include "freertos/task.h"

#include "user_led.h"
/*
===========================
全局变量定义
=========================== 
*/
typedef struct
{
  mbedtls_net_context *net_ctx;
  mbedtls_entropy_context *entropy;
  mbedtls_ctr_drbg_context *ctr_drbg;
  mbedtls_ssl_context *ssl_ctx;
  mbedtls_ssl_config *ssl_conf;
}tcp_ssl_connect_t;

tcp_ssl_connect_t tcp_ssl_connect =
{
  .net_ctx  = NULL,
  .entropy  = NULL,
  .ctr_drbg = NULL,
  .ssl_ctx  = NULL,
  .ssl_conf = NULL,
};
/* 存放接收到GET请求的数据 */
static uint8_t  gs_recv_buff[1024*2];

/* 存放token+user_api_key md5加密之后的数据 */
static char *md5_encrypted_token_user_api_key = NULL;
/*
===========================
函数定义
=========================== 
*/

/** 
* tcp+ssl连接的相关参数清空
* @param[in]   null
* @retval      null
* @note        修改日志 
*               Ver0.0.1: 
                  Helon_Chan, 2018/08/12, 初始化版本\n 
*/
static void tcp_ssl_parameters_clean(void)
{  
  mbedtls_ssl_close_notify(tcp_ssl_connect.ssl_ctx);
  mbedtls_net_free(tcp_ssl_connect.net_ctx);
  
  mbedtls_ssl_free(tcp_ssl_connect.ssl_ctx);
  mbedtls_ssl_config_free(tcp_ssl_connect.ssl_conf);
  mbedtls_ctr_drbg_free(tcp_ssl_connect.ctr_drbg);
  mbedtls_entropy_free(tcp_ssl_connect.entropy);

  os_free(tcp_ssl_connect.ssl_ctx);
  os_free(tcp_ssl_connect.net_ctx);
  os_free(tcp_ssl_connect.ctr_drbg);
  
  os_free(tcp_ssl_connect.entropy);
  os_free(tcp_ssl_connect.ssl_conf);
}
/** 
* 向云平台服务器发送数据,填充想要符合云平台通讯协议的json数据即可
* @param[in]   p_json_data:需要发送给云平台的数据,json的数据格式
* @retval      null
* @note        修改日志 
*               Ver0.0.1: 
                  Helon_Chan, 2018/08/12, 初始化版本\n 
*/
static void tcp_ssl_write(const char *p_json_data)
{
  int ret, len;    
  while ((ret = mbedtls_ssl_write(tcp_ssl_connect.ssl_ctx, (unsigned char*)p_json_data, strlen(p_json_data))) <= 0)
  {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
      return;
    }
  }
  len = ret;
  ESP_LOGI("tcp_ssl_write", " %d bytes written\n\n%s", len, (char *)p_json_data);  
}


/** 
* 按照云平台的格式，向云平台发送设备api key
* @param[in]   null
* @retval      null
* @note        修改日志 
*               Ver0.0.1: 
                  Helon_Chan, 2018/08/14, 初始化版本\n 
*/
static void send_device_api_key(void)
{
  char *p_json_device_api_key_info = NULL;
  char json_device_api_key_info[48] = {0};
  cJSON *device_api_key_info = cJSON_CreateObject();
  /* 拼装发送设备api_key的json数据 */
  cJSON_AddItemToObject(device_api_key_info,"M",cJSON_CreateString("checkin"));
  cJSON_AddItemToObject(device_api_key_info,"ID",cJSON_CreateString(DEVICE_ID));
  cJSON_AddItemToObject(device_api_key_info,"K",cJSON_CreateString(DEVICE_API_KEY));
  p_json_device_api_key_info = cJSON_PrintUnformatted(device_api_key_info);  
  /* 记得加换行符号,否则云平台无法识别 */
  memcpy(json_device_api_key_info,p_json_device_api_key_info,strlen(p_json_device_api_key_info));  
  json_device_api_key_info[strlen(json_device_api_key_info)] = '\n';
  // ESP_LOGI("send_device_api_key", " %d bytes written\n\n%s", strlen(json_device_api_key_info), (char *)json_device_api_key_info);
  tcp_ssl_write(json_device_api_key_info);
}

/** 
* 获取token之后,进行token+用户api key加密登陆
* @param[in]   token:表示从云平台上获取得到的token数据
* @retval      返回md5加密之后的token+用户api_key数据
* @note        修改日志 
*               Ver0.0.1: 
                  Helon_Chan, 2018/08/14, 初始化版本\n 
*/
static void token_user_api_key_md5_encrypted(const char *token)
{
  char token_user_api_key[48] = {0};  
  md5_encrypted_token_user_api_key = (char *)os_malloc(sizeof(char) * 48);  
  memset(md5_encrypted_token_user_api_key,0,48);
  unsigned char md5_output[16] = {0};

  /* 进行token+user api key拼装 */  
  memcpy(token_user_api_key, token + 1, strlen(token) - 2);

  memcpy(token_user_api_key + (strlen(token_user_api_key)), USER_API_KEY, strlen(USER_API_KEY));
  ESP_LOGI("token_user_api_key_md5_encrypted", "token_user_api_key is [%s] [%d]\n", token_user_api_key, strlen(token_user_api_key));
  /* md5加密 */
  mbedtls_md5_ret((unsigned char *)token_user_api_key, strlen(token_user_api_key), md5_output);
  
  /* 数组转为字符串 */
  for (unsigned char i = 0; i < 16; i++)
  {    
    sprintf(md5_encrypted_token_user_api_key+2*i, "%02x", md5_output[i]);
  }      
  ESP_LOGI("token_user_api_key_md5_encrypted", "md5_encrypted_token_user_api_key is [%s] [%d]\n", md5_encrypted_token_user_api_key, strlen(md5_encrypted_token_user_api_key));      
}


/** 
* 使用md5加密之后的token+用户api_key数据进行设备登陆
* @param[in]   token_user_api_key_md5_encrypted:表示md5加密之后的token+用户api_key数据
* @retval      null
* @note        修改日志 
*               Ver0.0.1: 
                  Helon_Chan, 2018/08/14, 初始化版本\n 
*               Ver0.0.2: 
                  Helon_Chan, 2018/08/15, 解决md5加密之后无不登陆的问题\n                   
*/
static void device_encrypted_sign_in(const char *token_user_api_key_md5_encrypted)
{
  char *p_json_token_user_api_key_md5 = NULL;
  cJSON *token_user_api_key = cJSON_CreateObject();
  /* 存放拼装的md5加密数据 */
  char json_device_api_key_info[8*9] = {0};
  /* 拼装发送设备api_key的json数据 */
  cJSON_AddItemToObject(token_user_api_key,"M",cJSON_CreateString("checkin"));
  cJSON_AddItemToObject(token_user_api_key,"ID",cJSON_CreateString(DEVICE_ID));
  cJSON_AddItemToObject(token_user_api_key,"K",cJSON_CreateString(token_user_api_key_md5_encrypted));
  p_json_token_user_api_key_md5 = cJSON_PrintUnformatted(token_user_api_key);  

  // ESP_LOGI("device_encrypted_sign_in", "p_json_token_user_api_key_md5 is [%s] [%d]\n", p_json_token_user_api_key_md5, strlen(p_json_token_user_api_key_md5));

  /* 记得加换行符号,否则云平台无法识别 */
  memcpy(json_device_api_key_info,p_json_token_user_api_key_md5,strlen(p_json_token_user_api_key_md5));
  json_device_api_key_info[strlen(json_device_api_key_info)] = '\n';  
  
  ESP_LOGI("device_encrypted_sign_in", "json_token_user_api_key_md5 is [%s] [%d]\n", json_device_api_key_info, strlen(json_device_api_key_info));
  /* 发送设备加密登陆信息 */
  tcp_ssl_write(json_device_api_key_info);
  /* 释放存放md5加密之后token+user_api_key数据的内存 */
  os_free(md5_encrypted_token_user_api_key);
} 


/** 
 * 心跳包任务处理函数
 * @param[in]   pvParameters: 表示任务所携带的参数 
 * @retval      null 
 *              其他:          失败 
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/08/15, 初始化版本\n 
 */
static void heart_beat_task(void *pvParameters)
{
  while (1)
  {
    /* 每隔30秒发送心跳包 */
    tcp_ssl_write(HEART_BEAT);
    vTaskDelay(30 * 1000 / portTICK_RATE_MS);
  }
  vTaskDelete(NULL);
}

/** 
 * 发送心跳包给云平台,每隔30秒发送一次心跳包，否则被云平台踢下线
 * @param[in]   null
 * @retval      null 
 *              其他:          失败 
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/08/15, 初始化版本\n 
 */
static void heart_beat_start(void)
{
  int err_code = xTaskCreate(heart_beat_task,
                             "heart_beat_task",
                             1024 * 4,
                             NULL,
                             3,
                             NULL);
  if (err_code != pdPASS)
  {
    ESP_LOGI("heart_beat_start", "heart_beat_task create failure,reason is %d\n", err_code);
  }    
}


/** 
 * 处理云平台的控制命令以及数据
 * @param[in]   json_parse_data: 存放解析之后的云平台json数据
 * @retval      null 
 *              其他:          失败 
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/08/14, 初始化版本\n 
 */
static void cloud_cmd_data_hander(const cJSON *json_parse_data)
{
  char *cmd = NULL;
  /* 获取云平台返回的json数据中当前的控制命令是什么 */
  cmd = cJSON_Print(cJSON_GetObjectItem(json_parse_data, "C"));
  /* 根据云平台返回的method执行不同的动作 */
    switch (strcmp(cmd, "\"play\"") == 0 ? ON : 
            strcmp(cmd, "\"stop\"") == 0 ? OFF : INVALID_METHOD)
    {
      case ON:
        r_fade_start();
        ESP_LOGI("cloud_cmd_data_hander", "Switch ON\n");
        break;
      case OFF:
        r_fade_stop();
        ESP_LOGI("cloud_cmd_data_hander", "Switch OFF\n");
        break;
      default:
        break;
    }            
}

/** 
 * 解析json数据
 * @param[in]   json_data: 需要解析的json数据
 * @retval      null 
 *              其他:          失败 
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/08/14, 初始化版本\n 
 */
static void json_parse(const char *json_data)
{
  cJSON *json_string = cJSON_Parse(json_data);
  char* token  = NULL;
  char* method = NULL;
  /* 如果解析失败,则打印解析失败的原因 */
  if(!json_string)
  {
    ESP_LOGI("json_parse","cJSON Parse failure,reason is [%s]\n", cJSON_GetErrorPtr());  
    return;
  }
  else
  {
    /* 获取云平台返回的json数据中当前的方法是什么 */
    method =  cJSON_Print(cJSON_GetObjectItem(json_string, "M"));

    /* 根据云平台返回的method执行不同的动作 */
    switch (strcmp(method, "\"checkinok\"") == 0 ? SIGN_IN_OK : 
            strcmp(method, "\"token\"") == 0 ? GET_TOKEN : 
            strcmp(method, "\"say\"") == 0 ? RECEVICE : 
            strcmp(method, "\"WELCOME TO BIGIOT\"") == 0 ? TCP_CONNECT_SUCCESS:INVALID_METHOD)
    {
      /* ESP32成功连接上云平台 */
      case TCP_CONNECT_SUCCESS:
        ESP_LOGI("json_parse", "big_iot_connect success\n"); 
        send_device_api_key();
        break;
      /* 发送设备API KEY之后，获取得到TOKEN */
      case GET_TOKEN:
        token = cJSON_Print(cJSON_GetObjectItem(json_string, "K"));
        ESP_LOGI("json_parse", "token is %s\n",token);         
        token_user_api_key_md5_encrypted(token);
        device_encrypted_sign_in(md5_encrypted_token_user_api_key);
        break;
      /* 设备加密登陆成功 */
      case SIGN_IN_OK:
        ESP_LOGI("json_parse", "sign success\n"); 
        heart_beat_start();
        break;
      /* 接收云平台的命令或者数据 */
      case RECEVICE:
        cloud_cmd_data_hander(json_string);
        break;
      default:
        break;
    }
  }
}

/** 
 * tcp连接任务处理函数
 * @param[in]   pvParameters: 表示任务所携带的参数 
 * @retval      null 
 *              其他:          失败 
 * @par         修改日志 
 *               Ver0.0.1:
                    Helon_Chan, 2018/08/12, 初始化版本\n 
 */
static void tcp_receive_task(void *pvParameters)
{
  int ret, len;
  do
  {
    len = sizeof(gs_recv_buff) - 1;
    memset(gs_recv_buff, 0, sizeof(gs_recv_buff));
    ret = mbedtls_ssl_read(tcp_ssl_connect.ssl_ctx, gs_recv_buff, len);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      continue;
    }

    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
      ESP_LOGI(TAG, "MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY\n");
      ret = 0;
      tcp_ssl_parameters_clean();
      break;
    }

    if (ret < 0)
    {
      ESP_LOGI(TAG, "failed\n  ! mbedtls_ssl_read returned %d\n\n", ret);
      
    }
    if (ret == 0)
    {
      ESP_LOGI(TAG, "\n\nEOF\n\n");      
    }
    else
    {
      len = ret;
      ESP_LOGI(TAG, " %d bytes read\n\n%s", len, gs_recv_buff);
      json_parse((char*)gs_recv_buff);     
    }
  } while (1);
  vTaskDelete(NULL);
}
/** 
* 通过TCP的方式连接贝壳物联的云平台接口
* @param[in]   url：表示贝壳物联的云平台接口地址
* @param[in]   port：表示贝壳物联云平台接口地址的端口
* @retval      null
* @note        修改日志 
*               Ver0.0.1: 
                  Helon_Chan, 2018/08/12, 初始化版本\n 
*/
void big_iot_cloud_connect(const char *url, const char *port)
{
  int ret;
  tcp_ssl_connect.net_ctx = (mbedtls_net_context *)os_malloc(sizeof(mbedtls_net_context));  
  tcp_ssl_connect.entropy = (mbedtls_entropy_context *)os_malloc(sizeof(mbedtls_entropy_context));
  tcp_ssl_connect.ctr_drbg = (mbedtls_ctr_drbg_context *)os_malloc(sizeof(mbedtls_ctr_drbg_context));
  tcp_ssl_connect.ssl_ctx = (mbedtls_ssl_context *)os_malloc(sizeof(mbedtls_ssl_context));
  tcp_ssl_connect.ssl_conf = (mbedtls_ssl_config *)os_malloc(sizeof(mbedtls_ssl_config));
/*
  * 0. Initialize the RNG and the session data
*/
  mbedtls_net_init(tcp_ssl_connect.net_ctx);
  mbedtls_ssl_init(tcp_ssl_connect.ssl_ctx);
  mbedtls_ssl_config_init(tcp_ssl_connect.ssl_conf);  
  mbedtls_ctr_drbg_init(tcp_ssl_connect.ctr_drbg);

/*
  * 1. Initialize the RNG and the session data
*/
  ESP_LOGI(TAG, "\n  . Seeding the random number generator...");

  mbedtls_entropy_init(tcp_ssl_connect.entropy);
  if ((ret = mbedtls_ctr_drbg_seed(tcp_ssl_connect.ctr_drbg, mbedtls_entropy_func, tcp_ssl_connect.entropy,
                                   (const unsigned char *)NULL,
                                   0)) != 0)
  {
    ESP_LOGI(TAG, "failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
    tcp_ssl_parameters_clean();
    return;
  }

  ESP_LOGI(TAG, " ok\n");
  

/*
  * 2. Start the connection
  * 进行TCP连接
*/
  ESP_LOGI(TAG, "  . Connecting to tcp/%s/%s...", url, port);
  //fflush( stdout );

  if ((ret = mbedtls_net_connect(tcp_ssl_connect.net_ctx, url,
                                 port, MBEDTLS_NET_PROTO_TCP)) != 0)
  {
    ESP_LOGI(TAG, " failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
    tcp_ssl_parameters_clean();
    return;
  }

  ESP_LOGI(TAG, " ok\n");

/*
  * 3. Setup SSL/TLS相关参数
*/
  ESP_LOGI(TAG, "  . Setting up the SSL/TLS structure...");

  if ((ret = mbedtls_ssl_config_defaults(tcp_ssl_connect.ssl_conf,
                                         MBEDTLS_SSL_IS_CLIENT,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
  {
    ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
    tcp_ssl_parameters_clean();
    return;
  }

  ESP_LOGI(TAG, " ok %d\n", esp_get_free_heap_size());

  /* 由于证书会过期,所以这些不进行证书认证 */
  mbedtls_ssl_conf_authmode(tcp_ssl_connect.ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
  /* 设置随机数生成的函数及方法 */
  mbedtls_ssl_conf_rng(tcp_ssl_connect.ssl_conf, mbedtls_ctr_drbg_random, tcp_ssl_connect.ctr_drbg);
  /* 这里不需要设置调试函数 */
  mbedtls_ssl_conf_dbg(tcp_ssl_connect.ssl_conf, NULL, NULL);

  /* 将ssl_conf的相关信息填充于ssl_ctx中去,用于进行SSL握手时使用 */
  if ((ret = mbedtls_ssl_setup(tcp_ssl_connect.ssl_ctx, tcp_ssl_connect.ssl_conf)) != 0)
  {
    ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
    tcp_ssl_parameters_clean();
    return;
  }

  if ((ret = mbedtls_ssl_set_hostname(tcp_ssl_connect.ssl_ctx, "mbed TLS Server")) != 0)
  {
    ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
    tcp_ssl_parameters_clean();
    return;
  }
  /* 设置发送以及接收的时候,调用的内部函数 */
  mbedtls_ssl_set_bio(tcp_ssl_connect.ssl_ctx, tcp_ssl_connect.net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

/*
  * 4. Handshake
*/
  ESP_LOGI(TAG, "  . Performing the SSL/TLS handshake...");

  while ((ret = mbedtls_ssl_handshake(tcp_ssl_connect.ssl_ctx)) != 0)
  {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
      tcp_ssl_parameters_clean();
    return;
    }
  }

  ESP_LOGI(TAG, " ok\n");
  

  /*
    * 5. Read the HTTP response
  */
  int err_code = xTaskCreate(tcp_receive_task,
                             "tcp_receive_task",
                             1024 * 8,
                             NULL,
                             3,
                             NULL);
  if (err_code != pdPASS)
  {
    ESP_LOGI("event_handler", "tcp_receive_task create failure,reason is %d\n", err_code);
  }    
}


