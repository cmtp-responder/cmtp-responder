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

#include <sys/vfs.h>
#include <linux/magic.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <vconf.h>
#include "mtp_support.h"
#include "ptp_datacodes.h"
#include "mtp_media_info.h"
#include "mtp_usb_driver.h"
#include "mtp_cmd_handler.h"
#include "mtp_cmd_handler_util.h"
#include "mtp_transport.h"
#include "mtp_thread.h"

/*
 * GLOBAL AND EXTERN VARIABLES
 */
extern mtp_mgr_t g_mtp_mgr;
extern mtp_bool g_is_full_enum;
extern pthread_mutex_t g_cmd_inoti_mutex;
extern mtp_config_t g_conf;

mtp_bool g_is_sync_estab = FALSE;

/*
 * STATIC VARIABLES
 */
static mtp_mgr_t *g_mgr = &g_mtp_mgr;
static mtp_bool g_has_round_trip = FALSE;
#ifdef MTP_USE_SKIP_CONTINUOUS_OPENSESSION
static mtp_uint16 g_count_open_session = 0;
static mtp_uint32 g_old_open_session_time = 0;
#endif/*MTP_USE_SKIP_CONTINUOUS_OPENSESSION*/

/*
 * STATIC FUNCTIONS
 */
static void __process_commands(mtp_handler_t *hdlr, cmd_blk_t *cmd);
static void __open_session(mtp_handler_t *hdlr);
static void __get_device_info(mtp_handler_t *hdlr);
static void __get_storage_ids(mtp_handler_t *hdlr);
static void __get_storage_info(mtp_handler_t *hdlr);
static void __get_num_objects(mtp_handler_t *hdlr);
static void __get_object_handles(mtp_handler_t *hdlr);
static void __get_object_info(mtp_handler_t *hdlr);
static void __get_object(mtp_handler_t *hdlr);
static void __send_object_info(mtp_handler_t *hdlr);
static void __send_object(mtp_handler_t *hdlr);
static void __delete_object(mtp_handler_t *hdlr);
static void __format_store(mtp_handler_t *hdlr);
static void __reset_device(mtp_handler_t *hdlr);
static void __get_device_prop_desc(mtp_handler_t *hdlr);
static void __get_device_prop_value(mtp_handler_t *hdlr);
static void __set_device_prop_value(mtp_handler_t *hdlr);
static void __get_partial_object(mtp_handler_t *hdlr);
static void __get_object_references(mtp_handler_t *hdlr);
static void __set_object_references(mtp_handler_t *hdlr);
static void __get_object_prop_desc(mtp_handler_t *hdlr);
static void __get_object_prop_supported(mtp_handler_t *hdlr);
static void __get_object_prop_value(mtp_handler_t *hdlr);
static void __set_object_prop_value(mtp_handler_t *hdlr);
static void __get_object_prop_list(mtp_handler_t *hdlr);
static void __set_object_prop_list(mtp_handler_t *hdlr);
static void __report_acquired_content(mtp_handler_t *hdlr);
static void __send_playback_skip(mtp_handler_t *hdlr);
#ifndef PMP_VER
static void __self_test(mtp_handler_t *hdlr);
#ifdef MTP_SUPPORT_SET_PROTECTION
static void __set_object_protection(mtp_handler_t *hdlr);
#endif /* MTP_SUPPORT_SET_PROTECTION */
static void __power_down(mtp_handler_t *hdlr);
static void __move_object(mtp_handler_t *hdlr);
static void __copy_object(mtp_handler_t *hdlr);
static void __reset_device_prop_value(mtp_handler_t *hdlr);
static void __vendor_command1(mtp_handler_t *hdlr);
static void __get_interdep_prop_desc(mtp_handler_t *hdlr);
static void __send_object_prop_list(mtp_handler_t *hdlr);
#endif /* PMP_VER */
static void __close_session(mtp_handler_t *hdlr);
#ifdef MTP_SUPPORT_PRINT_COMMAND
static void __print_command(mtp_uint16 code);
#endif /* MTP_SUPPORT_PRINT_COMMAND */
static void __enum_store_not_enumerated(mtp_uint32 obj_handle,
		mtp_uint32 fmt, mtp_uint32 depth);
static mtp_bool __receive_temp_file_first_packet(mtp_char *data,
		mtp_int32 data_len);
static mtp_bool __receive_temp_file_next_packets(mtp_char *data,
		mtp_int32 data_len);
static void __finish_receiving_file_packets(mtp_char *data, mtp_int32 data_len);

/*
 * FUNCTIONS
 */
void _cmd_hdlr_reset_cmd(mtp_handler_t *hdlr)
{
	if (hdlr->data4_send_obj.obj != NULL) {
		_entity_dealloc_mtp_obj(hdlr->data4_send_obj.obj);
	}

	memset(hdlr, 0x00, sizeof(mtp_handler_t));

#ifdef MTP_USE_SKIP_CONTINUOUS_OPENSESSION
	/* reset open session count */
	g_count_open_session = 0;
	g_old_open_session_time = 0;
#endif /* MTP_USE_SKIP_CONTINUOUS_OPENSESSION */
}

static void __process_commands(mtp_handler_t *hdlr, cmd_blk_t *cmd)
{
	mtp_store_t *store = NULL;

	/* Keep a local copy for this command */
	_hdlr_copy_cmd_container(cmd, &(hdlr->usb_cmd));

	if (hdlr->usb_cmd.code == PTP_OPCODE_GETDEVICEINFO) {
		DBG("COMMAND CODE = [0x%4x]!!\n", hdlr->usb_cmd.code);
#ifdef MTP_SUPPORT_PRINT_COMMAND
		__print_command(hdlr->usb_cmd.code);
#endif /*MTP_SUPPORT_PRINT_COMMAND*/
		__get_device_info(hdlr);
		goto DONE;
	}

	/*  Process OpenSession Command */
	if (hdlr->usb_cmd.code == PTP_OPCODE_OPENSESSION) {
		DBG("COMMAND CODE = [0x%4X]!!\n", hdlr->usb_cmd.code);
#ifdef MTP_SUPPORT_PRINT_COMMAND
		__print_command(hdlr->usb_cmd.code);
#endif /*MTP_SUPPORT_PRINT_COMMAND*/
#ifdef MTP_USE_SKIP_CONTINUOUS_OPENSESSION
		time_t t;
		mtp_uint32 cur_time = 0;
		time(&t);
		cur_time = (mtp_uint32)t;
		/*first opensession*/
		if (g_count_open_session == 0) {
			g_old_open_session_time = cur_time;
		} else if (cur_time <= g_old_open_session_time + 1) {
			/*under 1 sec. it might be skipped*/
			ERR("skip continuous OPEN session");
			goto DONE;
		}
		++g_count_open_session;

#endif /* MTP_USE_SKIP_CONTINUOUS_OPENSESSION */
		__open_session(hdlr);
		goto DONE;
	}
#ifdef MTP_USE_SKIP_CONTINUOUS_OPENSESSION
	g_count_open_session = 0;
#endif /* MTP_USE_SKIP_CONTINUOUS_OPENSESSION */

	/* Check if session is open or not, if not respond */
	if (hdlr->session_id == 0) {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_SESSIONNOTOPEN);
		_device_set_phase(DEVICE_PHASE_IDLE);
		goto DONE;
	}
	DBG("COMMAND CODE = [0x%4x]!!\n", hdlr->usb_cmd.code);
#ifdef MTP_SUPPORT_PRINT_COMMAND
	__print_command(hdlr->usb_cmd.code);
#endif /* MTP_SUPPORT_PRINT_COMMAND */

	switch (hdlr->usb_cmd.code) {
	case PTP_OPCODE_CLOSESESSION:
		__close_session(hdlr);
		break;
	case PTP_OPCODE_GETSTORAGEIDS:
		__get_storage_ids(hdlr);
		break;
	case PTP_OPCODE_GETSTORAGEINFO:
		__get_storage_info(hdlr);
		break;
	case PTP_OPCODE_GETNUMOBJECTS:
		__get_num_objects(hdlr);
		break;
	case PTP_OPCODE_GETOBJECTHANDLES:
		__get_object_handles(hdlr);
		break;
	case PTP_OPCODE_GETOBJECTINFO:
		__get_object_info(hdlr);
		break;
	case PTP_OPCODE_GETOBJECT:
		_eh_send_event_req_to_eh_thread(EVENT_START_DATAIN, 0, 0, NULL);
		__get_object(hdlr);
		_eh_send_event_req_to_eh_thread(EVENT_DONE_DATAIN, 0, 0, NULL);
		break;
	case PTP_OPCODE_DELETEOBJECT:
		__delete_object(hdlr);
		break;
	case PTP_OPCODE_FORMATSTORE:
		__format_store(hdlr);
		break;
	case PTP_OPCODE_GETDEVICEPROPDESC:
		__get_device_prop_desc(hdlr);
		break;
	case PTP_OPCODE_GETDEVICEPROPVALUE:
		__get_device_prop_value(hdlr);
		break;
	case PTP_OPCODE_GETPARTIALOBJECT:
		__get_partial_object(hdlr);
		break;
	case MTP_OPCODE_GETOBJECTREFERENCES:
		__get_object_references(hdlr);
		break;
	case MTP_OPCODE_GETOBJECTPROPDESC:
		__get_object_prop_desc(hdlr);
		break;
	case MTP_OPCODE_GETOBJECTPROPSUPPORTED:
		__get_object_prop_supported(hdlr);
		break;
	case MTP_OPCODE_GETOBJECTPROPVALUE:
		__get_object_prop_value(hdlr);
		break;
	case MTP_OPCODE_GETOBJECTPROPLIST:
		__get_object_prop_list(hdlr);
		break;
#ifndef PMP_VER
	case PTP_OPCODE_RESETDEVICE:
		__reset_device(hdlr);
		break;
	case PTP_OPCODE_SELFTEST:
		__self_test(hdlr);
		break;
#ifdef MTP_SUPPORT_SET_PROTECTION
	case PTP_OPCODE_SETOBJECTPROTECTION:
		__set_object_protection(hdlr);
		break;
#endif /* MTP_SUPPORT_SET_PROTECTION */
	case PTP_OPCODE_POWERDOWN:
		__power_down(hdlr);
		break;
	case PTP_OPCODE_RESETDEVICEPROPVALUE:
		__reset_device_prop_value(hdlr);
		break;
	case PTP_OPCODE_MOVEOBJECT:
		__move_object(hdlr);
		break;
	case PTP_OPCODE_COPYOBJECT:
		__copy_object(hdlr);
		break;
	case MTP_OPCODE_GETINTERDEPPROPDESC:
		__get_interdep_prop_desc(hdlr);
		break;
	case PTP_CODE_VENDOR_OP1:	/* Vendor-specific operations */
		__vendor_command1(hdlr);
		break;
#endif /* PMP_VER */
	case MTP_OPCODE_PLAYBACK_SKIP:	/* Playback control */
		__send_playback_skip(hdlr);
		break;

	case MTP_OPCODE_WMP_REPORTACQUIREDCONTENT:
		/* Windows Media 11 extension*/
		__report_acquired_content(hdlr);
		break;
	case PTP_OPCODE_SENDOBJECTINFO:
	case PTP_OPCODE_SENDOBJECT:
	case PTP_OPCODE_SETDEVICEPROPVALUE:
	case MTP_OPCODE_SETOBJECTREFERENCES:
	case MTP_OPCODE_SETOBJECTPROPVALUE:
	case MTP_OPCODE_SETOBJECTPROPLIST:
#ifndef PMP_VER
	case MTP_OPCODE_SENDOBJECTPROPLIST:
#endif /* PMP_VER */
		/* DATA_HANDLE_PHASE: Send operation will be blocked
		 * until data packet is received
		 */
		if (_device_get_phase() == DEVICE_PHASE_IDLE) {
			DBG("DATAOUT COMMAND PHASE!!");
			if (hdlr->usb_cmd.code == PTP_OPCODE_SENDOBJECT)
				_eh_send_event_req_to_eh_thread(EVENT_START_DATAOUT,
						0, 0, NULL);
			_device_set_phase(DEVICE_PHASE_DATAOUT);
			return;	/* in command phase, just return and wait next data */
		}
		switch (hdlr->usb_cmd.code) {
		case PTP_OPCODE_SENDOBJECTINFO:
			__send_object_info(hdlr);
			break;
		case PTP_OPCODE_SENDOBJECT:
			__send_object(hdlr);
			_eh_send_event_req_to_eh_thread(EVENT_DONE_DATAOUT,
					0, 0, NULL);
			break;
		case PTP_OPCODE_SETDEVICEPROPVALUE:
			__set_device_prop_value(hdlr);
			break;
		case MTP_OPCODE_SETOBJECTREFERENCES:
			__set_object_references(hdlr);
			break;
		case MTP_OPCODE_SETOBJECTPROPVALUE:
			__set_object_prop_value(hdlr);
			break;
		case MTP_OPCODE_SETOBJECTPROPLIST:
			__set_object_prop_list(hdlr);
			break;
#ifndef PMP_VER
		case MTP_OPCODE_SENDOBJECTPROPLIST:
			__send_object_prop_list(hdlr);
			break;
#endif /* PMP_VER */
		}
		break;

	default:
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_OP_NOT_SUPPORTED);
		DBG("Unsupported COMMAND[%d]\n", hdlr->usb_cmd.code);
		break;
	}
