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
static mtp_device_t _g_device = { 0 };
mtp_device_t *g_device = &_g_device;
extern mtp_config_t g_conf;

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
        MTP_OPCODE_GETOBJECTPROPDESC,
	MTP_OPCODE_GETOBJECTPROPVALUE,
	MTP_OPCODE_SETOBJECTPROPVALUE,
#ifdef MTP_SUPPORT_SET_PROTECTION
	PTP_OPCODE_SETOBJECTPROTECTION,
#endif /* MTP_SUPPORT_SET_PROTECTION */
/* Android Random I/O Extensions Codes which is a part of MTP protocol events
 * trigered  for file access operations */
        PTP_OC_ANDROID_BEGINEDITOBJECT,
        PTP_OC_ANDROID_ENDEDITOBJECT,
        PTP_OC_ANDROID_GETPARTIALOBJECT64,
        PTP_OC_ANDROID_SENDPARTIALOBJECT,
        PTP_OC_ANDROID_TRUNCATEOBJECT,
};

static mtp_uint16 g_event_supported[] = {
	PTP_EVENTCODE_OBJECTADDED,
	PTP_EVENTCODE_OBJECTREMOVED,
};

static mtp_uint16 g_capture_fmts[] = {
	0
};

static mtp_uint16 g_object_fmts[] = {
	PTP_FMT_ASSOCIATION,
};

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
	mtp_uint32 count = g_device->num_stores;

	for (idx = 0; idx < count; idx++) {
		if (g_device->store_list[idx].store_id == store_id) {

			_entity_destroy_mtp_store(&(g_device->store_list[idx]));

			store = &(g_device->store_list[idx]);
			store->store_id = 0;
			store->is_hidden = FALSE;
			g_free(store->root_path);
			store->root_path = NULL;

			for (; idx < count - 1; idx++) {
				_entity_copy_store_data(&(g_device->store_list[idx]),
						&(g_device->store_list[idx + 1]));
			}

			g_device->store_list[count - 1].store_id = 0;
			g_device->store_list[count - 1].root_path = NULL;
			g_device->store_list[count - 1].is_hidden = FALSE;
			_util_init_list(&(g_device->store_list[count - 1].obj_list));

			/*Initialize the destroyed store*/
			g_device->num_stores--;
			return MTP_ERROR_NONE;
		}
	}
	return MTP_ERROR_GENERAL;
}

void _init_mtp_device(void)
{
	device_info_t *info = &(g_device->device_info);

	g_device->status = DEVICE_STATUSOK;
	g_device->phase = DEVICE_PHASE_IDLE;
	g_device->num_stores = 0;
	g_device->store_list = g_store_list;
	g_device->default_store_id = MTP_EXTERNAL_STORE_ID;
	g_device->default_hparent = PTP_OBJECTHANDLE_ROOT;

	_prop_build_supp_props_default();
	info->ops_supported = g_ops_supported;
	info->events_supported = g_event_supported;
	info->capture_fmts = g_capture_fmts;
	info->object_fmts = g_object_fmts;
	info->std_version = MTP_STANDARD_VERSION;
	info->vendor_extn_id = MTP_VENDOR_EXTN_ID;
	info->vendor_extn_version = MTP_VENDOR_EXTN_VERSION;
	info->functional_mode = PTP_FUNCTIONMODE_SLEEP;

	_util_ptp_pack_string(&(info->vendor_extn_desc),
			     g_conf.device_info_vendor_extension_desc->str);
	_util_ptp_pack_string(&(info->manufacturer),
			      g_conf.device_info_manufacturer->str);
	_util_ptp_pack_string(&(info->model),
			      g_conf.device_info_model->str);
	_util_ptp_pack_string(&(info->device_version),
			      g_conf.device_info_device_version->str);
	_util_ptp_pack_string(&(info->serial_no),
			     g_conf.device_info_serial_number->str);
}

/* LCOV_EXCL_START */
mtp_uint32 _get_device_info_size(void)
{
	mtp_uint32 size = 0;
	device_info_t *info = &(g_device->device_info);

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
	device_info_t *info = &(g_device->device_info);

	retv_if(NULL == buf, 0);

	retvm_if(buf_sz < _get_device_info_size(), 0,
		"buffer size [%d] is less than device_info Size\n", buf_sz);

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
	g_status->ctrl_event_code = 0;
	g_status->mtp_op_state = MTP_STATE_ONSERVICE;

	/* resets device state to ok/Ready */
	/* reset device phase to idle */

	g_device->status = DEVICE_STATUSOK;
	g_device->phase = DEVICE_PHASE_IDLE;
}
/* LCOV_EXCL_STOP */

