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

#ifndef _MTP_MSGQ_H_
#define _MTP_MSGQ_H_

#include "mtp_datatype.h"
#include "mtp_util.h"

typedef mtp_int32 msgq_id_t;

typedef struct {
	msg_type_t mtype;
	mtp_uint32 length;
	mtp_uint32 signal;
	unsigned char *buffer;
} msgq_ptr_t;

mtp_bool _util_msgq_init(msgq_id_t *mq_id, mtp_uint32 flags);
mtp_bool _util_msgq_send(msgq_id_t mq_id, void *buf, mtp_uint32 size,
		mtp_uint32 flags);
mtp_bool _util_msgq_receive(msgq_id_t mq_id, void *buf, mtp_uint32 size,
		mtp_uint32 flags, mtp_int32 *nbytes);
mtp_bool _util_msgq_deinit(msgq_id_t *msgq_id);
mtp_bool _util_msgq_set_size(msgq_id_t mq_id, mtp_uint32 nbytes);
mtp_bool _util_rcv_msg_from_mq(msgq_id_t mq_id, unsigned char **pkt,
		mtp_uint32 *pkt_len, msg_type_t *mtype);

#endif/*_MTP_MSGQ_H_*/
