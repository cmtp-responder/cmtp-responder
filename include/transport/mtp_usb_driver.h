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

#ifndef _MTP_USB_DRIVER_H_
#define _MTP_USB_DRIVER_H_

#include "mtp_datatype.h"
#include "mtp_msgq.h"

/* Start of driver related defines */
#define MTP_DRIVER_PATH			"/dev/usb_mtp_gadget"

/* These values come from f_mtp_slp.h of kernel source */
#define MTP_IOCTL_LETTER		'Z'
#define MTP_GET_HIGH_FULL_SPEED		_IOR(MTP_IOCTL_LETTER, 1, int)
#define MTP_DISABLE			_IO(MTP_IOCTL_LETTER, 2)
#define MTP_CLEAR_HALT			_IO(MTP_IOCTL_LETTER, 3)
#define MTP_WRITE_INT_DATA		_IOW(MTP_IOCTL_LETTER, 4, char *)
#define MTP_SET_USER_PID		_IOW(MTP_IOCTL_LETTER, 5, int)
#define MTP_GET_SETUP_DATA		_IOR(MTP_IOCTL_LETTER, 6, char *)
#define MTP_SET_SETUP_DATA		_IOW(MTP_IOCTL_LETTER, 7, char *)
#define MTP_SEND_RESET_ACK		_IO(MTP_IOCTL_LETTER, 8)
#define MTP_SET_ZLP_DATA		_IO(MTP_IOCTL_LETTER, 9)
#define MTP_GET_MAX_PKT_SIZE		_IOR(MTP_IOCTL_LETTER, 22, void *)

#define SIG_SETUP			44
/* End of driver related defines */

typedef struct mtp_max_pkt_size {
	mtp_uint32 tx;
	mtp_uint32 rx;
} mtp_max_pkt_size_t;

/* Maximum repeat count for USB error recovery */
#define MTP_USB_ERROR_MAX_RETRY		5

mtp_bool _transport_init_usb_device(void);
void _transport_deinit_usb_device(void);
void *_transport_thread_usb_write(void *arg);
void *_transport_thread_usb_read(void *arg);
mtp_int32 _transport_mq_init(msgq_id_t *rx_mqid, msgq_id_t *tx_mqid);
mtp_bool _transport_mq_deinit(msgq_id_t *rx_mqid, msgq_id_t *tx_mqid);
mtp_uint32 _transport_get_usb_packet_len(void);
mtp_uint32 _get_tx_pkt_size(void);
mtp_uint32 _get_rx_pkt_size(void);

#endif /* _MTP_USB_DRIVER_H_ */
