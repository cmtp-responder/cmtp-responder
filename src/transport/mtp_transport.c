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

#include <unistd.h>
#include <glib.h>
#include "mtp_config.h"
#include "mtp_transport.h"
#include "mtp_support.h"
#include "mtp_util.h"
#include "ptp_datacodes.h"
#include "mtp_device.h"
#include "mtp_msgq.h"
#include "mtp_cmd_handler.h"
#include "mtp_thread.h"
#include "mtp_usb_driver.h"

/*
 * GLOBAL AND EXTERN VARIABLES
 */
extern mtp_config_t g_conf;
extern mtp_mgr_t g_mtp_mgr;

/*
 * STATIC VARIABLES
 */
static mtp_mgr_t *g_mgr = &g_mtp_mgr;
static mtp_bool g_usb_threads_created = FALSE;
static pthread_t g_tx_thrd = 0;
static pthread_t g_rx_thrd = 0;
static pthread_t g_data_rcv = 0;
static msgq_id_t mtp_to_usb_mqid;
static msgq_id_t g_usb_to_mtp_mqid;
static status_info_t g_status;

/*
 * STATIC FUNCTIONS
 */
static void *__transport_thread_data_rcv(void *func);
static mtp_err_t __transport_init_io();
static void __transport_deinit_io();

/*
 * FUNCTIONS
 */
void _transport_save_cmd_buffer(mtp_char *buffer, mtp_uint32 size)
{
	memcpy(g_mgr->ftemp_st.cmd_buf, buffer, size);
	g_mgr->ftemp_st.cmd_size = size;
	g_mgr->ftemp_st.data_count = 0;
	return;
}

mtp_err_t _transport_rcv_temp_file_data(mtp_byte *buffer, mtp_uint32 size,
		mtp_uint32 *count)
{
	mtp_uint32 h_file = INVALID_FILE;
	mtp_int32 error = 0;
	mtp_uint32 data_sz;


	h_file = _util_file_open(g_mgr->ftemp_st.filepath,
			MTP_FILE_READ, &error);
	if (h_file == INVALID_FILE) {
		DBG_SECURE("_util_file_open(%s) Fail", g_mgr->ftemp_st.filepath);
		return MTP_ERROR_NONE;
	}

	/*copy header */
	memcpy(buffer, g_mgr->ftemp_st.header_buf,
			sizeof(header_container_t));

	/*copy body packet */
	data_sz = size - sizeof(header_container_t);
	_util_file_read(h_file, &buffer[sizeof(header_container_t)],
			data_sz, count);
	if (*count <= 0) {
		ERR("file read error expected [%u] actual [%u]\n",
				data_sz, *count);
	}

	*count += sizeof(header_container_t);

	if (_util_file_close(h_file) != 0) {
		ERR("_util_file_close Fail");
		_util_print_error();
	}

	/* delete temp file, it have to be called in receive_data fn */
	if (remove(g_mgr->ftemp_st.filepath) < 0) {
		ERR_SECURE("remove(%s) Fail", g_mgr->ftemp_st.filepath);
		_util_print_error();
	}

	g_mgr->ftemp_st.data_size = 0;
	g_mgr->ftemp_st.data_count = 0;

	return MTP_ERROR_NONE;
}

mtp_err_t _transport_rcv_temp_file_info(mtp_byte *buf, char *filepath,
		mtp_uint64 *t_size, mtp_uint32 filepath_len)
{
	file_attr_t atttrs = { 0 };
	mtp_bool ret = FALSE;

	memcpy(buf, g_mgr->ftemp_st.header_buf,
			sizeof(header_container_t));

	ret = _util_get_file_attrs(g_mgr->ftemp_st.filepath, &atttrs);
	if (FALSE == ret) {
		ERR_SECURE("_util_get_file_attrs(%s) Fail", g_mgr->ftemp_st.filepath);
		return MTP_ERROR_GENERAL;
	}

	*t_size = sizeof(header_container_t) + atttrs.fsize;
	g_strlcpy(filepath, g_mgr->ftemp_st.filepath, filepath_len);

	g_mgr->ftemp_st.data_size = 0;
	g_mgr->ftemp_st.data_count = 0;

	g_strlcpy(g_mgr->ftemp_st.filepath, MTP_TEMP_FILE_DEFAULT,
			MTP_MAX_PATHNAME_SIZE + 1);
	g_mgr->ftemp_st.fhandle = INVALID_FILE;
	g_mgr->ftemp_st.file_size = 0;

	return MTP_ERROR_NONE;
}