DONE:
	if (((hdlr->last_opcode == PTP_OPCODE_SENDOBJECTINFO) ||
				(hdlr->last_opcode == MTP_OPCODE_SENDOBJECTPROPLIST)) &&
			((hdlr->last_fmt_code != PTP_FMT_ASSOCIATION) &&
			 (hdlr->last_fmt_code != PTP_FMT_UNDEF))) {
		DBG("Processed, last_opcode[0x%x], last_fmt_code[%d]\n",
				hdlr->last_opcode, hdlr->last_fmt_code);

		if ((hdlr->usb_cmd.code != PTP_OPCODE_SENDOBJECT) &&
				hdlr->data4_send_obj.is_valid &&
				hdlr->data4_send_obj.is_data_sent) {

			DBG("Processed, COMMAND[0x%x]!!\n", hdlr->usb_cmd.code);
			store = _device_get_store(hdlr->data4_send_obj.store_id);
			if (store != NULL) {
				/*Restore reserved space*/
				store->store_info.free_space +=
					hdlr->data4_send_obj.file_size;
			}

			if (hdlr->data4_send_obj.obj) {
				_entity_dealloc_mtp_obj(hdlr->data4_send_obj.obj);
				hdlr->data4_send_obj.obj = NULL;
			}
			memset(&(hdlr->data4_send_obj), 0x00,
					sizeof(hdlr->data4_send_obj));
		}
	}
	hdlr->last_opcode = hdlr->usb_cmd.code;	/* Last operation code*/
}

static void __open_session(mtp_handler_t *hdlr)
{
	mtp_uint32 session_id;
	/*Check the parameters*/
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		/*Unknown parameters*/
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}
	/* Validate parameters*/
	session_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);

	if (session_id == 0 || (hdlr->usb_cmd.tid != 0)) {
		/*Session Id cannot be zero, while TransactionId must be zero.*/
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_INVALIDPARAM);
		return;
	}

	if (hdlr->session_id) {	/*Session already open*/
		_cmd_hdlr_send_response(hdlr,
				PTP_RESPONSE_SESSIONALREADYOPENED, 1,
				(mtp_uint32 *)&(hdlr->session_id));
	} else {	/*Save the Session ID*/
		hdlr->session_id = session_id;
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
	}
}

static void __get_device_info(mtp_handler_t *hdlr)
{
	/* Check the parameters*/
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		/* Unknown parameters*/
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	/* When used outside a session, both the SessionID and TransactionID
	 * in the Operation Request dataset must be 0;
	 */

	if ((hdlr->session_id == 0) &&
			(hdlr->usb_cmd.tid != 0)) {	/*INVALIDPARAMETER*/
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		ERR("Invalid Session ID[%u], Transaction ID[%u]\n",
				hdlr->session_id, hdlr->usb_cmd.tid);
		return;
	}

	/*Build the data block for device info.*/
	data_blk_t blk = { 0 };
	mtp_uint32 num_bytes = 0;
	mtp_uchar *buf_ptr = NULL;

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	num_bytes = _get_device_info_size();
	buf_ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);
	if (num_bytes == _pack_device_info(buf_ptr, num_bytes)) {
		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
		} else {
			/* Host Cancelled data-in transfer */
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			DBG("Device phase is set to DEVICE_PHASE_NOTREADY");
		}
	} else {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

	g_free(blk.data);
}

static void __get_storage_ids(mtp_handler_t *hdlr)
{
	data_blk_t blk = { 0 };
	ptp_array_t ids = { 0 };
	mtp_uint32 resp = PTP_RESPONSE_GEN_ERROR;
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	_prop_init_ptparray(&ids, UINT32_TYPE);

	if (_hutil_get_storage_ids(&ids) == MTP_ERROR_NONE)
	{
		_hdlr_init_data_container(&blk, hdlr->usb_cmd.code,
				hdlr->usb_cmd.tid);
		num_bytes = _prop_get_size_ptparray(&ids);
		ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);
		if (NULL != ptr) {
			/*Pack storage IDs*/
			_prop_pack_ptparray(&ids, ptr, num_bytes);
			_device_set_phase(DEVICE_PHASE_DATAIN);
			/*Send the data block*/
			if (FALSE == _hdlr_send_data_container(&blk)) {
				/*Host Cancelled data-in transfer*/
				_device_set_phase(DEVICE_PHASE_NOTREADY);
				DBG("DEVICE_PHASE_NOTREADY!!");
			} else {
				resp = PTP_RESPONSE_OK;
			}
		}
		g_free(blk.data);
	}
	_prop_deinit_ptparray(&ids);
	_cmd_hdlr_send_response_code(hdlr, (mtp_uint16) resp);
}

static void __get_storage_info(mtp_handler_t *hdlr)
{
	mtp_uint32 store_id = 0;
	store_info_t store_info = { 0 };
	mtp_uint32 pkt_sz = 0;
	data_blk_t blk = { 0 };
	mtp_uchar *ptr = NULL;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		/*Unknown parameters*/
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	store_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);

	if (_hutil_get_storage_entry(store_id, &store_info) != MTP_ERROR_NONE) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_INVALID_STORE_ID);
		return;
	}

	/* Build the data block for StorageInfo Dataset.*/
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	pkt_sz = _hutil_get_storage_info_size(&store_info);
	ptr = _hdlr_alloc_buf_data_container(&blk, pkt_sz, pkt_sz);

	if (pkt_sz == _entity_pack_store_info(&store_info, ptr, pkt_sz)) {
		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
		} else {
			/*Host Cancelled data-in transfer*/
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			DBG("DEVICE_PHASE_NOTREADY!!");
		}
	} else {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

	g_free(blk.data);
}

static void __get_num_objects(mtp_handler_t *hdlr)
{
	mtp_uint16 resp = 0;
	mtp_uint32 store_id = 0;
	mtp_uint32 h_parent = 0;
	mtp_uint32 fmt = 0;
	mtp_uint32 num_obj = 0;

	store_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	h_parent = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);

	switch (_hutil_get_num_objects(store_id, h_parent, fmt,
				(mtp_uint32 *)(&num_obj))) {
	case MTP_ERROR_INVALID_STORE:
		resp = PTP_RESPONSE_INVALID_STORE_ID;
		break;
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_INVALID_PARENT:
		resp = PTP_RESPONSE_INVALIDPARENT;
		break;
	case MTP_ERROR_STORE_NOT_AVAILABLE:
		resp = PTP_RESPONSE_STORENOTAVAILABLE;
		break;
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	if (resp == PTP_RESPONSE_OK) {
		_cmd_hdlr_send_response(hdlr, resp, 1, (mtp_uint32 *)&num_obj);
	} else {
		_cmd_hdlr_send_response_code(hdlr, resp);
	}
}

static void __get_object_handles(mtp_handler_t *hdlr)
{
	mtp_uint32 store_id = 0;
	mtp_uint32 fmt = 0;
	mtp_uint32 h_parent = 0;
	ptp_array_t handle_arr = { 0 };
	data_blk_t blk = { 0 };
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;
	mtp_uint16 resp = 0;

	_prop_init_ptparray(&handle_arr, UINT32_TYPE);

	store_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	h_parent = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);

	DBG("store_id = [0x%x], Format Code = [0x%x], parent handle = [0x%x]\n",
			store_id, fmt, h_parent);
	switch (_hutil_get_object_handles(store_id, fmt, h_parent,
				&handle_arr)) {
	case MTP_ERROR_INVALID_STORE:
		resp = PTP_RESPONSE_INVALID_STORE_ID;
		break;
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_INVALID_PARENT:
		resp = PTP_RESPONSE_INVALIDPARENT;
		break;
	case MTP_ERROR_STORE_NOT_AVAILABLE:
		resp = PTP_RESPONSE_STORENOTAVAILABLE;
		break;
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	if (resp != PTP_RESPONSE_OK) {
		_prop_deinit_ptparray(&handle_arr);
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	num_bytes = _prop_get_size_ptparray(&handle_arr);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);
	if (NULL != ptr) {
		_prop_pack_ptparray(&handle_arr, ptr, num_bytes);
		_prop_deinit_ptparray(&handle_arr);
		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
		} else {
			/*Host Cancelled data-in transfer.*/
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			DBG("DEVICE_PHASE_NOTREADY!!");
		}
	} else {
		_prop_deinit_ptparray(&handle_arr);
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

	g_free(blk.data);
}

static void __get_object_info(mtp_handler_t *hdlr)
{
	mtp_uint32 obj_handle = 0;
	data_blk_t blk = { 0 };
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;
	mtp_obj_t *obj = NULL;
	mtp_char f_name[MTP_MAX_FILENAME_SIZE + 1] = { 0 };
	mtp_wchar wf_name[MTP_MAX_FILENAME_SIZE + 1] = { 0 };
	ptp_string_t ptp_string = { 0 };

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}
	obj_handle = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	if (MTP_ERROR_NONE != _hutil_get_object_entry(obj_handle, &obj)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_INVALID_OBJ_HANDLE);
		return;
	}

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);

	_util_get_file_name(obj->file_path, f_name);
	_util_utf8_to_utf16(wf_name, sizeof(wf_name) / WCHAR_SIZ, f_name);
	_prop_copy_char_to_ptpstring(&ptp_string, wf_name, WCHAR_TYPE);

	num_bytes = _entity_get_object_info_size(obj, &ptp_string);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);
	if (num_bytes == _entity_pack_obj_info(obj, &ptp_string, ptr,
				num_bytes)) {
		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
		} else {
			/* Host Cancelled data-in transfer*/
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			DBG("DEVICE_PHASE_NOTREADY!!");
		}
	} else {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

	g_free(blk.data);
}

static void __get_object(mtp_handler_t *hdlr)
{
	mtp_uint32 obj_handle;
	mtp_obj_t *obj;
	mtp_uchar *ptr;
	data_blk_t blk;
	mtp_char *path;
	mtp_uint64 num_bytes;
	mtp_uint64 total_len;
	mtp_uint64 sent = 0;
	mtp_uint16 resp = PTP_RESPONSE_OK;
	mtp_uint32 packet_len;
	mtp_uint32 read_len = 0;
	mtp_uint32 h_file = INVALID_FILE;
	mtp_int32 error = 0;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	obj_handle = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	obj = _device_get_object_with_handle(obj_handle);
	if (obj == NULL) {
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}

#ifdef MTP_SUPPORT_SET_PROTECTION
	/* Check to see if the data is non-transferable */
	if (obj->obj_info->protcn_status ==
			MTP_PROTECTIONSTATUS_NONTRANSFERABLE_DATA) {
		resp = PTP_RESPONSE_ACCESSDENIED;
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}
#endif /* MTP_SUPPORT_SET_PROTECTION */

	path = obj->file_path;
	num_bytes = obj->obj_info->file_size;
	total_len = num_bytes + sizeof(header_container_t);
	packet_len = total_len < g_conf.read_file_size ? num_bytes :
		(g_conf.read_file_size - sizeof(header_container_t));

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	ptr = _hdlr_alloc_buf_data_container(&blk, packet_len, num_bytes);
	if (NULL == ptr) {
		ERR("_hdlr_alloc_buf_data_container() Fail");
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		return;
	}

	_device_set_phase(DEVICE_PHASE_DATAIN);
	h_file = _util_file_open(path, MTP_FILE_READ, &error);
	if (h_file == INVALID_FILE) {
		ERR("_util_file_open() Fail");
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		if (EACCES == error) {
			_cmd_hdlr_send_response_code(hdlr,
					PTP_RESPONSE_ACCESSDENIED);
			return;
		}
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		return;
	}

	_util_file_read(h_file, ptr, packet_len, &read_len);
	if (0 == read_len) {
		ERR("_util_file_read() Fail");
		ERR_SECURE("filename[%s]", path);
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		resp = PTP_RESPONSE_INCOMPLETETRANSFER;
		goto Done;
	}

	/* First Packet with Header */
	if (PTP_EVENTCODE_CANCELTRANSACTION == _transport_get_control_event() ||
			FALSE == _hdlr_send_bulk_data(blk.data, blk.len)) {
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		resp = PTP_RESPONSE_INCOMPLETETRANSFER;
		ERR("First Packet send Fail");
		ERR_SECURE("filename[%s]\n", path);
		goto Done;
	}

	sent = sizeof(header_container_t) + read_len;
	ptr = blk.data;

	while (sent < total_len) {
		_util_file_read(h_file, ptr, g_conf.read_file_size, &read_len);
		if (0 == read_len) {
			ERR("_util_file_read() Fail");
			ERR_SECURE("filename[%s]\n", path);
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			resp = PTP_RESPONSE_INCOMPLETETRANSFER;
			goto Done;
		}

		if (PTP_EVENTCODE_CANCELTRANSACTION == _transport_get_control_event() ||
				FALSE == _hdlr_send_bulk_data(ptr, read_len)) {
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			resp = PTP_RESPONSE_INCOMPLETETRANSFER;
			ERR("Packet send Fail");
			ERR_SECURE("filename[%s]\n", path);
			goto Done;
		}

		sent += read_len;
	}

	if (total_len % ((mtp_uint64)_transport_get_usb_packet_len()) == 0) {
		_transport_send_zlp();
	}

Done:
	_util_file_close(h_file);

	g_free(blk.data);
	_cmd_hdlr_send_response_code(hdlr, resp);
}

