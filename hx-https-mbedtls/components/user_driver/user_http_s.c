/** 
* @file         user_http_s.c
* @brief        定义获取天气预报的相关函数
* @details      获取天气预报的相关函数的定义以及所需的相关变量定义
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                     Helon_Chan, 2018/06/27, 初始化版本\n 
*/

/*
===========================
头文件包含
=========================== 
*/
// #include "lwip/ip_addr.h"
#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "user_http_s.h"
#include "esp_event.h"
#include "os.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/queue.h"
#include "cJSON.h"
/*
===========================
全局变量
=========================== 
*/
// static ip_addr_t ip_addr;
static char host[32];
static char filename[1024];
/* 存放get请求的相关数据 */
static uint8_t https_get_buffer[1024];
/* 存放接收到GET请求的数据 */
static uint8_t  recv_buff[1024*2];
/* 存放接收到的josn数据 */
static char *recv_json_buff = NULL;

/*
===========================
枚举变量
=========================== 
*/
enum
{
  GET_REQ,
  POST_REQ,
};

/*
===========================
函数定义
=========================== 
*/

/** 
 * 从给定的网址中获取主机域名和域名后面的内容分别存放至HOST和Filename
 * @param[in]   URL     :需要解析的域名地址
 * @param[out]  host    :解析获取域名的主机地址
 * @param[out]  filename:解析获取从域名主机的内容地址
 * @retval      
 *              0: 解析成功
 *             -1: URL为空
 *             -2: host为空
 *             -3: filename为空
 *             -4: 传进来的网址中没有https://和http://
 * @par         示例:
 *              1.如果URL是https://www.sojson.com/open/api/weather/json.shtml?city=深圳
 *              2.host是"www.sojson.com/"
 *              3.filename是"open/api/weather/json.shtml?city=深圳"
 * 
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/27, 初始化版本\n 
 */
static esp_err_t http_url_parse(char *URL, char *host, char *filename)
{
  char *PA;
  char *PB;
  /* 分别判断传来的实参是不是为空 */
  if (URL == NULL)
  {
    return -1;
  }
  else if (host == NULL)
  {
    return -2;
  }
  else if (filename == NULL)
  {
    return -3;
  }
  /* 初始化传进来的用于存放host和filename的内存空间 */
  os_memset(host, 0, sizeof(host)/sizeof(char));
  os_memset(filename, 0, sizeof(filename)/sizeof(char));
  PA = URL;
  /* 判断传进来的网址是否带有"http://以及https://"内容 */
  if (!os_strncmp(PA, "http://", os_strlen("http://")))
  {
    PA = URL + os_strlen("http://");
  }
  else if (!os_strncmp(PA, "https://", os_strlen("https://")))
  {
    PA = URL + os_strlen("https://");
  }
  else
  {
    return -4;
  }
  /* 获取主页地址后面内容的首地址 */
  PB = os_strchr(PA, '/');
  if (PB)
  {
    /* 获取传进来网址的主页以及主页后面跟着的地址内容 */
    os_memcpy(host, PA, os_strlen(PA) - os_strlen(PB));
    if (PB + 1)
    {
      os_memcpy(filename, PB + 1, os_strlen(PB - 1));
      filename[os_strlen(PB) - 1] = 0;
    }
    host[os_strlen(PA) - os_strlen(PB)] = 0;
  }
  /* 如果传进来的网址是主页则直接获取主页地址内容 */
  else
  {
    os_memcpy(host, PA, os_strlen(PA));
    host[os_strlen(PA)] = 0;
  }
  return ESP_OK;
}

/** 
 * 解析获取得到的HTTPS的GET请求命令的内容
 * 根据不同的内容类型分别解析并打印出来
 * @param[in]   json_data:存放https的get请求有效的json数据   
 * @retval      null 
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/28, 初始化版本\n 
 */
static void https_get_reuest_json_data_parse(cJSON *json_data)
{
  cJSON *results = cJSON_GetObjectItem(json_data,"results");
  cJSON *location = cJSON_GetObjectItem(cJSON_GetArrayItem(results,0),"location");
  cJSON *daily = cJSON_GetObjectItem(cJSON_GetArrayItem(results,0),"daily");
  ESP_LOGI("", "city: %s\n%s\n%s\n%s\n",
           cJSON_Print(cJSON_GetObjectItem(location, "name")),
           cJSON_Print(cJSON_GetArrayItem(daily, 0)),
           cJSON_Print(cJSON_GetArrayItem(daily, 1)),
           cJSON_Print(cJSON_GetArrayItem(daily, 2)));
}
/** 
 * 解析获取得到的HTTPS的GET请求命令的内容关打印出来
 * 截取获取得到的相关内容
 * @param[in]   pvParameters:传入任务的值,这里传进来的get请求返回的json数据      
 * @retval      null 
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/28, 初始化版本\n 
 */
