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
#include <systemd/sd-daemon.h>
#include "mtp_usb_driver.h"

static const mtp_usb_driver_t *usb_driver;

/*
 * FUNCTIONS
 */
/* LCOV_EXCL_START */
mtp_bool _transport_select_driver(void)
{
	if (access(MTP_DRIVER_PATH, F_OK) == 0) {
		usb_driver = &mtp_usb_driver_slp;
		DBG("SLP driver selected");
		return TRUE;
	}

	if (access(MTP_EP0_PATH, F_OK) == 0 || sd_listen_fds(0) >= 4) {
		usb_driver = &mtp_usb_driver_ffs;
		DBG("FFS driver selected");
		return TRUE;
	}

	ERR("No suport for USB gadgets in kernel");
	return FALSE;
}

mtp_bool _transport_init_usb_device(void)
{
	return usb_driver->transport_init_usb_device();
}

void _transport_deinit_usb_device(void)
{
	usb_driver->transport_deinit_usb_device();
}

mtp_uint32 _get_tx_pkt_size(void)
{
	return usb_driver->get_rx_pkt_size();
}

mtp_uint32 _get_rx_pkt_size(void)
{
	return usb_driver->get_rx_pkt_size();
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
	return usb_driver->transport_mq_init(rx_mqid, tx_mqid);
}

void *_transport_thread_usb_write(void *arg)
{
	return usb_driver->transport_thread_usb_write(arg);
}

void *_transport_thread_usb_read(void *arg)
{
	return usb_driver->transport_thread_usb_read(arg);
}

void *_transport_thread_usb_control(void *arg)
{
	return usb_driver->transport_thread_usb_control(arg);
}

/*
 * mtp_bool __transport_mq_deinit()
 * This function destroy a message queue for MTP,
 * @return	This function returns TRUE on success or
 *		returns FALSE on failure.
 */
mtp_bool _transport_mq_deinit(msgq_id_t *rx_mqid, msgq_id_t *tx_mqid)
{
	return usb_driver->transport_mq_deinit(rx_mqid, tx_mqid);
}

mtp_uint32 _transport_get_usb_packet_len(void)
{
	return usb_driver->transport_get_usb_packet_len();
}

mtp_transport_type_t _transport_get_type(void)
{
	if (usb_driver == &mtp_usb_driver_slp)
		return MTP_TRANSPORT_SLP;

	if (usb_driver == &mtp_usb_driver_ffs)
		return MTP_TRANSPORT_FFS;

	return MTP_TRANSPORT_UNKNOWN;
}
/* LCOV_EXCL_STOP */