static void __send_object_info(mtp_handler_t *hdlr)
{
	mtp_uint16 resp = PTP_RESPONSE_UNDEFINED;
	mtp_uint32 store_id = 0;
	mtp_uint32 h_parent = 0;
	mtp_uint32 num_bytes = 0;
	data_blk_t blk = { 0 };
	mtp_uint32 resp_param[3] = { 0 };
	mtp_obj_t *obj = NULL;
	mtp_err_t ret = 0;
	obj_data_t obdata = { 0 };

	store_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	if (store_id == 0) {
		store_id = _device_get_default_store_id();
	}
	h_parent = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		resp = PTP_RESPONSE_PARAM_NOTSUPPORTED;
		if (_device_get_phase() != DEVICE_PHASE_NOTREADY) {
			_cmd_hdlr_send_response_code(hdlr, resp);
		}
		return;
	}

	_device_set_phase(DEVICE_PHASE_DATAOUT);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	num_bytes = MAX_SIZE_IN_BYTES_OF_OBJECT_INFO;
	if (_hdlr_rcv_data_container(&blk, num_bytes) == FALSE) {
		_device_set_phase(DEVICE_PHASE_NOTREADY);
	} else {
		if (TRUE == hdlr->data4_send_obj.is_valid) {
			obdata.store_id = hdlr->data4_send_obj.store_id;
			obdata.obj_size = hdlr->data4_send_obj.file_size;
			obdata.obj = hdlr->data4_send_obj.obj;
			hdlr->data4_send_obj.obj = NULL;
		}
		ret = _hutil_construct_object_entry(store_id, h_parent,
				((hdlr->data4_send_obj.is_valid == TRUE) ? (&obdata)
				 : (NULL)), &obj, _hdlr_get_payload_data(&blk),
				_hdlr_get_payload_size(&blk));
		hdlr->data4_send_obj.is_valid = FALSE;
		switch (ret) {
		case MTP_ERROR_NONE:
			if (obj->obj_info->obj_fmt == PTP_FMT_ASSOCIATION) {
				hdlr->data4_send_obj.obj = NULL;
				hdlr->data4_send_obj.is_valid = FALSE;
				hdlr->data4_send_obj.obj_handle = obj->obj_handle;
				hdlr->data4_send_obj.h_parent = h_parent;
				hdlr->data4_send_obj.store_id = store_id;
			}
#ifdef MTP_USE_SELFMAKE_ABSTRACTION
			else if (obj->obj_info->file_size == 0 &&
					obj->obj_info->obj_fmt > MTP_FMT_UNDEFINED_COLLECTION &&
					obj->obj_info->obj_fmt < MTP_FMT_UNDEFINED_DOC) {
				hdlr->data4_send_obj.obj = NULL;
				hdlr->data4_send_obj.is_valid = FALSE;
				hdlr->data4_send_obj.obj_handle = obj->obj_handle;
				hdlr->data4_send_obj.h_parent = h_parent;
				hdlr->data4_send_obj.store_id = store_id;
			}
#endif /*MTP_USE_SELFMAKE_ABSTRACTION*/
			else {
				hdlr->data4_send_obj.is_valid = TRUE;
				hdlr->data4_send_obj.obj_handle = obj->obj_handle;
				hdlr->data4_send_obj.h_parent = h_parent;
				hdlr->data4_send_obj.store_id = store_id;
				hdlr->data4_send_obj.obj = obj;
				hdlr->data4_send_obj.file_size =
					obj->obj_info->file_size;
			}
			resp = PTP_RESPONSE_OK;
			break;
		case MTP_ERROR_STORE_NOT_AVAILABLE:
			resp = PTP_RESPONSE_STORENOTAVAILABLE;
			DBG("PTP_RESPONSE_STORENOTAVAILABLE");
			break;
		case MTP_ERROR_INVALID_PARAM:
			resp = PTP_RESPONSE_PARAM_NOTSUPPORTED;
			DBG("PTP_RESPONSE_PARAM_NOTSUPPORTED");
			break;
		case MTP_ERROR_INVALID_STORE:
			resp = PTP_RESPONSE_INVALID_STORE_ID;
			DBG("PTP_RESPONSE_INVALID_STORE_ID");
			break;
		case MTP_ERROR_STORE_READ_ONLY:
			resp = PTP_RESPONSE_STORE_READONLY;
			break;
		case MTP_ERROR_STORE_FULL:
			resp = PTP_RESPONSE_STOREFULL;
			DBG("PTP_RESPONSE_STOREFULL");
			break;
		case MTP_ERROR_GENERAL:
			resp = PTP_RESPONSE_GEN_ERROR;
			DBG("PTP_RESPONSE_GEN_ERROR");
			break;
		case MTP_ERROR_INVALID_OBJECT_INFO:
			resp = MTP_RESPONSECODE_INVALIDDATASET;
			DBG("MTP_RESPONSECODE_INVALIDDATASET");
			break;
		case MTP_ERROR_INVALID_OBJECTHANDLE:
			resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
			DBG("PTP_RESPONSE_INVALID_OBJ_HANDLE");
			break;
		case MTP_ERROR_INVALID_PARENT:
			resp = PTP_RESPONSE_INVALIDPARENT;
			DBG("PTP_RESPONSE_INVALIDPARENT");
			break;
		case MTP_ERROR_ACCESS_DENIED:
			resp = PTP_RESPONSE_ACCESSDENIED;
			DBG("PTP_RESPONSE_ACCESSDENIED");
			break;
		default:
			resp = PTP_RESPONSE_GEN_ERROR;
			DBG("PTP_RESPONSE_GEN_ERROR");
			break;
		}
	}

	g_free(blk.data);
	if (_device_get_phase() != DEVICE_PHASE_NOTREADY) {
		if (resp == PTP_RESPONSE_OK) {
			hdlr->last_fmt_code = obj->obj_info->obj_fmt;
			resp_param[0] = hdlr->data4_send_obj.store_id;

			/* PTP spec here requires that 0xFFFFFFFF be sent if root is the parent object,
			 * while in some situations(e.g. MoveObject, CopyObject), 0x00000000 (as
			 * PTP_OBJECTHANDLE_ROOT is defined) represents the root.
			 */
			resp_param[1] = (hdlr->data4_send_obj.h_parent
					!= PTP_OBJECTHANDLE_ROOT) ?
				hdlr->data4_send_obj.h_parent :
				0xFFFFFFFF;
			resp_param[2] = hdlr->data4_send_obj.obj_handle;
			_cmd_hdlr_send_response(hdlr, resp, 3, resp_param);
		} else {
			_cmd_hdlr_send_response_code(hdlr, resp);
		}
	}
}

static void __send_object(mtp_handler_t *hdlr)
{
	data_blk_t blk = { 0 };
	mtp_uint16 resp = PTP_RESPONSE_OK;
	mtp_char temp_fpath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		ERR("unsupported parameter");
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_INVALIDPARAM);
		return;
	}
#ifndef MTP_USE_SELFMAKE_ABSTRACTION
	if (hdlr->data4_send_obj.is_valid != TRUE) {
		DBG("invalide object info");
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		return;
	}
#endif /*MTP_USE_SELFMAKE_ABSTRACTION*/

	_device_set_phase(DEVICE_PHASE_DATAOUT);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);

	if (_hdlr_rcv_file_in_data_container(&blk, temp_fpath,
				MTP_MAX_PATHNAME_SIZE + 1) == FALSE) {
		_device_set_phase(DEVICE_PHASE_IDLE);

		ERR("_hdlr_rcv_file_in_data_container() Fail");
		g_free(blk.data);
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		return;
	}

	switch (_hutil_write_file_data(hdlr->data4_send_obj.store_id,
				hdlr->data4_send_obj.obj, temp_fpath)) {

	case MTP_ERROR_INVALID_OBJECT_INFO:
		resp = PTP_RESPONSE_NOVALID_OBJINFO;
		DBG("PTP_RESPONSE_NOVALID_OBJINFO");
		break;
#ifdef MTP_USE_SELFMAKE_ABSTRACTION
	case MTP_ERROR_INVALID_PARAM:	/* association file*/
#endif /*MTP_USE_SELFMAKE_ABSTRACTION*/
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		DBG("PTP_RESPONSE_OK");
		break;
	case MTP_ERROR_STORE_FULL:
		resp = PTP_RESPONSE_STOREFULL;
		DBG("PTP_RESPONSE_STOREFULL");
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
		DBG("PTP_RESPONSE_GEN_ERROR");
	}
	_cmd_hdlr_send_response_code(hdlr, resp);

	/* This object info has been consumed.*/
	hdlr->data4_send_obj.obj = NULL;
	hdlr->data4_send_obj.is_valid = FALSE;

	g_free(blk.data);
}

static void __delete_object(mtp_handler_t *hdlr)
{
	mtp_uint32 obj_handle = 0;
	mtp_uint32 fmt = 0;
	mtp_uint16 resp = 0;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		ERR("Parameters not supported");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	obj_handle = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	if ((PTP_FORMATCODE_NOTUSED != fmt) &&
			(obj_handle != PTP_OBJECTHANDLE_ALL)) {
		ERR("Invalid object format");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_INVALID_OBJ_FMTCODE);
		return;
	}

	_transport_set_mtp_operation_state(MTP_STATE_DATA_PROCESSING);

	switch (_hutil_remove_object_entry(obj_handle, fmt)) {
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	case MTP_ERROR_STORE_READ_ONLY:
		resp = PTP_RESPONSE_STORE_READONLY;
		break;
	case MTP_ERROR_PARTIAL_DELETION:
		resp = PTP_RESPONSE_PARTIAL_DELETION;
		break;
	case MTP_ERROR_OBJECT_WRITE_PROTECTED:
		resp = PTP_RESPONSE_OBJ_WRITEPROTECTED;
		break;
	case MTP_ERROR_ACCESS_DENIED:
		resp = PTP_RESPONSE_ACCESSDENIED;
		break;
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);
	_cmd_hdlr_send_response_code(hdlr, resp);
}

static void __format_store(mtp_handler_t *hdlr)
{
	mtp_uint32 store_id = 0;
	mtp_uint32 fs_fmt = 0;
	mtp_uint16 ret = 0;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	store_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	fs_fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);

	ret = _hutil_format_storage(store_id, fs_fmt);
	
	/* although there is remain file, send OK */
	_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
}

static void __reset_device(mtp_handler_t *hdlr)
{
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
}

static void __get_device_prop_desc(mtp_handler_t *hdlr)
{
	device_prop_desc_t dev_prop = { { 0 }, };
	device_prop_desc_t *prop_ptr = NULL;
	ptp_string_t ptp_str = { 0, { 0 } };
	data_blk_t blk = { 0 };
	mtp_uint32 prop_id = 0;
	mtp_uint32 resp = 0;
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;
	mtp_wchar temp[MTP_MAX_REG_STRING + 1] = { 0 };

	prop_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	switch (prop_id) {
#ifdef MTP_SUPPORT_DEVICEPROP_BATTERYLEVEL
	case PTP_PROPERTYCODE_BATTERYLEVEL:
		{
			mtp_int32 batt = 0;

			prop_ptr = _device_get_device_property(prop_id);
			if (NULL == prop_ptr) {
				ERR("prop_ptr is NULL!");
				break;
			}

			batt = _util_get_battery_level();
			if (FALSE == _prop_set_current_integer(prop_ptr, batt))
				ERR("_util_get_battery_level() Fail");
			break;
		}
#endif /* MTP_SUPPORT_DEVICEPROP_BATTERYLEVEL */

	case MTP_PROPERTYCODE_DEVICEFRIENDLYNAME:
		{
			mtp_char *dev_name = _device_get_device_name();
			if (dev_name == NULL) {
				ERR("_device_get_device_name() Fail");
				break;
			}

			prop_ptr = _device_get_device_property(prop_id);
			if (NULL == prop_ptr) {
				ERR("prop_ptr is Null");
				g_free(dev_name);
				break;
			}
			_util_utf8_to_utf16(temp, sizeof(temp) / WCHAR_SIZ, dev_name);
			_prop_copy_char_to_ptpstring(&ptp_str, temp, WCHAR_TYPE);
			_prop_set_current_string(prop_ptr, &ptp_str);
			g_free(dev_name);
			break;
		}

	case MTP_PROPERTYCODE_SYNCHRONIZATIONPARTNER:
		{
			mtp_char *sync_ptr = _device_get_sync_partner();
			if (NULL == sync_ptr) {
				ERR("_device_get_sync_partner() Fail");
				break;
			}
			prop_ptr = _device_get_device_property(prop_id);
			if (NULL == prop_ptr) {
				ERR("prop_ptr is Null");
				g_free(sync_ptr);
				break;
			}

			_util_utf8_to_utf16(temp, sizeof(temp) / WCHAR_SIZ, sync_ptr);
			_prop_copy_char_to_ptpstring(&ptp_str, temp, WCHAR_TYPE);
			_prop_set_current_string(prop_ptr, &ptp_str);
			g_free(sync_ptr);
			break;
		}
	case MTP_PROPERTYCODE_DEVICEICON:
		{
			mtp_uint32 h_file;
			mtp_uint32 bytes_read = 0;
			mtp_uint32 file_size = 0;
			struct stat buf;
			mtp_uchar *data = NULL;
			mtp_int32 err = 0;

			prop_ptr = _device_get_device_property(prop_id);
			if (NULL == prop_ptr) {
				ERR("prop_ptr is Null");
				break;
			}

			h_file = _util_file_open(MTP_DEVICE_ICON, MTP_FILE_READ, &err);
			if (h_file == INVALID_FILE) {
				ERR("file handle is not valid");
				_cmd_hdlr_send_response_code(hdlr,
						PTP_RESPONSE_GEN_ERROR);
				return;
			}
			if (fstat(fileno((FILE *)h_file), &buf) != 0) {
				_util_file_close(h_file);
				_cmd_hdlr_send_response_code(hdlr,
						PTP_RESPONSE_GEN_ERROR);
				return;
			}
			file_size = buf.st_size;
			data = (mtp_uchar *)g_malloc(file_size + sizeof(mtp_uint32));
			if (data == NULL) {
				ERR("g_malloc() Fail");
				_util_file_close(h_file);
				_cmd_hdlr_send_response_code(hdlr,
						PTP_RESPONSE_GEN_ERROR);
				return;
			}
			memcpy(data, &file_size, sizeof(mtp_uint32));
			_util_file_read(h_file, &data[sizeof(mtp_uint32)], file_size,
					&bytes_read);
			if (bytes_read != file_size) {
				ERR("Number of read bytes less than requested");
				_util_file_close(h_file);
				g_free(data);
				_cmd_hdlr_send_response_code(hdlr,
						PTP_RESPONSE_GEN_ERROR);
				return;
			}

			_prop_set_current_array(prop_ptr, data);
			_prop_set_default_array(&(prop_ptr->propinfo),
					(mtp_uchar *)&data[sizeof(mtp_uint32)], bytes_read);
			g_free(data);
			_util_file_close(h_file);
			break;
		}

	default:
		ERR("Unknown PropId : [0x%x]\n", prop_id);
		break;
	}

	if (_hutil_get_device_property(prop_id, &dev_prop) != MTP_ERROR_NONE) {
		ERR("_hutil_get_device_property returned error");
		resp = PTP_RESPONSE_PROP_NOTSUPPORTED;
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}
	num_bytes = _prop_size_device_prop_desc(&dev_prop);

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);
	if (ptr == NULL) {
		resp = PTP_RESPONSE_GEN_ERROR;
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}

	if (_prop_pack_device_prop_desc(&dev_prop, ptr, num_bytes) != num_bytes) {
		resp = PTP_RESPONSE_GEN_ERROR;
		_cmd_hdlr_send_response_code(hdlr, resp);
		g_free(blk.data);
		return;
	}

	_device_set_phase(DEVICE_PHASE_DATAIN);
	if (_hdlr_send_data_container(&blk))
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
	else {
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

	g_free(blk.data);
	return;
}

