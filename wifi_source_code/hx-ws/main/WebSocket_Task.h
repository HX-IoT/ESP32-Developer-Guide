/**
 * @section License
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017, Thomas Barth, barth-dev.de
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifndef	_WEBSOCKET_TASK_H_
#define _WEBSOCKET_TASK_H_

#include "lwip/api.h"

#define WS_MASK_L		0x4		/**< \brief Length of MASK field in WebSocket Header*/


/** \brief Websocket frame header type*/
typedef struct {
	uint8_t opcode :WS_MASK_L;
	uint8_t reserved :3;
	uint8_t FIN :1;
	uint8_t payload_length :7;
	uint8_t mask :1;
} WS_frame_header_t;

/** \brief Websocket frame type*/
typedef struct{
	struct netconn* 	conenction;
	WS_frame_header_t	frame_header;
	size_t				payload_length;
	char*				payload;
}WebSocket_frame_t;


/**
 * \brief Send data to the websocket client
 *
 * \return 	#ERR_VAL: 	Payload length exceeded 2^7 bytes.
 * 			#ERR_CONN:	There is no open connection
 * 			#ERR_OK:	Header and payload send
 * 			all other values: derived from #netconn_write (sending frame header or payload)
 */
err_t WS_write_data(char* p_data, size_t length);

/**
 * \brief WebSocket Server task
 */
void ws_server(void *pvParameters);


#endif /* _WEBSOCKET_TASK_H_ */
