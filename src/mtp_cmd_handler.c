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

#include <sys/vfs.h>
#include <linux/magic.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "mtp_support.h"
#include "ptp_datacodes.h"
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
extern mtp_bool g_is_send_partial_object;

mtp_bool g_is_sync_estab = FALSE;
mtp_bool g_is_send_object = FALSE;

#define LEN 20

#define _device_set_phase(_phase) 			\
	do {						\
		DBG("Devie phase is set [%d]\n", (_phase));\
		g_device->phase = (_phase);		\
	} while (0)

#define _transport_get_control_event(...)		\
	({mtp_uint32 event_code = g_status->ctrl_event_code; g_status->ctrl_event_code = 0; event_code;})


static void __get_object_prop_desc(mtp_handler_t *hdlr);

/*
 * FUNCTIONS
 */
void _cmd_hdlr_reset_cmd(mtp_handler_t *hdlr)
{
	if (hdlr->data4_send_obj.obj != NULL)
		_entity_dealloc_mtp_obj(hdlr->data4_send_obj.obj);

	memset(hdlr, 0x00, sizeof(mtp_handler_t));
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
			DBG("Device phase is set to DEVICE_PHASE_NOTREADY\n");
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

	if (_hutil_get_storage_ids(&ids) == MTP_ERROR_NONE) {
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
				DBG("DEVICE_PHASE_NOTREADY!!\n");
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
			DBG("DEVICE_PHASE_NOTREADY!!\n");
		}
	} else {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
	}

	g_free(blk.data);
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
			DBG("DEVICE_PHASE_NOTREADY!!\n");
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
			DBG("DEVICE_PHASE_NOTREADY!!\n");
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
	FILE* h_file = NULL;
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
		ERR("_hdlr_alloc_buf_data_container() Fail\n");
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		g_free(blk.data);
		return;
	}

	_device_set_phase(DEVICE_PHASE_DATAIN);
	h_file = _util_file_open(path, MTP_FILE_READ, &error);
	if (h_file == NULL) {
		ERR("_util_file_open() Fail\n");
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		if (EACCES == error) {
			_cmd_hdlr_send_response_code(hdlr,
					PTP_RESPONSE_ACCESSDENIED);
			g_free(blk.data);
			return;
		}
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		g_free(blk.data);
		return;
	}

	_util_file_read(h_file, ptr, packet_len, &read_len);
	if (0 == read_len) {
		ERR("_util_file_read() Fail\n");
		ERR_SECURE("filename[%s]\n", path);
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		resp = PTP_RESPONSE_INCOMPLETETRANSFER;
		goto Done;
	}

	/* First Packet with Header */
	if (PTP_EVENTCODE_CANCELTRANSACTION == _transport_get_control_event() ||
			FALSE == _hdlr_send_bulk_data(blk.data, blk.len)) {
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		resp = PTP_RESPONSE_INCOMPLETETRANSFER;
		ERR("First Packet send Fail\n");
		ERR_SECURE("filename[%s]\n", path);
		goto Done;
	}

	sent = sizeof(header_container_t) + read_len;
	ptr = blk.data;

	while (sent < total_len) {
		_util_file_read(h_file, ptr, g_conf.read_file_size, &read_len);
		if (0 == read_len) {
			ERR("_util_file_read() Fail\n");
			ERR_SECURE("filename[%s]\n", path);
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			resp = PTP_RESPONSE_INCOMPLETETRANSFER;
			goto Done;
		}

		if (PTP_EVENTCODE_CANCELTRANSACTION == _transport_get_control_event() ||
				FALSE == _hdlr_send_bulk_data(ptr, read_len)) {
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			resp = PTP_RESPONSE_INCOMPLETETRANSFER;
			ERR("Packet send Fail\n");
			ERR_SECURE("filename[%s]\n", path);
			goto Done;
		}

		sent += read_len;
	}