static void __get_device_prop_value(mtp_handler_t *hdlr)
{

	ptp_string_t ptp_str = { 0, { 0 } };
	data_blk_t blk = { 0 };
	mtp_uint32 prop_id = 0;
	mtp_uint32 no_bytes = 0;
	mtp_uchar *ptr = NULL;
	mtp_wchar temp[MTP_MAX_REG_STRING + 1] = { 0 };

	prop_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);

	switch(prop_id) {
#ifdef MTP_SUPPORT_DEVICEPROP_BATTERYLEVEL
	case PTP_PROPERTYCODE_BATTERYLEVEL: {
											mtp_int32 batt = 0;

											batt = _util_get_battery_level();
											no_bytes = sizeof(batt);

											ptr = _hdlr_alloc_buf_data_container(&blk, no_bytes, no_bytes);
											if (ptr == NULL) {
												_cmd_hdlr_send_response_code(hdlr,
														PTP_RESPONSE_GEN_ERROR);
												return;
											}
											memcpy(ptr, &batt, no_bytes);
#ifdef __BIG_ENDIAN__
											_util_conv_byte_order(ptr, no_bytes);
#endif /*__BIG_ENDIAN__*/
											break;
										}
#endif /* MTP_SUPPORT_DEVICEPROP_BATTERYLEVEL */

	case MTP_PROPERTYCODE_DEVICEFRIENDLYNAME:
		{

												  mtp_char *device = _device_get_device_name();
												  if (device == NULL) {
													  ERR("_device_get_device_name() Fail");
													  _cmd_hdlr_send_response_code(hdlr,
															  PTP_RESPONSE_GEN_ERROR);
													  return;
												  }

												  _util_utf8_to_utf16(temp, sizeof(temp) / WCHAR_SIZ, device);
												  _prop_copy_char_to_ptpstring(&ptp_str, temp, WCHAR_TYPE);
												  no_bytes = _prop_size_ptpstring(&ptp_str);
												  g_free(device);

												  ptr = _hdlr_alloc_buf_data_container(&blk, no_bytes, no_bytes);
												  if (ptr == NULL) {
													  _cmd_hdlr_send_response_code(hdlr,
															  PTP_RESPONSE_GEN_ERROR);
													  return;
												  }

												  if (_prop_pack_ptpstring(&ptp_str, ptr, no_bytes) != no_bytes) {
													  _cmd_hdlr_send_response_code(hdlr,
															  PTP_RESPONSE_GEN_ERROR);
													  g_free(blk.data);
													  return;
												  }
												  break;
											  }

	case MTP_PROPERTYCODE_SYNCHRONIZATIONPARTNER:
		{

													  mtp_char *sync_ptr = _device_get_sync_partner();
													  if (sync_ptr == NULL) {
														  ERR("_device_get_sync_partner() Fail");
														  _cmd_hdlr_send_response_code(hdlr,
																  PTP_RESPONSE_GEN_ERROR);
														  return;
													  }
													  _util_utf8_to_utf16(temp, sizeof(temp) / WCHAR_SIZ, sync_ptr);
													  g_free(sync_ptr);

													  _prop_copy_char_to_ptpstring(&ptp_str, temp, WCHAR_TYPE);
													  no_bytes = _prop_size_ptpstring(&ptp_str);

													  ptr = _hdlr_alloc_buf_data_container(&blk, no_bytes, no_bytes);
													  if (ptr == NULL) {
														  _cmd_hdlr_send_response_code(hdlr,
																  PTP_RESPONSE_GEN_ERROR);
														  return;
													  }

													  if (_prop_pack_ptpstring(&ptp_str, ptr, no_bytes) != no_bytes) {
														  _cmd_hdlr_send_response_code(hdlr,
																  PTP_RESPONSE_GEN_ERROR);
														  g_free(blk.data);
														  return;
													  }
													  break;
												  }

	case MTP_PROPERTYCODE_DEVICEICON:
		{

										  mtp_uint32 h_file;
										  mtp_uint32 read_bytes = 0;
										  mtp_uint32 file_size = 0;
										  struct stat buf;
										  mtp_uchar *data = NULL;
										  mtp_int32 err = 0;
										  ptp_array_t val_arr = { 0 };
										  mtp_uint32 ii;

										  h_file = _util_file_open(MTP_DEVICE_ICON, MTP_FILE_READ, &err);
										  if (h_file == INVALID_FILE) {
											  ERR("file handle is not valid");
											  _cmd_hdlr_send_response_code(hdlr,
													  PTP_RESPONSE_GEN_ERROR);
											  return;
										  }
										  if (fstat(fileno((FILE *)h_file), &buf) != 0) {
											  _util_file_close(h_file);
											  _cmd_hdlr_send_response_code(hdlr,
													  PTP_RESPONSE_GEN_ERROR);
											  return;
										  }

										  file_size = buf.st_size;
										  data = (mtp_uchar *)g_malloc(file_size);
										  if (data == NULL) {
											  ERR("g_malloc() Fail");
											  _util_file_close(h_file);
											  _cmd_hdlr_send_response_code(hdlr,
													  PTP_RESPONSE_GEN_ERROR);
											  return;
										  }

										  _util_file_read(h_file, &data, file_size, &read_bytes);
										  if (read_bytes != file_size) {
											  ERR("Number of read bytes less than requested");
											  _util_file_close(h_file);
											  g_free(data);
											  _cmd_hdlr_send_response_code(hdlr,
													  PTP_RESPONSE_GEN_ERROR);
											  return;
										  }

										  _prop_init_ptparray(&val_arr, UINT8_TYPE);
										  _prop_grow_ptparray(&val_arr, read_bytes);
										  for (ii = 0; ii < read_bytes; ii++) {
											  _prop_append_ele_ptparray(&val_arr, data[ii]);
										  }

										  no_bytes = _prop_get_size_ptparray(&val_arr);
										  ptr = _hdlr_alloc_buf_data_container(&blk, no_bytes, no_bytes);
										  if (ptr == NULL) {
											  _cmd_hdlr_send_response_code(hdlr,
													  PTP_RESPONSE_GEN_ERROR);
											  g_free(data);
											  _util_file_close(h_file);
											  _prop_deinit_ptparray(&val_arr);
											  return;
										  }
										  if (_prop_pack_ptparray(&val_arr, ptr, no_bytes) != no_bytes) {
											  _cmd_hdlr_send_response_code(hdlr,
													  PTP_RESPONSE_GEN_ERROR);
											  g_free(blk.data);
											  g_free(data);
											  _prop_deinit_ptparray(&val_arr);
											  _util_file_close(h_file);
											  return;
										  }

										  g_free(data);
										  _prop_deinit_ptparray(&val_arr);
										  _util_file_close(h_file);

										  break;
									  }

	default:
									  ERR("Unknown PropId : [0x%x]\n", prop_id);
									  _cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
									  return;
	}

	_device_set_phase(DEVICE_PHASE_DATAIN);
	if (_hdlr_send_data_container(&blk)) {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
	} else {
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_INCOMPLETETRANSFER);
	}
	g_free(blk.data);
	return;
}

static void __set_device_prop_value(mtp_handler_t *hdlr)
{
	mtp_uint32 prop_id = 0;
	data_blk_t blk = { 0 };
	mtp_uint32 max_bytes = 0;
	mtp_uint16 resp = PTP_RESPONSE_OK;
	mtp_err_t ret = 0;
	mtp_char *d_raw = NULL;

	prop_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	_device_set_phase(DEVICE_PHASE_DATAOUT);

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	max_bytes = MAX_SIZE_IN_BYTES_OF_PROP_VALUE;

	if (FALSE == _hdlr_rcv_data_container(&blk, max_bytes)) {
		ERR("_hdlr_rcv_data_container() Fail");
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		return;
	}

	d_raw = (mtp_char *)_hdlr_get_payload_data(&blk);
	if (NULL != d_raw) {
		ret = _hutil_set_device_property(prop_id, d_raw,
				_hdlr_get_payload_size(&blk));
	} else {
		ret = MTP_ERROR_INVALID_OBJ_PROP_CODE;
	}

	switch (ret) {
	case MTP_ERROR_NONE: {
#ifdef MTP_USE_INFORMATION_REGISTRY

							 mtp_char temp[MTP_MAX_REG_STRING * 3 + 1] = { 0 };
							 mtp_wchar parsed_buf[MTP_MAX_REG_STRING + 1] = { 0 };
							 mtp_uchar parse_sz = 0;

							 if (MTP_PROPERTYCODE_SYNCHRONIZATIONPARTNER == prop_id) {
								 parse_sz = d_raw[0];
								 _util_wchar_ncpy(parsed_buf, (mtp_wchar *)&d_raw[1],
										 parse_sz > MTP_MAX_REG_STRING ?
										 MTP_MAX_REG_STRING : parse_sz);
								 _util_utf16_to_utf8(temp, sizeof(temp), parsed_buf);
								 _device_set_sync_partner(temp);
								 if (!g_strcmp0(temp,
											 MTP_DEV_PROPERTY_NULL_SYNCPARTNER)) {
									 vconf_set_str(VCONFKEY_MTP_SYNC_PARTNER_STR,
											 "");
								 } else {
									 vconf_set_str(VCONFKEY_MTP_SYNC_PARTNER_STR,
											 temp);
								 }
							 }
#endif /*MTP_USE_INFORMATION_REGISTRY*/
						 }
						 resp = PTP_RESPONSE_OK;
						 break;
	case MTP_ERROR_ACCESS_DENIED:
						 resp = PTP_RESPONSE_ACCESSDENIED;
						 break;
	case MTP_ERROR_INVALID_OBJ_PROP_VALUE:
						 resp = PTP_RESPONSE_INVALIDPROPVALUE;
						 break;
	default:
						 resp = PTP_RESPONSE_PROP_NOTSUPPORTED;
	}

	_cmd_hdlr_send_response_code(hdlr, resp);

	g_free(blk.data);
	return;
}

static void __get_partial_object(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	off_t offset = 0;
	mtp_uint32 data_sz = 0;
	mtp_uint32 send_bytes = 0;
	data_blk_t blk = { 0 };
	mtp_uchar *ptr = NULL;
	mtp_uint16 resp = 0;
	mtp_uint64 f_size = 0;
	mtp_uint64 total_sz = 0;

	offset = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	data_sz = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);
	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);

	switch (_hutil_get_object_entry_size(h_obj, &f_size)) {

	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_NONE:
		if (data_sz > (f_size - offset))
			send_bytes = f_size - offset;
		else
			send_bytes = data_sz;
		resp = PTP_RESPONSE_OK;
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
		break;
	}

	if (PTP_RESPONSE_OK != resp) {
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	ptr = _hdlr_alloc_buf_data_container(&blk, send_bytes, send_bytes);

	if (NULL != ptr) {
		switch (_hutil_read_file_data_from_offset(h_obj, offset, ptr, &send_bytes)) {
		case MTP_ERROR_NONE:
			resp = PTP_RESPONSE_OK;
			break;
		case MTP_ERROR_INVALID_OBJECTHANDLE:
			resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
			break;
		default:
			resp = PTP_RESPONSE_GEN_ERROR;
		}
	} else {
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	if (PTP_RESPONSE_OK == resp) {
		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			total_sz = send_bytes + sizeof(header_container_t);
			if (total_sz % _transport_get_usb_packet_len() == 0)
				_transport_send_zlp();
			_cmd_hdlr_send_response(hdlr, resp, 1, &send_bytes);
			g_free(blk.data);
			return;
		}

		/*Host Cancelled data-in transfer*/
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		g_free(blk.data);
		return;
	}

	_cmd_hdlr_send_response_code(hdlr, resp);

	g_free(blk.data);
	return;
}

static void __get_object_references(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	ptp_array_t ref_arr = { 0 };
	data_blk_t blk = { 0 };
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;
	mtp_uint32 num_ele = 0;
	mtp_uint16 resp = 0;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		ERR("Unsupported Parameters");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);

	switch (_hutil_get_object_references(h_obj, &ref_arr, &num_ele)) {
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	default :
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	if (resp != PTP_RESPONSE_OK) {
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}

	if (resp == PTP_RESPONSE_OK && num_ele == 0) {
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}

	num_bytes = _prop_get_size_ptparray(&ref_arr);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);

	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);
	if (num_bytes == _prop_pack_ptparray(&ref_arr, ptr, num_bytes)) {
		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
		} else {
			/* Host Cancelled data-in transfer*/
			_device_set_phase(DEVICE_PHASE_NOTREADY);
		}
	} else {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

	_prop_deinit_ptparray(&ref_arr);
	g_free(blk.data);

	return;
}