mtp_err_t _transport_send_event(mtp_byte *buf, mtp_uint32 size,
		mtp_uint32 *count)
{
	mtp_bool resp = FALSE;
	msgq_ptr_t pkt = { 0 };

	retv_if(buf == NULL, MTP_ERROR_INVALID_PARAM);
	retvm_if(size > _get_tx_pkt_size(), MTP_ERROR_INVALID_PARAM,
			"size = %d, _get_tx_pkt_size() = (%d)", size, _get_tx_pkt_size());

	pkt.mtype = MTP_EVENT_PACKET;
	pkt.signal = 0x0000;
	pkt.length = size;

	pkt.buffer = (mtp_uchar *)g_malloc(size);
	if (NULL == pkt.buffer) {
		ERR("g_malloc() Fail");
		return MTP_ERROR_GENERAL;
	}
	memcpy(pkt.buffer, buf, size);
	resp = _util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
			sizeof(msgq_ptr_t) - sizeof(long), 0);
	if (resp == FALSE) {
		ERR("_util_msgq_send() Fail");
		return MTP_ERROR_GENERAL;
	}

	*count = size;
	return MTP_ERROR_NONE;
}

/*
 * This function writes data to Message Queue, which will be read by a thread.
 * the thread will write message, read from MQ, on USB.
 * @param	buf		[in] A pointer to data written.
 * @param	pkt_len		[in] Specifies the number of bytes to write.
 * @return	This function returns length of written data in bytes
 */
mtp_uint32 _transport_send_pkt_to_tx_mq(const mtp_byte *buf,
		mtp_uint32 pkt_len)
{
	mtp_bool ret = 0;
	mtp_uint32 len = pkt_len;
	mtp_uint32 sent_len = 0;
	msgq_ptr_t pkt = { 0 };
	mtp_uint32 tx_size = _get_tx_pkt_size();
	const mtp_uchar *temp = (const mtp_uchar *)buf;

	retv_if(buf == NULL, 0);
	retv_if(pkt_len == 0, 0);

	pkt.mtype = MTP_DATA_PACKET;
	pkt.signal = 0x0000;

	while (len) {
		sent_len = len < tx_size ? len : tx_size;

		pkt.length = sent_len;
		pkt.buffer = (mtp_uchar *)g_malloc(sent_len);
		if (NULL == pkt.buffer) {
			ERR("g_malloc() Fail");
			return 0;
		}

		memcpy(pkt.buffer, temp, sent_len);
		ret = _util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
				sizeof(msgq_ptr_t) - sizeof(long), 0);
		if (ret == FALSE) {
			ERR("_util_msgq_send() Fail");
			g_free(pkt.buffer);
			return 0;
		}

		len -= sent_len;
		temp += sent_len;
	}

	return pkt_len;
}

mtp_uint32 _transport_send_bulk_pkt_to_tx_mq(const mtp_byte *buf,
		mtp_uint32 pkt_len)
{
	mtp_uint32 sent_len = 0;
	mtp_uint32 tx_size = _get_tx_pkt_size();
	msgq_ptr_t pkt = {MTP_BULK_PACKET, 0, 0, NULL};

	retv_if(buf == NULL, 0);
	retv_if(pkt_len == 0, 0);

	pkt.length = tx_size;
	while (pkt_len > tx_size) {
		pkt.buffer = (mtp_uchar *)g_malloc(pkt.length);
		if (NULL == pkt.buffer) {
			ERR("g_malloc() Fail");
			return 0;
		}
		memcpy(pkt.buffer, &buf[sent_len], pkt.length);

		if (!_util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
					sizeof(msgq_ptr_t) - sizeof(long), 0)) {
			ERR("_util_msgq_send() Fail");
			g_free(pkt.buffer);
			return 0;
		}

		pkt_len -= pkt.length;
		sent_len += pkt.length;
	}

	pkt.length = pkt_len;
	pkt.buffer = (mtp_uchar *)g_malloc(pkt.length);
	if (NULL == pkt.buffer) {
		ERR("g_malloc() Fail");
		return 0;
	}
	memcpy(pkt.buffer, &buf[sent_len], pkt.length);

	if (!_util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
				sizeof(msgq_ptr_t) - sizeof(long), 0)) {
		ERR("_util_msgq_send() Fail");
		g_free(pkt.buffer);
		return 0;
	}
	sent_len += pkt.length;

	return sent_len;
}

