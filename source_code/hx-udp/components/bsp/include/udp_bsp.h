
#ifndef __UDP_BSP_H__
#define __UDP_BSP_H__



#ifdef __cplusplus
extern "C" {
#endif


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define UDP_SERVER_CLIENT_OPTION FALSE              //esp32作为client
#define TAG                     "HX-UDP"            //打印的tag

//client
//STA模式配置信息,即要连上的路由器的账号密码
#define GATEWAY_SSID            "Massky_AP"         //账号
#define GATEWAY_PAS             "ztl62066206"       //密码
// #define GATEWAY_SSID            "stop"               //账号
// #define GATEWAY_PAS             "11111111111"       //密码
#define UDP_ADRESS              "255.255.255.255"   //作为client，要连接UDP服务器的地址
                                                    //如果为255.255.255.255：UDP广播
                                                    //如果为192.168.169.205：UDP单播:根据自己的server写

#define UDP_PORT                12345               //统一的端口号，包括UDP客户端或者服务端


// FreeRTOS event group to signal when we are connected to wifi
#define WIFI_CONNECTED_BIT BIT0
extern EventGroupHandle_t udp_event_group;

//wifi作为sta
void wifi_init_sta();

//创建USP client
esp_err_t create_udp_client();

//收发数据
void recv_data(void *pvParameters);

//close all socket
void close_socket();

//get socket error code. return: error code
int get_socket_error_code(int socket);

//show socket error code. return: error code
int show_socket_error_reason(const char* str, int socket);

//check working socket
int check_working_socket();


#ifdef __cplusplus
}
#endif


#endif /*#ifndef __UDP_BSP_H__*/