static void __set_object_references(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	data_blk_t blk = { 0 };
	mtp_uint32 max_bytes = 0;
	mtp_uint16 resp = PTP_RESPONSE_OK;
	mtp_uint32 num_ref = 0;
	mtp_uint32 idx = 0;
	mtp_uint32 *ref_ptr = NULL;
	mtp_uchar *ptr = NULL;
	mtp_uint32 ref_handle = 0;

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		resp = PTP_RESPONSE_PARAM_NOTSUPPORTED;
		_cmd_hdlr_send_response_code(hdlr, resp);
	}

	_device_set_phase(DEVICE_PHASE_DATAOUT);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);

	/* temporarily set a big number for data size received */

	max_bytes = MTP_MAX_REFDB_ROWCNT * sizeof(mtp_uint32);
	if (_hdlr_rcv_data_container(&blk, max_bytes) == FALSE) {
		DBG("_hdlr_rcv_data_container() Fail");
		_device_set_phase(DEVICE_PHASE_IDLE);
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	if (PTP_RESPONSE_OK != resp) {
		_cmd_hdlr_send_response_code(hdlr, resp);
		g_free(blk.data);
		return;
	}

	ptr = _hdlr_get_payload_data(&blk);
	if (ptr == NULL) {
		return;
	}

	memcpy(&num_ref, ptr, sizeof(mtp_uint32));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(&num_ref, sizeof(DWORD));
#endif /* __BIG_ENDIAN__ */

	ptr += sizeof(mtp_uint32);
	if (_hdlr_get_payload_size(&blk) < (num_ref + 1) * sizeof(mtp_uint32)) {

		resp = PTP_RESPONSE_GEN_ERROR;
		_cmd_hdlr_send_response_code(hdlr, resp);
		g_free(blk.data);
		return;
	}

	ref_ptr = (mtp_uint32 *)ptr;
	if (MTP_ERROR_NONE != _hutil_remove_object_reference(h_obj,
				PTP_OBJECTHANDLE_ALL)) {
		resp = PTP_RESPONSE_GEN_ERROR;
		_cmd_hdlr_send_response_code(hdlr, resp);
		g_free(blk.data);
	}

	for (idx = 0; idx < num_ref; idx++) {
		ref_handle = ref_ptr[idx];
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(&ref_handle, sizeof(mtp_uint32));
#endif /*__BIG_ENDIAN__*/
		_device_get_object_with_handle(ref_handle);
	}

	_hutil_add_object_references_enhanced(h_obj, (mtp_uchar *)ref_ptr,
			num_ref);
	_cmd_hdlr_send_response_code(hdlr, resp);

	g_free(blk.data);
	return;
}

static void __get_object_prop_desc(mtp_handler_t *hdlr)
{
	mtp_uint32 prop_id = 0;
	mtp_uint32 fmt = 0;
	obj_prop_desc_t prop = { { 0 }, };
	data_blk_t blk = { 0, };
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	prop_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);

	if (MTP_ERROR_NONE != _hutil_get_prop_desc(fmt, prop_id, &prop)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PROP_NOTSUPPORTED);
		return;
	}

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	num_bytes = _prop_size_obj_prop_desc(&prop);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);

	if (num_bytes == _prop_pack_obj_prop_desc(&prop, ptr, num_bytes)) {
		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
		} else {
			/* Host Cancelled data-in transfer */
			_device_set_phase(DEVICE_PHASE_NOTREADY);
		}
	} else {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_GEN_ERROR);
	}

	g_free(blk.data);
	return;
}

static void __get_object_prop_supported(mtp_handler_t *hdlr)
{
	mtp_uint32 fmt = 0;
	data_blk_t blk = { 0 };
	ptp_array_t props_supported = { 0 };
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	_prop_init_ptparray(&props_supported, UINT16_TYPE);

	if (MTP_ERROR_NONE != _hutil_get_object_prop_supported(fmt,
				&props_supported)) {
		_prop_deinit_ptparray(&props_supported);
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		return;
	}

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	num_bytes = _prop_get_size_ptparray(&props_supported);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);

	if (NULL != ptr) {
		_prop_pack_ptparray(&props_supported, ptr, num_bytes);
		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
		} else {
			/* Host Cancelled data-in transfer */
			_device_set_phase(DEVICE_PHASE_NOTREADY);
		}
	} else {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

	_prop_deinit_ptparray(&props_supported);

	g_free(blk.data);
	return;
}

static void __get_object_prop_value(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	mtp_uint32 prop_id = 0;
	mtp_uchar *ptr = NULL;
	mtp_uint32 num_bytes = 0;
	data_blk_t blk = { 0 };
	obj_prop_val_t prop_val = { 0 };
	mtp_err_t ret = 0;
	mtp_obj_t *obj = NULL;
#ifdef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	slist_node_t *node = NULL;
	slist_node_t *next_node = NULL;
	mtp_uint32 ii = 0;
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	prop_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	ret = _hutil_get_object_prop_value(h_obj, prop_id, &prop_val, &obj);

	if (MTP_ERROR_NONE == ret) {
		num_bytes = _prop_size_obj_propval(&prop_val);
		_hdlr_init_data_container(&blk, hdlr->usb_cmd.code,
				hdlr->usb_cmd.tid);
		ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes,
				num_bytes);
		if (num_bytes ==
				_prop_pack_obj_propval(&prop_val, ptr, num_bytes)) {

			_device_set_phase(DEVICE_PHASE_DATAIN);
			if (_hdlr_send_data_container(&blk)) {
				_cmd_hdlr_send_response_code(hdlr,
						PTP_RESPONSE_OK);
			} else {
				/* Host Cancelled data-in transfer */
				_device_set_phase(DEVICE_PHASE_NOTREADY);
			}
		} else {
			_cmd_hdlr_send_response_code(hdlr,
					PTP_RESPONSE_GEN_ERROR);
		}

		g_free(blk.data);

	} else if (ret == MTP_ERROR_INVALID_OBJECTHANDLE) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_INVALID_OBJ_HANDLE);
	} else {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

#ifdef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	if (NULL == obj) {
		ERR("Invalid object");
		return;
	}

	for (ii = 0, next_node = obj->propval_list.start;
			ii < obj->propval_list.nnodes; ii++) {
		node = next_node;
		next_node = node->link;
		_prop_destroy_obj_propval((obj_prop_val_t *)node->value);
		g_free(node);
	}
	obj->propval_list.start = NULL;
	obj->propval_list.end = NULL;
	obj->propval_list.nnodes = 0;
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/

	return;
}

static void __set_object_prop_value(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	mtp_uint32 prop_id = 0;
	mtp_uint16 resp = PTP_RESPONSE_OK;
	data_blk_t blk = { 0 };
	mtp_uint32 max_bytes = 0;
	mtp_err_t ret = 0;

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	prop_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		ERR("Unsupported parameters");
		resp = PTP_RESPONSE_PARAM_NOTSUPPORTED;
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;
	}

	_device_set_phase(DEVICE_PHASE_DATAOUT);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	max_bytes = MTP_MAX_PROP_DATASIZE;

	if (_hdlr_rcv_data_container(&blk, max_bytes) == FALSE) {
		ERR("_hdlr_rcv_data_container() Fail");
		_device_set_phase(DEVICE_PHASE_IDLE);
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_INVALIDPROPVALUE);
		g_free(blk.data);
		return;
	}

	ret = _hutil_update_object_property(h_obj, prop_id, NULL,
			_hdlr_get_payload_data(&blk),
			_hdlr_get_payload_size(&blk), NULL);
	switch (ret) {
	case MTP_ERROR_ACCESS_DENIED:
		resp = PTP_RESPONSE_ACCESSDENIED;
		break;
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_INVALID_OBJ_PROP_CODE:
		resp = PTP_RESPONSE_PROP_NOTSUPPORTED;
		break;
	case MTP_ERROR_GENERAL:
		resp = PTP_RESPONSE_GEN_ERROR;
		break;
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	default:
		resp = PTP_RESPONSE_INVALIDPROPVALUE;
		break;
	}

	_cmd_hdlr_send_response_code(hdlr, resp);

	g_free(blk.data);
	return;
}

static void __get_object_prop_list(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	mtp_uint32 fmt = 0;
	mtp_uint32 prop_id = 0;
	mtp_uint32 group_code = 0;
	mtp_uint32 depth = 0;
	mtp_err_t ret = 0;
	mtp_uint16 resp = 0;
	obj_proplist_t prop_list = { { 0 } };
	data_blk_t blk = { 0 };
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;
#ifdef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	ptp_array_t obj_arr = { 0 };
	slist_node_t *node = NULL;
	slist_node_t *next_node = NULL;
	mtp_uint32 ii = 0;
	mtp_uint32 jj = 0;
	mtp_obj_t **ptr_array = NULL;
	mtp_obj_t *obj = NULL;
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	prop_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);
	group_code = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 3);
	depth = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 4);

	__enum_store_not_enumerated(h_obj, fmt, depth);

#ifdef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	ret = _hutil_get_object_prop_list(h_obj, fmt, prop_id, group_code,
			depth, &prop_list, &obj_arr);
#else /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/
	ret = _hutil_get_object_prop_list(h_obj, fmt, prop_id, group_code,
			depth, &prop_list);
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/

	switch (ret) {
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_INVALID_PARAM:
		resp = PTP_RESPONSE_INVALIDPARAM;
		break;
	case MTP_ERROR_NO_SPEC_BY_FORMAT:
		resp = PTP_RESPONSE_NOSPECIFICATIONBYFORMAT;
		break;
	case MTP_ERROR_GENERAL:
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	if (PTP_RESPONSE_OK != resp) {
		_cmd_hdlr_send_response_code(hdlr, resp);
		_prop_deinit_ptparray(&obj_arr);
		return;
	}

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	num_bytes = _prop_size_obj_proplist(&prop_list);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);
	if (num_bytes == _prop_pack_obj_proplist(&prop_list, ptr, num_bytes)) {

		_device_set_phase(DEVICE_PHASE_DATAIN);
		if (_hdlr_send_data_container(&blk)) {
			_cmd_hdlr_send_response_code(hdlr, resp);
		} else {
			/* Host Cancelled data-in transfer*/
			_device_set_phase(DEVICE_PHASE_NOTREADY);
		}
	}

	_prop_destroy_obj_proplist(&prop_list);
	g_free(blk.data);

#ifdef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	if (resp == PTP_RESPONSE_OK && obj_arr.array_entry) {
		ptr_array = obj_arr.array_entry;

		for (ii = 0; ii < obj_arr.num_ele; ii++) {
			obj = ptr_array[ii];
			if (NULL == obj || obj->propval_list.nnodes == 0) {
				continue;
			}
			/*Remove all the old property value, and ready to set up new */
			for (jj = 0, next_node = obj->propval_list.start;
					jj < obj->propval_list.nnodes; jj++) {
				node = next_node;
				next_node = node->link;
				_prop_destroy_obj_propval
					((obj_prop_val_t *)node->value);
				g_free(node);
			}
			obj->propval_list.start = NULL;
			obj->propval_list.end = NULL;
			obj->propval_list.nnodes = 0;
			node = NULL;
		}
	}
	_prop_deinit_ptparray(&obj_arr);
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/
	return;
}

static void __set_object_prop_list(mtp_handler_t *hdlr)
{
	mtp_uint32 i = 0;
	mtp_uint16 resp = PTP_RESPONSE_OK;
	data_blk_t blk = { 0 };
	mtp_uint32 max_num_obj = 0;
	mtp_uint32 max_bytes = 0;
	mtp_uint32 h_obj = 0;
	mtp_uint32 prop_id = 0;
	mtp_uint32 data_type = 0;
	mtp_uchar *temp = NULL;
	mtp_int32 bytes_left = 0;
	mtp_uint32 prop_val_sz = 0;
	mtp_uint32 num_elem = 0;
	mtp_err_t ret = 0;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 3) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 4)) {
		resp = PTP_RESPONSE_PARAM_NOTSUPPORTED;
	}

	_device_set_phase(DEVICE_PHASE_DATAOUT);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);

	/* Gestimate the amount of data we will be receiving */
	_hutil_get_number_of_objects(PTP_STORAGEID_ALL, &max_num_obj);
	max_bytes = max_num_obj * 1000;

	/* If Host sent more data than we could receive, a device stall happens,
	 * which cancels the data transfer
	 */
	if (max_bytes > 1000000) {
		max_bytes = 1000000;
		ERR("max_bytes is overflowed");
	}
	if (FALSE == _hdlr_rcv_data_container(&blk, max_bytes)) {
		ERR("_hdlr_rcv_data_container() Fail");
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	if (PTP_RESPONSE_OK != resp) {
		_cmd_hdlr_send_response(hdlr, resp, 1, &i);
		g_free(blk.data);
		return;
	}

	temp = _hdlr_get_payload_data(&blk);
	bytes_left = (mtp_int32)_hdlr_get_payload_size(&blk);
	if (bytes_left < sizeof(mtp_uint32)) {

		resp = MTP_RESPONSE_INVALIDOBJPROPFORMAT;
		_cmd_hdlr_send_response(hdlr, resp, 1, &i);
		g_free(blk.data);

		ERR("invalid object format, bytes_left [%d]:[%u]\n", bytes_left,
				sizeof(mtp_uint32));
		return;
	}

	num_elem = 0;
	memcpy(&num_elem, temp, sizeof(mtp_uint32));
	temp += sizeof(mtp_uint32);
	bytes_left -= sizeof(mtp_uint32);

	for (i = 0; i < num_elem; i++) {
		if (bytes_left < 0)
			break;

		/*Get object handle*/
		memcpy(&h_obj, temp, sizeof(mtp_uint32));
		temp += sizeof(mtp_uint32);
		bytes_left -= sizeof(mtp_uint32);
		if (bytes_left < 0)
			break;

		/* Get property code */
		memcpy(&prop_id, temp, sizeof(mtp_uint16));
		temp += sizeof(mtp_uint16);
		bytes_left -= sizeof(mtp_uint16);
		if (bytes_left < 0)
			break;

		/* Get data type*/
		memcpy(&data_type, temp, sizeof(mtp_uint16));
		temp += sizeof(mtp_uint16);
		bytes_left -= sizeof(mtp_uint16);
		if (bytes_left < 0)
			break;

		/* Update property*/
		ret = _hutil_update_object_property(h_obj, prop_id,
				(mtp_uint16 *)&data_type, temp, bytes_left,
				&prop_val_sz);

		switch (ret) {
		case MTP_ERROR_INVALID_OBJECT_PROP_FORMAT:
			resp = MTP_RESPONSE_INVALIDOBJPROPFORMAT;
			_cmd_hdlr_send_response(hdlr, resp, 1, &i);
			g_free(blk.data);
			ERR("invalid object format");
			return;
			break;
		case MTP_ERROR_ACCESS_DENIED:
			resp = PTP_RESPONSE_ACCESSDENIED;
			_cmd_hdlr_send_response(hdlr, resp, 1, &i);
			g_free(blk.data);
			ERR("access denied");
			return;
			break;
		case MTP_ERROR_INVALID_OBJECTHANDLE:
			resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
			_cmd_hdlr_send_response(hdlr, resp, 1, &i);
			g_free(blk.data);
			ERR("invalid object handle");
			return;
			break;
		case MTP_ERROR_INVALID_OBJ_PROP_CODE:
			resp = PTP_RESPONSE_PROP_NOTSUPPORTED;
			_cmd_hdlr_send_response(hdlr, resp, 1, &i);
			g_free(blk.data);
			ERR("property not supported");
			return;
			break;
		case MTP_ERROR_NONE:
			temp += prop_val_sz;
			bytes_left -= prop_val_sz;
			break;
		default:
			resp = PTP_RESPONSE_INVALIDPROPVALUE;
			_cmd_hdlr_send_response(hdlr, resp, 1, &i);
			g_free(blk.data);
			ERR("invalid property value");
			return;
			break;
		}
	}
	i = 0;
	resp = PTP_RESPONSE_OK;
	_cmd_hdlr_send_response(hdlr, resp, 1, &i);
	g_free(blk.data);
	return;
}

