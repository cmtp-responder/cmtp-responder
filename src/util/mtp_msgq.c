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

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "mtp_msgq.h"

/*
 * FUNCTIONS
 */
mtp_bool _util_msgq_init(msgq_id_t *mq_id, mtp_uint32 flags)
{
	*mq_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
	if (*mq_id == -1) {
		ERR("msgget() Fail");
		_util_print_error();
		return FALSE;
	}
	return TRUE;
}

mtp_bool _util_msgq_send(msgq_id_t mq_id, void *buf, mtp_uint32 size,
		mtp_uint32 flags)
{
	if (-1 == msgsnd(mq_id, buf, size, flags)) {
		ERR("msgsnd() Fail : mq_id = [%d]", mq_id);
		_util_print_error();
		return FALSE;
	}
	return TRUE;
}

mtp_bool _util_msgq_receive(msgq_id_t mq_id, void *buf, mtp_uint32 size,
		mtp_uint32 flags, mtp_int32 *nbytes)
{
	int ret = 0;

	if (flags == 1) {
		ret = msgrcv(mq_id, buf, size, 0, IPC_NOWAIT);
	} else {
		ret = msgrcv(mq_id, buf, size, 0, 0);
	}

	if (ret == -1) {
		ERR("msgrcv() Fail");
		_util_print_error();
		*nbytes = 0;
		return FALSE;
	}

	*nbytes = ret;
	return TRUE;
}

mtp_bool _util_msgq_deinit(msgq_id_t *msgq_id)
{
	if (-1 == msgctl(*msgq_id, IPC_RMID, 0)) {
		ERR("msgctl(IPC_RMID) Fail");
		return FALSE;
	}
	return TRUE;
}

mtp_bool _util_msgq_set_size(msgq_id_t mq_id, mtp_uint32 nbytes)
{
	struct msqid_ds attr;

	/* Getting the attributes of Message Queue */
	if (msgctl(mq_id, MSG_STAT, &attr) == -1) {
		ERR("msgctl(MSG_STAT) Fail");
		return FALSE;
	}

	attr.msg_qbytes = nbytes;

	/* Setting the message queue size */
	if (msgctl(mq_id, IPC_SET, &attr) == -1) {
		ERR("msgctl(IPC_SET) Fail");
		return FALSE;
	}
	return TRUE;
}

/*
 * _util_rcv_msg_from_mq
 *
 * This function receives a message pointer from Message Queue
 * @param[in]	mq_id	Message Queue Id
 * @param[out]	pkt	Shall be assigned with received buffer ptr
 * @param[out]	pkt_len	Will hold the length of received data
 * @param[out]	mtype	Will be assigned the type of message received
 */
mtp_bool _util_rcv_msg_from_mq(msgq_id_t mq_id, unsigned char **pkt,
		mtp_uint32 *pkt_len, msg_type_t *mtype)
{
	msgq_ptr_t msg = { 0 };
	mtp_int32 nbytes = 0;

	if (FALSE == _util_msgq_receive(mq_id, (void *)&msg,
				sizeof(msgq_ptr_t) - sizeof(long), 0, &nbytes)) {
		ERR("_util_msgq_receive() Fail : mq_id = [%d]", mq_id);
		return FALSE;
	}

	*pkt_len = msg.length;
	*pkt = msg.buffer;
	*mtype = msg.mtype;

	return TRUE;
}
