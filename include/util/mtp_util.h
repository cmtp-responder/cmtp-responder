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

#ifndef _MTP_UTIL_H_
#define _MTP_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include "mtp_config.h"
#include "mtp_datatype.h"
#include "mtp_fs.h"
#include "ptp_datacodes.h"

#define MODEL "DUMMY MODEL"
#define DEVICE_VERSION "devv_DUMMY_VERSION"
#define SERIAL "DUMMY_SERIAL"

#define DBG(format, args...) FLOGD(format, ##args)
#define ERR(format, args...) FLOGD(format, ##args)
#define DBG_SECURE(format, args...) FLOGD(format, ##args)
#define ERR_SECURE(format, args...) FLOGD(format, ##args)

#define ret_if(expr) \
	do { \
		if (expr) { \
			ERR("(%s)", #expr); \
			return; \
		} \
	} while (0)

#define retv_if(expr, val) \
	do { \
		if (expr) { \
			ERR("(%s)", #expr); \
			return (val); \
		} \
	} while (0)

#define retm_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return; \
		} \
	} while (0)

#define retvm_if(expr, val, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return (val); \
		} \
	} while (0)

typedef enum {
	MTP_PHONE_USB_CONNECTED = 0,
	MTP_PHONE_USB_DISCONNECTED,
	MTP_PHONE_MMC_INSERTED,
	MTP_PHONE_MMC_NONE,
	MTP_PHONE_USB_MODE_OTHER,
	MTP_PHONE_LOCK_ON,
	MTP_PHONE_LOCK_OFF,
} phone_status_t;

typedef struct {
	phone_status_t usb_state;
	phone_status_t usb_mode_state;
} phone_state_t;

typedef enum {
	MTP_DATA_PACKET = 1,
	MTP_BULK_PACKET,
	MTP_EVENT_PACKET,
	MTP_UNDEFINED_PACKET
} msg_type_t;

typedef enum {
	MTP_STATE_STOPPED = 0,		/* stopped working */
	MTP_STATE_INITIALIZING,		/* initializing device or enumerating*/
	MTP_STATE_READY_SERVICE,	/* ready to handle commands */
	MTP_STATE_ONSERVICE,		/* handling a command */
	MTP_STATE_DATA_TRANSFER_DL,	/* file downloading */
	MTP_STATE_DATA_PROCESSING	/* data processing */
} mtp_state_t;

/*
 * PTP Cancellation Request
 * mtp_uint16 io_code : Identifier for cancellation.
 *	This must equal USB_PTPCANCELIO_ID.
 * mtp_uint32 tid : Transaction to cancel.
 */
typedef struct {
	mtp_uint16 io_code;
	mtp_uint32 tid;
} cancel_req_t;

/*
 * PTP Status Request
 * mtp_uint16 len : Total length of the status data.
 * mtp_uint16 code : Response code
 * mtp_uint32 params : Params depends on the status code.
 */
typedef struct {
	mtp_uint16 len;
	mtp_uint16 code;
	mtp_uint32 params[1];
} usb_status_req_t;

void _util_print_error();
void _util_get_external_path(char *external_path);
void _util_ptp_pack_string(ptp_string_t *string, char *data);

#ifdef __cplusplus
}
#endif

#endif /* _MTP_UTIL_H_ */
