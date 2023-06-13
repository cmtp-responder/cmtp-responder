/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 * Copyright (c) 2019 Collabora Ltd
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
static mtp_bool g_usb_threads_created = FALSE;
static pthread_t g_tx_thrd = 0;
static pthread_t g_rx_thrd = 0;
static pthread_t g_ctrl_thrd = 0;
static pthread_t g_data_rcv = 0;
static msgq_id_t mtp_to_usb_mqid;
static msgq_id_t g_usb_to_mtp_mqid;
static status_info_t _g_status;
status_info_t *g_status = &_g_status;

/*
 * FUNCTIONS
 */
/* LCOV_EXCL_START */
void _transport_save_cmd_buffer(mtp_char *buffer, mtp_uint32 size)
{
	memcpy(g_mtp_mgr.ftemp_st.cmd_buf, buffer, size);
	g_mtp_mgr.ftemp_st.cmd_size = size;
	g_mtp_mgr.ftemp_st.data_count = 0;
}
/* LCOV_EXCL_STOP */

mtp_err_t _transport_rcv_temp_file_data(mtp_byte *buffer, mtp_uint32 size,
		mtp_uint32 *count)
{
	FILE* h_file = NULL;
	mtp_int32 error = 0;
	mtp_uint32 data_sz;


	h_file = _util_file_open(g_mtp_mgr.ftemp_st.filepath,
			MTP_FILE_READ, &error);
	retvm_if(!h_file, MTP_ERROR_NONE, "_util_file_open(%s) Fail\n",
		g_mtp_mgr.ftemp_st.filepath);

	/*copy header */
	/* LCOV_EXCL_START */
	memcpy(buffer, g_mtp_mgr.ftemp_st.header_buf,
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
		ERR("_util_file_close Fail\n");
		_util_print_error();
	}

	/* delete temp file, it have to be called in receive_data fn */
	if (g_mtp_mgr.ftemp_st.filepath != NULL) {
		if (remove(g_mtp_mgr.ftemp_st.filepath) < 0) {
			ERR_SECURE("remove(%s) Fail\n", g_mtp_mgr.ftemp_st.filepath);
			_util_print_error();
		}

		g_free(g_mtp_mgr.ftemp_st.filepath);
		g_mtp_mgr.ftemp_st.filepath = NULL;
	}

	g_mtp_mgr.ftemp_st.data_size = 0;
	g_mtp_mgr.ftemp_st.data_count = 0;

	return MTP_ERROR_NONE;
}

mtp_err_t _transport_rcv_temp_file_info(mtp_byte *buf, char *filepath,
		mtp_uint64 *t_size, mtp_uint32 filepath_len)
{
	file_attr_t atttrs = { 0 };
	mtp_bool ret = FALSE;

	memcpy(buf, g_mtp_mgr.ftemp_st.header_buf,
			sizeof(header_container_t));

	ret = _util_get_file_attrs(g_mtp_mgr.ftemp_st.filepath, &atttrs);
	retvm_if(!ret, MTP_ERROR_GENERAL, "_util_get_file_attrs(%s) Fail\n",
		g_mtp_mgr.ftemp_st.filepath);

	*t_size = sizeof(header_container_t) + atttrs.fsize;
	g_strlcpy(filepath, g_mtp_mgr.ftemp_st.filepath, filepath_len);

	g_mtp_mgr.ftemp_st.data_size = 0;
	g_mtp_mgr.ftemp_st.data_count = 0;

	g_free(g_mtp_mgr.ftemp_st.filepath);
	g_mtp_mgr.ftemp_st.filepath = NULL;

	g_mtp_mgr.ftemp_st.fhandle = NULL;
	g_mtp_mgr.ftemp_st.file_size = 0;

	return MTP_ERROR_NONE;
}

