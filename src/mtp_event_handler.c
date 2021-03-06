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
#include <sys/types.h>
#include <signal.h>
#include <glib.h>
#include "mtp_event_handler.h"
#include "mtp_cmd_handler.h"
#include "mtp_util.h"
#include "mtp_thread.h"
#include "mtp_init.h"
#include "mtp_usb_driver.h"
#include "mtp_transport.h"

/*
 * GLOBAL AND EXTERN VARIABLES
 */
extern mtp_mgr_t g_mtp_mgr;
pthread_t g_eh_thrd;	/* event handler thread */
mtp_int32 g_pipefd[2];

/*
 * FUNCTIONS
 */
/* LCOV_EXCL_START */
static mtp_bool __send_events_from_device_to_pc(mtp_dword store_id,
		mtp_uint16 ptp_event, mtp_uint32 param1, mtp_uint32 param2)
{
	cmd_container_t event = { 0, };

	memset(&event, 0, sizeof(cmd_container_t));

	switch (ptp_event) {
	case PTP_EVENTCODE_OBJECTADDED:
		DBG("case PTP_EVENTCODE_OBJECTADDED\n");
		DBG("param1 : [0x%x]\n", param1);
		_hdlr_init_event_container(&event, PTP_EVENTCODE_OBJECTADDED,
				0, param1, 0);
		break;

	case PTP_EVENTCODE_OBJECTREMOVED:
		DBG("case PTP_EVENTCODE_OBJECTREMOVED\n");
		DBG("param1 [0x%x]\n", param1);
		_hdlr_init_event_container(&event, PTP_EVENTCODE_OBJECTREMOVED,
				0, param1 , 0);
		break;

	default:
		DBG("Event not supported\n");
		return FALSE;
	}

	return _hdlr_send_event_container(&event);
}

static mtp_bool __process_event_request(mtp_event_t *evt)
{
	retv_if(evt == NULL, FALSE);

	switch (evt->action) {
	case EVENT_CANCEL_INITIALIZATION:
		DBG("EVENT_CANCEL_INITIALIZATION entered.\n");
		_device_uninstall_storage();
		break;

	case EVENT_START_MAIN_OP:
		DBG("EVENT_START_MAIN_OP entered.\n");

		/* start MTP */
		g_status->cancel_intialization = FALSE;
		g_status->mtp_op_state = MTP_STATE_INITIALIZING;

		_mtp_init();
		g_status->mtp_op_state = MTP_STATE_READY_SERVICE;
		if (FALSE == _transport_init_interfaces(_receive_mq_data_cb)) {
			ERR("USB init fail\n");
			kill(getpid(), SIGTERM);
			break;
		}
		g_status->mtp_op_state = MTP_STATE_ONSERVICE;
		break;

	case EVENT_USB_REMOVED:
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

	case EVENT_CLOSE:
		break;

	default:
		ERR("Unknown action\n");
		break;
	}
	return TRUE;
}

static void *__thread_event_handler(void *arg)
{
	DBG("__thread_event_handler is started\n");

	mtp_int32 flag = 1;
	mtp_event_t evt;

	while (flag) {
		mtp_int32 status = 0;
		status = read(g_pipefd[0], &evt, sizeof(mtp_event_t));
		if ((status == -1) && errno == EINTR) {
			ERR("read() Fail\n");
			continue;
		}

		__process_event_request(&evt);

		if (evt.action == EVENT_CLOSE) {
			/* USB removed, terminate the thread */
			flag = 0;
		}
	}

	DBG("Event handler terminated\n");
	close(g_pipefd[0]);
	close(g_pipefd[1]);
	mtp_end_event();

	_util_thread_exit("__thread_event_handler thread is over.");
	return NULL;
}

static mtp_bool __send_start_event_to_eh_thread(void)
{
	mtp_event_t event;
	mtp_int32 status;

	event.action = EVENT_START_MAIN_OP;
	event.param1 = 0;
	event.param2 = 0;
	event.param3 = 0;

	DBG("Action : START MTP OPERATION\n");

	status = write(g_pipefd[1], &event, sizeof(mtp_event_t));
	retvm_if(status == -1 || errno == EINTR, FALSE,
		"Event write over pipe Fail, status= [%d],pipefd = [%d], errno [%d]\n",
		status, g_pipefd[1], errno);

	return TRUE;
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
	if (status == -1 || errno == EINTR) {
		ERR("Event write over pipe Fail, status = [%d], pipefd = [%d], errno [%d]\n",
				status, g_pipefd[1], errno);
	}
}

mtp_bool _eh_handle_usb_events(mtp_uint32 type)
{
	mtp_state_t state;
	mtp_int32 res = 0;
	/* Prevent repeated USB insert/remove mal-function */
	static mtp_int32 is_usb_inserted = 0;
	static mtp_int32 is_usb_removed = 0;

	state = g_status->mtp_op_state;

	switch (type) {
	case USB_INSERTED:
		retvm_if(is_usb_inserted == 1, TRUE, "USB is already connected\n");

		/* check USB connection state */
		is_usb_inserted = 1;

		g_status->is_usb_discon = FALSE;
		g_status->cancel_intialization = FALSE;

		if (state == MTP_STATE_INITIALIZING) {
			ERR("MTP is already being initialized\n");
			break;
		}

		res = pipe(g_pipefd);
		if (res < 0) {
			ERR("pipe() Fail\n");
			_util_print_error();
			return FALSE;
		}

		res = _util_thread_create(&g_eh_thrd,
				"Mtp Event Request Handler", PTHREAD_CREATE_JOINABLE,
				__thread_event_handler, NULL);
		retvm_if(!res, FALSE, "_util_thread_create() Fail\n");

		__send_start_event_to_eh_thread();

		break;

	case USB_REMOVED:
		retvm_if(is_usb_removed == 1, TRUE, "USB is already removed\n");

		is_usb_removed = 1;
		DBG("USB is disconnected\n");

		g_status->is_usb_discon = TRUE;
		g_status->cancel_intialization = TRUE;

		/* cancel all transaction */
		g_status->ctrl_event_code = PTP_EVENTCODE_CANCELTRANSACTION;

		_transport_usb_finalize();
		g_status->mtp_op_state = MTP_STATE_STOPPED;

		/*
		 * Temp file should be deleted after usb usb read/write threads
		 * are terminated. Because data receive thread tries to
		 * write the temp file until sink thread is terminated.
		 */
		if (g_mtp_mgr.ftemp_st.filepath != NULL &&
				(access(g_mtp_mgr.ftemp_st.filepath, F_OK) == 0)) {
			DBG("USB disconnected but temp file is remaind.\
					It will be deleted.\n");

			if (g_mtp_mgr.ftemp_st.fhandle != NULL) {
				DBG("handle is found. At first close file\n");
				_util_file_close(g_mtp_mgr.ftemp_st.fhandle);
				g_mtp_mgr.ftemp_st.fhandle = NULL;
			}
			if (remove(g_mtp_mgr.ftemp_st.filepath) < 0) {
				ERR_SECURE("remove(%s) Fail\n", g_mtp_mgr.ftemp_st.filepath);
				_util_print_error();
			}
			g_free(g_mtp_mgr.ftemp_st.filepath);
			g_mtp_mgr.ftemp_st.filepath = NULL;
		}

		_mtp_deinit();
		_device_uninstall_storage();
		_eh_send_event_req_to_eh_thread(EVENT_CLOSE, 1, 0, NULL);
		break;

	default:
		ERR("can be ignored notify [0x%x]\n", type);
		break;
	}
	return TRUE;
}

/* LCOV_EXCL_STOP */
