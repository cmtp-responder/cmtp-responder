/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _PTP_CONTAINER_H_
#define _PTP_CONTAINER_H_

#include "mtp_datatype.h"

#define MAX_MTP_PARAMS			5
#define MTP_USB_HEADER_LENGTH		12

typedef enum {
	CONTAINER_UNDEFINED = 0x00,
	CONTAINER_CMD_BLK,
	CONTAINER_DATA_BLK,
	CONTAINER_RESP_BLK,
	CONTAINER_EVENT_BLK
} container_type_t;

/*
 * A generic USB container header.
 */
typedef struct {
	mtp_uint32 len;		/* the valid container size in BYTES */
	mtp_uint16 type;	/* Container type */
	mtp_uint16 code;	/* Operation, response or Event code */
	mtp_uint32 tid;		/* host generated transaction id */
} header_container_t;

/*
 * A generic USB container.
 */
typedef struct {
	mtp_uint32 len;		/* the valid container size in BYTES */
	mtp_uint16 type;	/* Container type */
	mtp_uint16 code;	/* Operation, response or Event code */
	mtp_uint32 tid;		/* host generated transaction id */
	mtp_uint32 params[MAX_MTP_PARAMS];
	mtp_uint32 no_param;
} cmd_container_t;

/*
 * A USB container for sending data.
 */
typedef struct {
	mtp_uint32 len;		/* the valid container size in BYTES */
	mtp_uint16 type;	/* Container type */
	mtp_uint16 code;	/* Operation, response or Event code */
	mtp_uint32 tid;		/* host generated transaction id */
	mtp_uint32 params[MAX_MTP_PARAMS];
	mtp_uint32 no_param;
	mtp_uchar *data;
} data_container_t;

typedef cmd_container_t cmd_blk_t;
typedef cmd_container_t resp_blk_t;
typedef data_container_t data_blk_t;

void _hdlr_init_cmd_container(cmd_container_t *cntr);
mtp_uint32 _hdlr_get_param_cmd_container(cmd_container_t *cntr,
		mtp_uint32 index);
void _hdlr_copy_cmd_container_unknown_params(cmd_container_t *src,
		cmd_container_t *dst);
void _hdlr_copy_cmd_container(cmd_container_t *src, cmd_container_t *dst);
mtp_bool _hdlr_add_param_resp_container(resp_blk_t *dst, mtp_uint32 num,
		mtp_uint32 *params);
mtp_bool _hdlr_validate_cmd_container(mtp_uchar *blk, mtp_uint32 size);
void _hdlr_init_data_container(data_container_t *dst, mtp_uint16 code,
		mtp_uint32 trans_id);
mtp_uchar *_hdlr_alloc_buf_data_container(data_container_t *dst,
		mtp_uint32 bufsz, mtp_uint64 pkt_size);
mtp_bool _hdlr_send_data_container(data_container_t *dst);
mtp_bool _hdlr_send_bulk_data(mtp_uchar *dst, mtp_uint32 len);
mtp_bool _hdlr_rcv_data_container(data_container_t *dst, mtp_uint32 size);
mtp_bool _hdlr_rcv_file_in_data_container(data_container_t *dst,
		mtp_char *filepath, mtp_uint32 path_len);
mtp_uint32 _hdlr_get_payload_size(data_container_t *dst);
mtp_uchar *_hdlr_get_payload_data(data_container_t *dst);
void _hdlr_resp_container_init(cmd_container_t *dst, mtp_uint16 resp_code,
		mtp_uint32 tid);
mtp_bool _hdlr_send_resp_container(cmd_container_t *dst);
void _hdlr_init_event_container(cmd_container_t *dst, mtp_uint16 code,
		mtp_uint32 tid, mtp_uint32 param1, mtp_uint32 param2);
void _hdlr_init_event_container_with_param(cmd_container_t *dst,
		mtp_uint16 code, mtp_uint32 tid, mtp_uint32 param1, mtp_uint32 param2);
mtp_bool _hdlr_send_event_container(cmd_container_t *dst);
void _hdlr_conv_cmd_container_byte_order(cmd_container_t *dst);
void _hdlr_conv_data_container_byte_order(data_container_t *dst);

#endif	/* _PTP_CONTAINER_H_ */
