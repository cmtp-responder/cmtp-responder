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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>
#include "mtp_usb_driver.h"
#include "mtp_device.h"
#include "ptp_datacodes.h"
#include "mtp_support.h"
#include "ptp_container.h"
#include "mtp_msgq.h"
#include "mtp_util.h"
#include "mtp_thread.h"
#include "mtp_transport.h"
#include "mtp_event_handler.h"

/*
 * GLOBAL AND EXTERN VARIABLES
 */
extern mtp_config_t g_conf;

/*
 * STATIC VARIABLES AND FUNCTIONS
 */
static mtp_int32 g_usb_fd = -1;
static mtp_max_pkt_size_t pkt_size;
static mtp_uint32 rx_mq_sz;
static mtp_uint32 tx_mq_sz;

static mtp_int32 __handle_usb_read_err(mtp_int32 err,
		mtp_uchar *buf, mtp_int32 buf_len);
static void __clean_up_msg_queue(void *pmsqid);
static void __handle_control_request(mtp_int32 request);
static void __receive_signal(mtp_int32 n, siginfo_t *info, void *unused);

/*
 * FUNCTIONS
 */
mtp_bool _transport_init_usb_device(void)
{
	mtp_int32 status = 0;
	struct sigaction sig;
	pid_t mtp_pid = 0;
	int msg_size;

	/* Kernel will inform to User Space using signal. */
	memset(&sig, 0, sizeof(sig));
	sig.sa_sigaction = __receive_signal;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIG_SETUP, &sig, NULL);

	if (g_usb_fd > 0) {
		DBG("Device Already open");
		return TRUE;
	}

	g_usb_fd = open(MTP_DRIVER_PATH, O_RDWR);
	if (g_usb_fd < 0) {
		ERR("Device node [%s] open Fail,errno [%d]\n", MTP_DRIVER_PATH, errno);
		return FALSE;
	}

	mtp_pid = getpid();
	status = ioctl(g_usb_fd, MTP_SET_USER_PID, &mtp_pid);
	if (status < 0) {
		ERR("IOCTL MTP_SET_USER_PID Fail = [%d]\n", status);
		_transport_deinit_usb_device();
		return FALSE;
	}

	pkt_size.rx = g_conf.read_usb_size;
	pkt_size.tx = g_conf.write_usb_size;

	DBG("Final : Tx pkt size:[%u], Rx pkt size:[%u]\n", pkt_size.tx, pkt_size.rx);

	msg_size = sizeof(msgq_ptr_t) - sizeof(long);
	rx_mq_sz = (g_conf.max_io_buf_size / g_conf.max_rx_ipc_size) * msg_size;
	tx_mq_sz = (g_conf.max_io_buf_size / g_conf.max_tx_ipc_size) * msg_size;

	DBG("RX MQ size :[%u], TX MQ size:[%u]\n", rx_mq_sz, tx_mq_sz);

	return TRUE;
}

void _transport_deinit_usb_device(void)
{
	if (g_usb_fd >= 0)
		close(g_usb_fd);
	g_usb_fd = -1;

	return;
}

mtp_uint32 _get_tx_pkt_size(void)
{
	return pkt_size.tx;
}

mtp_uint32 _get_rx_pkt_size(void)
{
	return pkt_size.rx;
}

/*
 * static mtp_int32 _transport_mq_init()
 * This function create a message queue for MTP,
 * A created message queue will be used to help data transfer between
 * MTP module and usb buffer.
 * @return	This function returns TRUE on success or
 *		returns FALSE on failure.
 */
mtp_int32 _transport_mq_init(msgq_id_t *rx_mqid, msgq_id_t *tx_mqid)
{
	if (_util_msgq_init(rx_mqid, 0) == FALSE) {
		ERR("RX MQ init Fail [%d]\n", errno);
		return FALSE;
	}

	if (_util_msgq_set_size(*rx_mqid, rx_mq_sz) == FALSE)
		ERR("RX MQ setting size Fail [%d]\n", errno);

	if (_util_msgq_init(tx_mqid, 0) == FALSE) {
		ERR("TX MQ init Fail [%d]\n", errno);
		_util_msgq_deinit(rx_mqid);
		*rx_mqid = -1;
		return FALSE;
	}

	if (_util_msgq_set_size(*tx_mqid, tx_mq_sz) == FALSE)
		ERR("TX MQ setting size Fail [%d]\n", errno);

	return TRUE;
}