mtp_err_t _transport_send_event(mtp_byte *buf, mtp_uint32 size,
		mtp_uint32 *count)
{
	mtp_bool resp = FALSE;
	msgq_ptr_t pkt = { 0 };

	retv_if(buf == NULL, MTP_ERROR_INVALID_PARAM);
	retvm_if(size > g_conf.write_usb_size, MTP_ERROR_INVALID_PARAM,
			"size = %d, tx pkt size = (%d)\n", size, g_conf.write_usb_size);

	pkt.mtype = MTP_EVENT_PACKET;
	pkt.signal = 0x0000;
	pkt.length = size;

	pkt.buffer = (mtp_uchar *)g_malloc(size);
	retvm_if(!pkt.buffer, MTP_ERROR_GENERAL, "g_malloc() Fail\n");

	memcpy(pkt.buffer, buf, size);
	resp = _util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
			sizeof(msgq_ptr_t) - sizeof(long), 0);
	retvm_if(!resp, MTP_ERROR_GENERAL, "_util_msgq_send() Fail\n");

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
	mtp_uint32 tx_size = g_conf.write_usb_size;
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
			ERR("g_malloc() Fail\n");
			return 0;
		}

		memcpy(pkt.buffer, temp, sent_len);
		ret = _util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
				sizeof(msgq_ptr_t) - sizeof(long), 0);
		if (ret == FALSE) {
			ERR("_util_msgq_send() Fail\n");
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
	mtp_uint32 tx_size = g_conf.write_usb_size;
	msgq_ptr_t pkt = {MTP_BULK_PACKET, 0, 0, NULL};

	retv_if(buf == NULL, 0);
	retv_if(pkt_len == 0, 0);

	pkt.length = tx_size;
	while (pkt_len > tx_size) {
		pkt.buffer = (mtp_uchar *)g_malloc(pkt.length);
		retvm_if(!pkt.buffer, 0, "g_malloc() Fail\n");

		memcpy(pkt.buffer, &buf[sent_len], pkt.length);

		if (!_util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
					sizeof(msgq_ptr_t) - sizeof(long), 0)) {
			ERR("_util_msgq_send() Fail\n");
			g_free(pkt.buffer);
			return 0;
		}

		pkt_len -= pkt.length;
		sent_len += pkt.length;
	}

	pkt.length = pkt_len;
	pkt.buffer = (mtp_uchar *)g_malloc(pkt.length);
	retvm_if(!pkt.buffer, 0, "g_malloc() Fail\n");

	memcpy(pkt.buffer, &buf[sent_len], pkt.length);

	if (!_util_msgq_send(mtp_to_usb_mqid, (void *)&pkt,
				sizeof(msgq_ptr_t) - sizeof(long), 0)) {
		ERR("_util_msgq_send() Fail\n");
		g_free(pkt.buffer);
		return 0;
	}
	sent_len += pkt.length;

	return sent_len;
}

static mtp_err_t __transport_init_io()
{
	mtp_int32 res = 0;
	thread_func_t usb_write_thread = _transport_thread_usb_write;
	thread_func_t usb_read_thread = _transport_thread_usb_read;
	thread_func_t usb_control_thread = _transport_thread_usb_control;

	res = _util_thread_create(&g_tx_thrd, "usb write thread",
			PTHREAD_CREATE_JOINABLE, usb_write_thread,
			(void *)&mtp_to_usb_mqid);
	if (FALSE == res) {
		ERR("_util_thread_create(TX) Fail\n");
		goto cleanup;
	}

	res = _util_thread_create(&g_rx_thrd, "usb read thread",
			PTHREAD_CREATE_JOINABLE, usb_read_thread,
			(void *)&g_usb_to_mtp_mqid);
	if (FALSE == res) {
		ERR("_util_thread_create(RX) Fail\n");
		goto cleanup;
	}

	res = _util_thread_create(&g_ctrl_thrd, "usb control thread",
				  PTHREAD_CREATE_JOINABLE,
				  usb_control_thread,
				  NULL);
	if (FALSE == res) {
		ERR("CTRL thread creation failed\n");
		goto cleanup;
	}

	g_usb_threads_created = TRUE;

	return MTP_ERROR_NONE;

cleanup:
	_util_print_error();

	if (g_ctrl_thrd) {
		res = _util_thread_cancel(g_ctrl_thrd);
		DBG("pthread_cancel [%d]\n", res);
		g_ctrl_thrd = 0;
	}
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
	retm_if(!g_usb_threads_created, "io threads are not created.\n");

	errno = 0;

	if (FALSE == _util_thread_cancel(g_ctrl_thrd)) {
		ERR("Fail to cancel pthread of g_ctrl_thrd\n");
	} else {
		DBG("Succeed to cancel pthread of g_ctrl_thrd\n");
	}

	if (_util_thread_join(g_ctrl_thrd, 0) == FALSE)
		ERR("pthread_join of g_ctrl_thrd failed\n");

	g_ctrl_thrd = 0;

	if (FALSE == _util_thread_cancel(g_rx_thrd))
		ERR("_util_thread_cancel(rx) Fail\n");

	if (_util_thread_join(g_rx_thrd, 0) == FALSE)
		ERR("_util_thread_join(rx) Fail\n");

	g_rx_thrd = 0;

	if (FALSE == _util_thread_cancel(g_tx_thrd))
		ERR("_util_thread_cancel(tx) Fail\n");

	if (_util_thread_join(g_tx_thrd, 0) == FALSE)
		ERR("_util_thread_join(tx) Fail\n");

	g_tx_thrd = 0;

	g_usb_threads_created = FALSE;
}