Done:
	_util_file_close(h_file);

	g_free(blk.data);
	_cmd_hdlr_send_response_code(hdlr, resp);
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
	if (store_id == 0)
		store_id = g_device->default_store_id;

	h_parent = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		resp = PTP_RESPONSE_PARAM_NOTSUPPORTED;
		if (g_device->phase != DEVICE_PHASE_NOTREADY)
			_cmd_hdlr_send_response_code(hdlr, resp);

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
			DBG("PTP_RESPONSE_STORENOTAVAILABLE\n");
			break;
		case MTP_ERROR_INVALID_PARAM:
			resp = PTP_RESPONSE_PARAM_NOTSUPPORTED;
			DBG("PTP_RESPONSE_PARAM_NOTSUPPORTED\n");
			break;
		case MTP_ERROR_INVALID_STORE:
			resp = PTP_RESPONSE_INVALID_STORE_ID;
			DBG("PTP_RESPONSE_INVALID_STORE_ID\n");
			break;
		case MTP_ERROR_STORE_READ_ONLY:
			resp = PTP_RESPONSE_STORE_READONLY;
			break;
		case MTP_ERROR_STORE_FULL:
			resp = PTP_RESPONSE_STOREFULL;
			DBG("PTP_RESPONSE_STOREFULL\n");
			break;
		case MTP_ERROR_GENERAL:
			resp = PTP_RESPONSE_GEN_ERROR;
			DBG("PTP_RESPONSE_GEN_ERROR\n");
			break;
		case MTP_ERROR_INVALID_OBJECT_INFO:
			resp = MTP_RESPONSECODE_INVALIDDATASET;
			DBG("MTP_RESPONSECODE_INVALIDDATASET\n");
			break;
		case MTP_ERROR_INVALID_OBJECTHANDLE:
			resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
			DBG("PTP_RESPONSE_INVALID_OBJ_HANDLE\n");
			break;
		case MTP_ERROR_INVALID_PARENT:
			resp = PTP_RESPONSE_INVALIDPARENT;
			DBG("PTP_RESPONSE_INVALIDPARENT\n");
			break;
		case MTP_ERROR_ACCESS_DENIED:
			resp = PTP_RESPONSE_ACCESSDENIED;
			DBG("PTP_RESPONSE_ACCESSDENIED\n");
			break;
		default:
			resp = PTP_RESPONSE_GEN_ERROR;
			DBG("PTP_RESPONSE_GEN_ERROR\n");
			break;
		}
	}

	g_free(blk.data);
	if (g_device->phase != DEVICE_PHASE_NOTREADY) {
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

		ERR("unsupported parameter\n");
		_device_set_phase(DEVICE_PHASE_NOTREADY);
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_INVALIDPARAM);
		return;
	}

	_device_set_phase(DEVICE_PHASE_DATAOUT);
	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);

	if (_hdlr_rcv_file_in_data_container(&blk, temp_fpath,
				MTP_MAX_PATHNAME_SIZE + 1) == FALSE) {
		_device_set_phase(DEVICE_PHASE_IDLE);

		ERR("_hdlr_rcv_file_in_data_container() Fail\n");
		g_free(blk.data);
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		return;
	}

	switch (_hutil_write_file_data(hdlr->data4_send_obj.store_id,
				hdlr->data4_send_obj.obj, temp_fpath)) {

	case MTP_ERROR_INVALID_OBJECT_INFO:
		resp = PTP_RESPONSE_NOVALID_OBJINFO;
		DBG("PTP_RESPONSE_NOVALID_OBJINFO\n");
		break;
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		DBG("PTP_RESPONSE_OK\n");
		break;
	case MTP_ERROR_STORE_FULL:
		resp = PTP_RESPONSE_STOREFULL;
		DBG("PTP_RESPONSE_STOREFULL\n");
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
		DBG("PTP_RESPONSE_GEN_ERROR\n");
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

		ERR("Parameters not supported\n");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	obj_handle = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	fmt = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
	if ((PTP_FORMATCODE_NOTUSED != fmt) &&
			(obj_handle != PTP_OBJECTHANDLE_ALL)) {
		ERR("Invalid object format\n");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_INVALID_OBJ_FMTCODE);
		return;
	}

	g_status->mtp_op_state = MTP_STATE_DATA_PROCESSING;

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

	g_status->mtp_op_state = MTP_STATE_ONSERVICE;
	_cmd_hdlr_send_response_code(hdlr, resp);
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