void *_transport_thread_usb_write(void *arg)
{
	mtp_int32 status = 0;
	mtp_uint32 len = 0;
	unsigned char *mtp_buf = NULL;
	msg_type_t mtype = MTP_UNDEFINED_PACKET;
	msgq_id_t *mqid = (msgq_id_t *)arg;

	pthread_cleanup_push(__clean_up_msg_queue, mqid);

	do {
		/* original LinuxThreads cancelation didn't work right
		 * so test for it explicitly.
		 */
		pthread_testcancel();

		_util_rcv_msg_from_mq(*mqid, &mtp_buf, &len, &mtype);

		if (mtype == MTP_BULK_PACKET || mtype == MTP_DATA_PACKET) {
			status = write(g_usb_fd, mtp_buf, len);
			if (status < 0) {
				ERR("USB write fail : %d\n", errno);
				if (errno == ENOMEM || errno == ECANCELED) {
					status = 0;
					__clean_up_msg_queue(mqid);
				}
			}
			g_free(mtp_buf);
			mtp_buf = NULL;
		} else if (MTP_EVENT_PACKET == mtype) {
			/* Handling the MTP Asynchronous Events */
			DBG("Send Interrupt data to kernel by IOCTL ");
			status = ioctl(g_usb_fd, MTP_WRITE_INT_DATA, mtp_buf);
			g_free(mtp_buf);
			mtp_buf = NULL;
		} else if (MTP_ZLP_PACKET == mtype) {
			DBG("Send ZLP data to kernel by IOCTL ");
			status = ioctl(g_usb_fd, MTP_SET_ZLP_DATA, NULL);
		} else {
			DBG("mtype = %d is not valid\n", mtype);
			status = -1;
		}

		if (status < 0) {
			ERR("write data to the device node Fail:\
					status = %d\n", status);
			break;
		}
	} while (status >= 0);

	DBG("exited Source thread with status %d\n", status);
	pthread_cleanup_pop(1);
	g_free(mtp_buf);

	return NULL;
}

void *_transport_thread_usb_read(void *arg)
{
	mtp_int32 status = 0;
	msgq_ptr_t pkt = {MTP_DATA_PACKET, 0, 0, NULL};
	msgq_id_t *mqid = (msgq_id_t *)arg;
	mtp_uint32 rx_size = _get_rx_pkt_size();

	pthread_cleanup_push(__clean_up_msg_queue, mqid);

	do {
		pthread_testcancel();

		pkt.buffer = (mtp_uchar *)g_malloc(rx_size);
		if (NULL == pkt.buffer) {
			ERR("Sink thread: memalloc Fail.");
			break;
		}

		status = read(g_usb_fd, pkt.buffer, rx_size);
		if (status <= 0) {
			status = __handle_usb_read_err(status, pkt.buffer, rx_size);
			if (status <= 0) {
				ERR("__handle_usb_read_err Fail");
				g_free(pkt.buffer);
				break;
			}
		}

		pkt.length = status;
		if (FALSE == _util_msgq_send(*mqid, (void *)&pkt,
					sizeof(msgq_ptr_t) - sizeof(long), 0)) {
			ERR("msgsnd Fail");
			g_free(pkt.buffer);
		}
	} while (status > 0);

	DBG("status[%d] errno[%d]\n", status, errno);
	pthread_cleanup_pop(1);

	return NULL;
}

static mtp_int32 __handle_usb_read_err(mtp_int32 err,
		mtp_uchar *buf, mtp_int32 buf_len)
{
	mtp_int32 retry = 0;
	mtp_bool ret;

	while (retry++ < MTP_USB_ERROR_MAX_RETRY) {
		if (err == 0) {
			DBG("ZLP(Zero Length Packet). Skip");
		} else if (err < 0 && errno == EINTR) {
			DBG("read () is interrupted. Skip");
		} else if (err < 0 && errno == EIO) {
			DBG("EIO");

			if (MTP_PHONE_USB_CONNECTED !=
					_util_get_local_usb_status()) {
				ERR("USB is disconnected");
				break;
			}

			_transport_deinit_usb_device();
			ret = _transport_init_usb_device();
			if (ret == FALSE) {
				ERR("_transport_init_usb_device Fail");
				continue;
			}
		} else {
			ERR("Unknown error : %d, errno [%d] \n", err, errno);
			break;
		}

		err = read(g_usb_fd, buf, buf_len);
		if (err > 0)
			break;
	}

	if (err <= 0)
		ERR("USB error handling Fail");

	return err;
}

static void __clean_up_msg_queue(void *mq_id)
{
	mtp_int32 len = 0;
	msgq_ptr_t pkt = { 0 };
	msgq_id_t l_mqid = *(msgq_id_t *)mq_id;

	ret_if(mq_id == NULL);

	_transport_set_control_event(PTP_EVENTCODE_CANCELTRANSACTION);
	while (TRUE == _util_msgq_receive(l_mqid, (void *)&pkt,
				sizeof(msgq_ptr_t) - sizeof(long), 1, &len)) {
		g_free(pkt.buffer);
		memset(&pkt, 0, sizeof(msgq_ptr_t));
	}

	return;
}