static void __report_acquired_content(mtp_handler_t *hdlr)
{
	mtp_uint32 tid = 0;
	mtp_uint32 start_idx = 0;
	mtp_uint32 max_size = 0;
	mtp_uint16 resp = PTP_RESPONSE_OK;
	mtp_uint32 resp_param[3] = { 0 };
	data_blk_t blk = { 0 };
	mtp_uchar *ptr = NULL;
	ptp_array_t guid_arr = { 0 };
	mtp_uint32 num_mod = 0;
	mtp_uint32 num_bytes = 0;
	mtp_uint32 num_lines = 0;
	mtp_uint32 rem_modified = 0;
	mtp_uint32 h_file;
	time_t cur_time;
	time_t l_time;
	mtp_int32 diff_time;
	mtp_int32 err = 0;
	mtp_int32 ret = 0;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 3)) {

		ERR("Unsupported parameters");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	tid = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	start_idx = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	max_size = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);

	if (tid == 0) {

		if (access(MTP_FILES_MODIFIED_FILES, F_OK) == 0)
			if (remove(MTP_FILES_MODIFIED_FILES) < 0) {
				ERR("remove(%s) Fail", MTP_FILES_MODIFIED_FILES);
			}
		resp = PTP_RESPONSE_OK;
		_prop_grow_ptparray(&guid_arr, 1);
		_prop_append_ele_ptparray(&guid_arr, 0);
		goto DONE;
	}

	g_is_sync_estab = TRUE;

	if (!g_has_round_trip && start_idx == 0) {
		time(&cur_time);
		ret = vconf_get_int(VCONFKEY_MTP_SYNC_TIME_INT, (int *)&l_time);
		if (ret == -1) {
			ERR("Error to get key value at [%s] path\n",
					VCONFKEY_MTP_SYNC_TIME_INT);
			resp = PTP_RESPONSE_OK;
			_prop_grow_ptparray(&guid_arr, 1);
			_prop_append_ele_ptparray(&guid_arr, 0);
			goto DONE;
		}
		diff_time = (cur_time - l_time) / 60;
		if (diff_time < 0) {
			resp = PTP_RESPONSE_GEN_ERROR;
			_prop_init_ptparray(&guid_arr, UINT32_TYPE);
			_prop_append_ele_ptparray(&guid_arr, 0);
			goto DONE;
		}
		_entity_list_modified_files(diff_time);
	}

	h_file = _util_file_open(MTP_FILES_MODIFIED_FILES, MTP_FILE_READ, &err);
	if (h_file == INVALID_FILE) {
		resp = PTP_RESPONSE_GEN_ERROR;
		_prop_init_ptparray(&guid_arr, UINT32_TYPE);
		_prop_append_ele_ptparray(&guid_arr, 0);
		goto DONE;
	}

	if (!g_has_round_trip && start_idx == 0) {
		_util_count_num_lines(h_file, &g_mgr->meta_info.mod);
		_util_file_seek(h_file, 0, SEEK_SET);
	}
	num_lines = g_mgr->meta_info.mod;
	num_mod = ((num_lines - start_idx) > max_size) ?
		max_size : num_lines - start_idx;

	rem_modified = (num_lines - start_idx > max_size) ?
		(num_lines - start_idx- max_size) : 0;

	g_has_round_trip = FALSE;
	_prop_init_ptparray(&guid_arr, UINT32_TYPE);
	_prop_grow_ptparray(&guid_arr, (num_mod * sizeof(mtp_uint32)) + 1);
	_prop_append_ele_ptparray(&guid_arr, num_mod);
	_util_fill_guid_array(&guid_arr, start_idx, h_file, num_mod);

	_util_file_close(h_file);
	if (rem_modified == 0) {
		if (remove(MTP_FILES_MODIFIED_FILES) < 0) {
			ERR("remove(%s) Fail", MTP_FILES_MODIFIED_FILES);
		}
		g_mgr->meta_info.mod = 0;
	}

DONE:
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	num_bytes = _prop_get_size_ptparray_without_elemsize(&guid_arr);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);

	if (NULL != ptr) {
		_prop_pack_ptparray_without_elemsize(&guid_arr, ptr, num_bytes);
		_device_set_phase(DEVICE_PHASE_DATAIN);
	}

	if (_hdlr_send_data_container(&blk)) {
		resp = PTP_RESPONSE_OK;
	} else {
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	_prop_deinit_ptparray(&guid_arr);
	g_free(blk.data);

	if (PTP_RESPONSE_OK == resp) {

		resp_param[0] = hdlr->usb_cmd.tid;
		resp_param[1] = rem_modified;
		resp_param[2] = 0;
		_cmd_hdlr_send_response(hdlr, resp, 3, resp_param);
	} else {
		_cmd_hdlr_send_response_code(hdlr, resp);
	}
	return;
}

static void __send_playback_skip(mtp_handler_t *hdlr)
{
	mtp_int32 skip = 0;
	mtp_uint16 resp = PTP_RESPONSE_INVALIDPARAM;

	skip = (mtp_int32) _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	if (MTP_ERROR_NONE == _hutil_get_playback_skip(skip)) {
		resp = PTP_RESPONSE_OK;
	}
	_cmd_hdlr_send_response_code(hdlr, resp);
	return;
}

#ifndef PMP_VER
static void __self_test(mtp_handler_t *hdlr)
{
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		ERR("Unsupported parameters");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	/* Do device-specific tests */
	/* After the test */
	_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
	return;
}

#ifdef MTP_SUPPORT_SET_PROTECTION
static void __set_object_protection(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	mtp_uint16 protcn_status = 0;
	mtp_uint16 resp = 0;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		ERR("Unsupported parameter");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	protcn_status = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);

	if ((protcn_status != PTP_PROTECTIONSTATUS_NOPROTECTION) &&
			(protcn_status != PTP_PROTECTIONSTATUS_READONLY) &&
			(protcn_status != MTP_PROTECTIONSTATUS_READONLY_DATA) &&
			(protcn_status != MTP_PROTECTIONSTATUS_NONTRANSFERABLE_DATA)) {

		resp = PTP_RESPONSE_INVALIDPARAM;
		_cmd_hdlr_send_response_code(hdlr, resp);
		return;

	}
	switch (_hutil_set_protection(h_obj, protcn_status)) {
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_OBJECT_WRITE_PROTECTED:
		resp = PTP_RESPONSE_OBJ_WRITEPROTECTED;
		break;
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	case MTP_ERROR_OPERATION_NOT_SUPPORTED:
		resp = PTP_RESPONSE_OP_NOT_SUPPORTED;
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	_cmd_hdlr_send_response_code(hdlr, resp);
	return;
}
#endif /* MTP_SUPPORT_SET_PROTECTION */

static void __power_down(mtp_handler_t *hdlr)
{
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		ERR("Unsupported Parameter");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
	usleep(1000);
	_cmd_hdlr_reset_cmd(hdlr);
	return;
}

static void __move_object(mtp_handler_t *hdlr)
{
	mtp_uint32 store_id = 0;
	mtp_uint32 obj_handle = 0;
	mtp_uint32 h_parent = 0;
	mtp_uint32 resp = 0;

	obj_handle = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	store_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	h_parent = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);

	_transport_set_mtp_operation_state(MTP_STATE_DATA_PROCESSING);

	switch (_hutil_move_object_entry(store_id, h_parent, obj_handle)) {
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_OBJECT_WRITE_PROTECTED:
		resp = PTP_RESPONSE_OBJ_WRITEPROTECTED;
		break;
	case MTP_ERROR_ACCESS_DENIED:
		resp = PTP_RESPONSE_ACCESSDENIED;
		break;
	case MTP_ERROR_STORE_NOT_AVAILABLE:
		resp = PTP_RESPONSE_STORENOTAVAILABLE;
		break;
	case MTP_ERROR_INVALID_PARENT:
		resp = PTP_RESPONSE_INVALIDPARENT;
		break;
	case MTP_ERROR_INVALID_PARAM:
		resp = PTP_RESPONSE_INVALIDPARAM;
		break;
	case MTP_ERROR_STORE_FULL:
		resp = PTP_RESPONSE_STOREFULL;
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);
	_cmd_hdlr_send_response_code(hdlr, resp);
	return;
}

static void __copy_object(mtp_handler_t *hdlr)
{
	mtp_uint32 store_id = 0;
	mtp_uint32 h_obj = 0;
	mtp_uint32 h_parent = 0;
	mtp_uint32 new_handle = 0;
	mtp_uint16 resp = 0;

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	store_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	h_parent = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);

	_transport_set_mtp_operation_state(MTP_STATE_DATA_PROCESSING);

	switch (_hutil_duplicate_object_entry(store_id, h_parent, h_obj,
				&new_handle)) {
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_OBJECT_WRITE_PROTECTED:
		resp = PTP_RESPONSE_OBJ_WRITEPROTECTED;
		break;
	case MTP_ERROR_STORE_NOT_AVAILABLE:
		resp = PTP_RESPONSE_STORENOTAVAILABLE;
		break;
	case MTP_ERROR_STORE_READ_ONLY:
		resp = PTP_RESPONSE_STORE_READONLY;
		break;
	case MTP_ERROR_INVALID_PARENT:
		resp = PTP_RESPONSE_INVALIDPARENT;
		break;
	case MTP_ERROR_INVALID_PARAM:
		resp = PTP_RESPONSE_INVALIDPARAM;
		break;
	case MTP_ERROR_STORE_FULL:
		resp = PTP_RESPONSE_STOREFULL;
		break;
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	case MTP_ERROR_ACCESS_DENIED:
		resp = PTP_RESPONSE_ACCESSDENIED;
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
	}
	_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);

	if (resp == PTP_RESPONSE_OK) {
		_cmd_hdlr_send_response(hdlr, resp, 1, &new_handle);
	} else {
		_cmd_hdlr_send_response_code(hdlr, resp);
	}

	return;
}

static void __reset_device_prop_value(mtp_handler_t *hdlr)
{
	mtp_uint32 prop_id = 0;
	device_prop_desc_t *prop = NULL;
	mtp_char temp[MTP_MAX_REG_STRING * 3 + 1] = { 0 };

	prop_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);

	if (MTP_ERROR_NONE != _hutil_reset_device_entry(prop_id)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PROP_NOTSUPPORTED);
		return;
	}

	if (MTP_PROPERTYCODE_DEVICEFRIENDLYNAME == prop_id) {
		prop = _device_get_device_property(prop_id);
		if (prop == NULL) {
			_cmd_hdlr_send_response_code(hdlr,
					PTP_RESPONSE_PROP_NOTSUPPORTED);
			return;
		}

		_util_utf16_to_utf8(temp, sizeof(temp),
				prop->current_val.str->str);
		_device_set_device_name(temp);

	} else if (MTP_PROPERTYCODE_SYNCHRONIZATIONPARTNER == prop_id) {
		prop = _device_get_device_property(prop_id);
		if (NULL == prop) {
			_cmd_hdlr_send_response_code(hdlr,
					PTP_RESPONSE_PROP_NOTSUPPORTED);
			return;
		}

		_util_utf16_to_utf8(temp, sizeof(temp),
				prop->current_val.str->str);
		_device_set_sync_partner(temp);

		if (!g_strcmp0(temp, MTP_DEV_PROPERTY_NULL_SYNCPARTNER)) {
			vconf_set_str(VCONFKEY_MTP_SYNC_PARTNER_STR, "");
		} else {
			vconf_set_str(VCONFKEY_MTP_SYNC_PARTNER_STR, temp);
		}
	}
	_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);

	return;
}

/* Vendor-specific operations */
#define GET_DEVICEPC_NAME	1
static void __vendor_command1(mtp_handler_t *hdlr)
{
	switch (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0)) {
	case GET_DEVICEPC_NAME:
		break;
	default:
		break;
	}
	/* Vendor command not properly handled*/
	_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	return;
}