void _transport_send_zlp(void)
{
	msgq_ptr_t pkt = { 0 };
	mtp_bool resp = FALSE;

	pkt.mtype = MTP_ZLP_PACKET;
	pkt.signal = 0x0000;
	pkt.length = 0;
	pkt.buffer = NULL;

	resp = _util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
			sizeof(msgq_ptr_t) - sizeof(long), 0);
	if (resp == FALSE)
		ERR("_util_msgq_send() Fail");
	return;
}

static mtp_err_t __transport_init_io()
{
	mtp_int32 res = 0;
	thread_func_t usb_write_thread = _transport_thread_usb_write;
	thread_func_t usb_read_thread = _transport_thread_usb_read;

	res = _util_thread_create(&g_tx_thrd, "usb write thread",
			PTHREAD_CREATE_JOINABLE, usb_write_thread,
			(void *)&mtp_to_usb_mqid);
	if (FALSE == res) {
		ERR("_util_thread_create(TX) Fail");
		goto cleanup;
	}

	res = _util_thread_create(&g_rx_thrd, "usb read thread",
			PTHREAD_CREATE_JOINABLE, usb_read_thread,
			(void *)&g_usb_to_mtp_mqid);
	if (FALSE == res) {
		ERR("_util_thread_create(RX) Fail");
		goto cleanup;
	}

	g_usb_threads_created = TRUE;

	return MTP_ERROR_NONE;

cleanup:
	_util_print_error();

	if (g_rx_thrd) {
		res = _util_thread_cancel(g_rx_thrd);
		DBG("pthread_cancel [%d]\n", res);
		g_rx_thrd = 0;
	}
	if (g_tx_thrd) {
		res = _util_thread_cancel(g_tx_thrd);
		DBG("pthread_cancel [%d]\n", res);
		g_tx_thrd = 0;
	}
	g_usb_threads_created = FALSE;

	return MTP_ERROR_GENERAL;
}

static void __transport_deinit_io()
{
	if (g_usb_threads_created == FALSE) {
		ERR("io threads are not created.");
		return;
	}
	errno = 0;

	if (FALSE == _util_thread_cancel(g_rx_thrd))
		ERR("_util_thread_cancel(rx) Fail");

	if (_util_thread_join(g_rx_thrd, 0) == FALSE)
		ERR("_util_thread_join(rx) Fail");

	g_rx_thrd = 0;

	if (FALSE == _util_thread_cancel(g_tx_thrd))
		ERR("_util_thread_cancel(tx) Fail");

	if (_util_thread_join(g_tx_thrd, 0) == FALSE)
		ERR("_util_thread_join(tx) Fail");

	g_tx_thrd = 0;

	g_usb_threads_created = FALSE;
	return;
}

mtp_bool _transport_init_interfaces(_cmd_handler_cb func)
{
	mtp_int32 res = 0;
	mtp_bool ret = FALSE;

	ret = _transport_init_usb_device();
	if (ret == FALSE) {
		/* mtp driver open failed */
		ERR("_transport_init_usb_device() Fail");
		return FALSE;
	}

	if (_transport_mq_init(&g_usb_to_mtp_mqid, &mtp_to_usb_mqid) == FALSE) {
		ERR("_transport_mq_init() Fail");
		_transport_deinit_usb_device();
		return FALSE;
	}

	if (__transport_init_io() != MTP_ERROR_NONE) {
		ERR("__transport_init_io() Fail");
		_transport_mq_deinit(&g_usb_to_mtp_mqid, &mtp_to_usb_mqid);
		_transport_deinit_usb_device();
		return FALSE;
	}

	res = _util_thread_create(&g_data_rcv, "Data Receive thread",
			PTHREAD_CREATE_JOINABLE, __transport_thread_data_rcv,
			(void *)func);
	if (res == FALSE) {
		ERR("_util_thread_create(data_rcv) Fail");
		__transport_deinit_io();
		_transport_mq_deinit(&g_usb_to_mtp_mqid, &mtp_to_usb_mqid);
		_transport_deinit_usb_device();
		return FALSE;
	}

	return TRUE;
}