/*
 * static mtp_bool _device_add_store(void)
 * This function will add the store to the device.
 * @return	TRUE if success, otherwise FALSE.
 */
static mtp_bool __add_store_to_device(void)
{
	mtp_char *storage_path = NULL;
	mtp_uint32 store_id = 0;
	file_attr_t attrs = { 0, };
	char sto_path[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	_util_get_external_path(sto_path);
	storage_path = (mtp_char *)sto_path;
	store_id = MTP_EXTERNAL_STORE_ID;

	retvm_if(!_util_get_file_attrs(storage_path, &attrs), FALSE,
		"_util_get_file_attrs() Fail\n");

	retvm_if(MTP_FILE_ATTR_INVALID == attrs.attribute ||
		!(attrs.attribute & MTP_FILE_ATTR_MODE_DIR), FALSE,
		"attribute [0x%x], dir[0x%x]\nStorage [%d]\n",
		attrs.attribute, MTP_FILE_ATTR_MODE_DIR, store_id);

	retvm_if(g_device->num_stores + 1 > MAX_NUM_DEVICE_STORES, FALSE,
		"reached to max [%d]\n", MAX_NUM_DEVICE_STORES);

	retvm_if(!_entity_init_mtp_store(&(g_device->store_list[g_device->num_stores]),
		store_id, storage_path), FALSE,
		"_entity_init_mtp_store() Fail\n");

	g_device->num_stores++;
	g_device->is_mounted[0] = TRUE;

	return TRUE;
}

/*
 * static mtp_bool _device_remove_store(void)
 * This function will remove the store from the device.
 * @return	TRUE if success, otherwise FALSE.
 */
/* LCOV_EXCL_START */
static mtp_bool __remove_store_from_device(void)
{
	mtp_store_t *store = NULL;

	store = &(g_device->store_list[0]);
	__clear_store_data(store->store_id);
	g_device->is_mounted[0] = FALSE;

	return TRUE;
}

/* LCOV_EXCL_STOP */

mtp_bool _device_install_storage(void)
{
	DBG("ADD Storage\n");
	/* LCOV_EXCL_START */
	if (g_device->is_mounted[0] == FALSE)
		__add_store_to_device();
	/* LCOV_EXCL_STOP */

	return TRUE;
}

/* LCOV_EXCL_START */
mtp_bool _device_uninstall_storage(void)
{
	if (TRUE == g_device->is_mounted[0])
		__remove_store_from_device();

	return TRUE;
}

/* LCOV_EXCL_STOP */

mtp_store_t *_device_get_store(mtp_uint32 store_id)
{
	mtp_int32 ii = 0;
	mtp_store_t *store = NULL;

	for (ii = 0; ii < g_device->num_stores; ii++) {

		store = &(g_device->store_list[ii]);
		if (store->store_id == store_id)
			return store;
	}
	return NULL;
}

mtp_uint32 _device_get_store_ids(ptp_array_t *store_ids)
{
	mtp_store_t *store = NULL;
	mtp_int32 ii = 0;

	for (ii = g_device->num_stores - 1; ii >= 0; ii--) {

		store = &(g_device->store_list[ii]);
		if ((store != NULL) && (FALSE == store->is_hidden))
			_prop_append_ele_ptparray(store_ids, store->store_id);
	}
	return store_ids->num_ele;
}

mtp_obj_t *_device_get_object_with_handle(mtp_uint32 obj_handle)
{
	mtp_int32 ii = 0;
	mtp_obj_t *obj = NULL;
	mtp_store_t *store = NULL;

	for (ii = 0; ii < g_device->num_stores; ii++) {

		store = &(g_device->store_list[ii]);
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

	for (i = 0; i < g_device->num_stores; i++) {
		store = &(g_device->store_list[i]);
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

	for (ii = 0; ii < g_device->num_stores; ii++) {
		store = &(g_device->store_list[ii]);
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

	for (ii = 0; ii < g_device->num_stores; ii++) {
		store = &(g_device->store_list[ii]);
		obj = _entity_get_object_from_store(store, obj_handle);
		if (obj != NULL)
			return store;
	}
	return NULL;
}

mtp_store_t *_device_get_store_at_index(mtp_uint32 index)
{
	retvm_if(index >= g_device->num_stores, NULL, "Index not valid\n");

	return &(g_device->store_list[index]);
}
