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
#include <sys/types.h>
#include <signal.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <glib.h>
#include "mtp_event_handler.h"
#include "mtp_cmd_handler.h"
#include "mtp_util.h"
#include "mtp_thread.h"
#include "mtp_init.h"
#include "mtp_usb_driver.h"
#include "mtp_transport.h"
#include "mtp_media_info.h"

/*
 * GLOBAL AND EXTERN VARIABLES
 */
extern mtp_mgr_t g_mtp_mgr;
pthread_t g_eh_thrd;	/* event handler thread */
mtp_int32 g_pipefd[2];

/*
 * STATIC VARIABLES
 */
static mtp_mgr_t *g_mgr = &g_mtp_mgr;

/*
 * STATIC FUNCTIONS
 */
static mtp_bool __process_event_request(mtp_event_t *evt);
static void *__thread_event_handler(void *arg);
static mtp_bool __send_events_from_device_to_pc(mtp_dword store_id,
		mtp_uint16 ptp_event, mtp_uint32 param1, mtp_uint32 param2);
static void __handle_usb_notification(keynode_t *key, void *data);
static void __handle_usb_mode_notification(keynode_t *key, void *data);
static mtp_bool __send_start_event_to_eh_thread(void);

/*
 * FUNCTIONS
 */
mtp_bool _eh_register_notification_callbacks(void)
{
	mtp_int32 ret;
	phone_status_t val = 0;

	_util_get_usb_status(&val);
	_util_set_local_usb_status(val);
	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS,
			__handle_usb_notification, NULL);
	if (ret < 0) {
		ERR("vconf_notify_key_changed(%s) Fail", VCONFKEY_SYSMAN_USB_STATUS);
		return FALSE;
	}

	_util_get_usbmode_status(&val);
	_util_set_local_usbmode_status(val);
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_MODE_INT,
			__handle_usb_mode_notification, NULL);
	if (ret < 0) {
		ERR("vconf_notify_key_changed(%s) Fail", VCONFKEY_SETAPPL_USB_MODE_INT);
		return FALSE;
	}

	_util_get_mmc_status(&val);
	_util_set_local_mmc_status(val);

	DBG("Phone status: USB = [%d] MMC = [%d] USB_MODE = [%d]\n",
			_util_get_local_usb_status(), _util_get_local_mmc_status(),
			_util_get_local_usbmode_status());

	return TRUE;
}

mtp_bool _eh_handle_usb_events(mtp_uint32 type)
{
	mtp_state_t state = MTP_STATE_STOPPED;
	mtp_int32 res = 0;
	/* Prevent repeated USB insert/remove mal-function */
	static mtp_int32 is_usb_inserted = 0;
	static mtp_int32 is_usb_removed = 0;

	state = _transport_get_mtp_operation_state();

	switch (type) {
	case USB_INSERTED:
		if (is_usb_inserted == 1) {
			ERR("USB is already connected");
			return TRUE;
		}

		is_usb_inserted = 1;
		/* check USB connection state */
		if (MTP_PHONE_USB_DISCONNECTED == _util_get_local_usb_status()) {
			ERR("USB is disconnected. So just return.");
			return FALSE;
		}

		_transport_set_usb_discon_state(FALSE);
		_transport_set_cancel_initialization(FALSE);

		if (state == MTP_STATE_INITIALIZING) {
			ERR("MTP is already being initialized");
			break;
		}

		res = pipe(g_pipefd);
		if (res < 0) {
			ERR("pipe() Fail");
			_util_print_error();
			return FALSE;
		}

		res = _util_thread_create(&g_eh_thrd,
				"Mtp Event Request Handler", PTHREAD_CREATE_JOINABLE,
				__thread_event_handler, NULL);
		if (FALSE == res) {
			ERR("_util_thread_create() Fail");
			return FALSE;
		}

		__send_start_event_to_eh_thread();

		break;

	case USB_REMOVED:
		if (is_usb_removed == 1) {
			ERR("USB is already removed");
			return TRUE;
		}

		is_usb_removed = 1;
		DBG("USB is disconnected");

		_transport_set_usb_discon_state(TRUE);
		_transport_set_cancel_initialization(TRUE);

		/* cancel all transaction */
		_transport_set_control_event(PTP_EVENTCODE_CANCELTRANSACTION);

		_transport_usb_finalize();
		_transport_set_mtp_operation_state(MTP_STATE_STOPPED);

		/*
		 * Temp file should be deleted after usb usb read/write threads
		 * are terminated. Because data receive thread tries to
		 * write the temp file until sink thread is terminated.
		 */
		if (g_mgr->ftemp_st.filepath != NULL &&
				(access(g_mgr->ftemp_st.filepath, F_OK) == 0)) {
			DBG("USB disconnected but temp file is remaind.\
					It will be deleted.");

			if (g_mgr->ftemp_st.fhandle != INVALID_FILE) {
				DBG("handle is found. At first close file");
				_util_file_close(g_mgr->ftemp_st.fhandle);
				g_mgr->ftemp_st.fhandle = INVALID_FILE;
			}
			if (remove(g_mgr->ftemp_st.filepath) < 0) {
				ERR_SECURE("remove(%s) Fail", g_mgr->ftemp_st.filepath);
				_util_print_error();
			}
			g_free(g_mgr->ftemp_st.filepath);
			g_mgr->ftemp_st.filepath = NULL;
		}

		_mtp_deinit();
		_device_uninstall_storage(MTP_ADDREM_AUTO);
		_eh_send_event_req_to_eh_thread(EVENT_CLOSE, 1, 0, NULL);
		break;

	default:
		ERR("can be ignored notify [0x%x]\n", type);
		break;
	}
	return TRUE;
}