static void __send_partial_object(mtp_handler_t *hdlr)
{
	mtp_uint32 obj_handle;
	mtp_int32 error = 0;
	mtp_uint64 offset;
	mtp_uint16 resp;
	mtp_obj_t *obj;

	obj_handle = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	obj = _device_get_object_with_handle(obj_handle);
	if (obj == NULL) {
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		goto out;
	}

	offset = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);
	offset <<= 32;
	offset |= _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);

	if (!obj->file_path || obj->file_path[0] == '\0') {
		ERR("Object file_path was not set!\n");
		resp = PTP_RESPONSE_GEN_ERROR;
		goto out;
	}

	g_is_send_object = FALSE;
	g_is_send_partial_object = TRUE;

	if (offset == 0)
		_util_file_copy(g_mtp_mgr.ftemp_st.filepath, obj->file_path, &error);
	else
		_util_file_append(g_mtp_mgr.ftemp_st.filepath, obj->file_path, offset, &error);
	if (error) {
		ERR("Failed to copy %s to %s [%d]\n",
		    g_mtp_mgr.ftemp_st.filepath, obj->file_path, error);
		resp = PTP_RESPONSE_GEN_ERROR;
		goto out;
	}

	resp = PTP_RESPONSE_OK;
out:
	_device_set_phase(DEVICE_PHASE_DATAIN);
	_cmd_hdlr_send_response_code(hdlr, resp);
}

static void __get_partial_object(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	mtp_uint64 offset = 0;
	mtp_uint32 data_sz = 0;
	mtp_uint32 send_bytes = 0;
	data_blk_t blk = { 0 };
	mtp_uchar *ptr = NULL;
	mtp_uint16 resp = 0;
	mtp_uint64 f_size = 0;

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);

	if (hdlr->usb_cmd.code == PTP_OC_ANDROID_GETPARTIALOBJECT64) {
		offset = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);
		offset <<= 32;
		offset |= _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
		data_sz = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 3);
	} else {
		offset = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);
		data_sz = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);
	}

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
}

#ifdef MTP_SUPPORT_SET_PROTECTION
static void __set_object_protection(mtp_handler_t *hdlr)
{
	mtp_uint32 h_obj = 0;
	mtp_uint16 protcn_status = 0;
	mtp_uint16 resp = 0;

	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		ERR("Unsupported parameter\n");
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
}
#endif /* MTP_SUPPORT_SET_PROTECTION */

static void __begin_end_edit_object(mtp_handler_t *hdlr)
{
	_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
}

static void __truncate_object(mtp_handler_t *hdlr)
{
	mtp_uint64 length;
	mtp_uint32 h_obj;
	mtp_uint16 resp;

	h_obj = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0);
	length = _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2);
	length <<= 32;
	length |= _hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1);

	switch (_hutil_truncate_file(h_obj, length)) {
	case MTP_ERROR_INVALID_OBJECTHANDLE:
		resp = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		break;
	case MTP_ERROR_NONE:
		resp = PTP_RESPONSE_OK;
		break;
	case MTP_ERROR_INVALID_OBJECT_INFO:
		resp = PTP_RESPONSE_NOVALID_OBJINFO;
		break;
	default:
		resp = PTP_RESPONSE_GEN_ERROR;
	}

	_cmd_hdlr_send_response_code(hdlr, resp);
}

static void __power_down(mtp_handler_t *hdlr)
{
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {
		ERR("Unsupported Parameter\n");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_PARAM_NOTSUPPORTED);
		return;
	}

	_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_OK);
	usleep(1000);
	_cmd_hdlr_reset_cmd(hdlr);
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
		ERR("Invalid format code\n");
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_INVALIDCODEFORMAT);
		return;
	}

	_hdlr_init_data_container(&blk, hdlr->usb_cmd.code, hdlr->usb_cmd.tid);
	_hutil_get_interdep_prop_config_list_size(&num_bytes, fmt);
	ptr = _hdlr_alloc_buf_data_container(&blk, num_bytes, num_bytes);

	if (MTP_ERROR_NONE != _hutil_get_interdep_prop_config_list_data(ptr,
				num_bytes, fmt)) {
		ERR("_hutil_get_interdep_prop_config_list_data() Fail\n");
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_GEN_ERROR);
		g_free(blk.data);
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
}

void __close_session(mtp_handler_t *hdlr)
{
	if (_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 0) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 1) ||
			_hdlr_get_param_cmd_container(&(hdlr->usb_cmd), 2)) {

		ERR("PTP_RESPONSE_PARAM_NOTSUPPORTED\n");
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
		ERR("PTP_RESPONSE_SESSIONNOTOPEN\n");
	}
}
/* LCOV_EXCL_STOP */

