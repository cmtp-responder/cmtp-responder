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

#ifndef _MTP_UTIL_H_
#define _MTP_UTIL_H_

#include <errno.h>
#include "mtp_config.h"
#include "mtp_datatype.h"

#ifndef LOG_TAG
#define LOG_TAG "MTP-RESPONDER"
#endif /* LOG_TAG */
#include <dlog.h>

#define FIND_CMD_LEN				300
#define FIND_CMD	"/usr/bin/find %s \\( -iname '*.jpg' -o -iname '*.gif' " \
	"-o -iname '*.exif' -o -iname '*.png' " \
	"-o -iname '*.mpeg' -o -iname '*.asf' " \
	"-o -iname '*.wmv' -o -iname '*.avi' -o -iname '*.wma' " \
	"-o -iname '*.mp3' \\) -mmin -%d >> %s"

#define DBG(format, args...) SLOGD(format, ##args)
#define ERR(format, args...) SLOGE(format, ##args)
#define DBG_SECURE(format, args...) SECURE_SLOGD(format, ##args)
#define ERR_SECURE(format, args...) SECURE_SLOGE(format, ##args)

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
} phone_status_t;

typedef struct {
	phone_status_t mmc_state;
	phone_status_t usb_state;
	phone_status_t usb_mode_state;
	phone_status_t lock_state;
} phone_state_t;

typedef enum {
	MTP_DATA_PACKET = 1,
	MTP_BULK_PACKET,
	MTP_EVENT_PACKET,
	MTP_ZLP_PACKET,
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
mtp_int32 _util_get_battery_level(void);
mtp_bool _util_get_serial(mtp_char *serial, mtp_uint32 len);
void _util_get_model_name(mtp_char *model_name, mtp_uint32 len);
void _util_get_vendor_ext_desc(mtp_char *vendor_ext_desc, mtp_uint32 len);
void _util_get_device_version(mtp_char *device_version, mtp_uint32 len);
void _util_gen_alt_serial(mtp_char *serial, mtp_uint32 len);
void _util_get_usb_status(phone_status_t *val);
phone_status_t _util_get_local_usb_status(void);
void _util_set_local_usb_status(const phone_status_t val);
void _util_get_mmc_status(phone_status_t *val);
phone_status_t _util_get_local_mmc_status(void);
void _util_set_local_mmc_status(const phone_status_t val);
void _util_get_usbmode_status(phone_status_t *val);
phone_status_t _util_get_local_usbmode_status(void);
void _util_set_local_usbmode_status(const phone_status_t val);

#endif /* _MTP_UTIL_H_ */
