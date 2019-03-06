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
#include <glib.h>
#include "mtp_support.h"
#include "mtp_util.h"
#include "ptp_datacodes.h"
#include "mtp_device.h"
#include "mtp_transport.h"
#include "ptp_container.h"

#define MTP_DEVICE_VERSION_CHAR		"V1.0"

/*
 * GLOBAL AND EXTERN VARIABLES
 */
static mtp_device_t g_device = { 0 };

/*
 * STATIC VARIABLES
 */
static mtp_store_t g_store_list[MAX_NUM_DEVICE_STORES];

static mtp_uint16 g_ops_supported[] = {
	PTP_OPCODE_GETDEVICEINFO,
	PTP_OPCODE_OPENSESSION,
	PTP_OPCODE_CLOSESESSION,
	PTP_OPCODE_GETSTORAGEIDS,
	PTP_OPCODE_GETSTORAGEINFO,
	PTP_OPCODE_GETOBJECTHANDLES,
	PTP_OPCODE_GETOBJECTINFO,
	PTP_OPCODE_GETOBJECT,
	PTP_OPCODE_DELETEOBJECT,
	PTP_OPCODE_SENDOBJECTINFO,
	PTP_OPCODE_SENDOBJECT,
	PTP_OPCODE_GETPARTIALOBJECT,
#ifndef PMP_VER
	/*PTP_OPCODE_RESETDEVICE,*/
#ifdef MTP_SUPPORT_SET_PROTECTION
	PTP_OPCODE_SETOBJECTPROTECTION,
#endif /* MTP_SUPPORT_SET_PROTECTION */
	/*PTP_OPCODE_POWERDOWN,*/
	/*PTP_CODE_VENDOR_OP1,*/
#endif /*PMP_VER*/
};

static mtp_uint16 g_event_supported[] = {
	/*PTP_EVENTCODE_CANCELTRANSACTION,*/
	PTP_EVENTCODE_OBJECTADDED,
	PTP_EVENTCODE_OBJECTREMOVED,
	/* PTP_EVENTCODE_DEVICEPROPCHANGED,
	   PTP_EVENTCODE_OBJECTINFOCHANGED,
	   PTP_EVENTCODE_DEVICEINFOCHANGED,
	   PTP_EVENTCODE_REQUESTOBJECTTRANSFER,
	   PTP_EVENTCODE_STOREFULL,
	   PTP_EVENTCODE_DEVICERESET,
	   PTP_EVENTCODE_STORAGEINFOCHANGED,
	   PTP_EVENTCODE_CAPTURECOMPLETE,
	   PTP_EVENTCODE_UNREPORTEDSTATUS,
	   PTP_EVENTCODE_VENDOREXTENTION1,
	   PTP_EVENTCODE_VENDOREXTENTION2 */
};

static mtp_uint16 g_capture_fmts[] = {
	/* PTP_FORMATCODE_IMAGE_EXIF */
	0
};

static mtp_uint16 g_object_fmts[] = {
	PTP_FMT_ASSOCIATION,
#ifndef PMP_VER
	/* PTP_FORMATCODE_IMAGE_JFIF, */
	/* MTP_FORMATCODE_WINDOWS_IMAGE_FORMAT,
	   MTP_FORMATCODE_UNDEFINED_AUDIO,
	   MTP_FORMATCODE_UNDEFINED_VIDEO,
	   MTP_FORMATCODE_UNDEFINED_COLLECTION,
	   MTP_FORMATCODE_ABSTRACT_MULTIMEDIA_ALBUM,
	   MTP_FORMATCODE_ABSTRACT_IMAGE_ALBUM,
	   MTP_FORMATCODE_ABSTRACT_VIDEO_ALBUM,
	   MTP_FORMATCODE_ABSTRACT_CONTACT_GROUP,
	   MTP_FORMATCODE_UNDEFINED_DOCUMENT,
	   MTP_FORMATCODE_ABSTRACT_DOCUMENT,
	   MTP_FORMATCODE_UNDEFINED_MESSAGE,
	   MTP_FORMATCODE_ABSTRACT_MESSAGE,
	   MTP_FORMATCODE_UNDEFINED_CALENDAR_ITEM,
	   MTP_FORMATCODE_ABSTRACT_CALENDAR_ITEM,
	   MTP_FORMATCODE_VCALENDAR1,
	   MTP_FORMATCODE_UNDEFINED_WINDOWS_EXECUTABLE,*/
#endif /*PMP_VER*/
	/* MTP_FORMATCODE_UNDEFINED_FIRMWARE */
};