#ifdef MTP_SUPPORT_PRINT_COMMAND
static void __print_command(cmd_container_t *usb_cmd)
{
	const mtp_char *cmdstr;
	mtp_uint32 i;

	switch (usb_cmd->code) {
	case PTP_OPCODE_GETDEVICEINFO:
		cmdstr = "GET DEVICE INFO";
		break;
	case PTP_OPCODE_OPENSESSION:
		cmdstr = "OPEN SESSION";
		break;
	case PTP_OPCODE_CLOSESESSION:
		cmdstr = "CLOSE SESSION";
		break;
	case PTP_OPCODE_GETSTORAGEIDS:
		cmdstr = "GET STORAGE IDS";
		break;
	case PTP_OPCODE_GETSTORAGEINFO:
		cmdstr = "GET STORAGE INFO";
		break;
	case PTP_OPCODE_GETOBJECTHANDLES:
		cmdstr = "GET OBJECT HANDLES";
		break;
	case PTP_OPCODE_GETOBJECTINFO:
		cmdstr = "GET OBJECT INFO";
		break;
	case PTP_OPCODE_GETOBJECT:
		cmdstr = "GET OBJECT";
		break;
	case PTP_OPCODE_DELETEOBJECT:
		cmdstr = "DELETE OBJECT";
		break;
	case PTP_OPCODE_SENDOBJECTINFO:
		cmdstr = "SEND OBJECT INFO";
		break;
	case MTP_OPCODE_SETOBJECTPROPVALUE:
		cmdstr = "SET OBJECT PROP VALUE";
		break;
	case PTP_OPCODE_SENDOBJECT:
		cmdstr = "SEND OBJECT";
		break;
	case PTP_OC_ANDROID_SENDPARTIALOBJECT:
		cmdstr = "SEND PARTIAL OBJECT (ANDROID)";
		break;
	case PTP_OPCODE_INITIATECAPTURE:
		cmdstr = "INITIATE CAPTURE";
		break;
	case PTP_OPCODE_RESETDEVICE:
		cmdstr = "RESET DEVICE";
		break;
	case PTP_OPCODE_SETOBJECTPROTECTION:
		cmdstr = "SET OBJECT PROTECTION";
		break;
	case PTP_OPCODE_POWERDOWN:
		cmdstr = "POWER DOWN";
		break;
	case PTP_OPCODE_TERMINATECAPTURE:
		cmdstr = "TERMINATE CAPTURE";
		break;
	case PTP_OC_ANDROID_GETPARTIALOBJECT64:
		cmdstr = "GET PARTIAL OBJECT (ANDROID)";
		break;
	case PTP_OPCODE_GETPARTIALOBJECT:
		cmdstr = "GET PARTIAL OBJECT";
		break;
	case PTP_OPCODE_INITIATEOPENCAPTURE:
		cmdstr = "INITIATE OPEN CAPTURE";
		break;
	case MTP_OPCODE_WMP_UNDEFINED:
		cmdstr = "WMP UNDEFINED";
		break;
	case MTP_OPCODE_GETINTERDEPPROPDESC:
		cmdstr = "GET INTERDEP PROP DESC";
		break;
	case MTP_OPCODE_GETOBJECTPROPDESC:
		cmdstr = "GET OBJECT PROP DESC";
		break;
        case PTP_OC_ANDROID_BEGINEDITOBJECT:
		cmdstr = "BEGIN EDIT OBJECT (ANDROID)";
		break;
        case PTP_OC_ANDROID_ENDEDITOBJECT:
		cmdstr = "END EDIT OBJECT (ANDROID)";
		break;
	case PTP_OC_ANDROID_TRUNCATEOBJECT:
		cmdstr = "TRUNCATE OBJECT (ANDROID)";
                break;
	default:
		cmdstr = "UNKNOWN COMMAND";
		break;
	}

	DBG("======= %s - [0x%4X] =======\n", cmdstr, usb_cmd->code);
	DBG("\ttype:\t\t0x%x\n", usb_cmd->type);
	DBG("\tlen:\t\t%u\n", usb_cmd->len);
	for (i = 0; i < usb_cmd->no_param; i++)
		DBG("\tparam[%u]:\t0x%x\n", i, usb_cmd->params[i]);
}
#endif /*MTP_SUPPORT_PRINT_COMMAND*/