static void __get_interdep_prop_desc(mtp_handler_t *hdlr)
{
	mtp_uint32 fmt = 0;
	data_blk_t blk = { 0 };
	mtp_uint32 num_bytes = 0;
	mtp_uchar *ptr = NULL;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	if (0x0 == fmt || 0xFFFFFFFF == fmt) {
		ERR("Invalid format code");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_INVALIDCODEFORMAT);
		return;
	}

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	_hutil_get_interdep_prop_config_list_size(&num_bytes, fmt);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);

	if (MTP_ERROR_NONE != _hutil_get_interdep_prop_config_list_data(ptr,
				num_bytes, fmt)) {
		ERR("_hutil_get_interdep_prop_config_list_data() Fail");
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		return;
	}

	_device_set_phase(DEVICE_PHASE_DATAIN);
	if (_hdlr_send_data_container(&blk)) {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
	} else {
		/*Host Cancelled data-in transfer*/
		_device_set_phase(DEVICE_PHASE_NOTREADY);
	}

	g_free(blk.data);
	return;
}

/*
 * void __cmd_hdlr_send_object_prop_list(mtp_handler_t *hdlr)
 * This function is used as an alternative first operation when the initiator
 * wants to send an object to the responder. This operation sends a
 * modified ObjectPropList dataset from the initiator to the Responder.
 *@param[in]		hdlr
 *@return		none
 */
static void __send_object_prop_list(mtp_handler_t *hdlr)
{
	mtp_uint32 idx = 0;
	mtp_uint16 resp = PTP_RESPONSE_OK;
	mtp_uint32 resp_param[MAX_MTP_PARAMS] = { 0 };
	mtp_obj_t *obj = NULL;
	mtp_uint16 fmt = 0;
	mtp_uint64 f_size = 0;
	mtp_uint64 fsize_hbits = 0;
	mtp_uint32 store_id;
	mtp_uint32 h_parent;
	data_blk_t blk = { 0 };
	mtp_uint32 max_bytes = 0;
	mtp_err_t ret = 0;
	obj_data_t objdata = { 0 };

	store_id = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	h_parent = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);
	fsize_hbits = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 3);

	f_size = (fsize_hbits << 32) + _hdlr_get_param_cmd_container
		(&(hdlr->usb_cmd), 4);

	_device_set_phase(DEVICE_PHASE_DATAOUT);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);

	max_bytes = MTP_MAX_METADATA;
	if (FALSE == _hdlr_rcv_data_container(&blk, max_bytes)) {
		_device_set_phase(DEVICE_PHASE_IDLE);
		ERR("_hdlr_rcv_data_container() Fail");
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	idx = 0;
	if (store_id) {
		if (!h_parent) {
			h_parent = _device_get_default_parent_handle();
		} else if (h_parent == 0xFFFFFFFF) {
			h_parent = PTP_OBJECTHANDLE_ROOT;
		}
	} else {
		store_id = _device_get_default_store_id();
		if (!store_id)
			resp = PTP_RESPONSE_STORENOTAVAILABLE;
		if (h_parent)
			/* If the second parameter is used, the first must
			 * also be used
			 * */
			resp = PTP_RESPONSE_PARAM_NOTSUPPORTED;
		else
			h_parent = _device_get_default_parent_handle();
	}

	if (f_size >= MTP_FILESIZE_4GB) {

		mtp_store_t *store = NULL;
		struct statfs buf = { 0 };
		mtp_int32 ret = 0;

		store = _device_get_store(store_id);
		if (store == NULL) {
			ERR("Store Not Available");
			resp = PTP_RESPONSE_STORENOTAVAILABLE;
		} else {
			DBG("StorePath = [%s]\n", store->root_path);
			ret = statfs(store->root_path, &buf);
			if (ret < 0 || buf.f_type == MSDOS_SUPER_MAGIC) {
				ERR("File System does not support files over 4gb");
				resp = MTP_RESPONSE_OBJECT_TOO_LARGE;
			}
		}
	}

	if (PTP_RESPONSE_OK == resp) {

		mtp_uchar *data = NULL;

		if (TRUE == hdlr->data4_send_obj.is_valid) {

			objdata.store_id = hdlr->data4_send_obj.store_id;
			objdata.obj_size = hdlr->data4_send_obj.file_size;
			objdata.obj = hdlr->data4_send_obj.obj;
			hdlr->data4_send_obj.obj = NULL;
		}

		data = _hdlr_get_payload_data(&blk);
		if (data == NULL)
			return;

		ret = _hutil_construct_object_entry_prop_list(store_id, h_parent,
				fmt, f_size, ((hdlr->data4_send_obj.is_valid == TRUE) ?
					(&objdata) : (NULL)), &obj, data,
				_hdlr_get_payload_size(&blk), &idx);
		hdlr->data4_send_obj.is_valid = FALSE;

		switch (ret) {
		case MTP_ERROR_INVALID_STORE:
			resp = PTP_RESPONSE_INVALID_STORE_ID;
			break;
		case MTP_ERROR_STORE_READ_ONLY:
			resp = PTP_RESPONSE_STORE_READONLY;
			break;
		case MTP_ERROR_STORE_FULL:
			resp = PTP_RESPONSE_STOREFULL;
			break;
		case MTP_ERROR_GENERAL:
			resp = PTP_RESPONSE_GEN_ERROR;
			break;
		case MTP_ERROR_INVALID_DATASET:
			resp = MTP_RESPONSECODE_INVALIDDATASET;
			break;
		case MTP_ERROR_INVALID_OBJECTHANDLE:
			resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
			break;
		case MTP_ERROR_INVALID_OBJ_PROP_CODE:
			resp = MTP_RESPONSE_INVALIDOBJPROPCODE;
			break;
		case MTP_ERROR_INVALID_OBJECT_PROP_FORMAT:
			resp = MTP_RESPONSE_INVALIDOBJPROPFORMAT;
			break;
		case MTP_ERROR_INVALID_OBJ_PROP_VALUE:
			resp = MTP_RESPONSE_INVALIDOBJPROPVALUE;
			break;
		case MTP_ERROR_INVALID_PARENT:
			resp = PTP_RESPONSE_INVALIDPARENT;
			break;
		case MTP_ERROR_ACCESS_DENIED:
			resp = PTP_RESPONSE_ACCESSDENIED;
			break;
		case MTP_ERROR_NONE:
#ifdef MTP_USE_SELFMAKE_ABSTRACTION
			if ((obj->obj_info->obj_fmt == PTP_FMT_ASSOCIATION) ||
					(obj->obj_info->file_size == 0 &&
					 obj->obj_info->obj_fmt >
					 MTP_FMT_UNDEFINED_COLLECTION &&
					 obj->obj_info->obj_fmt <
					 MTP_FMT_UNDEFINED_DOC))
#else /*MTP_USE_SELFMAKE_ABSTRACTION*/
				if (obj->obj_info->obj_fmt == PTP_FMT_ASSOCIATION)
#endif /*MTP_USE_SELFMAKE_ABSTRACTION*/
				{
					hdlr->data4_send_obj.obj = NULL;
					hdlr->data4_send_obj.is_valid = FALSE;
					hdlr->data4_send_obj.obj_handle = obj->obj_handle;
					hdlr->data4_send_obj.h_parent = h_parent;
					hdlr->data4_send_obj.store_id = store_id;
					hdlr->data4_send_obj.file_size = 0;
				} else {
					hdlr->data4_send_obj.is_valid = TRUE;
					hdlr->data4_send_obj.obj_handle = obj->obj_handle;
					hdlr->data4_send_obj.h_parent = h_parent;
					hdlr->data4_send_obj.store_id = store_id;
					hdlr->data4_send_obj.obj = obj;
					hdlr->data4_send_obj.file_size =
						obj->obj_info->file_size;
				}
			resp = PTP_RESPONSE_OK;
			break;
		default:
			resp = PTP_RESPONSE_GEN_ERROR;
		}
	}

	if (PTP_RESPONSE_OK != resp) {
		if (hdlr->data4_send_obj.obj) {
			_entity_dealloc_mtp_obj(hdlr->data4_send_obj.obj);
		}

		hdlr->data4_send_obj.obj = NULL;
		hdlr->data4_send_obj.is_valid = FALSE;
		resp_param[3] = idx;
	} else {
		resp_param[0] = hdlr->data4_send_obj.store_id;

		/* PTP spec here requires that 0xFFFFFFFF be sent if root is the parent,
		 * while in some situations (e.g. Move Object, Copy Object), 0x00000000
		 *(as PTP_OBJECTHANDLE_ROOT is defined) represents the root
		 */
		resp_param[1] = (hdlr->data4_send_obj.h_parent !=
				PTP_OBJECTHANDLE_ROOT) ? hdlr->data4_send_obj.h_parent
			: 0xFFFFFFFF;
		resp_param[2] = hdlr->data4_send_obj.obj_handle;
		resp_param[3] = 0;
	}

	_cmd_hdlr_send_response(hdlr, resp, 4, resp_param);

	g_free(blk.data);
	return;
}
#endif /*PMP_VER*/

void __close_session(mtp_handler_t *hdlr)
{
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		ERR("PTP_RESPONSE_PARAM_NOTSUPPORTED");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);

		return;
	}

	if (hdlr->session_id) {
		hdlr->session_id = 0;
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
	} else {
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_SESSIONNOTOPEN);
		ERR("PTP_RESPONSE_SESSIONNOTOPEN");
	}
	return;
}

mtp_bool _cmd_hdlr_send_response(mtp_handler_t *hdlr, mtp_uint16 resp,
		mtp_uint32 num_param, mtp_uint32 *params)
{
	mtp_bool ret = FALSE;
	resp_blk_t blk = { 0 };

	_hdlr_resp_container_init(&blk, resp, hdlr->usb_cmd.tid);

	ret = _hdlr_add_param_resp_container(&blk, num_param, params);

	_device_set_phase(DEVICE_PHASE_RESPONSE);
	ret = _hdlr_send_resp_container(&blk);
	_device_set_phase(DEVICE_PHASE_IDLE);

	if ((resp == PTP_RESPONSE_OK) && (ret == TRUE)) {
		DBG("[%s], Opcode[0x%4x], ResponseCode[0x%4x], NumParams[%d]\n",
				"SUCCESS", hdlr->usb_cmd.code, resp, num_param);
	} else {
		ERR("[%s], Opcode = [0x%4x] ResponseCode[0x%4x], NumParams[%u]\n",
				"FAIL", hdlr->usb_cmd.code, resp, num_param);
	}
	return ret;
}

mtp_bool _cmd_hdlr_send_response_code(mtp_handler_t *hdlr, mtp_uint16 resp)
{
	return _cmd_hdlr_send_response(hdlr, resp, 0, NULL);
}

#ifdef MTP_SUPPORT_PRINT_COMMAND
static void __print_command(mtp_uint16 code)
{
	switch(code) {
	case PTP_OPCODE_GETDEVICEINFO :
		DBG("COMMAND ======== GET DEVICE INFO===========");
		break;
	case PTP_OPCODE_OPENSESSION :
		DBG("COMMAND ======== OPEN SESSION ===========");
		break;
	case PTP_OPCODE_CLOSESESSION :
		DBG("COMMAND ======== CLOSE SESSION ===========");
		break;
	case PTP_OPCODE_GETSTORAGEIDS :
		DBG("COMMAND ======== GET STORAGE IDS ===========");
		break;
	case PTP_OPCODE_GETSTORAGEINFO :
		DBG("COMMAND ======== GET STORAGE INFO ===========");
		break;
	case PTP_OPCODE_GETNUMOBJECTS :
		DBG("COMMAND ======== GET NUM OBJECTS ===========");
		break;
	case PTP_OPCODE_GETOBJECTHANDLES :
		DBG("COMMAND ======== GET OBJECT HANDLES ===========");
		break;
	case PTP_OPCODE_GETOBJECTINFO :
		DBG("COMMAND ======== GET OBJECT INFO ===========");
		break;
	case PTP_OPCODE_GETOBJECT :
		DBG("COMMAND ======== GET OBJECT ===========");
		break;
	case PTP_OPCODE_DELETEOBJECT :
		DBG("COMMAND ======== DELETE OBJECT ===========");
		break;
	case PTP_OPCODE_SENDOBJECTINFO :
		DBG("COMMAND ======== SEND OBJECT INFO ===========");
		break;
	case PTP_OPCODE_SENDOBJECT :
		DBG("COMMAND ======== SEND OBJECT ===========");
		break;
	case PTP_OPCODE_INITIATECAPTURE :
		DBG("COMMAND ======== INITIATE CAPTURE ===========");
		break;
	case PTP_OPCODE_FORMATSTORE :
		DBG("COMMAND ======== FORMAT STORE ===========");
		break;
	case PTP_OPCODE_RESETDEVICE :
		DBG("COMMAND ======== RESET DEVICE ===========");
		break;
	case PTP_OPCODE_SELFTEST :
		DBG("COMMAND ======== SELF TEST ===========");
		break;
	case PTP_OPCODE_SETOBJECTPROTECTION :
		DBG("COMMAND ======== SET OBJECT PROTECTION ===========");
		break;
	case PTP_OPCODE_POWERDOWN :
		DBG("COMMAND ======== POWER DOWN ===========");
		break;
	case PTP_OPCODE_GETDEVICEPROPDESC :
		DBG("COMMAND ======== GET DEVICE PROP DESC ===========");
		break;
	case PTP_OPCODE_GETDEVICEPROPVALUE :
		DBG("COMMAND ======== GET DEVICE PROP VALUE ===========");
		break;
	case PTP_OPCODE_SETDEVICEPROPVALUE :
		DBG("COMMAND ======== SET DEVICE PROP VALUE ===========");
		break;
	case PTP_OPCODE_RESETDEVICEPROPVALUE :
		DBG("COMMAND ======== RESET DEVICE PROP VALUE ===========");
		break;
	case PTP_OPCODE_TERMINATECAPTURE :
		DBG("COMMAND ======== TERMINATE CAPTURE ===========");
		break;
	case PTP_OPCODE_MOVEOBJECT :
		DBG("COMMAND ======== MOVE OBJECT ===========");
		break;
	case PTP_OPCODE_COPYOBJECT :
		DBG("COMMAND ======== COPY OBJECT ===========");
		break;
	case PTP_OPCODE_GETPARTIALOBJECT :
		DBG("COMMAND ======== GET PARTIAL OBJECT ===========");
		break;
	case PTP_OPCODE_INITIATEOPENCAPTURE :
		DBG("COMMAND ======== INITIATE OPEN CAPTURE ===========");
		break;
	case MTP_OPCODE_WMP_UNDEFINED :
		DBG("COMMAND ======== WMP UNDEFINED ==========");
		break;
	case MTP_OPCODE_WMP_REPORTACQUIREDCONTENT :
		DBG("COMMAND ======= REPORT ACQUIRED CONTENT =========");
		break;
	case MTP_OPCODE_GETOBJECTPROPSUPPORTED :
		DBG("COMMAND ======= GET OBJECT PROP SUPPORTED ========");
		break;
	case MTP_OPCODE_GETOBJECTPROPDESC :
		DBG("COMMAND ======== GET OBJECT PROP DESC ==========");
		break;
	case MTP_OPCODE_GETOBJECTPROPVALUE :
		DBG("COMMAND ======== GET OBJECT PROP VALUE ==========");
		break;
	case MTP_OPCODE_SETOBJECTPROPVALUE :
		DBG("COMMAND ======== SET OBJECT PROP VALUE ==========");
		break;
	case MTP_OPCODE_GETOBJECTPROPLIST :
		DBG("COMMAND ======== GET OBJECT PROP LIST ==========");
		break;
	case MTP_OPCODE_SETOBJECTPROPLIST :
		DBG("COMMAND ======== SET OBJECT PROP LIST ==========");
		break;
	case MTP_OPCODE_GETINTERDEPPROPDESC :
		DBG("COMMAND ======== GET INTERDEP PROP DESC ==========");
		break;
	case MTP_OPCODE_SENDOBJECTPROPLIST :
		DBG("COMMAND ======== SEND OBJECT PROP LIST ==========");
		break;
	case MTP_OPCODE_GETOBJECTREFERENCES :
		DBG("COMMAND ======== GET OBJECT REFERENCES ==========");
		break;
	case MTP_OPCODE_SETOBJECTREFERENCES :
		DBG("COMMAND ======== SET OBJECT REFERENCES ==========");
		break;
	case MTP_OPCODE_PLAYBACK_SKIP :
		DBG("COMMAND ======== PLAYBACK SKIP ==========");
		break;
	default :
		DBG("======== UNKNOWN COMMAND ==========");
		break;
	}

	return;
}
#endif /*MTP_SUPPORT_PRINT_COMMAND*/