static mtp_bool __process_event_request(mtp_event_t *evt)
{
	retv_if(evt == NULL, FALSE);

	switch (evt->action) {
	case EVENT_CANCEL_INITIALIZATION:
		DBG("EVENT_CANCEL_INITIALIZATION entered.");
		_device_uninstall_storage(MTP_ADDREM_AUTO);
		break;

	case EVENT_START_MAIN_OP:
		DBG("EVENT_START_MAIN_OP entered.");

		/* start MTP */
		_transport_set_cancel_initialization(FALSE);
		_transport_set_mtp_operation_state(MTP_STATE_INITIALIZING);

		_mtp_init(evt->param1);
		_transport_set_mtp_operation_state(MTP_STATE_READY_SERVICE);
		if (FALSE == _transport_init_interfaces(_receive_mq_data_cb)) {
			ERR("USB init fail");
			kill(getpid(), SIGTERM);
			break;
		}
		_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);
		break;

	case EVENT_USB_REMOVED:
		_util_flush_db();
		_eh_handle_usb_events(USB_REMOVED);
		break;

	case EVENT_OBJECT_ADDED:
		__send_events_from_device_to_pc(0, PTP_EVENTCODE_OBJECTADDED,
				evt->param1, 0);
		break;

	case EVENT_OBJECT_REMOVED:
		__send_events_from_device_to_pc(0,
				PTP_EVENTCODE_OBJECTREMOVED, evt->param1, 0);
		break;

	case EVENT_OBJECT_PROP_CHANGED:
		__send_events_from_device_to_pc(0,
				MTP_EVENTCODE_OBJECTPROPCHANGED, evt->param1,
				evt->param2);
		break;

	case EVENT_CLOSE:
		break;

	default:
		ERR("Unknown action");
		break;
	}
	return TRUE;
}

static void *__thread_event_handler(void *arg)
{
	DBG("__thread_event_handler is started ");

	mtp_int32 flag = 1;
	mtp_event_t evt;

	while (flag) {
		mtp_int32 status = 0;
		status = read(g_pipefd[0], &evt, sizeof(mtp_event_t));
		if(( status== -1) && errno == EINTR) {
			ERR("read() Fail");
			continue;
		}

		__process_event_request(&evt);

		if (evt.action == EVENT_CLOSE) {
			/* USB removed, terminate the thread */
			flag = 0;
		}
	}

	DBG("######### MTP TERMINATED #########");
	close(g_pipefd[0]);
	close(g_pipefd[1]);

	_util_thread_exit("__thread_event_handler thread is over.");
	return NULL;
}

