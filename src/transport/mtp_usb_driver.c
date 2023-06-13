/* -*- mode: C; c-file-style: "linux" -*-
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
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

#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>
#include "mtp_usb_driver.h"
#include "mtp_device.h"
#include "mtp_descs_strings.h"
#include "ptp_datacodes.h"
#include "mtp_support.h"
#include "ptp_container.h"
#include "mtp_msgq.h"
#include "mtp_thread.h"
#include "mtp_transport.h"
#include "mtp_event_handler.h"
#include "mtp_init.h"
#include <sys/prctl.h>
#include <systemd/sd-daemon.h>

/*
 * GLOBAL AND EXTERN VARIABLES
 */
extern mtp_config_t g_conf;
extern phone_state_t *g_ph_status;

/*
 * STATIC VARIABLES AND FUNCTIONS
 */

/*PIMA15740-2000 spec*/
#define USB_PTPREQUEST_CANCELIO   0x64    /* Cancel request */
#define USB_PTPREQUEST_GETEVENT   0x65    /* Get extened event data */
#define USB_PTPREQUEST_RESET      0x66    /* Reset Device */
#define USB_PTPREQUEST_GETSTATUS  0x67    /* Get Device Status */
#define USB_PTPREQUEST_CANCELIO_SIZE 6
#define USB_PTPREQUEST_GETSTATUS_SIZE 12

static mtp_int32 g_usb_ep0 = -1;       /* read (g_usb_ep0, ...) */
static mtp_int32 g_usb_ep_in = -1;     /* write (g_usb_ep_in, ...) */
static mtp_int32 g_usb_ep_out = -1;    /* read (g_usb_ep_out, ...) */
static mtp_int32 g_usb_ep_status = -1; /* write (g_usb_ep_status, ...) */

static mtp_uint32 rx_mq_sz;
static mtp_uint32 tx_mq_sz;
static mtp_int32 __handle_usb_read_err(mtp_int32 err,
		mtp_uchar *buf, mtp_int32 buf_len);
static void __clean_up_msg_queue(void *param);
static void __handle_control_request(mtp_int32 request);

/*
 * FUNCTIONS
 */

/* LCOV_EXCL_START */
mtp_bool _transport_init_usb_device(void)
{
	int msg_size;

	if (g_usb_ep0 > 0) {
		DBG("Device Already open\n");
		return TRUE;
	}

	if (sd_listen_fds(0) < 4) {
		char error[256];
		ERR("Inheriting FunctionFS descriptors from systemd failed, errno [%s]\n",
		    strerror_r(errno, error, sizeof(error)));
		return FALSE;
	}
	DBG("socket-activated\n");
	g_usb_ep0 = SD_LISTEN_FDS_START;
	g_usb_ep_in = SD_LISTEN_FDS_START + 1;
	g_usb_ep_out = SD_LISTEN_FDS_START + 2;
	g_usb_ep_status = SD_LISTEN_FDS_START + 3;

	DBG("Final : Tx pkt size:[%u], Rx pkt size:[%u]\n", g_conf.write_usb_size, g_conf.read_usb_size);

	msg_size = sizeof(msgq_ptr_t) - sizeof(long);
	rx_mq_sz = (g_conf.max_io_buf_size / g_conf.max_rx_ipc_size) * msg_size;
	tx_mq_sz = (g_conf.max_io_buf_size / g_conf.max_tx_ipc_size) * msg_size;

	DBG("RX MQ size :[%u], TX MQ size:[%u]\n", rx_mq_sz, tx_mq_sz);

	return TRUE;
}

void _transport_deinit_usb_device(void)
{
	if (g_usb_ep0 >= 0)
		close(g_usb_ep0);
	g_usb_ep0 = -1;

	if (g_usb_ep_in >= 0)
		close(g_usb_ep_in);
	g_usb_ep_in = -1;

	if (g_usb_ep_out >= 0)
		close(g_usb_ep_out);
	g_usb_ep_out = -1;

	if (g_usb_ep_status >= 0)
		close(g_usb_ep_status);
	g_usb_ep_status = -1;

	return;
}

