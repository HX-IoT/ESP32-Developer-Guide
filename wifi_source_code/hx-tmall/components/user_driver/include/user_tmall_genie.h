/** 
* @file         user_tmall_genie.h 
* @brief        声明接收天猫精灵控制命令以及其他相关的宏定义
* @details      实质上esp32模块最终的命令均来自于贝壳物联的云平台,因为贝壳物联已经跟天猫精灵已经对接好了
* @author       Helon_Chan 
* @par Copyright (c):  
*               红旭无线开发团队
* @par History:          
*               Ver0.0.1:
                    Helon_Chan, 2018/08/12, 初始化版本\n 
*/
#ifndef USER_TMALL_GENIE_H_
#define USER_TMALL_GENIE_H_


/*
===========================
宏定义
=========================== 
*/
#define BIG_IOT_URL                 "www.bigiot.net"
#define BIG_IOT_PORT                "8585"
#define TAG                         "big_iot_cloud_connect"

/* 设备ID,当使用时请更改为您自己的设备ID */
#define DEVICE_ID                   "7130"
/* 设备API KEY,当使用时请更改为您自己的设备API KEY */
#define DEVICE_API_KEY              "eb195935a"
/* 用户API KEY,当使用时请更改为您自己的用户API KEY*/
#define USER_API_KEY                "6dc19ba6a5"
/* 心跳包 */
#define HEART_BEAT                  "{\"M\":\"heart beat\"}\n"
  
/*
===========================
枚举变量定义
=========================== 
*/
enum
{
  INVALID_METHOD,                     ///< 无效的方法
  TCP_CONNECT_SUCCESS,                ///< tcp连接成功
  GET_TOKEN,                          ///< 获取token成功
  SIGN_IN_OK,                         ///< 设备登陆成功
  RECEVICE,                           ///< 接收云平台的命令
  ON,                                 ///< 打开
  OFF,                                ///< 关闭            
};

/*
===========================
函数声明
=========================== 
*/
/** 
* 通过TCP的方式连接贝壳物联的云平台接口
* @param[in]   url：表示贝壳物联的云平台接口地址
* @param[in]   port：表示贝壳物联云平台接口地址的端口
* @retval      null
* @note        修改日志 
*               Ver0.0.1: 
                  Helon_Chan, 2018/08/12, 初始化版本\n 
*/
void big_iot_cloud_connect(const char *url, const char *port);



#endif /* USER_TMALL_GENIE_H_ */