static void __enum_store_not_enumerated(mtp_uint32 obj_handle,
		mtp_uint32 fmt, mtp_uint32 depth)
{
	mtp_uint32 ii;
	mtp_store_t *store = NULL;
	mtp_obj_t *obj = NULL;

	if (TRUE == g_is_full_enum) {
		DBG("Full Enumeration has been already done");
		return;
	}

	DBG("obj_handle = [%u], format =[ %u], depth = [%u]\n", obj_handle,
			fmt, depth);
	if (obj_handle == PTP_OBJECTHANDLE_ALL || obj_handle == PTP_OBJECTHANDLE_ROOT) {
		for (ii = 0; ii < _device_get_num_stores(); ii++) {
			store = _device_get_store_at_index(ii);
			if (store && store->obj_list.nnodes == 0)
				_entity_store_recursive_enum_folder_objects(store, NULL);
		}
		g_is_full_enum = TRUE;
	} else if (obj_handle != PTP_OBJECTHANDLE_ROOT) {
		store = _device_get_store_containing_obj(obj_handle);
		obj = _entity_get_object_from_store(store, obj_handle);
		if (obj == NULL) {
			ERR("pObject is NULL");
			return;
		}
		_entity_store_recursive_enum_folder_objects(store, obj);
	}
	return;
}

void _receive_mq_data_cb(mtp_char *buffer, mtp_int32 buf_len)
{
	cmd_blk_t cmd = { 0 };
	mtp_uint32 rx_size = _get_rx_pkt_size();

	if (_transport_get_mtp_operation_state() < MTP_STATE_READY_SERVICE) {
		ERR("MTP is stopped or initializing. ignore all");
		return;
	}

#ifdef MTP_SUPPORT_CONTROL_REQUEST
	/* process control request */
	switch (_transport_get_control_event()) {
	case PTP_EVENTCODE_CANCELTRANSACTION:
		DBG("PTP_EVENTCODE_CANCELTRANSACTION, just change state to IDLE");
		_transport_set_control_event(PTP_EVENTCODE_CANCELTRANSACTION);
		_device_set_phase(DEVICE_PHASE_IDLE);
		if ((buf_len == rx_size) ||
				(buf_len < sizeof(header_container_t))) {
			DBG("Cancelling Transaction. data length [%d]\n",
					buf_len);
			_transport_set_control_event(PTP_EVENTCODE_CANCELTRANSACTION);
			return;
		}

		mtp_int32 i = 0;
		cmd_container_t *tmp;
		mtp_dword len = 0;
		mtp_word type = 0;
		mtp_word code = 0;
		mtp_dword trid = 0;

		for (i = 0; i < MAX_MTP_PARAMS; i++) {	/* check size */
			/* check number of parameter */
			tmp = (cmd_container_t *)&buffer[buf_len -
				sizeof(header_container_t) -
				sizeof(mtp_dword) * i];

			len = tmp->len;
			type = tmp->type;

			if ((len == sizeof(header_container_t)
						+ sizeof(mtp_dword) * i) &&
					(type == CONTAINER_CMD_BLK)) {
				DBG("Found Command in remaining data");
				memcpy(buffer, tmp, len);
				buf_len = len;
				break;
			}
			DBG("Not found command, length[%lu]\n", len);
		}

		len = tmp->len;
		type = tmp->type;
		code = tmp->code;
		trid = tmp->tid;

		DBG("len[%ld], type[0x%x], code [0x%x], trid[0x%x]\n",
				len, type, code, trid);

		if (_hdlr_validate_cmd_container((mtp_byte *)tmp, len)
				== FALSE) {
			ERR("Cancelling Transaction, invalid header, but last packet");
		} else {	/*another command, cancelling is finished*/
			DBG("Cancelling Transaction, Finished.");
		}

		_transport_set_control_event(0);
		_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);
		if (g_mgr->ftemp_st.fhandle != INVALID_FILE) {
			DBG("In Cancel Transaction fclose ");
			_util_file_close(g_mgr->ftemp_st.fhandle);
			g_mgr->ftemp_st.fhandle = INVALID_FILE;
			DBG("In Cancel Transaction, remove ");
			if (remove(g_mgr->ftemp_st.filepath) < 0) {
				ERR_SECURE("remove(%s) Fail", g_mgr->ftemp_st.filepath);
			}
		} else {
			DBG("g_mgr->ftemp_st.fhandle is not valid, return");
		}
		break;

	case PTP_EVENTCODE_DEVICERESET:
		DBG("Implement later, PTP_EVENTCODE_DEVICERESET");
		_device_set_phase(DEVICE_PHASE_IDLE);
		/* do close session, just skip save reference
		 * mtp_save_object_references_mtp_device(MtpHandler.pDevice);
		 */
		g_mgr->hdlr.session_id = 0;
		_transport_set_control_event(0);
		_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);
		break;

	default:
		break;
	}
#endif/* MTP_SUPPORT_CONTROL_REQUEST */

	/* main processing */
	if (_device_get_phase() == DEVICE_PHASE_IDLE) {
		if (_hdlr_validate_cmd_container((mtp_uchar *)buffer, buf_len)
				== FALSE) {
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			ERR("MTP device phase NOT READY, invalid Command block");
			return;
		}

		_transport_save_cmd_buffer(buffer, buf_len);
		_hdlr_copy_cmd_container_unknown_params((cmd_container_t *)buffer,
				&cmd);
#ifdef __BIG_ENDIAN__
		_hdlr_conv_cmd_container_byte_order(&cmd);
#endif /* __BIG_ENDIAN__ */

		UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
		__process_commands(&g_mtp_mgr.hdlr, &cmd);
		UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);
	} else if (_device_get_phase() == DEVICE_PHASE_DATAOUT) {
		if (g_mgr->ftemp_st.data_count == 0)
			__receive_temp_file_first_packet(buffer, buf_len);
		else
			__receive_temp_file_next_packets(buffer, buf_len);
		return;
	} else {
		/* ignore other case */
		ERR("MTP device phase[%d], unknown device PHASE\n",
				_device_get_phase());
		ERR("PhaseUnknown-> pData[0x%x], length=[%d]", buffer, buf_len);
		_device_set_phase(DEVICE_PHASE_IDLE);
		_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);
	}
	return;
}

static mtp_bool __receive_temp_file_first_packet(mtp_char *data,
		mtp_int32 data_len)
{
	mtp_char *filepath = g_mgr->ftemp_st.filepath;
	mtp_uint32 *fhandle = &g_mgr->ftemp_st.fhandle;
	mtp_int32 error = 0;
	mtp_uint32 *data_sz = &g_mgr->ftemp_st.data_size;
	mtp_char *buffer = g_mgr->ftemp_st.temp_buff;

	_transport_set_mtp_operation_state(MTP_STATE_DATA_TRANSFER_DL);
	if (access(filepath, F_OK) == 0) {
		if (*fhandle != INVALID_FILE) {
			_util_file_close(*fhandle);
			*fhandle = INVALID_FILE;	/* initialize */
		}

		if (remove(filepath) < 0) {
			ERR_SECURE("remove(%s) Fail", filepath);
			__finish_receiving_file_packets(data, data_len);
			return FALSE;
		}
	}

	*fhandle = _util_file_open(filepath, MTP_FILE_WRITE, &error);
	if (*fhandle == INVALID_FILE) {
		ERR("First file handle is invalid!!");
		__finish_receiving_file_packets(data, data_len);
		return FALSE;
	}
	/* consider header size */
	memcpy(&g_mgr->ftemp_st.header_buf, data, sizeof(header_container_t));

	g_mgr->ftemp_st.file_size = ((header_container_t *)data)->len -
		sizeof(header_container_t);
	*data_sz = data_len - sizeof(header_container_t);

	/* check whether last data packet */
	if (*data_sz == g_mgr->ftemp_st.file_size) {
		if (_util_file_write(*fhandle, &data[sizeof(header_container_t)],
					data_len - sizeof(header_container_t)) !=
				data_len - sizeof(header_container_t)) {
			ERR("fwrite error!");
		}
		*data_sz = 0;
		_util_file_close(*fhandle);
		*fhandle = INVALID_FILE;	/* initialize */
		__finish_receiving_file_packets(data, data_len);
	} else {
		g_mgr->ftemp_st.data_count++;
		g_mgr->ftemp_st.size_remaining = *data_sz;

		memcpy(buffer, data + sizeof(header_container_t), *data_sz);
	}
	return TRUE;
}

static mtp_bool __receive_temp_file_next_packets(mtp_char *data,
		mtp_int32 data_len)
{
	mtp_uint32 rx_size = _get_rx_pkt_size();
	mtp_uint32 *data_sz = &g_mgr->ftemp_st.data_size;
	mtp_char *buffer = g_mgr->ftemp_st.temp_buff;
	mtp_uint32 *fhandle = &g_mgr->ftemp_st.fhandle;

	g_mgr->ftemp_st.data_count++;
	g_mgr->ftemp_st.size_remaining += data_len;

	if ((*data_sz + (mtp_uint32)data_len) > g_conf.write_file_size) {
		/* copy oversized packet to temp file */
		if (_util_file_write(*fhandle, buffer, *data_sz) != *data_sz)
			ERR("fwrite error writeSize=[%u]\n", *data_sz);

		*data_sz = 0;
	}

	memcpy(&buffer[*data_sz], data, data_len);
	*data_sz += data_len;

	/*Complete file is recieved, so close the file*/
	if (data_len < rx_size ||
			g_mgr->ftemp_st.size_remaining == g_mgr->ftemp_st.file_size) {

		if (_util_file_write(*fhandle, buffer, *data_sz) != *data_sz) {
			ERR("fwrite error write size=[%u]\n", *data_sz);
		}
		*data_sz = 0;
		_util_file_close(*fhandle);
		*fhandle = INVALID_FILE;	/* initialize */
		__finish_receiving_file_packets(data, data_len);
	}
	return TRUE;
}

static void __finish_receiving_file_packets(mtp_char *data, mtp_int32 data_len)
{
	cmd_blk_t cmd = { 0 };
	mtp_uchar *cmd_buf = NULL;

	g_mgr->ftemp_st.data_count = 0;
	cmd_buf = (mtp_uchar *)data;

	if (!_hdlr_validate_cmd_container(cmd_buf, (mtp_uint32)data_len)) {
		cmd_buf = (mtp_uchar *)g_mgr->ftemp_st.cmd_buf;
		if (!_hdlr_validate_cmd_container(cmd_buf,
					g_mgr->ftemp_st.cmd_size)) {
			_device_set_phase(DEVICE_PHASE_IDLE);
			ERR("DATA PROCESS, device phase[%d], invalid Command\
					block\n", _device_get_phase());
			return;
		}
	}

	_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);
	_hdlr_copy_cmd_container_unknown_params((cmd_container_t *)cmd_buf,
			&cmd);

#ifdef __BIG_ENDIAN__
	_hdlr_conv_cmd_container_byte_order(&cmd);
#endif /* __BIG_ENDIAN__ */

	UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
	__process_commands(&g_mtp_mgr.hdlr, &cmd);
	UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);

	DBG("MTP device phase[%d], processing Command is complete\n",
			_device_get_phase());

	return;
}
