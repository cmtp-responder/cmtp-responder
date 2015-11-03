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

#ifndef _MTP_EVENT_HANDLER_H_
#define _MTP_EVENT_HANDLER_H_

#include <vconf.h>
#include "mtp_datatype.h"
#include "mtp_util.h"

typedef enum {
	MTP_TURN_OFF,
	MTP_ASK_TURN_ON,
	MTP_ALWAYS_TURN_ON
} on_off_state_t;

typedef enum {
	USB_INSERTED,
	USB_REMOVED
} usb_state_t;

typedef struct {
	mtp_uint32 action;
	mtp_ulong param1;
	mtp_ulong param2;
	mtp_ulong param3;
} mtp_event_t;

typedef enum {
	EVENT_CANCEL_INITIALIZATION,
	EVENT_START_MAIN_OP,
	EVENT_CLOSE,
	EVENT_USB_REMOVED,
	EVENT_OBJECT_ADDED,
	EVENT_OBJECT_REMOVED,
	EVENT_OBJECT_PROP_CHANGED,
	EVENT_START_DATAIN,
	EVENT_DONE_DATAIN,
	EVENT_START_DATAOUT,
	EVENT_DONE_DATAOUT,
	EVENT_MAX
} event_code_t;

mtp_bool _eh_register_notification_callbacks(void);
mtp_bool _eh_handle_usb_events(mtp_uint32 type);
void _eh_deregister_notification_callbacks(void);
void _handle_mmc_notification(keynode_t *key, void *data);
void _eh_send_event_req_to_eh_thread(event_code_t action, mtp_ulong param1,
		mtp_ulong param2, void *param3);

#endif	/* _MTP_EVENT_HANDLER_H_ */
