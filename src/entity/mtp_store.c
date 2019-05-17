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

#include <glib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "mtp_util.h"
#include "mtp_support.h"
#include "mtp_device.h"
#include "mtp_transport.h"
#include "mtp_inoti_handler.h"


extern mtp_char g_last_deleted[MTP_MAX_PATHNAME_SIZE + 1];
mtp_uint32 g_next_obj_handle = 1;


static inline mtp_bool UTIL_CHECK_LIST_NEXT(slist_iterator *iter)
{
	return (iter && iter->node_ptr) ? TRUE : FALSE;
}

static void __init_store_info(store_info_t *info)
{
	ret_if(info == NULL);

	memset(info, 0x00, sizeof(store_info_t));

	info->store_type = PTP_STORAGETYPE_FIXEDRAM;
	info->fs_type = PTP_FILESYSTEMTYPE_DCF;
	info->access = PTP_STORAGEACCESS_RWD;

	info->capacity = MTP_MAX_STORAGE;
	info->free_space = MTP_MAX_STORAGE - 1;
	info->free_space_in_objs = MTP_MAX_STORAGE_IN_OBJTS;
}

static void __init_store_info_params(store_info_t *info,
		mtp_uint64 capacity, mtp_uint16 store_type, mtp_uint16 fs_type,
		mtp_uint16 access, mtp_wchar *store_desc, mtp_wchar *label)
{
	info->store_type = store_type;
	info->fs_type = fs_type;
	info->access = access;

	info->capacity = capacity;
	info->free_space = capacity;
	info->free_space_in_objs = MTP_MAX_STORAGE_IN_OBJTS;

	_prop_copy_char_to_ptpstring(&(info->store_desc), store_desc, WCHAR_TYPE);
	_prop_copy_char_to_ptpstring(&(info->vol_label), label, WCHAR_TYPE);
}

void _entity_update_store_info_run_time(store_info_t *info,
		mtp_char *root_path)
{
	fs_info_t fs_info = { 0 };

	ret_if(info == NULL);
	ret_if(root_path == NULL);

	retm_if(!_util_get_filesystem_info(root_path, &fs_info),
		"_util_get_filesystem_info fail [%s]\n", root_path);

	info->capacity = fs_info.disk_size;
	info->free_space = fs_info.avail_size;
}

/* LCOV_EXCL_START */
mtp_uint32 _entity_get_store_info_size(store_info_t *info)
{
	mtp_uint32 size = FIXED_LENGTH_MEMBERS_MTPSTORE_SIZE;

	size += _prop_size_ptpstring(&(info->store_desc));
	size += _prop_size_ptpstring(&(info->vol_label));

	return (size);
}

mtp_uint32 _entity_pack_store_info(store_info_t *info, mtp_uchar *buf,
		mtp_uint32 buf_sz)
{
	mtp_uint32 num_bytes = 0;

	retv_if(buf == NULL, 0);

	retvm_if(buf_sz < _entity_get_store_info_size(info), 0,
		"Buffer size is less [%u]\n", buf_sz);

#ifdef __BIG_ENDIAN__
	memcpy(&(buf[num_bytes]), &(info->store_type),
			sizeof(info->StorageType));
	_util_conv_byte_order(buf, sizeof(info->store_type));
	num_bytes += sizeof(info->store_type);

	memcpy(&(buf[num_bytes]), &(info->fs_type), sizeof(info->fs_type));
	_util_conv_byte_order(buf, sizeof(info->fs_type));
	num_bytes += sizeof(info->fs_type);

	memcpy(&(buf[num_bytes]), &(info->access), sizeof(info->access));
	_util_conv_byte_order(buf, sizeof(info->access));
	num_bytes += sizeof(info->access);

	memcpy(&(buf[num_bytes]), &(info->capacity), sizeof(info->capacity));
	_util_conv_byte_order(buf, sizeof(info->capacity));
	num_bytes += sizeof(info->capacity);

	memcpy(&(buf[num_bytes]), &(info->free_space),
			sizeof(info->free_space));
	_util_conv_byte_order(buf, sizeof(info->free_space));
	num_bytes += sizeof(info->free_space);

	memcpy(&(buf[num_bytes]), &(info->free_space_in_objs),
			sizeof(info->free_space_in_objs));
	_util_conv_byte_order(buf, sizeof(info->free_space_in_objs));
	num_bytes += sizeof(info->free_space_in_objs);
#else /*__BIG_ENDIAN__*/
	memcpy(buf, &(info->store_type), sizeof(mtp_uint16) *3);
	num_bytes += sizeof(mtp_uint16) * 3;
	memcpy(&(buf[num_bytes]), &(info->capacity),
			sizeof(mtp_uint64) * 2 + sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint64) * 2 + sizeof(mtp_uint32);
#endif /*__BIG_ENDIAN__*/
	num_bytes += _prop_pack_ptpstring(&(info->store_desc), buf + num_bytes,
			_prop_size_ptpstring(&(info->store_desc)));

	num_bytes += _prop_pack_ptpstring(&(info->vol_label), buf + num_bytes,
			_prop_size_ptpstring(&(info->vol_label)));

	return num_bytes;
}

