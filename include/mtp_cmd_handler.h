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

#ifndef _MTP_CMD_HANDLER_H_
#define _MTP_CMD_HANDLER_H_

#include "mtp_object.h"
#include "mtp_device.h"
#include "ptp_container.h"
#include "mtp_event_handler.h"

#ifdef MTP_SUPPORT_PRINT_COMMAND
#define MTP_MAX_CMD_LENGTH 40
#endif /* MTP_SUPPORT_PRINT_COMMAND */

/*
 * A structure for containing the object infomation. It is used for saving
 * information in between paired operations like send_obj_info
 * and send_object.
 * @see obj_info
 */
typedef struct {
	mtp_uint32 store_id;	/* The storage ID. */
	mtp_uint32 h_parent;	/* Handle of the parent object */
	/* Reserved Handle for the incoming object. */
	mtp_uint32 obj_handle;
	/* Indicates if the ObjectInfo dataset is valid. */
	mtp_bool is_valid;
	mtp_bool is_data_sent;	/* Indicates whether data was actually sent */
	mtp_uint64 file_size;
	mtp_obj_t *obj;		/* Object containing info for SendObject */
} data_4send_object_t;

/*
 * A structure for handling the operations of the MTP protocol.
 * It is the main driving structure behind the MTP protocol handling. The
 * MTP device contains device information and other data buffers.
 *
 * @see mtp_device
 * @see cmd_blk_t
 * @see data4_send_obj
 */
typedef struct {
	cmd_container_t usb_cmd;	/* A USB command container block. */
	/* Data saved during paired operations: SendObjectInfo/SendObject. */
	data_4send_object_t data4_send_obj;
	mtp_uint32 session_id;		/* Current session ID. */
	mtp_uint32 last_opcode;		/* Current transaction ID. */
	mtp_uint16 last_fmt_code;	/* Last Format Code */
} mtp_handler_t;

typedef struct {
	mtp_uint32 recent_tid;
	mtp_uint32 cur_index;
	mtp_uint32 mod;
} wmp_meta_info_t;

/*
 * This structure specifies the buffer of Mtp.
 */
typedef struct {
	mtp_char cmd_buf[MTP_MAX_CMD_BLOCK_SIZE];
	mtp_char header_buf[MTP_USB_HEADER_LENGTH];
	mtp_uint32 cmd_size;
	mtp_uint32 data_size;
	mtp_uint32 data_count;
	mtp_uint32 fhandle;	/* for temporary mtp file */
	mtp_char *filepath;
	mtp_uint32 file_size;
	mtp_uint32 size_remaining;
	/* PC-> Device file transfer user space buffering till 512K*/
	mtp_char *temp_buff;
} temp_file_struct_t;

typedef struct {
	temp_file_struct_t ftemp_st;
	mtp_handler_t hdlr;
	wmp_meta_info_t meta_info;
} mtp_mgr_t;

void _cmd_hdlr_reset_cmd(mtp_handler_t *hdlr);
mtp_bool _cmd_hdlr_send_response(mtp_handler_t *hdlr, mtp_uint16 resp,
		mtp_uint32 num_param, mtp_uint32 *params);
mtp_bool _cmd_hdlr_send_response_code(mtp_handler_t *hdlr, mtp_uint16 resp);
void _receive_mq_data_cb(mtp_char *buffer, mtp_int32 buf_len);

#endif /* _MTP_CMD_HANDLER_H_ */