static void https_get_reuest_json_data_task(void *pvParameters)
{
  char *json_data = (char *)pvParameters;
  https_get_reuest_json_data_parse(cJSON_Parse(os_strchr(json_data, '{')));  
  os_free(json_data);
  vTaskDelete(NULL);
}


/** 
 * 建立TCP连接,并创建TLS环境,并执行HTTPS GET/POST请求
 * 必须先使用TLS创建好加密环境才能执行HTTPS的GET请求,否则强制执行GET请求会报400错误
 * @param[in]   method:
 *                  1.GET_REQ表示GET请求
 *                  2.POST_REQ表示POST请求        
 * @retval      
 *              0:成功
 *              其他:失败
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/27, 初始化版本\n 
 */
static int16_t tls_client_handle(uint8_t method)
{
  int ret, len;  
  recv_json_buff = (char*)os_malloc(sizeof(char)*1280);
  mbedtls_net_context *net_ctx = (mbedtls_net_context *)os_malloc(sizeof(mbedtls_net_context));
  mbedtls_entropy_context *entropy = (mbedtls_entropy_context *)os_malloc(sizeof(mbedtls_entropy_context));
  mbedtls_ctr_drbg_context *ctr_drbg = (mbedtls_ctr_drbg_context *)os_malloc(sizeof(mbedtls_ctr_drbg_context));
  mbedtls_ssl_context *ssl_ctx = (mbedtls_ssl_context *)os_malloc(sizeof(mbedtls_ssl_context));
  mbedtls_ssl_config *ssl_conf = (mbedtls_ssl_config *)os_malloc(sizeof(mbedtls_ssl_config));  
  /* 当需要进行CA证书认证时才需要定义 */
  // mbedtls_x509_crt ca_cert;

  /* 当需要进行双向认证时还需要定义 */
  // mbedtls_x509_crt client_cert;
  // mbedtls_pk_context pk_ctx;
  // uint32_t flags;

  /*
     * 0. Initialize the RNG and the session data
     */
  mbedtls_net_init(net_ctx);
  mbedtls_ssl_init(ssl_ctx);
  mbedtls_ssl_config_init(ssl_conf);
  /* 当需要进行CA证书认证时才需要初始化 */
  // mbedtls_x509_crt_init( ca_cert );
  /* 当需要进行双向认证时还需要初始化 */
  // mbedtls_x509_crt_init( client_cert );
  // mbedtls_pk_init( pk_ctx );
  mbedtls_ctr_drbg_init(ctr_drbg);

  /*
     * 0. Initialize the RNG and the session data
     */
  ESP_LOGI(TAG, "\n  . Seeding the random number generator...");

  mbedtls_entropy_init(entropy);
  if ((ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                                   (const unsigned char *)NULL,
                                   0)) != 0)
  {
    ESP_LOGI(TAG, "failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
    goto exit;
  }

  ESP_LOGI(TAG, " ok\n");
  /*
     * 1.1. Load the trusted CA
     * 只有当需要认证CA证书时，才会打开下面的注释,进行CA证书解析认证
     */
  // mbedtls_printf("  . Loading the CA root certificate ...");
  // load_buffer = Ssl_obj_load(default_cas_certificate, default_cas_certificate_len);
  // ret = mbedtls_x509_crt_parse(cacert, (const unsigned char *)load_buffer, default_cas_certificate_len);
  // os_free(load_buffer);
  // load_buffer = NULL;
  // if (ret < 0)
  // {
  //     mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
  //     goto exit;
  // }

  // mbedtls_printf(" ok (%d skipped)\n", ret);

  /*
     * 1.2. Load own certificate and private key
     * 只有当需要对Client的证书和秘钥进行自校验,才打开下面的注释
     * (can be skipped if client authentication is not required)
     */
  // os_printf("  . Loading the client cert. and key...\n");
  // load_buffer = Ssl_obj_load(default_cli_certificate, default_cli_certificate_len);
  // ret = mbedtls_x509_crt_parse(clicert, (const unsigned char *)load_buffer, default_cli_certificate_len);
  // free(load_buffer);
  // load_buffer = NULL;
  // if (ret != 0)
  // {
  //     mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
  //     goto exit;
  // }

  // load_buffer = Ssl_obj_load(default_cli_key, default_cli_key_len);
  // ret = mbedtls_pk_parse_key( pkey, (const unsigned char *) load_buffer, default_cli_key_len, NULL, 0 );
  // free(load_buffer);
  // load_buffer = NULL;
  // if( ret != 0 )
  // {
  //     mbedtls_printf( " failed\n  !  mbedtls_pk_parse_key returned -0x%x\n\n", -ret );
  //     goto exit;
  // }
  // mbedtls_printf( " ok (%d skipped)\n", ret );
  // os_free(load_buffer);

  /*
     * 2. Start the connection
     * 进行TCP连接
     */
  ESP_LOGI(TAG, "  . Connecting to tcp/%s/%s...", host, REMOTE_PORT);
  //fflush( stdout );

  if ((ret = mbedtls_net_connect(net_ctx, host,
                                 REMOTE_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
  {
    ESP_LOGI(TAG, " failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
    goto exit;
  }

  ESP_LOGI(TAG, " ok\n");

  /*
     * 3. Setup SSL/TLS相关参数
     */
  ESP_LOGI(TAG, "  . Setting up the SSL/TLS structure...");

  if ((ret = mbedtls_ssl_config_defaults(ssl_conf,
                                         MBEDTLS_SSL_IS_CLIENT,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
  {
    ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
    goto exit;
  }

  ESP_LOGI(TAG, " ok %d\n", esp_get_free_heap_size());

  /* 由于证书会过期,所以这些不进行证书认证 */
  mbedtls_ssl_conf_authmode(ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
  /* 只有需要进行CA证书认证才需要打开下面的注释 */
  // mbedtls_ssl_conf_ca_chain(ssl_conf, cacert, NULL);
  /* 只有需要进行双向认证才需要打开下面的注释 */
  // if ((ret = mbedtls_ssl_conf_own_cert(ssl_conf, clicert, pkey)) != 0)
  // {
  //     mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
  //     goto exit;
  // }
  /* 设置随机数生成的函数及方法 */
  mbedtls_ssl_conf_rng(ssl_conf, mbedtls_ctr_drbg_random, ctr_drbg);
  /* 这里不需要设置调试函数 */
  mbedtls_ssl_conf_dbg(ssl_conf, NULL, NULL);

  /* 将ssl_conf的相关信息填充于ssl_ctx中去,用于进行SSL握手时使用 */
  if ((ret = mbedtls_ssl_setup(ssl_ctx, ssl_conf)) != 0)
  {
    ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
    goto exit;
  }

  if ((ret = mbedtls_ssl_set_hostname(ssl_ctx, "mbed TLS Server")) != 0)
  {
    ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
    goto exit;
  }
  /* 设置发送以及接收的时候,调用的内部函数 */
  mbedtls_ssl_set_bio(ssl_ctx, net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

  /*
     * 4. Handshake
     */
  ESP_LOGI(TAG, "  . Performing the SSL/TLS handshake...");

  while ((ret = mbedtls_ssl_handshake(ssl_ctx)) != 0)
  {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
      goto exit;
    }
  }

  ESP_LOGI(TAG, " ok\n");

  /*
     * 5. Verify the server certificate
     * 当且仅当认证模式为MBEDTLS_SSL_VERIFY_OPTIONAL时,握手完成之后才打开下面的注释
     */
  // mbedtls_printf( "  . Verifying peer X.509 certificate..." );
  /* In real life, we probably want to bail out when ret != 0 */

  // if ((flags = mbedtls_ssl_get_verify_result(ssl_ctx)) != 0)
  // {
  //     char vrfy_buf[512];

  //     mbedtls_printf(" failed\n");

  //     mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

  //     mbedtls_printf("%s\n", vrfy_buf);
  // }
  // else
  //     mbedtls_printf(" ok\n");

  /*
     * 3. Write the GET request
     */
  ESP_LOGI(TAG, "  > Write to server:");
  memset(https_get_buffer, 0, sizeof(https_get_buffer));
  if (method == GET_REQ)
  {
    len = sprintf((char*)https_get_buffer, GET, filename, host);
  }
  else
  {
    len = sprintf((char*)https_get_buffer, POST, filename, strlen(POST_CONTENT), host, POST_CONTENT);
  }

  while ((ret = mbedtls_ssl_write(ssl_ctx, https_get_buffer, len)) <= 0)
  {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
      goto exit;
    }
  }

  len = ret;
  ESP_LOGI(TAG, " %d bytes written\n\n%s", len, (char *)https_get_buffer);

  /*
     * 7. Read the HTTP response
     */
  ESP_LOGI(TAG, "  < Read from server:");
  do
  {
    len = sizeof(recv_buff) - 1;
    memset(recv_buff, 0, sizeof(recv_buff));
    ret = mbedtls_ssl_read(ssl_ctx, recv_buff, len);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      continue;
    }

    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
      ESP_LOGI(TAG, "MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY\n");
      ret = 0;
      goto exit;
    }

    if (ret < 0)
    {
      ESP_LOGI(TAG, "failed\n  ! mbedtls_ssl_read returned %d\n\n", ret);
      break;
    }
    if (ret == 0)
    {
      ESP_LOGI(TAG, "\n\nEOF\n\n");      
      break;
    }
    else
    {
      len = ret;
      os_memcpy(recv_json_buff,recv_buff,len);
      // recv_json_buff = os_strchr((char *)recv_buff, '{');
      // ESP_LOGI(TAG, " %d bytes read\n\n%s", len, recv_json_buff);
      ret = xTaskCreate(https_get_reuest_json_data_task,
                        "https_get_reuest_json_data_task",
                        1024 * 4,
                        recv_json_buff,
                        3,
                        NULL);
      if(ret != pdPASS)
      {
        ESP_LOGI(TAG, "https_get_reuest_json_data_task create failure,reason is %d\n\n", ret);
      }
    }
  } while (1);
exit:
  mbedtls_ssl_close_notify(ssl_ctx);
  mbedtls_net_free(net_ctx);
  /* 当需要进行CA证书认证以及双向认证时才会机会释放 */
  // mbedtls_x509_crt_free( cacert );
  // mbedtls_x509_crt_free( clicert );
  // mbedtls_pk_free( pkey );
  mbedtls_ssl_free(ssl_ctx);
  mbedtls_ssl_config_free(ssl_conf);
  mbedtls_ctr_drbg_free(ctr_drbg);
  mbedtls_entropy_free(entropy);

  os_free(ssl_ctx);
  os_free(net_ctx);
  os_free(ctr_drbg);
  /* 当需要进行CA证书认证以及双向认证时才会机会释放 */
  // free(cacert);
  // free(clicert);
  // free(pkey);
  os_free(entropy);
  os_free(ssl_conf);
  return ret;
}

/** 
 * 发起HTTPS的GET请求
 * @param[in]   URL:HTTP的地址
 * @retval      0:成功
 *              -1:失败
 *              -2:URL解析失败
 *              -其他的值表示获取URL的IP失败
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/27, 初始化版本\n 
 */
int https_request_by_GET(char *URL)
{
  int ret = 0;
  
  /* 如果当前的状态不是STATION_GOT_IP,则直接返回 */  
  // if (SYSTEM_EVENT_STA_GOT_IP != wifi_station_get_connect_status())
  // {
  //   return -1;
  // }
  if (http_url_parse(URL, host, filename))
  {
    return -2;
  }  
  ESP_LOGI("https_request_by_GET",
  "URL is %s\nhost is %s\nfilename is %s\n", URL,host,filename);
  ret = tls_client_handle(GET_REQ);
  return ret;
}


/** 
 * 发起HTTPS的POST请求
 * @param[in]   URL:HTTP的地址
 * @retval      0:成功
 *              -1:失败
 *              -2:URL解析失败
 *              -其他的值表示获取URL的IP失败
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/27, 初始化版本\n 
 */
int https_request_by_POST(char *URL)
{
  int ret = 0;
  // /* 如果当前的状态不是STATION_GOT_IP,则直接返回 */
  // if (SYSTEM_EVENT_STA_GOT_IP != wifi_station_get_connect_status())
  // {
  //   return -1;
  // }
  if (http_url_parse(URL, host, filename))
  {
    return -2;
  }
  // ret = tls_client_handle(POST_REQ);
  ESP_LOGI("https_request_by_POST","ret is %d\n", ret);
  return ret;
}