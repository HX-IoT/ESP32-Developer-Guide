/*
* @file         WebSocket_Task.c 
* @brief        websocket server建立和数据接收
* @details      源码来源，http://www.barth-dev.de/websockets-on-the-esp32
* @author       hx-zsj 
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
* @par History:          
*               Ver0.0.1:
                     hx-zsj, 2018/11/22, 初始化版本\n 
*/

/* 
=============
头文件包含
=============
*/
#include "WebSocket_Task.h"

#include "freertos/FreeRTOS.h"
#include "esp_heap_alloc_caps.h"
#include "hwcrypto/sha.h"
#include "esp_system.h"
#include "wpa2/utils/base64.h"
#include <string.h>
#include <stdlib.h>

#define WS_PORT				9998	/*server tcp端口*/
#define WS_CLIENT_KEY_L		24		/*client key 长度*/
#define SHA1_RES_L			20		/*SHA1 result*/

#define WS_STD_LEN			125		/*数据帧长度*/
#define WS_SPRINTF_ARG_L	4		/*sprintf长度*/

//帧类型
typedef enum {
	WS_OP_CON = 0x0, 				/*!< Continuation Frame*/
	WS_OP_TXT = 0x1, 				/*!< Text Frame*/
	WS_OP_BIN = 0x2, 				/*!< Binary Frame*/
	WS_OP_CLS = 0x8, 				/*!< Connection Close Frame*/
	WS_OP_PIN = 0x9, 				/*!< Ping Frame*/
	WS_OP_PON = 0xa 				/*!< Pong Frame*/
} WS_OPCODES;

//接收数据队列
extern QueueHandle_t WebSocket_rx_queue;

//websocket connect 句柄
static struct netconn* WS_conn = NULL;