/*
 * STATIC FUNCTIONS
 */
static void __init_device_info(void);
static mtp_err_t __clear_store_data(mtp_uint32 store_id);
static mtp_bool __remove_store_from_device(store_type_t store_type);
static mtp_bool __add_store_to_device(store_type_t store_type);

/*
 * FUNCTIONS
 */

/*
 * static void __device_init_device_info()
 * This function initializes MTP device information
 *@return	none
 */
static void __init_device_info(void)
{
	device_info_t *info = &(g_device.device_info);
	mtp_char model_name[MTP_MODEL_NAME_LEN_MAX + 1] = { 0 };
	mtp_char device_version[MAX_PTP_STRING_CHARS + 1] = { 0 };
	mtp_char vendor_ext_desc[MAX_PTP_STRING_CHARS + 1] = { 0 };
	mtp_char serial_no[MTP_SERIAL_LEN_MAX + 1] = { 0 };
	mtp_wchar wtemp[MAX_PTP_STRING_CHARS + 1] = { 0 };

	info->ops_supported = g_ops_supported;
	info->events_supported = g_event_supported;
	info->capture_fmts = g_capture_fmts;
	info->object_fmts = g_object_fmts;
	info->std_version = MTP_STANDARD_VERSION;
	info->vendor_extn_id = MTP_VENDOR_EXTN_ID;
	info->vendor_extn_version = MTP_VENDOR_EXTN_VERSION;
	info->functional_mode = PTP_FUNCTIONMODE_SLEEP;

	_prop_init_ptpstring(&(info->vendor_extn_desc));
	_util_get_vendor_ext_desc(vendor_ext_desc, sizeof(vendor_ext_desc));
	_util_utf8_to_utf16(wtemp, sizeof(wtemp) / WCHAR_SIZ, vendor_ext_desc);
	_prop_copy_char_to_ptpstring(&(info->vendor_extn_desc), wtemp, WCHAR_TYPE);

	_prop_init_ptpstring(&(info->manufacturer));
	_util_utf8_to_utf16(wtemp, sizeof(wtemp) / WCHAR_SIZ, MTP_MANUFACTURER_CHAR);
	_prop_copy_char_to_ptpstring(&(info->manufacturer), wtemp, WCHAR_TYPE);

	_prop_init_ptpstring(&(info->model));
	_util_get_model_name(model_name, sizeof(model_name));
	_util_utf8_to_utf16(wtemp, sizeof(wtemp) / WCHAR_SIZ, model_name);
	_prop_copy_char_to_ptpstring(&(info->model), wtemp, WCHAR_TYPE);

	_prop_init_ptpstring(&(info->device_version));
	_util_get_device_version(device_version, sizeof(device_version));
	_util_utf8_to_utf16(wtemp, sizeof(wtemp) / WCHAR_SIZ, device_version);
	_prop_copy_char_to_ptpstring(&(info->device_version), wtemp, WCHAR_TYPE);

	_prop_init_ptpstring(&(info->serial_no));
	if (FALSE == _util_get_serial(serial_no, sizeof(serial_no))) {
		ERR("_util_get_serial() Fail");
	}
	_util_utf8_to_utf16(wtemp, sizeof(wtemp) / WCHAR_SIZ, serial_no);
	_prop_copy_char_to_ptpstring(&(info->serial_no), wtemp, WCHAR_TYPE);

	return;
}