/*
 * mtp_int32 _transport_mq_init()
 * This function create a message queue for MTP,
 * A created message queue will be used to help data transfer between
 * MTP module and usb buffer.
 * @return	This function returns TRUE on success or
 *		returns FALSE on failure.
 */
mtp_int32 _transport_mq_init(msgq_id_t *rx_mqid, msgq_id_t *tx_mqid)
{
	retvm_if(!_util_msgq_init(rx_mqid, 0), FALSE, "RX MQ init Fail [%d]\n", errno);

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
			status = write(g_usb_ep_in, mtp_buf, len);
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
			DBG("Send Interrupt data to kernel via g_usb_ep_status\n");
			status = write(g_usb_ep_status, mtp_buf, len);
			g_free(mtp_buf);
			mtp_buf = NULL;
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

static int __setup(int ep0, struct usb_ctrlrequest *ctrl)
{
	const char* requests[] = {
		"CANCELIO",	/* 0x64 */
		"GETEVENT",	/* 0x65 */
		"RESET",	/* 0x66 */
		"GETSTATUS",	/* 0x67 */
	};
	__u16 wIndex = le16_to_cpu(ctrl->wIndex);
	__u16 wValue = le16_to_cpu(ctrl->wValue);
	__u16 wLength = le16_to_cpu(ctrl->wLength);
	int rc = -EOPNOTSUPP;
	int status = 0;

	if ((ctrl->bRequestType & 0x7f) != (USB_TYPE_CLASS | USB_RECIP_INTERFACE)) {
		DBG(__FILE__ "(%s):%d: Invalid request type: %d\n",
		    __func__, __LINE__, ctrl->bRequestType);
		goto stall;
	}

	switch (((ctrl->bRequestType & 0x80) << 8) | ctrl->bRequest) {
	case ((USB_DIR_OUT << 8) | USB_PTPREQUEST_CANCELIO):

		DBG(__FILE__ "(%s):%d: USB_PTPREQUEST_%s\n",
		    __func__, __LINE__, requests[ctrl->bRequest-0x64]);
		if (wValue != 0 || wIndex != 0 || wLength != 6) {
			DBG("Invalid request parameters: wValue:%hu wIndex:%hu wLength:%hu\n", wIndex, wValue, wLength);
			rc = -EINVAL;
			goto stall;
		}
		__handle_control_request(ctrl->bRequest);
		break;

	case ((USB_DIR_IN << 8) | USB_PTPREQUEST_GETSTATUS):
	case ((USB_DIR_OUT << 8) | USB_PTPREQUEST_RESET):

		DBG(__FILE__ "(%s):%d: USB_PTPREQUEST_%s\n",
		    __func__, __LINE__, requests[ctrl->bRequest-0x64]);
		__handle_control_request(ctrl->bRequest);
		break;

	case ((USB_DIR_IN << 8) | USB_PTPREQUEST_GETEVENT):

		/* Optional, may stall */
		DBG(__FILE__ "(%s):%d: USB_PTPREQUEST_%s\n",
		    __func__, __LINE__, requests[ctrl->bRequest-0x64]);
		rc = -EOPNOTSUPP;
		goto stall;
		break;

	default:
		DBG(__FILE__ "(%s):%d: Invalid request: %d\n", __func__,
		    __LINE__, ctrl->bRequest);
		goto stall;
	}
	return 0;

stall:

	DBG(__FILE__"(%s):%d:stall %0x2x.%02x\n",
	    __func__, __LINE__, ctrl->bRequestType, ctrl->bRequest);
	if ((ctrl->bRequestType & 0x80) == USB_DIR_IN)
		status = read(g_usb_ep0, NULL, 0);
	else
		status = write(g_usb_ep0, NULL, 0);

	if (status != -1 || errno != EL2HLT) {
		ERR(__FILE__"(%s):%d:stall error\n",
		    __func__, __LINE__);
		rc = errno;
	}
	return rc;
}

void *_transport_thread_usb_read(void *arg)
{
	mtp_int32 status = 0;
	msgq_ptr_t pkt = {MTP_DATA_PACKET, 0, 0, NULL};
	msgq_id_t *mqid = (msgq_id_t *)arg;
	mtp_uint32 rx_size = g_conf.read_usb_size;

	pthread_cleanup_push(__clean_up_msg_queue, mqid);

	do {
		pthread_testcancel();

		pkt.buffer = (mtp_uchar *)g_malloc(rx_size);
		if (NULL == pkt.buffer) {
			ERR("Sink thread: memalloc failed.\n");
			break;
		}

		status = read(g_usb_ep_out, pkt.buffer, rx_size);
		if (status <= 0) {
			status = __handle_usb_read_err(status, pkt.buffer, rx_size);
			if (status <= 0) {
				ERR("__handle_usb_read_err is failed\n");
				g_free(pkt.buffer);
				break;
			}
		}

		pkt.length = status;
		if (FALSE == _util_msgq_send(*mqid, (void *)&pkt,
					     sizeof(msgq_ptr_t) - sizeof(long), 0)) {
			ERR("msgsnd Fail\n");
			g_free(pkt.buffer);
		}
	} while (status > 0);

	DBG("status[%d] errno[%d]\n", status, errno);
	pthread_cleanup_pop(1);

	return NULL;
}

void *_transport_thread_usb_control(void *arg)
{
	mtp_int32 status = 0;
	struct usb_functionfs_event event;
	msgq_id_t *mqid = (msgq_id_t *)arg;

	pthread_cleanup_push(__clean_up_msg_queue, mqid);

	do {
		pthread_testcancel();

		status = read(g_usb_ep0, &event, sizeof(event));
		if (status < 0) {
			char error[256];
			ERR("read from ep0 failed: %s\n",
			    strerror_r(errno, error, sizeof(error)));
			continue;
		}
		DBG("FUNCTIONFS event received: %d\n", event.type);

		switch (event.type) {
		case FUNCTIONFS_SETUP:
			DBG("SETUP: bmRequestType:%d bRequest:%d wValue:%d wIndex:%d wLength:%d\n",
			    event.u.setup.bRequestType,
			    event.u.setup.bRequest,
			    event.u.setup.wValue,
			    event.u.setup.wIndex,
			    event.u.setup.wLength);
			__setup(g_usb_ep0, &event.u.setup);
			break;
		case FUNCTIONFS_ENABLE:
			DBG("ENABLE\n");
			g_ph_status->usb_state = MTP_PHONE_USB_CONNECTED;
			break;
		case FUNCTIONFS_DISABLE:
			DBG("DISABLE\n");
			g_ph_status->usb_state = MTP_PHONE_USB_DISCONNECTED;
			_eh_send_event_req_to_eh_thread(EVENT_USB_REMOVED, 0, 0, NULL);
			break;
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
			DBG("ZLP(Zero Length Packet). Skip\n");
		} else if (err < 0 && errno == EINTR) {
			DBG("read () is interrupted. Skip\n");
		} else if (err < 0 && errno == EIO) {
			DBG("EIO\n");

			if (MTP_PHONE_USB_CONNECTED !=
			    g_ph_status->usb_state) {
				ERR("USB is disconnected\n");
				break;
			}

			_transport_deinit_usb_device();
			ret = _transport_init_usb_device();
			if (ret == FALSE) {
				ERR("_transport_init_usb_device Fail\n");
				continue;
			}
		} else if (err < 0 && errno == ESHUTDOWN) {
			DBG("ESHUTDOWN\n");
		} else {
			ERR("Unknown error : %d, errno [%d]\n", err, errno);
			break;
		}

		err = read(g_usb_ep_out, buf, buf_len);
		if (err > 0)
			break;
	}

	if (err <= 0)
		ERR("USB error handling Fail\n");

	return err;
}

static void __clean_up_msg_queue(void *mq_id)
{
	mtp_int32 len = 0;
	msgq_ptr_t pkt = { 0 };
	msgq_id_t l_mqid;

	ret_if(mq_id == NULL);

	l_mqid = *(msgq_id_t *)mq_id;
	g_status->ctrl_event_code = PTP_EVENTCODE_CANCELTRANSACTION;
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
		// XXX: Convert cancel request data from little-endian
		// before use:  le32_to_cpu(x), le16_to_cpu(x).
		DBG("USB_PTPREQUEST_CANCELIO\n");
		cancel_req_t cancelreq_data;

		host_cancel = TRUE;
		g_status->ctrl_event_code = PTP_EVENTCODE_CANCELTRANSACTION;
		status = read(g_usb_ep0, &cancelreq_data, sizeof(cancelreq_data));
		if (status < 0) {
			char error[256];
			ERR("Failed to read data for CANCELIO request\n: %s",
					strerror_r(errno, error, sizeof(error)));
		}
		break;

	case USB_PTPREQUEST_RESET:

		DBG("USB_PTPREQUEST_RESET\n");
		_reset_mtp_device();
		if (kernel_reset == FALSE) {
			kernel_reset = TRUE;
		}

		status = read(g_usb_ep0, NULL, 0);
		if (status < 0) {
			ERR("IOCTL MTP_SEND_RESET_ACK Failed [%d]\n",
				status);
		}
		break;
	case USB_PTPREQUEST_GETSTATUS:

		DBG("USB_PTPREQUEST_GETSTATUS\n");

		/* Send busy status response just once. This flag is also for
		 * the case that mtp misses the cancel request packet.
		 */
		static mtp_bool sent_busy = FALSE;
		usb_status_req_t statusreq_data = { 0 };
		mtp_dword num_param = 0;

		memset(&statusreq_data, 0x00, sizeof(usb_status_req_t));
		if (host_cancel == TRUE || (sent_busy == FALSE &&
					    kernel_reset == FALSE)) {
			DBG("Send busy response, set host_cancel to FALSE\n");
			statusreq_data.len = 0x08;
			statusreq_data.code = PTP_RESPONSE_DEVICEBUSY;
			host_cancel = FALSE;
		} else if (g_device->phase == DEVICE_PHASE_NOTREADY) {
			statusreq_data.code =
				PTP_RESPONSE_TRANSACTIONCANCELLED;
			DBG("PTP_RESPONSE_TRANSACTIONCANCELLED\n");
			statusreq_data.len = (mtp_word)(sizeof(usb_status_req_t) +
							(num_param - 2) * sizeof(mtp_dword));
		} else if (g_device->status == DEVICE_STATUSOK) {
			DBG("PTP_RESPONSE_OK\n");
			statusreq_data.len = 0x08;
			statusreq_data.code = PTP_RESPONSE_OK;

			if (kernel_reset == TRUE)
				kernel_reset = FALSE;
		} else {
			DBG("PTP_RESPONSE_GEN_ERROR\n");
			statusreq_data.len = 0x08;
			statusreq_data.code = PTP_RESPONSE_GEN_ERROR;
		}

		if (statusreq_data.code == PTP_RESPONSE_DEVICEBUSY) {
			sent_busy = TRUE;
		} else {
			sent_busy = FALSE;
		}

		/* status = ioctl(g_usb_fd, MTP_SET_SETUP_DATA, &statusreq_data);
		if (status < 0) {
			DBG("IOCTL MTP_SET_SETUP_DATA Fail [%d]\n",
					status);
			return;
		} */

		break;

	case USB_PTPREQUEST_GETEVENT:
		DBG("USB_PTPREQUEST_GETEVENT\n");
		break;

	default:
		DBG("Invalid class specific setup request\n");
		break;
	}
	return;
}

/*
 * mtp_bool _transport_mq_deinit()
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
/* LCOV_EXCL_STOP */