mtp_uint32 _entity_get_store_id_by_path(const mtp_char *path_name)
{
	mtp_uint32 store_id = 0;
	char ext_path[] = MTP_EXTERNAL_PATH_CHAR;

	retv_if(NULL == path_name, FALSE);

	if (!strncmp(path_name, ext_path,
				strlen(ext_path))) {
		store_id = MTP_EXTERNAL_STORE_ID;
	}

	DBG_SECURE("Path : %s, store_id : 0x%x\n", path_name, store_id);

	return store_id;
}
/* LCOV_EXCL_STOP */

mtp_bool _entity_init_mtp_store(mtp_store_t *store, mtp_uint32 store_id,
		mtp_char *store_path)
{
	mtp_char temp[MTP_SERIAL_LEN_MAX + 1] = { 0 };
	mtp_wchar wtemp[MTP_MAX_REG_STRING + 1] = { 0 };
	mtp_char serial[MTP_MAX_REG_STRING + 1] = { 0 };
	mtp_wchar wserial[MTP_MAX_REG_STRING + 1] = { 0 };

	retv_if(store == NULL, FALSE);

	store->store_id = store_id;
	store->root_path = g_strdup(store_path);

	__init_store_info(&(store->store_info));
	_entity_update_store_info_run_time(&(store->store_info),
			store->root_path);

	_util_utf16_to_utf8(temp, sizeof(temp),
			g_device->device_info.serial_no.str);
	g_snprintf(serial, sizeof(serial), "%s-%x", temp, store_id);
	_util_utf8_to_utf16(wserial, sizeof(wserial) / WCHAR_SIZ, serial);

	switch (store_id) {
	case MTP_EXTERNAL_STORE_ID:
	/* LCOV_EXCL_START */
		store->is_hidden = FALSE;
		_util_utf8_to_utf16(wtemp, sizeof(wtemp) / WCHAR_SIZ,
				MTP_STORAGE_DESC_EXT);
		__init_store_info_params(&(store->store_info),
				store->store_info.capacity,
				PTP_STORAGETYPE_FIXEDRAM,
				PTP_FILESYSTEMTYPE_HIERARCHICAL,
				PTP_STORAGEACCESS_RWD, wtemp, wserial);
		break;

	default:
		ERR("store initialization Fail\n");
		return FALSE;
	}
	/* LCOV_EXCL_STOP */
	_util_init_list(&(store->obj_list));

	return TRUE;
}

mtp_obj_t *_entity_add_file_to_store(mtp_store_t *store, mtp_uint32 h_parent,
		mtp_char *file_path, mtp_char *file_name, dir_entry_t *file_info)
{
	mtp_obj_t *obj = NULL;

	retv_if(NULL == store, NULL);

	obj = _entity_alloc_mtp_object();
	retvm_if(!obj, NULL, "Memory allocation Fail\n");

	if (_entity_init_mtp_object_params(obj, store->store_id, h_parent,
				file_path, file_name, file_info) == FALSE) {
		/* LCOV_EXCL_START */
		ERR("_entity_init_mtp_object_params Fail\n");
		g_free(obj);
		return NULL;
		/* LCOV_EXCL_STOP */
	}

	_entity_add_object_to_store(store, obj);
	return obj;
}

mtp_obj_t *_entity_add_folder_to_store(mtp_store_t *store, mtp_uint32 h_parent,
		mtp_char *file_path, mtp_char *file_name, dir_entry_t *file_info)
{
	mtp_obj_t *obj = NULL;

	retv_if(NULL == store, NULL);

	obj = _entity_alloc_mtp_object();
	retvm_if(!obj, NULL, "Memory allocation Fail\n");

	if (_entity_init_mtp_object_params(obj, store->store_id, h_parent,
				file_path, file_name, file_info) == FALSE) {
		/* LCOV_EXCL_START */
		ERR("_entity_init_mtp_object_params Fail\n");
		g_free(obj);
		return NULL;
		/* LCOV_EXCL_STOP */

	}

	_entity_add_object_to_store(store, obj);
	return obj;
}