void _init_mtp_device(void)
{
	static mtp_bool already_init = FALSE;

	if (TRUE == already_init) {
		DBG("Device is already initialized");
		return;
	}

	already_init = TRUE;

	g_device.status = DEVICE_STATUSOK;
	g_device.phase = DEVICE_PHASE_IDLE;
	g_device.num_stores = 0;
	g_device.store_list = g_store_list;
	g_device.default_store_id = MTP_EXTERNAL_STORE_ID;
	g_device.default_hparent = PTP_OBJECTHANDLE_ROOT;

	_prop_build_supp_props_mp3();
	_prop_build_supp_props_wmv();
	_prop_build_supp_props_wma();
	_prop_build_supp_props_album();
	_prop_build_supp_props_default();
	__init_device_info();

	return;
}

/* LCOV_EXCL_START */
mtp_uint32 _get_device_info_size(void)
{
	mtp_uint32 size = 0;
	device_info_t *info = &(g_device.device_info);

	size += sizeof(info->std_version);
	size += sizeof(info->vendor_extn_id);
	size += sizeof(info->vendor_extn_version);
	size += _prop_size_ptpstring(&(info->vendor_extn_desc));
	size += sizeof(info->functional_mode);
	size += sizeof(mtp_uint32);	/*for number of supported ops*/
	size += sizeof(g_ops_supported);
	size += sizeof(mtp_uint32);
	size += sizeof(g_event_supported);
	size += sizeof(mtp_uint32);
	size += sizeof(mtp_uint32);
	size += sizeof(g_capture_fmts);
	size += sizeof(mtp_uint32);
	size += sizeof(g_object_fmts);
	size += _prop_size_ptpstring(&(info->manufacturer));
	size += _prop_size_ptpstring(&(info->model));
	size += _prop_size_ptpstring(&(info->device_version));
	size += _prop_size_ptpstring(&(info->serial_no));

	return size;
}

mtp_uint32 _pack_device_info(mtp_uchar *buf, mtp_uint32 buf_sz)
{
	mtp_uint16 ii = 0;
	mtp_uchar *ptr = buf;
	mtp_uint32 count = 0;
	device_info_t *info = &(g_device.device_info);

	retv_if(NULL == buf, 0);

	if (buf_sz < _get_device_info_size()) {
		ERR("buffer size [%d] is less than device_info Size\n", buf_sz);
		return 0;
	}

	memcpy(ptr, &info->std_version, sizeof(info->std_version));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(info->std_version));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(info->std_version);

	memcpy(ptr, &info->vendor_extn_id, sizeof(info->vendor_extn_id));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(info->vendor_extn_id));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(info->vendor_extn_id);

	memcpy(ptr, &info->vendor_extn_version,
			sizeof(info->vendor_extn_version));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(info->vendor_extn_version));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(info->vendor_extn_version);

	ptr += _prop_pack_ptpstring(&(info->vendor_extn_desc), ptr,
			_prop_size_ptpstring(&(info->vendor_extn_desc)));

	memcpy(ptr, &info->functional_mode, sizeof(info->functional_mode));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(info->functional_mode));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(info->functional_mode);

	count = (sizeof(g_ops_supported) / sizeof(mtp_uint16));
	memcpy(ptr, &count, sizeof(count));