static void *__transport_thread_data_rcv(void *func)
{
	msgq_ptr_t pkt = { 0 };
	mtp_uchar *pkt_data = NULL;
	mtp_uint32 pkt_len = 0;
	mtp_int32 flag = 1;
	mtp_int32 len = 0;
	_cmd_handler_cb _cmd_handler_func = (_cmd_handler_cb)func;

	while (flag) {
		if (_util_msgq_receive(g_usb_to_mtp_mqid, (void *)&pkt,
					sizeof(pkt) - sizeof(long), 0, &len) == FALSE) {
			ERR("_util_msgq_receive() Fail\n");
			flag = 0;
			break;
		}
		if (len == sizeof(msgq_ptr_t) - sizeof(long)) {
			len = pkt.length;
			if (pkt.length == 6 && pkt.signal == 0xABCD) {
				ERR("Got NULL character in MQ\n");
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
			ERR("Received packet is less than real size\n");
		}
	}

	ERR("thread_data_rcv[%lu] exiting\n", g_data_rcv);
	_util_thread_exit("__transport_thread_data_rcv is over");
	return NULL;
}

mtp_bool _transport_init_interfaces(_cmd_handler_cb func)
{
	mtp_int32 res = 0;
	mtp_bool ret = FALSE;

	ret = _transport_init_usb_device();
	/* mtp driver open failed */
	retvm_if(!ret, FALSE, "_transport_init_usb_device() Fail\n");

	if (_transport_mq_init(&g_usb_to_mtp_mqid, &mtp_to_usb_mqid) == FALSE) {
		ERR("_transport_mq_init() Fail\n");
		_transport_deinit_usb_device();
		return FALSE;
	}

	if (__transport_init_io() != MTP_ERROR_NONE) {
		ERR("__transport_init_io() Fail\n");
		_transport_mq_deinit(&g_usb_to_mtp_mqid, &mtp_to_usb_mqid);
		_transport_deinit_usb_device();
		return FALSE;
	}

	res = _util_thread_create(&g_data_rcv, "Data Receive thread",
			PTHREAD_CREATE_JOINABLE, __transport_thread_data_rcv,
			(void *)func);
	if (res == FALSE) {
		ERR("_util_thread_create(data_rcv) Fail\n");
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
	mtp_uint32 rx_size = g_conf.read_usb_size;

	__transport_deinit_io();

	if (g_data_rcv != 0) {
		pkt.buffer = (mtp_uchar *)g_malloc(rx_size);
		retm_if(!pkt.buffer, "g_malloc() Fail\n");

		pkt.mtype = MTP_DATA_PACKET;
		pkt.signal = 0xABCD;
		pkt.length = 6;
		memset(pkt.buffer, 0, rx_size);
		if (FALSE == _util_msgq_send(g_usb_to_mtp_mqid, (void *)&pkt,
					sizeof(msgq_ptr_t) - sizeof(long), 0)) {
			ERR("_util_msgq_send() Fail\n");
		}
		g_free(pkt.buffer);

		res = _util_thread_join(g_data_rcv, &th_result);
		if (res == FALSE)
			ERR("_util_thread_join(data_rcv) Fail\n");
	}

	if (_transport_mq_deinit(&g_usb_to_mtp_mqid, &mtp_to_usb_mqid) == FALSE)
		ERR("_transport_mq_deinit() Fail\n");

	_transport_deinit_usb_device();
}