/* LCOV_EXCL_START */
static void __process_commands(mtp_handler_t *hdlr, cmd_blk_t *cmd)
{
	mtp_store_t *store = NULL;

	/* Keep a local copy for this command */
	_hdlr_copy_cmd_container(cmd, &(hdlr->usb_cmd));

	if (hdlr->usb_cmd.code == PTP_OPCODE_GETDEVICEINFO) {
#ifdef MTP_SUPPORT_PRINT_COMMAND
		__print_command(&hdlr->usb_cmd);
#endif /*MTP_SUPPORT_PRINT_COMMAND*/
		__get_device_info(hdlr);
		goto DONE;
	}

	/*  Process OpenSession Command */
	if (hdlr->usb_cmd.code == PTP_OPCODE_OPENSESSION) {
#ifdef MTP_SUPPORT_PRINT_COMMAND
		__print_command(&hdlr->usb_cmd);
#endif /*MTP_SUPPORT_PRINT_COMMAND*/
		__open_session(hdlr);
		goto DONE;
	}

	/* Check if session is open or not, if not respond */
	if (hdlr->session_id == 0) {
		_cmd_hdlr_send_response_code(hdlr, PTP_RESPONSE_SESSIONNOTOPEN);
		_device_set_phase(DEVICE_PHASE_IDLE);
		goto DONE;
	}
#ifdef MTP_SUPPORT_PRINT_COMMAND
	__print_command(&hdlr->usb_cmd);
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
	case PTP_OC_ANDROID_GETPARTIALOBJECT64:
	case PTP_OPCODE_GETPARTIALOBJECT:
		__get_partial_object(hdlr);
		break;
	case PTP_OPCODE_RESETDEVICE:
		__reset_device(hdlr);
		break;
	case MTP_OPCODE_GETOBJECTPROPDESC:
		__get_object_prop_desc(hdlr);
		break;
#ifdef MTP_SUPPORT_SET_PROTECTION
	case PTP_OPCODE_SETOBJECTPROTECTION:
		__set_object_protection(hdlr);
		break;
#endif /* MTP_SUPPORT_SET_PROTECTION */
	case PTP_OPCODE_POWERDOWN:
		__power_down(hdlr);
		break;
	case MTP_OPCODE_GETINTERDEPPROPDESC:
		__get_interdep_prop_desc(hdlr);
		break;
	case PTP_CODE_VENDOR_OP1:	/* Vendor-specific operations */
		__vendor_command1(hdlr);
		break;

	case PTP_OPCODE_SENDOBJECTINFO:
	case PTP_OPCODE_SENDOBJECT:
	case MTP_OPCODE_SETOBJECTPROPVALUE:
		/* DATA_HANDLE_PHASE: Send operation will be blocked
		 * until data packet is received
		 */
		if (g_device->phase == DEVICE_PHASE_IDLE) {
			DBG("DATAOUT COMMAND PHASE!!\n");
			if (hdlr->usb_cmd.code == PTP_OPCODE_SENDOBJECT) {
				mtp_char parent_path[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

				if (g_mtp_mgr.ftemp_st.filepath) {
					_util_get_parent_path(g_mtp_mgr.ftemp_st.filepath, parent_path);
					DBG("g_mtp_mgr.ftemp_st.filepath:[%s], parent_path[%s]\n", g_mtp_mgr.ftemp_st.filepath,  parent_path);

					if ((g_strcmp0(parent_path, "/tmp")) != 0)
						g_is_send_object = TRUE;
				}

				_eh_send_event_req_to_eh_thread(EVENT_START_DATAOUT,
					0, 0, NULL);
			}

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
			g_is_send_object = FALSE;

			_eh_send_event_req_to_eh_thread(EVENT_DONE_DATAOUT,
					0, 0, NULL);
			break;
                case MTP_OPCODE_SETOBJECTPROPVALUE:
                        __set_object_prop_value(hdlr);
                        break;
		}
		break;

        case PTP_OC_ANDROID_BEGINEDITOBJECT:
        case PTP_OC_ANDROID_ENDEDITOBJECT:
                __begin_end_edit_object(hdlr);
                break;

	case PTP_OC_ANDROID_TRUNCATEOBJECT:
		__truncate_object(hdlr);
                break;

	case PTP_OC_ANDROID_SENDPARTIALOBJECT:
		if (g_device->phase == DEVICE_PHASE_IDLE) {
			_eh_send_event_req_to_eh_thread(EVENT_START_DATAOUT, 0, 0, NULL);
			_device_set_phase(DEVICE_PHASE_DATAOUT);
			return;
		}
		__send_partial_object(hdlr);
		_eh_send_event_req_to_eh_thread(EVENT_DONE_DATAOUT, 0, 0, NULL);
		break;

	default:
		_cmd_hdlr_send_response_code(hdlr,
				PTP_RESPONSE_OP_NOT_SUPPORTED);
		DBG("Unsupported COMMAND[%d]\n", hdlr->usb_cmd.code);
		break;
	}
DONE:
	if ((hdlr->last_opcode == PTP_OPCODE_SENDOBJECTINFO) &&
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

mtp_bool _cmd_hdlr_send_response(mtp_handler_t *hdlr, mtp_uint16 resp,
		mtp_uint32 num_param, mtp_uint32 *params)
{
	mtp_bool ret = FALSE;
	resp_blk_t blk = { 0 };

	if (hdlr == NULL)
		return FALSE;
	/* LCOV_EXCL_START */
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
	/* LCOV_EXCL_STOP */
	return ret;
}

mtp_bool _cmd_hdlr_send_response_code(mtp_handler_t *hdlr, mtp_uint16 resp)
{
	return _cmd_hdlr_send_response(hdlr, resp, 0, NULL);
}

/* LCOV_EXCL_START */
static void __finish_receiving_file_packets(mtp_char *data, mtp_int32 data_len)
{
	cmd_blk_t cmd = { 0 };
	mtp_uchar *cmd_buf = NULL;

	g_mtp_mgr.ftemp_st.data_count = 0;
	cmd_buf = (mtp_uchar *)data;

	if (!_hdlr_validate_cmd_container(cmd_buf, (mtp_uint32)data_len)) {
		cmd_buf = (mtp_uchar *)g_mtp_mgr.ftemp_st.cmd_buf;
		if (!_hdlr_validate_cmd_container(cmd_buf,
					g_mtp_mgr.ftemp_st.cmd_size)) {
			_device_set_phase(DEVICE_PHASE_IDLE);
			ERR("DATA PROCESS, device phase[%d], invalid Command\
					block\n", g_device->phase);
			return;
		}
	}

	g_status->mtp_op_state = MTP_STATE_ONSERVICE;
	_hdlr_copy_cmd_container_unknown_params((cmd_container_t *)cmd_buf,
			&cmd);

#ifdef __BIG_ENDIAN__
	_hdlr_conv_cmd_container_byte_order(&cmd);
#endif /* __BIG_ENDIAN__ */

	UTIL_LOCK_MUTEX(&g_cmd_inoti_mutex);
	__process_commands(&g_mtp_mgr.hdlr, &cmd);
	UTIL_UNLOCK_MUTEX(&g_cmd_inoti_mutex);

	DBG("MTP device phase[%d], processing Command is complete\n",
			g_device->phase);
}
static mtp_bool __receive_temp_file_first_packet(mtp_char *data,
		mtp_int32 data_len)
{
	temp_file_struct_t *t = &g_mtp_mgr.ftemp_st;
	mtp_int32 error = 0;
	mtp_uint32 *data_sz = &g_mtp_mgr.ftemp_st.data_size;
	mtp_char *buffer = g_mtp_mgr.ftemp_st.temp_buff;
	mtp_char buff[LEN], *ptr;
	mtp_char filename[MTP_MAX_FILENAME_SIZE] = {0};
	mtp_uint32 i, num, start, range;
	unsigned int seed;

	g_status->mtp_op_state = MTP_STATE_DATA_TRANSFER_DL;
	if (!g_is_send_object) {
		/*create a unique filename for /tmp/.mtptemp.tmp only if
		 is_send_object = 0. If is_send_object = 0 implies t->filepath
		 is set in send_object_proplist command to receive the
		 incoming file */
		start = 'A';
		range = 'Z' - 'A';

		seed = time(NULL);
		for (ptr = buff, i = 1; i < LEN; ++ptr, ++i) {
			num = rand_r(&seed) % range;
			*ptr = num+start;
		}
		*ptr = '\0';

		g_snprintf(filename, MTP_MAX_FILENAME_SIZE, "%s%s%s", "/tmp/.mtptemp", buff, ".tmp");

		g_free(t->filepath);
		t->filepath = NULL;

		t->filepath = g_strdup(filename);
	}

	DBG("t->filepath :%s\n", t->filepath);

	if (access(t->filepath, F_OK) == 0) {
		if (g_mtp_mgr.ftemp_st.fhandle != NULL) {
			_util_file_close(g_mtp_mgr.ftemp_st.fhandle);
			g_mtp_mgr.ftemp_st.fhandle = NULL;	/* initialize */
		}
		if (remove(t->filepath) < 0) {
			ERR_SECURE("remove(%s) Fail\n", t->filepath);
			__finish_receiving_file_packets(data, data_len);
			return FALSE;
		}
	}

	g_mtp_mgr.ftemp_st.fhandle = _util_file_open(t->filepath, MTP_FILE_WRITE, &error);
	if (g_mtp_mgr.ftemp_st.fhandle == NULL) {
		ERR("First file handle is invalid!!\n");
		__finish_receiving_file_packets(data, data_len);
		return FALSE;
	}
	/* consider header size */
	memcpy(&g_mtp_mgr.ftemp_st.header_buf, data, sizeof(header_container_t));

	g_mtp_mgr.ftemp_st.file_size = ((header_container_t *)data)->len -
		sizeof(header_container_t);
	*data_sz = data_len - sizeof(header_container_t);

	/* check whether last data packet */
	if (*data_sz == g_mtp_mgr.ftemp_st.file_size) {
		if (_util_file_write(g_mtp_mgr.ftemp_st.fhandle, &data[sizeof(header_container_t)],
					data_len - sizeof(header_container_t)) !=
				data_len - sizeof(header_container_t)) {
			ERR("fwrite error!\n");
		}
		*data_sz = 0;
		_util_file_close(g_mtp_mgr.ftemp_st.fhandle);
		g_mtp_mgr.ftemp_st.fhandle = NULL;	/* initialize */
		__finish_receiving_file_packets(data, data_len);
	} else {
		g_mtp_mgr.ftemp_st.data_count++;
		g_mtp_mgr.ftemp_st.size_remaining = *data_sz;

		memcpy(buffer, data + sizeof(header_container_t), *data_sz);
	}
	return TRUE;
}

static mtp_bool __receive_temp_file_next_packets(mtp_char *data,
		mtp_int32 data_len)
{
	mtp_uint32 rx_size = g_conf.read_usb_size;
	mtp_uint32 *data_sz = &g_mtp_mgr.ftemp_st.data_size;
	mtp_char *buffer = g_mtp_mgr.ftemp_st.temp_buff;

	g_mtp_mgr.ftemp_st.data_count++;
	g_mtp_mgr.ftemp_st.size_remaining += data_len;

	if ((*data_sz + (mtp_uint32)data_len) > g_conf.write_file_size) {
		/* copy oversized packet to temp file */
		if (_util_file_write(g_mtp_mgr.ftemp_st.fhandle, buffer, *data_sz) != *data_sz)
			ERR("fwrite error writeSize=[%u]\n", *data_sz);

		*data_sz = 0;
	}

	memcpy(&buffer[*data_sz], data, data_len);
	*data_sz += data_len;

	/*Complete file is recieved, so close the file*/
	if (data_len < rx_size ||
			g_mtp_mgr.ftemp_st.size_remaining == g_mtp_mgr.ftemp_st.file_size) {

		if (_util_file_write(g_mtp_mgr.ftemp_st.fhandle, buffer, *data_sz) != *data_sz)
			ERR("fwrite error write size=[%u]\n", *data_sz);

		*data_sz = 0;
		_util_file_close(g_mtp_mgr.ftemp_st.fhandle);
		g_mtp_mgr.ftemp_st.fhandle = NULL;
		__finish_receiving_file_packets(data, data_len);
	}
	return TRUE;
}

void _receive_mq_data_cb(mtp_char *buffer, mtp_int32 buf_len)
{
	cmd_blk_t cmd = { 0 };
	mtp_uint32 rx_size = g_conf.read_usb_size;

	retm_if(g_status->mtp_op_state < MTP_STATE_READY_SERVICE,
		"MTP is stopped or initializing. ignore all\n");

#ifdef MTP_SUPPORT_CONTROL_REQUEST
	/* process control request */
	switch (_transport_get_control_event()) {
	case PTP_EVENTCODE_CANCELTRANSACTION:
		DBG("PTP_EVENTCODE_CANCELTRANSACTION, just change state to IDLE\n");
		g_status->ctrl_event_code = PTP_EVENTCODE_CANCELTRANSACTION;
		_device_set_phase(DEVICE_PHASE_IDLE);
		if ((buf_len == rx_size) ||
				(buf_len < sizeof(header_container_t))) {
			DBG("Cancelling Transaction. data length [%d]\n",
					buf_len);
			g_status->ctrl_event_code = PTP_EVENTCODE_CANCELTRANSACTION;
			return;
		}

		mtp_int32 i = 0;
		cmd_container_t *tmp;
		mtp_dword len = 0;
		mtp_word type = 0;
		mtp_word code = 0;
		mtp_dword trid = 0;

		if (buffer == NULL)
			return;

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
				DBG("Found Command in remaining data\n");
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

		DBG("len[%lu], type[0x%x], code [0x%x], trid[0x%lu]\n",
				len, type, code, trid);

		if (_hdlr_validate_cmd_container((mtp_byte *)tmp, len)
				== FALSE) {
			ERR("Cancelling Transaction, invalid header, but last packet\n");
		} else {	/*another command, cancelling is finished*/
			DBG("Cancelling Transaction, Finished.\n");
		}

		g_status->ctrl_event_code = 0;
		g_status->mtp_op_state = MTP_STATE_ONSERVICE;
		if (g_mtp_mgr.ftemp_st.fhandle != NULL) {
			DBG("In Cancel Transaction fclose\n");
			_util_file_close(g_mtp_mgr.ftemp_st.fhandle);
			g_mtp_mgr.ftemp_st.fhandle = NULL;
			DBG("In Cancel Transaction, remove\n");
			if (remove(g_mtp_mgr.ftemp_st.filepath) < 0)
				ERR_SECURE("remove(%s) Fail\n", g_mtp_mgr.ftemp_st.filepath);
		} else {
			DBG("g_mtp_mgr.ftemp_st.fhandle is not valid, return\n");
		}
		break;

	case PTP_EVENTCODE_DEVICERESET:
		DBG("Implement later, PTP_EVENTCODE_DEVICERESET\n");
		_device_set_phase(DEVICE_PHASE_IDLE);
		/* do close session, just skip save reference
		 * mtp_save_object_references_mtp_device(MtpHandler.pDevice);
		 */
		g_mtp_mgr.hdlr.session_id = 0;
		g_status->ctrl_event_code = 0;
		g_status->mtp_op_state = MTP_STATE_ONSERVICE;
		break;

	default:
		break;
	}
#endif/* MTP_SUPPORT_CONTROL_REQUEST */

	/* main processing */
	if (g_device->phase == DEVICE_PHASE_IDLE) {
		if (_hdlr_validate_cmd_container((mtp_uchar *)buffer, buf_len)
				== FALSE) {
			_device_set_phase(DEVICE_PHASE_NOTREADY);
			ERR("MTP device phase NOT READY, invalid Command block\n");
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
	} else if (g_device->phase == DEVICE_PHASE_DATAOUT) {
		if (g_mtp_mgr.ftemp_st.data_count == 0)
			__receive_temp_file_first_packet(buffer, buf_len);
		else
			__receive_temp_file_next_packets(buffer, buf_len);
		return;
	} else {
		/* ignore other case */
		ERR("MTP device phase[%d], unknown device PHASE\n",
				g_device->phase);
		ERR("PhaseUnknown-> pData[0x%p], length=[%d]\n", buffer, buf_len);
		_device_set_phase(DEVICE_PHASE_IDLE);
		g_status->mtp_op_state = MTP_STATE_ONSERVICE;
	}
}

/* LCOV_EXCL_STOP */