#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(count));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(count);
	for (ii = 0; ii < count; ii++) {
		memcpy(ptr, (void *)&(info->ops_supported[ii]),
				sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(ptr, sizeof(mtp_uint16));
#endif /*__BIG_ENDIAN__*/
		ptr += sizeof(mtp_uint16);
	}

	count = (sizeof(g_event_supported) / sizeof(mtp_uint16));
	memcpy(ptr, &count, sizeof(count));

#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(count));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(count);
	for (ii = 0; ii < count; ii++) {
		memcpy(ptr, (void *)&(info->events_supported[ii]),
				sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(ptr, sizeof(mtp_uint16));
#endif /*__BIG_ENDIAN__*/
		ptr += sizeof(mtp_uint16);
	}

	count = 0;
	memcpy(ptr, &count, sizeof(count));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(count));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(count);

	count = (sizeof(g_capture_fmts) / sizeof(mtp_uint16));
	memcpy(ptr, &count, sizeof(count));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(count));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(count);

	for (ii = 0; ii < count; ii++) {
		memcpy(ptr, (void *)&(info->capture_fmts[ii]),
				sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(ptr, sizeof(mtp_uint16));
#endif /*__BIG_ENDIAN__*/
		ptr += sizeof(mtp_uint16);
	}

	count = (sizeof(g_object_fmts) / sizeof(mtp_uint16));
	memcpy(ptr, &count, sizeof(count));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(ptr, sizeof(count));
#endif /*__BIG_ENDIAN__*/
	ptr += sizeof(count);
	for (ii = 0; ii < count; ii++) {
		memcpy(ptr, (void *)&(info->object_fmts[ii]),
				sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(ptr, sizeof(mtp_uint16));
#endif /*__BIG_ENDIAN__*/
		ptr += sizeof(mtp_uint16);
	}

	ptr += _prop_pack_ptpstring(&(info->manufacturer), ptr,
			_prop_size_ptpstring(&(info->manufacturer)));

	ptr += _prop_pack_ptpstring(&(info->model), ptr,
			_prop_size_ptpstring(&(info->model)));

	ptr += _prop_pack_ptpstring(&(info->device_version), ptr,
			_prop_size_ptpstring(&(info->device_version)));

	ptr += _prop_pack_ptpstring(&(info->serial_no), ptr,
			_prop_size_ptpstring(&(info->serial_no)));

	return (mtp_uint32)(ptr - buf);
}

void _reset_mtp_device(void)
{
	_transport_set_control_event(0);
	_transport_set_mtp_operation_state(MTP_STATE_ONSERVICE);

	/* resets device state to ok/Ready */
	/* reset device phase to idle */

	g_device.status = DEVICE_STATUSOK;
	g_device.phase = DEVICE_PHASE_IDLE;
	return;
}
/* LCOV_EXCL_STOP */

device_prop_desc_t *_device_get_device_property(mtp_uint32 prop_code)
{
	return NULL;
}

/*
 * static mtp_bool _device_add_store(store_type_t store_type)
 * This function will add the store to the device.
 * @param[in]	store_type	Store Number
 * @return	TRUE if success, otherwise FALSE.
 */
static mtp_bool __add_store_to_device(store_type_t store_type)
{
	mtp_char *storage_path = NULL;
	mtp_uint32 store_id = 0;
	file_attr_t attrs = { 0, };
	char sto_path[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	switch (store_type) {
	case MTP_STORAGE_EXTERNAL:
		_util_get_external_path(sto_path);
		storage_path = (mtp_char *)sto_path;
		store_id = MTP_EXTERNAL_STORE_ID;
		break;
	default:
		ERR("Unknown Storage [%d]\n", store_id);
		return FALSE;
	}

	if (FALSE == _util_get_file_attrs(storage_path, &attrs)) {
		ERR("_util_get_file_attrs() Fail");
		return FALSE;
	}

	if (MTP_FILE_ATTR_INVALID == attrs.attribute ||
			!(attrs.attribute & MTP_FILE_ATTR_MODE_DIR)) {
		ERR("attribute [0x%x], dir[0x%x]\n",
				attrs.attribute, MTP_FILE_ATTR_MODE_DIR);
		ERR("Storage [%d]\n", store_id);
		return FALSE;
	}


	if (g_device.num_stores + 1 > MAX_NUM_DEVICE_STORES) {
		ERR("reached to max [%d]\n", MAX_NUM_DEVICE_STORES);
		return FALSE;
	}

	if (FALSE == _entity_init_mtp_store(&(g_device.store_list[g_device.num_stores]),
				store_id, storage_path)) {
		ERR("_entity_init_mtp_store() Fail");
		return FALSE;
	}

	g_device.num_stores++;
	g_device.is_mounted[store_type] = TRUE;

	return TRUE;
}

/*
 * static mtp_bool _device_remove_store(store_type_t store_type)
 * This function will remove the store from the device.
 * @param[in]	store_type	Store type
 * @return	TRUE if success, otherwise FALSE.
 */
/* LCOV_EXCL_START */
static mtp_bool __remove_store_from_device(store_type_t store_type)
{
	mtp_int32 ii = 0;
	mtp_device_t *device = &g_device;
	mtp_store_t *store = NULL;
	mtp_uint32 store_id = 0;

	switch (store_type) {
	case MTP_STORAGE_EXTERNAL:
		store_id = MTP_EXTERNAL_STORE_ID;
		break;
	default:
		ERR("Unknown Storage [%d]\n", store_type);
		return FALSE;
	}

	for (ii = 0; ii < device->num_stores; ii++) {
		store = &(device->store_list[ii]);
		if (store->store_id == store_id) {
			__clear_store_data(store->store_id);
			device->is_mounted[store_type] = FALSE;
			break;
		}
	}

	return TRUE;
}

mtp_bool _device_is_store_mounted(mtp_int32 store_type)
{
	if (store_type < MTP_STORAGE_EXTERNAL ||
			store_type >= MTP_STORAGE_ALL) {
		ERR("unknown storage(%d)\n", store_type);
		return FALSE;
	}

	return g_device.is_mounted[store_type];
}
/* LCOV_EXCL_STOP */

mtp_bool _device_install_storage(mtp_int32 type)
{
	mtp_int32 ext_status = TRUE;
	mtp_bool mounted;

	switch (type) {
	case MTP_ADDREM_AUTO:
		DBG(" case MTP_ADDREM_AUTO:");
		/* LCOV_EXCL_START */
		ext_status =
			_device_is_store_mounted(MTP_STORAGE_EXTERNAL);
		if (ext_status == FALSE)
			__add_store_to_device(MTP_STORAGE_EXTERNAL);
		/* LCOV_EXCL_STOP */
		break;

	case MTP_ADDREM_EXTERNAL:
		DBG(" case MTP_ADDREM_EXTERNAL:");
		/* LCOV_EXCL_START */
		mounted = _device_is_store_mounted(MTP_STORAGE_EXTERNAL);
		if (mounted == FALSE) {
			if (__add_store_to_device(MTP_STORAGE_EXTERNAL) == FALSE) {
				ERR("__add_store_to_device() Fail");
				return FALSE;
			}
		}
		/* LCOV_EXCL_STOP */
		break;

	default:
		ERR("_device_install_storage : unknown type [%d]\n", type);
		break;
	}

	return TRUE;
}

/* LCOV_EXCL_START */
mtp_bool _device_uninstall_storage(mtp_int32 type)
{
	switch (type) {
	case MTP_ADDREM_AUTO:
		if (TRUE == _device_is_store_mounted(MTP_STORAGE_EXTERNAL))
			__remove_store_from_device(MTP_STORAGE_EXTERNAL);
		break;
	case MTP_ADDREM_EXTERNAL:
		if (TRUE == _device_is_store_mounted(MTP_STORAGE_EXTERNAL))
			__remove_store_from_device(MTP_STORAGE_EXTERNAL);
		break;
	default:
		ERR("unknown mode [%d]\n", type);
		break;
	}

	return TRUE;
}

/*
 * static mtp_err_t __device_clear_store_data(mtp_uint32 store_id)
 * This function removes the storage entry.
 * @param[in]	store_id	Specifies the storage id to remove
 * @return	This function returns MTP_ERROR_NONE on success
 *		ERROR number on failure.
 */
static mtp_err_t __clear_store_data(mtp_uint32 store_id)
{
	mtp_uint16 idx = 0;
	mtp_store_t *store = NULL;
	mtp_uint32 count = g_device.num_stores;

	for (idx = 0; idx < count; idx++) {
		if (g_device.store_list[idx].store_id == store_id) {

			_entity_destroy_mtp_store(&(g_device.store_list[idx]));

			store = &(g_device.store_list[idx]);
			store->store_id = 0;
			store->is_hidden = FALSE;
			g_free(store->root_path);
			store->root_path = NULL;

			for (; idx < count - 1; idx++) {
				_entity_copy_store_data(&(g_device.store_list[idx]),
						&(g_device.store_list[idx + 1]));
			}

			g_device.store_list[count - 1].store_id = 0;
			g_device.store_list[count - 1].root_path = NULL;
			g_device.store_list[count - 1].is_hidden = FALSE;
			_util_init_list(&(g_device.store_list[count - 1].obj_list));

			/*Initialize the destroyed store*/
			g_device.num_stores--;
			return MTP_ERROR_NONE;
		}
	}
	return MTP_ERROR_GENERAL;
}
/* LCOV_EXCL_STOP */

mtp_store_t *_device_get_store(mtp_uint32 store_id)
{
	mtp_int32 ii = 0;
	mtp_store_t *store = NULL;

	for (ii = 0; ii < g_device.num_stores; ii++) {

		store = &(g_device.store_list[ii]);
		if (store->store_id == store_id)
			return store;
	}
	return NULL;
}

mtp_uint32 _device_get_store_ids(ptp_array_t *store_ids)
{
	mtp_store_t *store = NULL;
	mtp_int32 ii = 0;

	for (ii = g_device.num_stores - 1; ii >= 0; ii--) {

		store = &(g_device.store_list[ii]);
		if ((store != NULL) && (FALSE == store->is_hidden))
			_prop_append_ele_ptparray(store_ids, store->store_id);
	}
	return store_ids->num_ele;
}

mtp_uint32 _device_get_num_objects(mtp_uint32 store_id)
{
	mtp_uint32 num_objs = 0;
	mtp_store_t *store = NULL;
	mtp_uint32 ii = 0;

	for (ii = 0; ii < g_device.num_stores; ii++) {

		store = &(g_device.store_list[ii]);
		if ((store->store_id == store_id) ||
				(store_id == PTP_STORAGEID_ALL)) {
			num_objs += store->obj_list.nnodes;
		}
	}

	return num_objs;
}

/* LCOV_EXCL_START */
mtp_uint32 _device_get_num_objects_with_format(mtp_uint32 store_id,
		mtp_uint32 format)
{
	mtp_uint32 num_objs = 0;
	mtp_store_t *store = NULL;
	mtp_uint32 ii = 0;

	for (ii = 0; ii < g_device.num_stores; ii++) {

		store = &(g_device.store_list[ii]);
		if ((store->store_id != store_id) &&
				(store_id != PTP_STORAGEID_ALL)) {
			continue;
		}
		num_objs += _entity_get_num_object_with_same_format(store,
				format);
	}

	return num_objs;
}
/* LCOV_EXCL_STOP */

mtp_obj_t *_device_get_object_with_handle(mtp_uint32 obj_handle)
{
	mtp_int32 ii = 0;
	mtp_obj_t *obj = NULL;
	mtp_store_t *store = NULL;

	for (ii = 0; ii < g_device.num_stores; ii++) {

		store = &(g_device.store_list[ii]);
		obj = _entity_get_object_from_store(store, obj_handle);
		if (obj == NULL)
			continue;
		break;
	}
	return obj;
}

/* LCOV_EXCL_START */
mtp_obj_t* _device_get_object_with_path(mtp_char *full_path)
{
	mtp_int32 i = 0;
	mtp_obj_t *obj = NULL;
	mtp_store_t *store = NULL;

	retv_if(NULL == full_path, NULL);

	for (i = 0; i < g_device.num_stores; i++) {
		store = &(g_device.store_list[i]);
		obj = _entity_get_object_from_store_by_path(store, full_path);
		if (obj == NULL)
			continue;
		break;
	}
	return obj;
}
/* LCOV_EXCL_STOP */

mtp_uint16 _device_delete_object(mtp_uint32 obj_handle, mtp_uint32 fmt)
{
	mtp_uint32 ii = 0;
	mtp_uint16 response = PTP_RESPONSE_OK;
	mtp_store_t *store = NULL;
	mtp_bool all_del = TRUE;
	mtp_bool atlst_one = FALSE;

	if (PTP_OBJECTHANDLE_ALL != obj_handle) {
		store = _device_get_store_containing_obj(obj_handle);
		if (store == NULL) {
			response = PTP_RESPONSE_INVALID_OBJ_HANDLE;
		} else {
		/* LCOV_EXCL_START */
			response = _entity_delete_obj_mtp_store(store,
					obj_handle, fmt, TRUE);
		}
		return response;
	}

	for (ii = 0; ii < g_device.num_stores; ii++) {
		store = &(g_device.store_list[ii]);
		response = _entity_delete_obj_mtp_store(store, obj_handle,
				fmt, TRUE);
		switch (response) {
		case PTP_RESPONSE_STORE_READONLY:
			all_del = FALSE;
			break;
		case PTP_RESPONSE_PARTIAL_DELETION:
			all_del = FALSE;
			atlst_one = TRUE;
			break;
		case PTP_RESPONSE_OBJ_WRITEPROTECTED:
		case PTP_RESPONSE_ACCESSDENIED:
			all_del = FALSE;
			break;
		case PTP_RESPONSE_OK:
		default:
			atlst_one = TRUE;
			break;
		}
	}

	if (all_del)
		response = PTP_RESPONSE_OK;
	else if (atlst_one)
		response = PTP_RESPONSE_PARTIAL_DELETION;

	return response;
	/* LCOV_EXCL_STOP */
}

mtp_store_t *_device_get_store_containing_obj(mtp_uint32 obj_handle)
{
	mtp_uint32 ii = 0;
	mtp_obj_t *obj = NULL;
	mtp_store_t *store = NULL;

	for (ii = 0; ii < g_device.num_stores; ii++) {
		store = &(g_device.store_list[ii]);
		obj = _entity_get_object_from_store(store, obj_handle);
		if (obj != NULL)
			return store;
	}
	return NULL;
}

mtp_store_t *_device_get_store_at_index(mtp_uint32 index)
{
	if (index >= g_device.num_stores) {
		ERR("Index not valid");
		return NULL;
	}

	return &(g_device.store_list[index]);
}

/* LCOV_EXCL_START */
device_prop_desc_t *_device_get_ref_prop_list(void)
{
	return g_device.device_prop_list;
}

mtp_bool _device_get_playback_obj(mtp_uint32 *playback_obj)
{
	device_prop_desc_t *device_prop = NULL;

	device_prop = _device_get_device_property(MTP_PROPERTYCODE_PLAYBACK_OBJECT);
	if (NULL == device_prop) {
		ERR("device_prop is NULL");
		return FALSE;
	}
	memcpy(playback_obj, device_prop->current_val.integer,
			sizeof(mtp_uint32));

	return TRUE;
}

mtp_bool _device_set_playback_obj(mtp_uint32 playback_obj)
{
	device_prop_desc_t *device_prop = NULL;
	cmd_container_t event = { 0, };

	device_prop = _device_get_device_property(MTP_PROPERTYCODE_PLAYBACK_OBJECT);
	if (device_prop == NULL) {
		ERR("device_prop is NULL");
		return FALSE;
	}
	_prop_set_current_integer(device_prop, playback_obj);
	_hdlr_init_event_container(&event,
			PTP_EVENTCODE_DEVICEPROPCHANGED, 0, 0, 0);
	_hdlr_send_event_container(&event);
	return TRUE;
}
/* LCOV_EXCL_STOP */

void _device_get_serial(mtp_char *serial_no, mtp_uint32 len)
{
	ret_if(serial_no == NULL);

	_util_utf16_to_utf8(serial_no, len,
			g_device.device_info.serial_no.str);
	return;
}

/* LCOV_EXCL_START */
void _device_set_phase(device_phase_t phase)
{
	DBG("Devie phase is set [%d]\n", phase);
	g_device.phase = phase;
	return;
}

device_phase_t _device_get_phase(void)
{
	return g_device.phase;
}
/* LCOV_EXCL_STOP */

mtp_uint32 _device_get_default_store_id(void)
{
	return g_device.default_store_id;
}

mtp_uint32 _device_get_default_parent_handle(void)
{
	return g_device.default_hparent;
}

mtp_uint32 _device_get_num_stores(void)
{
	return g_device.num_stores;
}

/* LCOV_EXCL_START */
device_status_t _device_get_status(void)
{
	return g_device.status;
}
/* LCOV_EXCL_STOP */