void _transport_usb_finalize(void)
{
	mtp_int32 res = 0;
	void *th_result = NULL;
	msgq_ptr_t pkt;
	mtp_uint32 rx_size = _get_rx_pkt_size();

	__transport_deinit_io();

	if (g_data_rcv != 0) {
		pkt.buffer = (mtp_uchar *)g_malloc(rx_size);
		if (pkt.buffer == NULL) {
			ERR("g_malloc() Fail");
			return;
		}
		pkt.mtype = MTP_DATA_PACKET;
		pkt.signal = 0xABCD;
		pkt.length = 6;
		memset(pkt.buffer, 0, rx_size);
		if (FALSE == _util_msgq_send(g_usb_to_mtp_mqid, (void *)&pkt,
					sizeof(msgq_ptr_t) - sizeof(long), 0)) {
			ERR("_util_msgq_send() Fail");
		}

		res = _util_thread_join(g_data_rcv, &th_result);
		if (res == FALSE) {
			ERR("_util_thread_join(data_rcv) Fail");
		}
	}

	if (_transport_mq_deinit(&g_usb_to_mtp_mqid, &mtp_to_usb_mqid) == FALSE) {
		ERR("_transport_mq_deinit() Fail");
	}

	_transport_deinit_usb_device();

	return;
}

static void *__transport_thread_data_rcv(void *func)
{
	msgq_ptr_t pkt = { 0 };
	mtp_uchar *pkt_data = NULL;
	mtp_uint32 pkt_len = 0;
	mtp_int32 flag = 1;
	mtp_int32 len = 0;
	_cmd_handler_cb _cmd_handler_func = (_cmd_handler_cb )func;

	while (flag) {
		if (_util_msgq_receive(g_usb_to_mtp_mqid, (void *)&pkt,
					sizeof(pkt) - sizeof(long), 0, &len) == FALSE) {
			ERR("_util_msgq_receive() Fail");
			flag = 0;
			break;
		}
		if (len == sizeof(msgq_ptr_t) - sizeof(long)) {
			len = pkt.length;
			if (pkt.length == 6 && pkt.signal == 0xABCD) {
				ERR("Got NULL character in MQ");
				flag = 0;
				break;
			}
			pkt_data = pkt.buffer;
			pkt_len = pkt.length;
			_cmd_handler_func((mtp_char *)pkt_data, pkt_len);
			g_free(pkt_data);
			pkt_data = NULL;
			pkt_len = 0;
			memset(&pkt, 0, sizeof(pkt));
		} else {
			g_free(pkt.buffer);
			pkt.buffer = NULL;
			ERR("Received packet is less than real size");
		}
	}

	ERR("thread_data_rcv[%u] exiting\n", g_data_rcv);
	_util_thread_exit("__transport_thread_data_rcv is over");
	return NULL;
}

void _transport_init_status_info(void)
{
	memset((void *)&g_status, 0, sizeof(status_info_t));
	return;
}

mtp_int32 _transport_get_control_event(void)
{
	mtp_uint32 event_code;

	event_code = g_status.ctrl_event_code;

	/* initialize for next usage */
	if (event_code != 0)
		g_status.ctrl_event_code = 0;

	return event_code;
}

void _transport_set_control_event(mtp_int32 event_code)
{
	g_status.ctrl_event_code = event_code;
}

mtp_state_t _transport_get_mtp_operation_state(void)
{
	return g_status.mtp_op_state;
}

void _transport_set_mtp_operation_state(mtp_state_t state)
{
	g_status.mtp_op_state = state;
	return;
}

void _transport_set_usb_discon_state(mtp_bool is_usb_discon)
{
	g_status.is_usb_discon = is_usb_discon;
	return;
}

mtp_bool _transport_get_usb_discon_state(void)
{
	return g_status.is_usb_discon;
}

void _transport_set_cancel_initialization(mtp_bool value)
{
	g_status.cancel_intialization = value;
}

mtp_bool _transport_get_cancel_initialization(void)
{
	return g_status.cancel_intialization;
}