static mtp_bool __send_events_from_device_to_pc(mtp_dword store_id,
		mtp_uint16 ptp_event, mtp_uint32 param1, mtp_uint32 param2)
{
	cmd_container_t event = { 0, };

	memset(&event, 0, sizeof(cmd_container_t));

	switch (ptp_event) {
	case PTP_EVENTCODE_STOREADDED:
		DBG("case PTP_EVENTCODE_STOREADDED:");
		DBG("store_id [0x%x]\n", store_id);
		_hdlr_init_event_container(&event, PTP_EVENTCODE_STOREADDED, 0,
				store_id, 0);
		break;

	case PTP_EVENTCODE_STOREREMOVED:
		DBG("case PTP_EVENTCODE_STOREREMOVED");
		DBG("store_id [0x%x]\n", store_id);
		_hdlr_init_event_container(&event,
				PTP_EVENTCODE_STOREREMOVED, 0, store_id, 0);
		break;

	case PTP_EVENTCODE_OBJECTADDED:
		DBG("case PTP_EVENTCODE_OBJECTADDED");
		DBG("param1 : [0x%x]\n", param1);
		_hdlr_init_event_container(&event, PTP_EVENTCODE_OBJECTADDED,
				0, param1, 0);
		break;

	case PTP_EVENTCODE_OBJECTREMOVED:
		DBG("case PTP_EVENTCODE_OBJECTREMOVED");
		DBG("param1 [0x%x]\n", param1);
		_hdlr_init_event_container(&event, PTP_EVENTCODE_OBJECTREMOVED,
				0, param1 , 0);
		break;

	case MTP_EVENTCODE_OBJECTPROPCHANGED:
		DBG("case MTP_EVENTCODE_OBJECTPROPCHANGED");
		DBG("param1 [0x%x]\n", param1);
		DBG("param2 [0x%x]\n", param2);
		_hdlr_init_event_container_with_param(&event,
				MTP_EVENTCODE_OBJECTPROPCHANGED, 0, param1 , param2);
		break;

	default:
		DBG("Event not supported");
		return FALSE;
	}

	return _hdlr_send_event_container(&event);
}

static void __handle_usb_notification(keynode_t *key, void *data)
{
	phone_status_t val = MTP_PHONE_USB_DISCONNECTED;
	mtp_int32 intval = VCONFKEY_SYSMAN_USB_DISCONNECTED;

	ret_if(key == NULL);

	intval = vconf_keynode_get_int(key);
	if (-1 == intval) {
		ERR("vconf_keynode_get_int() Fail");
		return;
	}

	if (VCONFKEY_SYSMAN_USB_DISCONNECTED == intval) {
		DBG("USB Disconnected");
		_util_set_local_usb_status(val);
		mtp_end_event();
		return;
	}

	val = MTP_PHONE_USB_CONNECTED;
	_util_set_local_usb_status(val);
	DBG("USB Connected. Just return.");
	return;
}

static void __handle_usb_mode_notification(keynode_t *key, void *data)
{
	phone_status_t val = MTP_PHONE_USB_MODE_OTHER;

	ret_if(key == NULL);

	val = vconf_keynode_get_int(key);

	_util_set_local_usbmode_status(val);
	return;
}

void _handle_mmc_notification(keynode_t *key, void *data)
{
	phone_status_t val = MTP_PHONE_MMC_NONE;

	_util_get_mmc_status(&val);
	_util_set_local_mmc_status(val);

	if (MTP_PHONE_MMC_INSERTED == val) {
		_device_install_storage(MTP_ADDREM_EXTERNAL);
		__send_events_from_device_to_pc(MTP_EXTERNAL_STORE_ID,
				PTP_EVENTCODE_STOREADDED, 0, 0);

	} else if (MTP_PHONE_MMC_NONE == val) {
		_device_uninstall_storage(MTP_ADDREM_EXTERNAL);

		__send_events_from_device_to_pc(MTP_EXTERNAL_STORE_ID,
				PTP_EVENTCODE_STOREREMOVED, 0, 0);
	}

	return;
}

void _eh_deregister_notification_callbacks(void)
{
	vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_STATUS,
			__handle_usb_notification);
	vconf_ignore_key_changed(VCONFKEY_SETAPPL_USB_MODE_INT,
			__handle_usb_mode_notification);

	return;
}

void _eh_send_event_req_to_eh_thread(event_code_t action, mtp_ulong param1,
		mtp_ulong param2, void *param3)
{
	mtp_event_t event = { 0 };
	mtp_int32 status;

	event.action = action;
	event.param1 = param1;
	event.param2 = param2;
	event.param3 = (mtp_ulong)param3;

	DBG("action[%d], param1[%ld], param2[%ld]\n", action, param1, param2);

	status = write(g_pipefd[1], &event, sizeof(mtp_event_t));
	if(status== -1 || errno == EINTR) {
		ERR("Event write over pipe Fail, status = [%d], pipefd = [%d], errno [%d]\n",
				status, g_pipefd[1], errno);
	}
	return;
}

static mtp_bool __send_start_event_to_eh_thread(void)
{
	mtp_event_t event;
	mtp_int32 status;

	event.action = EVENT_START_MAIN_OP;
	event.param1 = (mtp_ulong) MTP_ADDREM_AUTO;
	event.param2 = 0;
	event.param3 = 0;

	DBG("Action : START MTP OPERATION");

	status = write(g_pipefd[1], &event, sizeof(mtp_event_t));
	if(status== -1 || errno == EINTR) {
		ERR("Event write over pipe Fail, status= [%d],pipefd = [%d], errno [%d]\n",
				status, g_pipefd[1], errno);
		return FALSE;
	}

	return TRUE;
}