mtp_bool _entity_add_object_to_store(mtp_store_t *store, mtp_obj_t *obj)
{
	mtp_obj_t *par_obj = NULL;

	retv_if(obj == NULL, FALSE);
	retv_if(NULL == store, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	retvm_if(!_util_add_node(&(store->obj_list), obj), FALSE,
		"Node add to list Fail\n");

	/* references */
	if (PTP_OBJECTHANDLE_ROOT != obj->obj_info->h_parent) {
		par_obj = _entity_get_object_from_store(store, obj->obj_info->h_parent);
		if (NULL != par_obj)
			_entity_add_reference_child_array(par_obj, obj->obj_handle);
	}

	return TRUE;
}

mtp_obj_t *_entity_get_object_from_store(mtp_store_t *store, mtp_uint32 handle)
{
	mtp_obj_t *obj = NULL;
	slist_iterator *iter = NULL;

	retv_if(NULL == store, NULL);

	iter = (slist_iterator *)_util_init_list_iterator(&(store->obj_list));
	retvm_if(!iter, NULL, "Iterator init Fail, Store id = [0x%x]\n", store->store_id);

	while (UTIL_CHECK_LIST_NEXT(iter) == TRUE) {
		obj = (mtp_obj_t *)_util_get_list_next(iter);

		if (obj && obj->obj_handle == handle) {
			g_free(iter);
			return obj;
		}
	}

	ERR("Object not found in the list handle [%d] in store[0x%x]\n", handle, store->store_id);
	g_free(iter);
	return NULL;
}

/* LCOV_EXCL_START */
mtp_obj_t *_entity_get_last_object_from_store(mtp_store_t *store,
		mtp_uint32 handle)
{
	mtp_obj_t *obj = NULL;
	mtp_obj_t *temp_obj = NULL;
	slist_iterator *iter = NULL;

	retv_if(NULL == store, NULL);

	iter = (slist_iterator *)_util_init_list_iterator(&(store->obj_list));
	retvm_if(!iter, NULL, "Iterator init Fail Store id = [0x%x]\n", store->store_id);

	while (UTIL_CHECK_LIST_NEXT(iter) == TRUE) {

		temp_obj = (mtp_obj_t *)_util_get_list_next(iter);
		if (temp_obj && temp_obj->obj_handle == handle)
			obj = temp_obj;
	}

	g_free(iter);
	return obj;
}

mtp_obj_t *_entity_get_object_from_store_by_path(mtp_store_t *store,
		const mtp_char *file_path)
{
	mtp_obj_t *obj = NULL;
	slist_iterator *iter = NULL;

	retv_if(NULL == store, NULL);

	iter = (slist_iterator *)_util_init_list_iterator(&(store->obj_list));
	retvm_if(!iter, NULL, "Iterator init Fail Store id = [0x%x]\n", store->store_id);

	while (UTIL_CHECK_LIST_NEXT(iter) == TRUE) {
		obj = (mtp_obj_t *)_util_get_list_next(iter);
		if (obj == NULL) {
			ERR("Object is NULL\n");
			continue;
		}

		if (!g_strcmp0(file_path, obj->file_path)) {
			g_free(iter);
			return obj;
		}
	}
	ERR_SECURE("Object [%s] not found in the list\n", file_path);
	g_free(iter);
	return NULL;
}

/*
 * _entity_get_objects_from_store
 * called in case of PTP_OBJECTHANDLE_ALL
 * fills the obj_array with objects matching the format code
 */
mtp_uint32 _entity_get_objects_from_store(mtp_store_t *store,
		mtp_uint32 obj_handle, mtp_uint32 fmt, ptp_array_t *obj_arr)
{
	mtp_obj_t *obj = NULL;
	slist_iterator *iter = NULL;

	retv_if(store == NULL, 0);
	retv_if(obj_arr == NULL, 0);

	retvm_if(obj_handle != PTP_OBJECTHANDLE_ALL, 0, 
		"Object Handle is not PTP_OBJECTHANDLE_ALL\n");

	iter = (slist_iterator *)_util_init_list_iterator(&(store->obj_list));
	retvm_if(!iter, 0, "Iterator init Fail Store id = [0x%x]\n", store->store_id);

	while (UTIL_CHECK_LIST_NEXT(iter) == TRUE) {

		obj = (mtp_obj_t *)_util_get_list_next(iter);
		if (obj == NULL) {
			ERR("Object is NULL\n");
			continue;
		}
		if ((fmt == obj->obj_info->obj_fmt) ||
				(fmt == PTP_FORMATCODE_ALL) ||
				(fmt == PTP_FORMATCODE_NOTUSED)) {
			_prop_append_ele_ptparray(obj_arr, obj->obj_handle);
		}
	}
	g_free(iter);
	return obj_arr->num_ele;
}

mtp_uint32 _entity_get_objects_from_store_till_depth(mtp_store_t *store,
		mtp_uint32 obj_handle, mtp_uint32 fmt_code, mtp_uint32 depth,
		ptp_array_t *obj_arr)
{
	retv_if(store == NULL, 0);
	retv_if(obj_arr == NULL, 0);

	if (PTP_OBJECTHANDLE_ALL == obj_handle) {
		_entity_get_objects_from_store(store, obj_handle, fmt_code,
				obj_arr);
		DBG("Number of object filled [%u]\n", obj_arr->num_ele);
		return obj_arr->num_ele;
	}

	if (PTP_OBJECTHANDLE_ROOT != obj_handle)
		_prop_append_ele_ptparray(obj_arr, obj_handle);

	if (depth > 0) {
		ptp_array_t *child_arr = NULL;
		mtp_uint32 *ptr = NULL;
		mtp_uint32 ii = 0;

		child_arr = _prop_alloc_ptparray(UINT32_TYPE);
		if (child_arr == NULL) {
			return 0;
		}
		if (child_arr->array_entry == NULL) {
			_prop_destroy_ptparray(child_arr);
			return 0;
		}

		depth--;

		_entity_get_child_handles_with_same_format(store, obj_handle,
				fmt_code, child_arr);
		ptr = child_arr->array_entry;

		for (ii = 0; ii < child_arr->num_ele; ii++) {
			_entity_get_objects_from_store_till_depth(store,
					ptr[ii], fmt_code, depth, obj_arr);
		}
		_prop_destroy_ptparray(child_arr);
	}

	DBG("Handle[%u] : array count [%u]!!\n", obj_handle,
			obj_arr->num_ele);
	return obj_arr->num_ele;
}

mtp_uint32 _entity_get_objects_from_store_by_format(mtp_store_t *store,
		mtp_uint32 format, ptp_array_t *obj_arr)
{
	mtp_obj_t *obj = NULL;
	slist_iterator *iter = NULL;

	retv_if(store == NULL, 0);
	retv_if(obj_arr == NULL, 0);

	iter = (slist_iterator *)_util_init_list_iterator(&(store->obj_list));
	retvm_if(!iter, 0, "Iterator init Fail Store id = [0x%x]\n", store->store_id);

	while (UTIL_CHECK_LIST_NEXT(iter) == TRUE) {

		obj = (mtp_obj_t *)_util_get_list_next(iter);
		if (obj == NULL || obj->obj_info == NULL)
			continue;
		if ((format == PTP_FORMATCODE_NOTUSED) ||
				(format == obj->obj_info->obj_fmt) ||
				((format == PTP_FORMATCODE_ALL) &&
				 (obj->obj_info->obj_fmt !=
				  PTP_FMT_ASSOCIATION))) {
			_prop_append_ele_ptparray(obj_arr,
					(mtp_uint32)obj->obj_handle);
		}

	}

	g_free(iter);
	return (obj_arr->num_ele);
}

mtp_uint32 _entity_get_child_handles(mtp_store_t *store, mtp_uint32 h_parent,
		ptp_array_t *child_arr)
{
	mtp_obj_t *obj = NULL;
	slist_iterator *iter = NULL;
	mtp_obj_t *parent_obj = NULL;

	retv_if(store == NULL, 0);
	retv_if(child_arr == NULL, 0);

	parent_obj = _entity_get_object_from_store(store, h_parent);

	retvm_if(!parent_obj, FALSE, "parent object is NULL\n");

	iter = (slist_iterator *)_util_init_list_iterator(&(store->obj_list));
	retvm_if(!iter, 0, "Iterator init Fail Store id = [0x%x]\n", store->store_id);

	while (UTIL_CHECK_LIST_NEXT(iter) == TRUE) {

		obj = (mtp_obj_t *)_util_get_list_next(iter);
		if (obj == NULL || obj->obj_info == NULL)
			continue;

		if (obj->obj_info->h_parent == h_parent)
			_prop_append_ele_ptparray(child_arr, obj->obj_handle);
	}

	g_free(iter);
	return child_arr->num_ele;
}

mtp_uint32 _entity_get_child_handles_with_same_format(mtp_store_t *store,
		mtp_uint32 h_parent, mtp_uint32 format, ptp_array_t *child_arr)
{
	mtp_obj_t *obj = NULL;
	slist_iterator *iter = NULL;

	retv_if(store == NULL, 0);
	retv_if(child_arr == NULL, 0);

	iter = (slist_iterator *)_util_init_list_iterator(&(store->obj_list));
	retvm_if(!iter, 0, "Iterator init Fail Store id = [0x%x]\n", store->store_id);

	while (UTIL_CHECK_LIST_NEXT(iter) == TRUE) {

		obj = (mtp_obj_t *)_util_get_list_next(iter);
		if (obj == NULL || obj->obj_info == NULL)
			continue;

		if ((obj->obj_info->h_parent == h_parent) &&
				((obj->obj_info->obj_fmt == format) ||
				 (format == PTP_FORMATCODE_NOTUSED))) {

			_prop_append_ele_ptparray(child_arr, obj->obj_handle);
		}

	}

	g_free(iter);
	return child_arr->num_ele;
}

mtp_bool _entity_remove_object_mtp_store(mtp_store_t *store, mtp_obj_t *obj,
		mtp_uint32 format, mtp_uint16 *response, mtp_bool *atleast_one,
		mtp_bool read_only)
{
	mtp_uint64 size = 0;
	mtp_bool all_del = TRUE;
	mtp_uint32 h_parent = 0;
	obj_info_t *objinfo = NULL;
	mtp_int32 ret = MTP_ERROR_NONE;

	retv_if(store == NULL, 0);

	if (TRUE == g_status->cancel_intialization ||
			TRUE == g_status->is_usb_discon) {
		ERR("Delete operation cancelled or USB is disconnected\n");
		*response = PTP_RESPONSE_PARTIAL_DELETION;
		return FALSE;
	}

	if (NULL == obj || NULL == obj->obj_info || NULL == store) {
		*response = PTP_RESPONSE_UNDEFINED;
		return FALSE;
	}

	objinfo = obj->obj_info;

	if ((objinfo->obj_fmt != format) && (format != PTP_FORMATCODE_ALL) &&
			(format != PTP_FORMATCODE_NOTUSED)) {
		*response = PTP_RESPONSE_UNDEFINED;
		return FALSE;
	}

	if ((PTP_FORMATCODE_ALL == format) &&
			(PTP_FMT_ASSOCIATION == objinfo->obj_fmt)) {

		*response = PTP_RESPONSE_UNDEFINED;
		return FALSE;
	}

#ifdef MTP_SUPPORT_SET_PROTECTION
	/* Delete readonly files/folder */
	if (!read_only)
		objinfo->protcn_status = PTP_PROTECTIONSTATUS_NOPROTECTION;
#endif /* MTP_SUPPORT_SET_PROTECTION */

	if (PTP_FMT_ASSOCIATION == objinfo->obj_fmt) {
		/* If this is an association, delete children first
		 * the reference list contain all the children handle
		 * or find a child each time;
		 */

		mtp_uint32 i = 0;
		ptp_array_t child_arr = { 0 };
		mtp_obj_t *child_obj = NULL;

		_prop_init_ptparray(&child_arr, UINT32_TYPE);
		_entity_get_child_handles(store, obj->obj_handle, &child_arr);

		if (child_arr.num_ele) {

			for (i = 0; i < child_arr.num_ele; i++) {
				mtp_uint32 *ptr32 = child_arr.array_entry;
				/*check cancel transaction*/
				if (g_status->cancel_intialization == TRUE ||
						g_status->is_usb_discon == TRUE) {
					ERR("child handle. USB is disconnected \
							format operation is cancelled.\n");
					*response =
						PTP_RESPONSE_PARTIAL_DELETION;
					_prop_deinit_ptparray(&child_arr);
					return FALSE;
				}

				child_obj = _entity_get_object_from_store(store,
						ptr32[i]);
				if (NULL == child_obj)
					continue;

				if (_entity_remove_object_mtp_store(store, child_obj,
							format, response, atleast_one,
							read_only)) {
					slist_node_t *node;
					node = _util_delete_node(&(store->obj_list),
							child_obj);
					g_free(node);
					*atleast_one = TRUE;
					_entity_dealloc_mtp_obj(child_obj);
				} else {
					all_del = FALSE;
					/*check cancel transaction*/
					if (TRUE == g_status->cancel_intialization ||
							TRUE ==	g_status->is_usb_discon) {
						ERR("USB is disconnected format\
								operation is cancelled.\n");
						*response =
							PTP_RESPONSE_PARTIAL_DELETION;
						_prop_deinit_ptparray(&child_arr);
						return FALSE;
					}
				}

			}
		} else {
			/* Non-Enumerated Folder */
			mtp_uint32 num_of_deleted_file = 0;
			mtp_uint32 num_of_file = 0;

			ret = _util_remove_dir_children_recursive(obj->file_path,
					&num_of_deleted_file, &num_of_file, read_only);
			if (MTP_ERROR_GENERAL == ret ||
					MTP_ERROR_ACCESS_DENIED == ret) {
				ERR_SECURE("directory children deletion Fail [%s]\n",
						obj->file_path);
				*response = PTP_RESPONSE_GEN_ERROR;
				if (MTP_ERROR_ACCESS_DENIED == ret)
					*response =
						PTP_RESPONSE_ACCESSDENIED;
				_prop_deinit_ptparray(&child_arr);
				return FALSE;
			}
			if (num_of_file == 0)
				DBG_SECURE("Folder[%s] is empty\n", obj->file_path);
			else if (num_of_deleted_file == 0) {
				DBG_SECURE("Folder[%s] contains only read-only files\n",
						obj->file_path);
				all_del = FALSE;
			} else if (num_of_deleted_file < num_of_file) {
				DBG("num of files[%d] is present in folder[%s]\
						and number of deleted files[%d]\n",
						num_of_file, obj->file_path,
						num_of_deleted_file);
				*atleast_one = TRUE;
				all_del = FALSE;
			}
		}
		_prop_deinit_ptparray(&child_arr);

		if (all_del) {
			g_snprintf(g_last_deleted,
					MTP_MAX_PATHNAME_SIZE + 1, "%s",
					obj->file_path);

			if (rmdir(obj->file_path) < 0) {
				memset(g_last_deleted, 0,
						MTP_MAX_PATHNAME_SIZE + 1);
				*response = PTP_RESPONSE_GEN_ERROR;
				if (EACCES == errno)
					*response =
						PTP_RESPONSE_ACCESSDENIED;
				return FALSE;
			}
		} else {
			ERR("all member in this folder is not deleted.\n");
		}
	} else {
#ifdef MTP_SUPPORT_SET_PROTECTION
		size = objinfo->file_size;
#endif /* MTP_SUPPORT_SET_PROTECTION */

		if (objinfo->protcn_status ==
				PTP_PROTECTIONSTATUS_READONLY ||
				objinfo->protcn_status ==
				MTP_PROTECTIONSTATUS_READONLY_DATA) {
			*response = PTP_RESPONSE_OBJ_WRITEPROTECTED;
			return FALSE;
		}

		/* delete the real file */
		g_snprintf(g_last_deleted, MTP_MAX_PATHNAME_SIZE + 1,
				"%s", obj->file_path);
		if (remove(obj->file_path) < 0) {
			memset(g_last_deleted, 0,
					MTP_MAX_PATHNAME_SIZE + 1);
			*response = PTP_RESPONSE_GEN_ERROR;
			if (EACCES == errno)
				*response = PTP_RESPONSE_ACCESSDENIED;
			return FALSE;
		}
		*atleast_one = TRUE;

		/* Upate store's available space */
		store->store_info.free_space += size;
	}

	if (all_del) {
		*response = PTP_RESPONSE_OK;
		/* delete references;*/
		h_parent = objinfo->h_parent;
		if (h_parent != PTP_OBJECTHANDLE_ROOT) {
			mtp_obj_t *parent_obj =
				_entity_get_object_from_store(store, h_parent);
			if (NULL != parent_obj) {
				_entity_remove_reference_child_array(parent_obj,
						obj->obj_handle);
			}
		}
	} else if (*atleast_one) {
		*response = PTP_RESPONSE_PARTIAL_DELETION;
		return FALSE;
	} else {
		*response = PTP_RESPONSE_OBJ_WRITEPROTECTED;
		return FALSE;
	}

	return TRUE;
}

mtp_uint16 _entity_delete_obj_mtp_store(mtp_store_t *store,
		mtp_uint32 obj_handle, mtp_uint32 fmt, mtp_bool read_only)
{
	mtp_uint16 response;
	mtp_obj_t *obj = NULL;
	mtp_bool all_del = TRUE;
	mtp_bool atleas_one = FALSE;

	retv_if(store == NULL, PTP_RESPONSE_GEN_ERROR);

	retvm_if(PTP_STORAGEACCESS_R == store->store_info.access,
		PTP_RESPONSE_STORE_READONLY, "Read only store\n");

	if (PTP_OBJECTHANDLE_ALL == obj_handle) {
		slist_node_t *node = NULL;
		node = store->obj_list.start;
		while (NULL != node) {
			if (TRUE == g_status->cancel_intialization ||
					TRUE == g_status->is_usb_discon) {
				ERR("USB is disconnected format\
						operation is cancelled.\n");
				response = PTP_RESPONSE_GEN_ERROR;
				return response;
			}
			/* protect from disconnect USB */
			if (NULL == store || NULL == node ||
					NULL == node->value) {
				response = PTP_RESPONSE_GEN_ERROR;
				return response;
			}

			obj = (mtp_obj_t *)node->value;
			if (_entity_remove_object_mtp_store(store, obj,
						fmt, &response, &atleas_one, read_only)) {

				slist_node_t *temp = NULL;

				node = node->link;
				temp = _util_delete_node(&(store->obj_list), obj);
				g_free(temp);
				_entity_dealloc_mtp_obj(obj);
			} else {
				node = node->link;
			}

			switch (response) {
			case PTP_RESPONSE_PARTIAL_DELETION:
				all_del = FALSE;
				break;
			case PTP_RESPONSE_OBJ_WRITEPROTECTED:
			case PTP_RESPONSE_ACCESSDENIED:
				all_del = FALSE;
				break;
			case PTP_RESPONSE_UNDEFINED:
			default:
				break;
			}

		}
	} else {
		DBG("object handle is not PTP_OBJECTHANDLE_ALL. [%u]\n",
				obj_handle);
		obj = _entity_get_object_from_store(store, obj_handle);

		if (NULL != obj) {
			if (_entity_remove_object_mtp_store(store, obj, PTP_FORMATCODE_NOTUSED,
						&response, &atleas_one, read_only)) {
				slist_node_t *temp = NULL;

				temp = _util_delete_node(&(store->obj_list), obj);
				g_free(temp);
				_entity_dealloc_mtp_obj(obj);
			} else {
				switch (response) {
				case PTP_RESPONSE_PARTIAL_DELETION:
					all_del = FALSE;
					break;
				case PTP_RESPONSE_OBJ_WRITEPROTECTED:
				case PTP_RESPONSE_ACCESSDENIED:
					all_del = FALSE;
					break;
				case PTP_RESPONSE_UNDEFINED:
				default:
					/* do nothing */
					break;
				}
			}
		}
	}

	if (all_del)
		response = PTP_RESPONSE_OK;
	else if (atleas_one)
		response = PTP_RESPONSE_PARTIAL_DELETION;

	return response;
}

mtp_uint32 _entity_get_object_tree_size(mtp_store_t *store, mtp_obj_t *obj)
{
	mtp_uint32 i = 0;
	mtp_uint64 size = 0;

	retv_if(store == NULL, 0);
	retv_if(obj == NULL, 0);

	if (obj->obj_info->obj_fmt != PTP_FMT_ASSOCIATION) {
		size = obj->obj_info->file_size;
	} else {
		ptp_array_t child_arr = { 0 };
		mtp_obj_t *child_obj = NULL;

		_prop_init_ptparray(&child_arr, UINT32_TYPE);

		_entity_get_child_handles(store, obj->obj_handle, &child_arr);

		for (i = 0; i < child_arr.num_ele; i++) {
			mtp_uint32 *ptr32 = child_arr.array_entry;
			child_obj = _entity_get_object_from_store(store,
					ptr32[i]);
			size += _entity_get_object_tree_size(store, child_obj);
		}
		_prop_deinit_ptparray(&child_arr);
	}

	return size;
}

mtp_bool _entity_check_if_B_parent_of_A(mtp_store_t *store,
		mtp_uint32 handleA, mtp_uint32 handleB)
{
	mtp_uint32 i = 0;
	mtp_obj_t *obj = NULL;
	ptp_array_t child_arr = {0};

	retv_if(store == NULL, FALSE);

	_prop_init_ptparray(&child_arr, UINT32_TYPE);
	_entity_get_child_handles(store, handleB, &child_arr);

	for (i = 0; i < child_arr.num_ele; i++) {
		mtp_uint32 *ptr32 = child_arr.array_entry;
		if (handleA == ptr32[i]) {
			_prop_deinit_ptparray(&child_arr);
			return TRUE;
		}

		obj = _entity_get_object_from_store(store, ptr32[i]);
		if (obj == NULL || obj->obj_info == NULL ||
				obj->obj_info->obj_fmt ==
				PTP_FMT_ASSOCIATION) {
			continue;
		}
		if (_entity_check_if_B_parent_of_A(store, handleA, ptr32[i])) {
			_prop_deinit_ptparray(&child_arr);
			return TRUE;
		}
	}
	_prop_deinit_ptparray(&child_arr);
	return FALSE;
}

void _entity_destroy_mtp_store(mtp_store_t *store)
{
	mtp_uint32 ii = 0;
	slist_node_t *node = NULL;
	slist_node_t *next_node = NULL;

	ret_if(store == NULL);

	for (ii = 0, next_node = store->obj_list.start;
			ii < store->obj_list.nnodes; ii++) {

		node = next_node;
		next_node = node->link;
		_entity_dealloc_mtp_obj((mtp_obj_t *)node->value);
		g_free(node);
	}

	_util_init_list(&(store->obj_list));
}
/* LCOV_EXCL_STOP */

void _entity_store_recursive_enum_folder_objects(mtp_store_t *store,
		mtp_obj_t *pobj)
{
	DIR *h_dir = 0;
	mtp_char file_name[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_bool status = FALSE;
	mtp_obj_t *obj = NULL;
	dir_entry_t entry = { { 0 }, 0 };
	mtp_char *folder_name;
	mtp_uint32 h_parent;

	ret_if(NULL == store);

	if (!pobj) {
		folder_name = store->root_path;
		h_parent = PTP_OBJECTHANDLE_ROOT;
	} else {
		folder_name = pobj->file_path;
		h_parent = pobj->obj_handle;
	}

	retm_if(folder_name == NULL || folder_name[0] != '/',
		"foldername has no root slash!!\n");

	retm_if(!_util_ifind_first(folder_name, &h_dir, &entry), "No more files\n");

	do {
		if (TRUE == g_status->is_usb_discon) {
			/* LCOV_EXCL_START */
			DBG("USB is disconnected\n");
			if (closedir(h_dir) < 0)
				ERR("Close directory Fail\n");

			return;
			/* LCOV_EXCL_STOP */
		}

		_util_get_file_name(entry.filename, file_name);
		if (0 == strlen(file_name)) {
			ERR("szFilename size is 0\n");
			goto NEXT;
		}

		if (file_name[0] == '.') {
			DBG_SECURE("Hidden file [%s]\n", entry.filename);
		} else if (entry.type == MTP_DIR_TYPE) {
			obj = _entity_add_folder_to_store(store, h_parent,
					entry.filename, file_name, &entry);

			if (NULL == obj) {
				ERR("pObject is NULL\n");
				goto NEXT;
			}

			_entity_store_recursive_enum_folder_objects(store, obj);
		} else if (entry.type == MTP_FILE_TYPE) {
			_entity_add_file_to_store(store, h_parent,
					entry.filename, file_name, &entry);
		} else {
			DBG("UNKNOWN TYPE\n");
		}
NEXT:
		status = (mtp_bool)_util_ifind_next(folder_name, h_dir,
				&entry);
	} while (status);

	if (closedir(h_dir) < 0)
		ERR("close directory fail\n");

#ifdef MTP_SUPPORT_OBJECTADDDELETE_EVENT
	_inoti_add_watch_for_fs_events(folder_name);
#endif /*MTP_SUPPORT_OBJECTADDDELETE_EVENT*/
}

/* LCOV_EXCL_START */
void _entity_copy_store_data(mtp_store_t *dst, mtp_store_t *src)
{
	dst->store_id = src->store_id;
	dst->root_path = src->root_path;
	dst->is_hidden = src->is_hidden;

	memcpy(&(dst->obj_list), &(src->obj_list), sizeof(slist_t));
	_entity_update_store_info_run_time(&(dst->store_info), dst->root_path);
	_prop_copy_ptpstring(&(dst->store_info.store_desc), &(src->store_info.store_desc));
	_prop_copy_ptpstring(&(dst->store_info.vol_label), &(src->store_info.vol_label));
}
/* LCOV_EXCL_STOP */
