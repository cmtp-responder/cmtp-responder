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

#ifndef _MTP_TRANSPORT_H_
#define _MTP_TRANSPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mtp_datatype.h"
#include "mtp_util.h"

/*
 * This structure specifies the status information of Mtp.
 */
typedef struct {
	mtp_int32 ctrl_event_code;
	mtp_state_t mtp_op_state;
	mtp_bool cancel_intialization;
	mtp_bool is_usb_discon;
} status_info_t;

typedef void (*_cmd_handler_cb)(mtp_char *buf, mtp_int32 pkt_len);

extern status_info_t *g_status;

void _transport_save_cmd_buffer(mtp_char *buffer, mtp_uint32 size);
mtp_err_t _transport_rcv_temp_file_data(mtp_byte *buffer, mtp_uint32 size,
		mtp_uint32 *count);
mtp_err_t _transport_rcv_temp_file_info(mtp_byte *buf, char *filepath,
		mtp_uint64 *t_size, mtp_uint32 filepath_len);
mtp_err_t _transport_send_event(mtp_byte *buf, mtp_uint32 size, mtp_uint32 *count);
mtp_uint32 _transport_send_pkt_to_tx_mq(const mtp_byte *buf, mtp_uint32 pkt_len);
mtp_uint32 _transport_send_bulk_pkt_to_tx_mq(const mtp_byte *buf,
		mtp_uint32 pkt_len);
mtp_bool _transport_init_interfaces(_cmd_handler_cb func);
void _transport_usb_finalize(void);
void _transport_init_status_info(void);

#ifdef __cplusplus
}
#endif

#endif /* _MTP_TRANSPORT_H_ */