static void __handle_control_request(mtp_int32 request)
{
	static mtp_bool kernel_reset = FALSE;
	static mtp_bool host_cancel = FALSE;
	mtp_int32 status = 0;

	switch (request) {
	case USB_PTPREQUEST_CANCELIO:
		DBG("USB_PTPREQUEST_CANCELIO");
		cancel_req_t cancelreq_data;
		mtp_byte buffer[USB_PTPREQUEST_CANCELIO_SIZE + 1] = { 0 };

		host_cancel = TRUE;
		_transport_set_control_event(PTP_EVENTCODE_CANCELTRANSACTION);
		status = ioctl(g_usb_fd, MTP_GET_SETUP_DATA, buffer);
		if (status < 0) {
			ERR("IOCTL GET_SETUP_DATA Fail [%d]\n", status);
			return;
		}

		memcpy(&(cancelreq_data.io_code), buffer, sizeof(mtp_word));
		memcpy(&(cancelreq_data.tid), &buffer[2], sizeof(mtp_dword));
		DBG("cancel io code [%d], transaction id [%ld]\n",
				cancelreq_data.io_code, cancelreq_data.tid);
		break;

	case USB_PTPREQUEST_RESET:

		DBG("USB_PTPREQUEST_RESET");
		_reset_mtp_device();
		if (kernel_reset == FALSE) {
			kernel_reset = TRUE;
		}

		status = ioctl(g_usb_fd, MTP_SEND_RESET_ACK, NULL);
		if (status < 0) {
			ERR("IOCTL MTP_SEND_RESET_ACK Fail [%d]\n",
					status);
		}
		break;

	case USB_PTPREQUEST_GETSTATUS:

		DBG("USB_PTPREQUEST_GETSTATUS");

		/* Send busy status response just once. This flag is also for
		 * the case that mtp misses the cancel request packet.
		 */
		static mtp_bool sent_busy = FALSE;
		usb_status_req_t statusreq_data = { 0 };
		mtp_dword num_param = 0;

		memset(&statusreq_data, 0x00, sizeof(usb_status_req_t));
		if (host_cancel == TRUE || (sent_busy == FALSE &&
					kernel_reset == FALSE)) {
			DBG("Send busy response, set host_cancel to FALSE");
			statusreq_data.len = 0x08;
			statusreq_data.code = PTP_RESPONSE_DEVICEBUSY;
			host_cancel = FALSE;
		} else if (_device_get_phase() == DEVICE_PHASE_NOTREADY) {
			statusreq_data.code =
				PTP_RESPONSE_TRANSACTIONCANCELLED;
			DBG("PTP_RESPONSE_TRANSACTIONCANCELLED");
			statusreq_data.len = (mtp_word)(sizeof(usb_status_req_t) +
					(num_param - 2) * sizeof(mtp_dword));
		} else if (_device_get_status() == DEVICE_STATUSOK) {
			DBG("PTP_RESPONSE_OK");
			statusreq_data.len = 0x08;
			statusreq_data.code = PTP_RESPONSE_OK;

			if (kernel_reset == TRUE)
				kernel_reset = FALSE;
		} else {
			DBG("PTP_RESPONSE_GEN_ERROR");
			statusreq_data.len = 0x08;
			statusreq_data.code = PTP_RESPONSE_GEN_ERROR;
		}

		if (statusreq_data.code == PTP_RESPONSE_DEVICEBUSY) {
			sent_busy = TRUE;
		} else {
			sent_busy = FALSE;
		}

		status = ioctl(g_usb_fd, MTP_SET_SETUP_DATA, &statusreq_data);
		if (status < 0) {
			DBG("IOCTL MTP_SET_SETUP_DATA Fail [%d]\n",
					status);
			return;
		}
		break;

	case USB_PTPREQUEST_GETEVENT:
		DBG("USB_PTPREQUEST_GETEVENT");
		break;

	default:
		DBG("Invalid class specific setup request");
		break;
	}
	return;
}

static void __receive_signal(mtp_int32 n, siginfo_t *info, void *arg)
{
	mtp_int32 request = info->si_int;

	DBG("Received SIgnal From Kernel");
	__handle_control_request(request);
	return;
}

/*
 * mtp_bool __transport_mq_deinit()
 * This function destroy a message queue for MTP,
 * @return	This function returns TRUE on success or
 *		returns FALSE on failure.
 */
mtp_bool _transport_mq_deinit(msgq_id_t *rx_mqid, msgq_id_t *tx_mqid)
{
	mtp_int32 res = TRUE;

	if (*rx_mqid) {
		res = _util_msgq_deinit(rx_mqid);
		if (res == FALSE) {
			ERR("rx_mqid deinit Fail [%d]\n", errno);
		} else {
			*rx_mqid = 0;
		}
	}

	if (*tx_mqid) {
		res = _util_msgq_deinit(tx_mqid);
		if (res == FALSE) {
			ERR("tx_mqid deinit fail [%d]\n", errno);
		} else {
			*tx_mqid = 0;
		}
	}

	return res;
}

mtp_uint32 _transport_get_usb_packet_len(void)
{
	mtp_int32 status = 0;
	static mtp_int32 usb_speed = 0;

	if (usb_speed == 0) {

		status = ioctl(g_usb_fd, MTP_GET_HIGH_FULL_SPEED, &usb_speed);
		if (status < 0) {
			ERR("MTP_GET_HIGH_FULL_SPEED Fail [%d]\n", status);
			return MTP_MAX_PACKET_SIZE_SEND_FS;
		}
	}

	if (usb_speed % MTP_MAX_PACKET_SIZE_SEND_HS) {
		return MTP_MAX_PACKET_SIZE_SEND_FS;
	}

	return MTP_MAX_PACKET_SIZE_SEND_HS;
}