//websocket关键参数
const char WS_sec_WS_keys[] = "Sec-WebSocket-Key:";
const char WS_sec_conKey[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char WS_srv_hs[] ="HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.*s\r\n\r\n";

/*
* websocket发送数据
* @param[in]   p_data  		       :数据指针
* @param[in]   length  		       :数据长度
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/11/22, 初始化版本\n 
*/
err_t WS_write_data(char* p_data, size_t length) {

	//websocket未连接，直接退出
	if (WS_conn == NULL)
		return ERR_CONN;
	//数据帧长度溢出，直接退出
	if (length > WS_STD_LEN)
		return ERR_VAL;
	err_t result;
	//报头
	WS_frame_header_t hdr;
	hdr.FIN = 0x1;
	hdr.payload_length = length;
	hdr.mask = 0;
	hdr.reserved = 0;
	hdr.opcode = WS_OP_TXT;
	//发送报头
	result = netconn_write(WS_conn, &hdr, sizeof(WS_frame_header_t), NETCONN_COPY);
	if (result != ERR_OK)
		return result;
	//发送数据
	return netconn_write(WS_conn, p_data, length, NETCONN_COPY);
}
/*
* websocket server 连接、握手、数据读取
* @param[in]   conn  		       :websocket connect句柄
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/11/22, 初始化版本\n 
*/
static void ws_server_netconn_serve(struct netconn *conn) {

	//Netbuf
	struct netbuf *inbuf;
	//数据包
	char *buf;
	//pointer to buffer (multi purpose)
	char* p_buf;
	//Pointer to SHA1 input
	char* p_SHA1_Inp;
	//Pointer to SHA1 result
	char* p_SHA1_result;
	//multi purpose number buffer
	uint16_t i;
	//will point to payload (send and receive
	char* p_payload;
	//Frame header pointer
	WS_frame_header_t* p_frame_hdr;
	//申请SHA1
	p_SHA1_Inp = pvPortMallocCaps(WS_CLIENT_KEY_L + sizeof(WS_sec_conKey),
			MALLOC_CAP_8BIT);
	//申请SHA1 result
	p_SHA1_result = pvPortMallocCaps(SHA1_RES_L, MALLOC_CAP_8BIT);
	//Check if malloc suceeded
	if ((p_SHA1_Inp != NULL) && (p_SHA1_result != NULL)) {

		//接收“连接”过程的数据
		if (netconn_recv(conn, &inbuf) == ERR_OK) {
			//读取“连接”过程的数据到buf
			netbuf_data(inbuf, (void**) &buf, &i);
			//把server的key传给SHA1
			for (i = 0; i < sizeof(WS_sec_conKey); i++)
			{
				//放在后24字节
				p_SHA1_Inp[i + WS_CLIENT_KEY_L] = WS_sec_conKey[i];
			}
			//搜索client的key
			p_buf = strstr(buf, WS_sec_WS_keys);
			//找到key
			if (p_buf != NULL) {
				//get Client Key
				for (i = 0; i < WS_CLIENT_KEY_L; i++)
				{
					//放在前24字节
					p_SHA1_Inp[i] = *(p_buf + sizeof(WS_sec_WS_keys) + i);
				}
				// 计算 hash
				esp_sha(SHA1, (unsigned char*) p_SHA1_Inp, strlen(p_SHA1_Inp),
						(unsigned char*) p_SHA1_result);
				//转base64
				p_buf = (char*) _base64_encode((unsigned char*) p_SHA1_result,
						SHA1_RES_L, (size_t*) &i);
				//free SHA1 input
				free(p_SHA1_Inp);
				//free SHA1 result
				free(p_SHA1_result);
				//申请“握手”内存
				p_payload = pvPortMallocCaps(
						sizeof(WS_srv_hs) + i - WS_SPRINTF_ARG_L,
						MALLOC_CAP_8BIT);
				if (p_payload != NULL) {
					//准备“握手”帧
					sprintf(p_payload, WS_srv_hs, i - 1, p_buf);
					//发送“握手”帧
					netconn_write(conn, p_payload, strlen(p_payload),NETCONN_COPY);
					//free base64
					free(p_buf);
					//free “握手”内存
					free(p_payload);
					//websocket连接成功
					WS_conn = conn;
					//“接收数据”
					while (netconn_recv(conn, &inbuf) == ERR_OK) {
						//读取数据到buf
						netbuf_data(inbuf, (void**) &buf, &i);
						//扔到p_frame_hdr
						p_frame_hdr = (WS_frame_header_t*) buf;
						//此帧是“连接关闭”帧，直接退出
						if (p_frame_hdr->opcode == WS_OP_CLS)
							break;
						//有效数据帧长度判断
						if (p_frame_hdr->payload_length <= WS_STD_LEN) {
							//数据扔到p_buf
							p_buf = (char*) &buf[sizeof(WS_frame_header_t)];
							//check if content is masked
							if (p_frame_hdr->mask) {
								//申请内存
								p_payload = pvPortMallocCaps(
										p_frame_hdr->payload_length + 1,
										MALLOC_CAP_8BIT);
								//申请内存成功
								if (p_payload != NULL) {
									//解码
									for (i = 0; i < p_frame_hdr->payload_length;
											i++)
										p_payload[i] = (p_buf + WS_MASK_L)[i]
												^ p_buf[i % WS_MASK_L];
											
									//加个尾巴
									p_payload[p_frame_hdr->payload_length] = 0;
								}
							} else{
								//content is not masked
								p_payload = p_buf;
							}

							//有效数据
							if ((p_payload != NULL)	&& (p_frame_hdr->opcode == WS_OP_TXT)) {
								//组包
								WebSocket_frame_t __ws_frame;
								__ws_frame.conenction=conn;
								__ws_frame.frame_header=*p_frame_hdr;
								__ws_frame.payload_length=p_frame_hdr->payload_length;
								__ws_frame.payload=p_payload;

								//发送给另一个任务解析
								xQueueSendFromISR(WebSocket_rx_queue,&__ws_frame,0);
							}
							//free payload buffer (in this demo done by the receive task)
//							if (p_frame_hdr->mask && p_payload != NULL)
//								free(p_payload);
						} //数据中超长
						//清空buf
						netbuf_delete(inbuf);
					} //有效数据读取失败
				} //握手内存申请失败
			} //连接过程无key
		} //连接数据读取失败
	} //p_SHA1_Inp!=NULL&p_SHA1_result!=NULL

	//清空连接
	WS_conn = NULL;
	//清空buf
	netbuf_delete(inbuf);
	//关闭websocket server connect
	netconn_close(conn);
	netconn_delete(conn);

}

/*
* websocket server 建立服务
* @param[in]   conn  		       :websocket connect句柄
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/11/22, 初始化版本\n 
*/
void ws_server(void *pvParameters) 
{
	struct netconn *conn, *newconn;
	//获取tcp socket connect
	conn = netconn_new(NETCONN_TCP);
	//绑定port
	netconn_bind(conn, NULL, WS_PORT);
	//监听
	netconn_listen(conn);
	//等待client连接
	while (netconn_accept(conn, &newconn) == ERR_OK)
	{
		//新连接：等待连接、连接过程、数据读取
		ws_server_netconn_serve(newconn);
	}
	//关闭websocket server connect
	netconn_close(conn);
	netconn_delete(conn);
}
