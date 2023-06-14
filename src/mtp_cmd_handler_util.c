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
#include <glib/gprintf.h>
#include "mtp_cmd_handler.h"
#include "mtp_cmd_handler_util.h"
#include "mtp_support.h"
#include "mtp_transport.h"

/*
 * GLOBAL AND EXTERN VARIABLES
 */
mtp_bool g_is_full_enum = FALSE;
extern mtp_mgr_t g_mtp_mgr;
extern obj_interdep_proplist_t interdep_proplist;
extern mtp_char g_last_created_dir[MTP_MAX_PATHNAME_SIZE + 1];
extern mtp_char g_last_moved[MTP_MAX_PATHNAME_SIZE + 1];
extern mtp_char g_last_copied[MTP_MAX_PATHNAME_SIZE + 1];
extern mtp_char g_last_deleted[MTP_MAX_PATHNAME_SIZE + 1];
extern mtp_uint32 g_next_obj_handle;
extern phone_state_t *g_ph_status;

/*
 * STATIC VARIABLES
 */
static mtp_mgr_t *g_mgr = &g_mtp_mgr;

/*
 * FUNCTIONS
 */

/*
 * This function gets the storage entry.
 * @param[in]	store_id	Specifies the storage id to get.
 * @param[in]	info		Points the storage info structure
 * @return	This function returns MTP_ERROR_NONE on success or
 *		ERROR_No on failure.
 */
mtp_err_t _hutil_get_storage_entry(mtp_uint32 store_id, store_info_t *info)
{
	mtp_store_t *store = NULL;

	store = _device_get_store(store_id);
	retvm_if(!store, MTP_ERROR_GENERAL, "Not able to retrieve store\n");

	/* LCOV_EXCL_START */
	_entity_update_store_info_run_time(&(store->store_info),
			store->root_path);

	_prop_copy_ptpstring(&(info->store_desc), &(store->store_info.store_desc));
	_prop_copy_ptpstring(&(info->vol_label), &(store->store_info.vol_label));
	info->access = store->store_info.access;
	info->fs_type = store->store_info.fs_type;
	info->free_space = store->store_info.free_space;
	info->capacity = store->store_info.capacity;
	info->store_type = store->store_info.store_type;
	info->free_space_in_objs = store->store_info.free_space_in_objs;
	return MTP_ERROR_NONE;
	/* LCOV_EXCL_STOP */
}

mtp_err_t _hutil_get_storage_ids(ptp_array_t *store_ids)
{
	mtp_uint32 num_elem = 0;
	mtp_uint32 num_stores = 0;

	num_elem = _device_get_store_ids(store_ids);
	num_stores = g_device->num_stores;
	if (num_elem == num_stores)
		return MTP_ERROR_NONE;

	ERR("get storage id Fail. num_elem[%d], num_stores[%d]\n",
			num_elem, num_stores);
	return MTP_ERROR_GENERAL;
}

/*
 * This function adds the object entry.
 * @param[in]	obj_info	Points the objectinfo.
 * @param[in]	file_name	Points the file name of the object.
 * @param[out]	new_obj		Points to the new object added.
 * @return	This function returns MTP_ERROR_NONE on success or
 *		appropriate error on failure.
 */
mtp_err_t _hutil_add_object_entry(obj_info_t *obj_info, mtp_char *file_name,
		mtp_obj_t **new_obj)
{
	mtp_obj_t *obj = NULL;
	mtp_obj_t *par_obj = NULL;
	mtp_store_t *store = NULL;
	mtp_wchar temp_wfname[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char utf8_temp[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char new_f_path[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_int32 i = 0;
	mtp_int32 error;
	mtp_bool is_made_by_mtp = FALSE;
	mtp_wchar w_file_name[MTP_MAX_FILENAME_SIZE + 1] = { 0 };

	if (obj_info == NULL)
		return MTP_ERROR_INVALID_PARAM;

	if (obj_info->h_parent != PTP_OBJECTHANDLE_ROOT) {
		/* LCOV_EXCL_START */
		par_obj = _device_get_object_with_handle(obj_info->h_parent);
		if (par_obj == NULL) {
			ERR("parent is not existed.\n");
			_entity_dealloc_obj_info(obj_info);
			return MTP_ERROR_INVALID_OBJECTHANDLE;
		}

		if (par_obj->obj_info->obj_fmt != PTP_FMT_ASSOCIATION) {
			ERR("parent handle is not association format[0x%x]\
					handle[%d]\n", par_obj->obj_info->obj_fmt,
					par_obj->obj_info->h_parent);
			_entity_dealloc_obj_info(obj_info);
			return MTP_ERROR_INVALID_PARENT;
		}
		/* LCOV_EXCL_STOP */
	}

	store = _device_get_store(obj_info->store_id);
	if (store == NULL) {
		ERR("store is null\n");
		_entity_dealloc_obj_info(obj_info);
		return MTP_ERROR_GENERAL;
	}

	if (store->store_info.free_space < obj_info->file_size) {
		ERR("free space is not enough [%llu] bytes, object size[%llu]\n",
				store->store_info.free_space, obj_info->file_size);
		_entity_dealloc_obj_info(obj_info);
		return MTP_ERROR_STORE_FULL;
	}

	obj = _entity_alloc_mtp_object();
	if (obj == NULL) {
		ERR("allocation memory Fail\n");
		_entity_dealloc_obj_info(obj_info);
		return MTP_ERROR_GENERAL;
	}

	memset(obj, 0, sizeof(mtp_obj_t));
	obj->child_array.type = UINT32_TYPE;
	obj->obj_handle = g_next_obj_handle++;
	obj->obj_info = obj_info;

	/* For PC->MMC read-only file/folder transfer
	 * and for PC->Phone read-only folder transfer
	 * store PTP_PROTECTIONSTATUS_NOPROTECTION in
	 * the field obj_info->ProtectionStatus
	 */
	if (MTP_EXTERNAL_STORE_ID == obj_info->store_id ||
			obj_info->obj_fmt == PTP_FMT_ASSOCIATION) {
		obj_info->protcn_status = PTP_PROTECTIONSTATUS_NOPROTECTION;
	}

	if (file_name == NULL) {
		_entity_dealloc_obj_info(obj_info);
		_entity_dealloc_mtp_obj(obj);
		return MTP_ERROR_INVALID_PARAM;
	}
	if (strlen(file_name) == 0) {
		/* Generate a filename in 8.3 format for this object */
		g_snprintf(utf8_temp, MTP_MAX_PATHNAME_SIZE + 1,
				"Tmp_%04u.dat", obj->obj_handle);
		_util_utf8_to_utf16(temp_wfname,
				sizeof(temp_wfname) / WCHAR_SIZ, utf8_temp);
	} else {
		_util_utf8_to_utf16(temp_wfname,
				sizeof(temp_wfname) / WCHAR_SIZ, file_name);
	}

	/* Does this path/filename already exist ? */
	for (i = 0; ; i++) {
		mtp_bool file_exist = FALSE;
		mtp_uint32 path_len = 0;

		/* Get/Generate the Full Path for the new Object; */
		if (obj_info->h_parent == PTP_OBJECTHANDLE_ROOT) {
			_util_utf16_to_utf8(utf8_temp, sizeof(utf8_temp),
					temp_wfname);

			if (_util_create_path(new_f_path, sizeof(new_f_path),
						store->root_path, utf8_temp) == FALSE) {
				/* LCOV_EXCL_START */
				_entity_dealloc_mtp_obj(obj);
				return MTP_ERROR_GENERAL;
			}
		} else {
			_util_utf16_to_utf8(utf8_temp, sizeof(utf8_temp),
					temp_wfname);
			if (_util_create_path(new_f_path, sizeof(new_f_path),
						par_obj->file_path, utf8_temp) == FALSE) {
				_entity_dealloc_mtp_obj(obj);
				return MTP_ERROR_GENERAL;
			}
		}
		/* LCOV_EXCL_STOP */
		/*
		 * g_mgr->ftemp_st.filepath was allocated g_strdup("/tmp/.mtptemp.tmp");
		 * So if we need to change the path we need to allocate sufficient memory
		 * otherwise memory corruption may happen
		 */

		path_len = strlen(store->root_path) + strlen(MTP_TEMP_FILE) + 2;
		g_mgr->ftemp_st.filepath = (mtp_char*)g_malloc0(path_len);
		if (g_mgr->ftemp_st.filepath == NULL) {
			ERR("g_realloc Fail\n");
			_entity_dealloc_mtp_obj(obj);
			return MTP_ERROR_GENERAL;
		}

		if (_util_create_path(g_mgr->ftemp_st.filepath, path_len,
					store->root_path, MTP_TEMP_FILE) == FALSE) {
			ERR("Tempfile fullPath is too long\n");
			_entity_dealloc_mtp_obj(obj);
			return MTP_ERROR_GENERAL;
		}

		DBG_SECURE("Temp file path [%s]\n", g_mgr->ftemp_st.filepath);

		if (file_exist == FALSE) {
			DBG_SECURE("Found a unique file name for the incoming object\
					[0x%p]\n", temp_wfname);
			break;
		}

	/* LCOV_EXCL_START */
		if (new_obj == NULL) {
			_entity_dealloc_obj_info(obj_info);
			_entity_dealloc_mtp_obj(obj);
			return MTP_ERROR_INVALID_PARAM;
		}
		if (obj_info->obj_fmt == PTP_FMT_ASSOCIATION ||
				is_made_by_mtp) {
			*new_obj = _device_get_object_with_path(new_f_path);
			if (*new_obj) {
				_entity_dealloc_mtp_obj(obj);
				return MTP_ERROR_NONE;
			} else {
				DBG("+ not found object. [%s]\n", new_f_path);
				break;
			}
		}

		/* Rename the Filename for this object */
		memset(temp_wfname, 0, (MTP_MAX_PATHNAME_SIZE + 1) * 2);
		_util_utf8_to_utf16(w_file_name, sizeof(w_file_name) / WCHAR_SIZ,
				file_name);
		_util_wchar_swprintf(temp_wfname,
				MTP_MAX_PATHNAME_SIZE + 1, "COPY%d_%s", i, w_file_name);
	}
	/* LCOV_EXCL_STOP */
	/* Save the full path to this object */
	_entity_set_object_file_path(obj, new_f_path, CHAR_TYPE);

	/*
	 * Is this an association (or folder)?
	 * Associations are fully qualified by the ObjectInfo Dataset.
	 */

	if (obj_info->obj_fmt == PTP_FMT_ASSOCIATION || is_made_by_mtp) {
		/* Create the new object */
		/* LCOV_EXCL_START */
		DBG("[normal] create association file/folder[%s][0x%x]\n",
				new_f_path, obj_info->association_type);

		if ((obj_info->association_type !=
					PTP_ASSOCIATIONTYPE_UNDEFINED &&
					obj_info->association_type !=
					PTP_ASSOCIATIONTYPE_FOLDER) ||
				is_made_by_mtp) {
			FILE* h_abs_file = NULL;
			h_abs_file = _util_file_open(new_f_path,
					MTP_FILE_WRITE, &error);
			if (h_abs_file == NULL) {
				ERR("create file fail!!\n");
				_entity_dealloc_mtp_obj(obj);
				return MTP_ERROR_GENERAL;
			}
			_util_file_close(h_abs_file);
		} else {
			g_snprintf(g_last_created_dir,
					MTP_MAX_PATHNAME_SIZE + 1, "%s", new_f_path);
			if (_util_dir_create(new_f_path, &error) == FALSE) {
				/* We failed to create the folder */
				ERR("create directory Fail\n");
				memset(g_last_created_dir, 0,
						sizeof(g_last_created_dir));
				_entity_dealloc_mtp_obj(obj);
				return MTP_ERROR_GENERAL;
			}
		}

		_entity_set_object_file_path(obj, new_f_path, CHAR_TYPE);
		if (_entity_add_object_to_store(store, obj) == FALSE) {
			ERR("_entity_add_object_to_store Fail\n");
			_entity_dealloc_mtp_obj(obj);
			return MTP_ERROR_STORE_FULL;
		}
		/* LCOV_EXCL_STOP */
	} else {
		/* Reserve space for the object: Object itself, and probably
		 * some Filesystem-specific overhead
		 */
		store->store_info.free_space -= obj_info->file_size;
	}

	*new_obj = obj;

	return MTP_ERROR_NONE;
}

/*
 * This function removes the object entry.
 * @param[in]	obj_handle	Specifies the object to remove.
 * @param[in]	format		Specifies the format code.
 * @return	This function returns MTP_ERROR_NONE on success or
 *		appropriate error on failure.
 */
mtp_err_t _hutil_remove_object_entry(mtp_uint32 obj_handle, mtp_uint32 format)
{
	mtp_err_t resp = MTP_ERROR_GENERAL;
	mtp_uint16 ret = 0;

#ifdef MTP_SUPPORT_SET_PROTECTION
	/* this will check to see if the protection is set */
	mtp_obj_t *obj = NULL;
	if (obj_handle != 0xFFFFFFFF) {
		obj = _device_get_object_with_handle(obj_handle);
		if (obj != NULL) {
			retvm_if((obj->obj_info->protcn_status ==
				MTP_PROTECTIONSTATUS_READONLY_DATA) ||
				(obj->obj_info->protcn_status ==
				 PTP_PROTECTIONSTATUS_READONLY),
					MTP_ERROR_OBJECT_WRITE_PROTECTED,
					"Protection status is [0x%x]\n", obj->obj_info->protcn_status);
		}
	}
#endif /*MTP_SUPPORT_SET_PROTECTION*/

	ret = _device_delete_object(obj_handle, format);
	switch (ret) {
	case PTP_RESPONSE_OK:
		resp = MTP_ERROR_NONE;
		break;
	case PTP_RESPONSE_STORE_READONLY:
		resp = MTP_ERROR_STORE_READ_ONLY;
		break;
	case PTP_RESPONSE_PARTIAL_DELETION:
		resp = MTP_ERROR_PARTIAL_DELETION;
		break;
	case PTP_RESPONSE_OBJ_WRITEPROTECTED:
		resp = MTP_ERROR_OBJECT_WRITE_PROTECTED;
		break;
	case PTP_RESPONSE_ACCESSDENIED:
		resp = MTP_ERROR_ACCESS_DENIED;
		break;
	case PTP_RESPONSE_INVALID_OBJ_HANDLE:
		resp = MTP_ERROR_INVALID_OBJECTHANDLE;
		break;
	default:
		break;
	}

	return resp;
}

/*
 * This function gets the object entry.
 * @param[in]	obj_handle	Specifies the object to get.
 * @param[out]	obj_ptr		Points to object found.
 * @return	This function returns MTP_ERROR_NONE on success
 *		or appropriate error on failure.
 */
mtp_err_t _hutil_get_object_entry(mtp_uint32 obj_handle, mtp_obj_t **obj_ptr)
{
	mtp_obj_t *obj = NULL;

	obj = _device_get_object_with_handle(obj_handle);
	if (NULL == obj || NULL == obj->obj_info)
		return MTP_ERROR_GENERAL;

	if (obj_ptr == NULL)
		return MTP_ERROR_INVALID_PARAM;

	*obj_ptr = obj;
	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_copy_object_entries(mtp_uint32 dst_store_id,
		mtp_uint32 src_store_id, mtp_uint32 h_parent, mtp_uint32 obj_handle,
		mtp_uint32 *new_hobj, mtp_bool keep_handle)
{
	mtp_store_t *dst = NULL;
	mtp_store_t *src = NULL;
	mtp_obj_t *obj = NULL;
	mtp_obj_t *par_obj = NULL;
	mtp_obj_t *new_obj = NULL;
	mtp_int32 error = 0;
	mtp_char fpath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char utf8_temp[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_err_t ret = MTP_ERROR_NONE;
	mtp_uint32 num_of_deleted_file = 0;
	mtp_uint32 num_of_file = 0;
	ptp_array_t child_arr = { 0 };
	mtp_obj_t *child_obj = NULL;
	mtp_uint32 ii = 0;

	src = _device_get_store(src_store_id);
	dst = _device_get_store(dst_store_id);
	obj = _device_get_object_with_handle(obj_handle);

	retvm_if(!src || !dst || !obj, MTP_ERROR_GENERAL, "NULL!!\n");

	/* LCOV_EXCL_START */
	if (h_parent > 0)	{
		if (keep_handle) {
			/* two sam handle can occurs in folder copy case
			 * (find last appended object in target storage)
			 */
			par_obj = _entity_get_last_object_from_store(dst,
					h_parent);
		} else
			par_obj = _device_get_object_with_handle(h_parent);
	} else {
		par_obj = NULL;
	}

	_util_get_file_name(obj->file_path, utf8_temp);

	if (par_obj == NULL) {
		/* Parent is the root of this store */
		retvm_if(!_util_create_path(fpath, sizeof(fpath), dst->root_path,
				utf8_temp), MTP_ERROR_GENERAL, "new path is too LONG\n");
	} else {
		retvm_if(!_util_create_path(fpath, sizeof(fpath), par_obj->file_path,
				utf8_temp), MTP_ERROR_GENERAL, "New path is too LONG!!\n");
	}

	retvm_if(!strcasecmp(fpath, obj->file_path), MTP_ERROR_GENERAL,
		"Identical path of source and destination[%s]\n", fpath);

	new_obj = _entity_alloc_mtp_object();
	retvm_if(!new_obj, MTP_ERROR_GENERAL, "_entity_alloc_mtp_object Fail\n");

	memset(new_obj, 0, sizeof(mtp_obj_t));
	new_obj->child_array.type = UINT32_TYPE;

	_entity_copy_mtp_object(new_obj, obj);
	if (new_obj->obj_info == NULL) {
		_entity_dealloc_mtp_obj(new_obj);
		return MTP_ERROR_GENERAL;
	}

	new_obj->obj_info->store_id = dst_store_id;
	_entity_set_object_file_path(new_obj, fpath, CHAR_TYPE);

	if (MTP_EXTERNAL_STORE_ID == new_obj->obj_info->store_id) {
		new_obj->obj_info->protcn_status =
			PTP_PROTECTIONSTATUS_NOPROTECTION;
	}

	new_obj->obj_handle = (keep_handle) ? obj->obj_handle :
		g_next_obj_handle++;
	new_obj->obj_info->h_parent = (par_obj == NULL) ? PTP_OBJECTHANDLE_ROOT :
		par_obj->obj_handle;

	if (new_obj->obj_info->obj_fmt != PTP_FMT_ASSOCIATION) {
		DBG("Non-association type!!\n");
		g_snprintf(g_last_copied, MTP_MAX_PATHNAME_SIZE + 1, "%s",
				new_obj->file_path);
		if (_util_file_copy(obj->file_path, new_obj->file_path,
					&error) == FALSE) {
			memset(g_last_copied, 0, MTP_MAX_PATHNAME_SIZE + 1);
			ERR("Copy file Fail\n");
			_entity_dealloc_mtp_obj(new_obj);
			if (EACCES == error)
				return MTP_ERROR_ACCESS_DENIED;
			else if (ENOSPC == error)
				return MTP_ERROR_STORE_FULL;
			return MTP_ERROR_GENERAL;

		}
#ifdef MTP_SUPPORT_SET_PROTECTION
		file_attr_t attr = { 0 };
		attr.attribute = MTP_FILE_ATTR_MODE_REG;
		if (PTP_PROTECTIONSTATUS_READONLY ==
				new_obj->obj_info->protcn_status) {
			if (FALSE == _util_set_file_attrs(new_obj->file_path,
						attr.attribute | MTP_FILE_ATTR_MODE_READ_ONLY))
				return MTP_ERROR_GENERAL;
		}
#endif /* MTP_SUPPORT_SET_PROTECTION */

		/* Update the storeinfo after successfully copy of the object */
		dst->store_info.free_space -= obj->obj_info->file_size;

		/* move case */
		if (keep_handle) {
			_entity_add_object_to_store(dst, new_obj);
			/* Reference Copy */
			_prop_copy_ptparray(&(new_obj->child_array),
					&(obj->child_array));
		} else {
			_entity_add_object_to_store(dst, new_obj);
		}

		*new_hobj = new_obj->obj_handle;
		return MTP_ERROR_NONE;
	}

	DBG("Association type!!\n");
	if (access(new_obj->file_path, F_OK) == 0) {
		if (TRUE == keep_handle) {
			/*generate unique_path*/
			mtp_char unique_fpath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
			if (FALSE == _util_get_unique_dir_path(new_obj->file_path,
						unique_fpath, sizeof(unique_fpath))) {
				_entity_dealloc_mtp_obj(new_obj);
				return MTP_ERROR_GENERAL;
			}
			_entity_set_object_file_path(new_obj, unique_fpath, CHAR_TYPE);
			g_snprintf(g_last_created_dir, MTP_MAX_PATHNAME_SIZE + 1,
					"%s", new_obj->file_path);
			if (_util_dir_create(new_obj->file_path, &error) == FALSE) {
				memset(g_last_created_dir, 0,
						MTP_MAX_PATHNAME_SIZE + 1);
				ERR("Creating folder Fail!!\n");
				_entity_dealloc_mtp_obj(new_obj);
				if (ENOSPC == error)
					return MTP_ERROR_STORE_FULL;
				return MTP_ERROR_GENERAL;
			}

			/* Add the new object to this store's object list */
			_entity_add_object_to_store(dst, new_obj);
		} else {
			DBG("Already existed association type!!\n");
			_entity_dealloc_mtp_obj(new_obj);
			new_obj = _entity_get_object_from_store_by_path(dst, fpath);
			retvm_if(!new_obj, MTP_ERROR_GENERAL, "But object is not registered!!\n");
		}
	} else {
		g_snprintf(g_last_created_dir, MTP_MAX_PATHNAME_SIZE + 1,
				"%s", new_obj->file_path);
		if (_util_dir_create(new_obj->file_path, &error) == FALSE) {
			memset(g_last_created_dir, 0,
					MTP_MAX_PATHNAME_SIZE + 1);
			ERR("Creating folder Fail!!\n");
			_entity_dealloc_mtp_obj(new_obj);
			if (ENOSPC == error)
				return MTP_ERROR_STORE_FULL;
			return MTP_ERROR_GENERAL;
		}

		/* Add the new object to this store's object list */
		_entity_add_object_to_store(dst, new_obj);
	}

	/* Child addition to data structures is not required in Copy
	 * case as on demand enumeration is supported
	 */
	if (FALSE == keep_handle) {
		if (FALSE == _util_copy_dir_children_recursive(obj->file_path,
					new_obj->file_path, dst_store_id, &error)) {
			ERR_SECURE("Recursive copy Fail  [%s]->[%s]\n",
					obj->file_path, new_obj->file_path);
			ret = MTP_ERROR_GENERAL;
			if (EACCES == error)
				ret = MTP_ERROR_ACCESS_DENIED;
			else if (ENOSPC == error)
				ret = MTP_ERROR_STORE_FULL;
			if (_util_remove_dir_children_recursive(new_obj->file_path,
						&num_of_deleted_file, &num_of_file,
						FALSE) == MTP_ERROR_NONE) {
				g_snprintf(g_last_deleted,
						MTP_MAX_PATHNAME_SIZE + 1,
						"%s", new_obj->file_path);
				if (rmdir(new_obj->file_path) < 0) {
					memset(g_last_deleted, 0,
							MTP_MAX_PATHNAME_SIZE + 1);
				}
			}
			return ret;
		}

		*new_hobj = new_obj->obj_handle;

		return MTP_ERROR_NONE;
	}


	/* Since this is an association, copy its children as well*/
	_prop_init_ptparray(&child_arr, UINT32_TYPE);
	_entity_get_child_handles(src, obj->obj_handle, &child_arr);

	for (ii = 0; ii < child_arr.num_ele; ii++)	{
		mtp_uint32 *ptr32 = child_arr.array_entry;

		child_obj = _entity_get_object_from_store(src, ptr32[ii]);
		if (child_obj == NULL)
			continue;

		ret = _hutil_copy_object_entries(dst_store_id, src_store_id,
				new_obj->obj_handle, child_obj->obj_handle,
				new_hobj, keep_handle);
		if (ret != MTP_ERROR_NONE) {
			ERR("Copy file Fail\n");
			_prop_deinit_ptparray(&child_arr);
			return ret;
		}
	}

	/* Recursive copy is required when folder is not enumerated so it may
	 * return 0 child handles
	 */
	if (!((child_arr.num_ele > 0) ||
				_util_copy_dir_children_recursive(obj->file_path,
					new_obj->file_path, dst_store_id, &error))) {
		ERR_SECURE("Recursive copy Fail [%d], [%s]->[%s]\n",
				child_arr.num_ele,
				obj->file_path, new_obj->file_path);
		_prop_deinit_ptparray(&child_arr);
		if (_util_remove_dir_children_recursive(new_obj->file_path,
					&num_of_deleted_file, &num_of_file, FALSE) ==
				MTP_ERROR_NONE) {
			g_snprintf(g_last_deleted, MTP_MAX_PATHNAME_SIZE + 1,
					"%s", new_obj->file_path);
			if (rmdir(new_obj->file_path) < 0) {
				memset(g_last_deleted, 0,
						MTP_MAX_PATHNAME_SIZE + 1);
			}
		}
		if (error == ENOSPC)
			return MTP_ERROR_STORE_FULL;
		return MTP_ERROR_GENERAL;
	}

	_prop_deinit_ptparray(&child_arr);
	*new_hobj = new_obj->obj_handle;

	return MTP_ERROR_NONE;
}
/* LCOV_EXCL_STOP */

mtp_err_t _hutil_read_file_data_from_offset(mtp_uint32 obj_handle, mtp_uint64 offset,
		void *data, mtp_uint32 *data_sz)
{
	mtp_obj_t *obj = NULL;
	FILE* h_file = NULL;
	mtp_int32 error = 0;
	mtp_char fname[MTP_MAX_PATHNAME_SIZE + 1];
	off_t result = 0;
	mtp_uint32 num_bytes;

	obj = _device_get_object_with_handle(obj_handle);
	retvm_if(!obj, MTP_ERROR_INVALID_OBJECTHANDLE,
		"_device_get_object_with_handle returned NULL object\n");

	/* LCOV_EXCL_START */
	retvm_if(obj->obj_info->protcn_status ==
			MTP_PROTECTIONSTATUS_NONTRANSFERABLE_DATA,
			MTP_ERROR_GENERAL, "protection data, NONTRANSFERABLE_OBJECT\n");

	g_strlcpy(fname, obj->file_path, MTP_MAX_PATHNAME_SIZE + 1);
	h_file = _util_file_open(fname, MTP_FILE_READ, &error);
	retvm_if(!h_file, MTP_ERROR_GENERAL, "file open Fail[%s]\n", fname);

	result = _util_file_seek(h_file, offset, SEEK_SET);
	if (result == FALSE) {
		ERR("file seek Fail [%d]\n", errno);
		_util_file_close(h_file);
		return MTP_ERROR_GENERAL;
	}

	num_bytes = *data_sz;
	_util_file_read(h_file, data, *data_sz, data_sz);

	if (num_bytes != *data_sz) {
		ERR("requested[%d] and read[%d] number of bytes do not match\n",
				*data_sz, num_bytes);
		_util_file_close(h_file);
		return MTP_ERROR_GENERAL;
	}

	_util_file_close(h_file);
	/* LCOV_EXCL_STOP */
	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_write_file_data(mtp_uint32 store_id, mtp_obj_t *obj,
		mtp_char *fpath)
{
	mtp_store_t *store;
	mtp_char fname[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_int32 error = 0;

	retv_if(obj == NULL, MTP_ERROR_INVALID_PARAM);
	retv_if(obj->obj_info == NULL, MTP_ERROR_INVALID_PARAM);
	/* LCOV_EXCL_START */
	store = _device_get_store(store_id);
	retvm_if(!store, MTP_ERROR_INVALID_OBJECT_INFO, "destination store is not valid\n");

	g_strlcpy(fname, obj->file_path, MTP_MAX_PATHNAME_SIZE + 1);
	retvm_if(access(fpath, F_OK) < 0, MTP_ERROR_GENERAL, "temp file does not exist\n");

	g_snprintf(g_last_moved, MTP_MAX_PATHNAME_SIZE + 1, "%s", fpath);
	if (FALSE == _util_file_move(fpath, fname, &error)) {
		memset(g_last_moved, 0, MTP_MAX_PATHNAME_SIZE + 1);
		ERR("move to real file fail [%s]->[%s] \n", fpath, fname);
		_entity_dealloc_mtp_obj(obj);

		return MTP_ERROR_STORE_FULL;
	}

#ifdef MTP_SUPPORT_SET_PROTECTION
	if ((obj_info->protcn_status == PTP_PROTECTIONSTATUS_READONLY) ||
			(obj_info->ProtectionStatus ==
			 MTP_PROTECTIONSTATUS_READONLY_DATA)) {
		file_attr_t attrs;
		if (_util_get_file_attrs(fname, &attrs) == FALSE) {
			ERR("real file get attributes Fail\n");
			_entity_dealloc_mtp_obj(obj);
			return MTP_ERROR_GENERAL;
		}
		_util_set_file_attrs(fname, attrs.attribute |
				MTP_FILE_ATTR_MODE_READ_ONLY);
	}
#endif /* MTP_SUPPORT_SET_PROTECTION */

#ifndef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	if (updatePropertyValuesMtpObject(obj) == FALSE) {
		ERR("update property values mtp obj Fail\n");
		_entity_dealloc_mtp_obj(obj);
		return MTP_ERROR_GENERAL;
	}
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/

	_entity_add_object_to_store(store, obj);
	/* LCOV_EXCL_STOP */
	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_get_object_entry_size(mtp_uint32 obj_handle,
		mtp_uint64 *obj_sz)
{
	mtp_obj_t *obj = NULL;

	obj = _device_get_object_with_handle(obj_handle);
	retvm_if(!obj, MTP_ERROR_INVALID_OBJECTHANDLE,
		"_device_get_object_with_handle returned Null object\n");

	*obj_sz = obj->obj_info->file_size;
	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_truncate_file(mtp_uint32 obj_handle, mtp_uint64 length)
{
	mtp_char fname[MTP_MAX_PATHNAME_SIZE + 1];
	mtp_bool result;
	mtp_obj_t *obj;

	obj = _device_get_object_with_handle(obj_handle);
	retvm_if(!obj, MTP_ERROR_INVALID_OBJECTHANDLE,
		"_device_get_object_with_handle returned Null object\n");

	if (obj->obj_info->association_type == PTP_ASSOCIATIONTYPE_FOLDER)
		return MTP_ERROR_INVALID_OBJECT_INFO;

	g_strlcpy(fname, obj->file_path, MTP_MAX_PATHNAME_SIZE + 1);

	result = _util_file_truncate(fname, length);
	if (result == FALSE) {
		ERR("file truncate Fail [%d]\n", errno);
		return MTP_ERROR_GENERAL;
	}

	obj->obj_info->file_size = length;

	return MTP_ERROR_NONE;
}

#ifdef MTP_SUPPORT_SET_PROTECTION
mtp_err_t _hutil_set_protection(mtp_uint32 obj_handle, mtp_uint16 prot_status)
{
	mtp_obj_t *obj = NULL;
	mtp_char fname[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	file_attr_t attrs = { 0 };

	obj = _device_get_object_with_handle(obj_handle);
	if (obj == NULL)
		return MTP_ERROR_INVALID_OBJECTHANDLE;

	retvm_if(obj->obj_info->store_id == MTP_EXTERNAL_STORE_ID,
		MTP_ERROR_OPERATION_NOT_SUPPORTED, "Storage is external\n");

	g_strlcpy(fname, obj->file_path, MTP_MAX_PATHNAME_SIZE + 1);
	obj->obj_info->protcn_status = prot_status;

	retvm_if(!_util_get_file_attrs(fname, &attrs), MTP_ERROR_GENERAL,
		"Failed to get file[%s] attrs\n", fname);

	if (MTP_FILE_ATTR_MODE_NONE == attrs.attribute)
		return MTP_ERROR_GENERAL;

	if (prot_status == PTP_PROTECTIONSTATUS_READONLY)
		attrs.attribute |= MTP_FILE_ATTR_MODE_READ_ONLY;
	else
		attrs.attribute &= ~MTP_FILE_ATTR_MODE_READ_ONLY;

	retvm_if(!_util_set_file_attrs(fname, attrs.attribute), MTP_ERROR_GENERAL,
		"Failed to set file[%s] attrs\n", fname);

	return MTP_ERROR_NONE;
}
#endif /* MTP_SUPPORT_SET_PROTECTION */

mtp_err_t _hutil_get_object_handles(mtp_uint32 store_id, mtp_uint32 format,
		mtp_uint32 h_parent, ptp_array_t *handle_arr)
{
	mtp_store_t *store = NULL;
	mtp_int32 i = 0;

	if (h_parent == PTP_OBJECTHANDLE_ALL || h_parent == PTP_OBJECTHANDLE_ROOT) {
		for (i = 0; i < g_device->num_stores; i++) {
			store = _device_get_store_at_index(i);
			if (store && store->obj_list.nnodes == 0)
				_entity_store_recursive_enum_folder_objects(store, NULL);
		}
		g_is_full_enum = TRUE;
	}

	if (store_id == PTP_STORAGEID_ALL && h_parent == PTP_OBJECTHANDLE_ROOT) {
		for (i = 0; i < g_device->num_stores; i++) {	//	LCOV_EXCL_LINE
			store = _device_get_store_at_index(i);	//	LCOV_EXCL_LINE
			_entity_get_objects_from_store_by_format(store, format, handle_arr);	//	LCOV_EXCL_LINE
		}
		return MTP_ERROR_NONE;

	} else if (store_id == PTP_STORAGEID_ALL && h_parent == PTP_OBJECTHANDLE_ALL) {
		h_parent = PTP_OBJECTHANDLE_ROOT;
		for (i = 0; i < g_device->num_stores; i++) {	//	LCOV_EXCL_LINE
			store = _device_get_store_at_index(i);	//	LCOV_EXCL_LINE
			_entity_get_child_handles_with_same_format(store, h_parent, format, handle_arr);	//	LCOV_EXCL_LINE
		}
		return MTP_ERROR_NONE;

	} else if (store_id != PTP_STORAGEID_ALL && h_parent == PTP_OBJECTHANDLE_ROOT) {
		store = _device_get_store(store_id);
		retvm_if(!store,MTP_ERROR_INVALID_STORE, "invalid store id [%d]\n", store_id);

		/* LCOV_EXCL_START */
		_entity_get_objects_from_store_by_format(store, format, handle_arr);
		return MTP_ERROR_NONE;

	} else if (store_id != PTP_STORAGEID_ALL && h_parent == PTP_OBJECTHANDLE_ALL) {
		h_parent = PTP_OBJECTHANDLE_ROOT;
		store = _device_get_store(store_id);
		retvm_if(!store, MTP_ERROR_INVALID_STORE, "invalid store id [%d]\n", store_id);

		_entity_get_child_handles_with_same_format(store, h_parent, format, handle_arr);
		return MTP_ERROR_NONE;
	}

	store = _device_get_store(store_id);
	retvm_if(!store, MTP_ERROR_INVALID_STORE, "invalid store id [%d]\n", store_id);

	_entity_get_child_handles_with_same_format(store, h_parent, format, handle_arr);
	return MTP_ERROR_NONE;
	/* LCOV_EXCL_STOP */
}

mtp_err_t _hutil_construct_object_entry(mtp_uint32 store_id,
		mtp_uint32 h_parent, obj_data_t *objdata, mtp_obj_t **obj, void *data,
		mtp_uint32 data_sz)
{
	mtp_store_t *store = NULL;
	mtp_obj_t *tobj = NULL;
	obj_info_t *obj_info = NULL;
	mtp_char file_name[MTP_MAX_FILENAME_SIZE + 1] = { 0 };

	if (store_id) {
		if (!h_parent)
			h_parent = g_device->default_hparent;
		else if (h_parent == 0xFFFFFFFF)
			h_parent = PTP_OBJECTHANDLE_ROOT;
	} else {
		store_id = g_device->default_store_id;

		retvm_if(!store_id, MTP_ERROR_STORE_NOT_AVAILABLE, "_device_get_default_store_id Fail\n");

		if (h_parent) {
			/* If the second parameter is used,
			 * the first must also be used.
			 */
			return MTP_ERROR_INVALID_PARAM;
		} else {
			h_parent = g_device->default_hparent;
		}
	}

	if (objdata != NULL) {
		store = _device_get_store(objdata->store_id);
		if (store != NULL) {
			DBG("check free size instead of re-calculation\n");
			_entity_update_store_info_run_time(&(store->store_info),
					(store->root_path));
		}

		/* Delete and invalidate the old obj_info for send object */
		if (objdata->obj != NULL)
			_entity_dealloc_mtp_obj(objdata->obj);
	}

	store = _device_get_store(store_id);
	retvm_if(!store, MTP_ERROR_INVALID_STORE, "Store not found\n");

	retvm_if(store->store_info.access == PTP_STORAGEACCESS_R, MTP_ERROR_STORE_READ_ONLY,
		"Read only storage\n");

	retvm_if((store->store_info.free_space) == 0 ||
			(store->store_info.free_space >
			 store->store_info.capacity), MTP_ERROR_STORE_FULL,
				"free space is not enough [%llu:%llu]\n",
				store->store_info.free_space,
				store->store_info.capacity);

	obj_info = _entity_alloc_object_info();
	retvm_if(!obj_info, MTP_ERROR_GENERAL, "_entity_alloc_object_info Fail\n");

	if (_entity_parse_raw_obj_info(data, data_sz, obj_info, file_name,
				sizeof(file_name)) != data_sz) {
		/* wrong object info sent from Host.*/
		ERR("Invalid objet info\n");
		_entity_dealloc_obj_info(obj_info);
		return MTP_ERROR_INVALID_OBJECT_INFO;
	}
	obj_info->store_id = store_id;
	obj_info->h_parent = h_parent;

	switch (_hutil_add_object_entry(obj_info, file_name, &tobj)) {
	case MTP_ERROR_NONE:
		*obj = tobj;
		break;

	case MTP_ERROR_STORE_FULL:
		return MTP_ERROR_STORE_FULL;

	case MTP_ERROR_INVALID_OBJECTHANDLE:
		return MTP_ERROR_INVALID_OBJECTHANDLE;

	case MTP_ERROR_INVALID_PARENT:
		return MTP_ERROR_INVALID_PARENT;

	default:
		return MTP_ERROR_GENERAL;
	}

	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_construct_object_entry_prop_list(mtp_uint32 store_id,
		mtp_uint32 h_parent, mtp_uint16 format, mtp_uint64 obj_sz,
		obj_data_t *obj_data, mtp_obj_t **obj_ptr, void *data,
		mtp_int32 data_sz, mtp_uint32 *err_idx)
{
	mtp_uint32 index = 0;
	mtp_store_t *store = NULL;
	mtp_obj_t *obj = NULL;
	obj_prop_desc_t *prop_desc = NULL;
	obj_prop_val_t *prop_val = NULL;
	mtp_uint32 num_elem = 0;
	mtp_int32 quad_sz = 0;
	mtp_uint32 obj_handle = 0;
	mtp_uint16 prop_code = 0;
	mtp_uint16 data_type = 0;
	obj_info_t *obj_info = NULL;
	mtp_uchar *temp = NULL;
	mtp_int32 bytes_left = data_sz;
	mtp_err_t resp = 0;

	mtp_char file_name[MTP_MAX_FILENAME_SIZE + 1] = { 0 };

	if (obj_data != NULL && obj_data->obj != NULL) {
		/* LCOV_EXCL_START */
		store = _device_get_store(obj_data->store_id);
		if (store != NULL) {
			DBG("check free size instead of re-calculation\n");
			_entity_update_store_info_run_time(&(store->store_info),
					(store->root_path));
		}
		_entity_dealloc_mtp_obj(obj_data->obj);
		/* LCOV_EXCL_STOP */
	}

	store = _device_get_store(store_id);
	retvm_if(!store, MTP_ERROR_INVALID_STORE, "Could not get the store\n");

	/* LCOV_EXCL_START */
	retvm_if(store->store_info.access == PTP_STORAGEACCESS_R, MTP_ERROR_STORE_READ_ONLY,
		"Only read access allowed on store\n");

	retvm_if((store->store_info.free_space) == 0 ||
			(store->store_info.free_space > store->store_info.capacity),
			MTP_ERROR_STORE_FULL, "free space is not enough [%llu bytes]\n",
				store->store_info.free_space);

	retvm_if(store->store_info.free_space <= obj_sz, MTP_ERROR_STORE_FULL,
		"free space is not enough [%llu bytes]\n",
				store->store_info.free_space);

	obj_info = _entity_alloc_object_info();
	retvm_if(!obj_info, MTP_ERROR_GENERAL, "_entity_alloc_object_info Fail\n");

	obj_info->obj_fmt = format;
	if (obj_info->obj_fmt == PTP_FMT_ASSOCIATION)
		obj_info->association_type = PTP_ASSOCIATIONTYPE_FOLDER;

	obj_info->file_size = obj_sz;

	/* Prop value quadruple: Object Handle, PropertyCode, DataType
	 * and DTS Prop Value (assume a byte value)
	 */
	temp = (mtp_uchar *) data;
	bytes_left = data_sz;
	quad_sz = sizeof(mtp_uint32) + sizeof(mtp_uint16) + sizeof(mtp_uint16) +
		sizeof(mtp_char);
	memcpy(&num_elem, temp, sizeof(mtp_uint32));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(&num_elem, sizeof(mtp_uint32));
#endif
	temp += sizeof(mtp_uint32);
	bytes_left -= sizeof(mtp_uint32);

	/* frequent disconnect/connect make bluscreen since below process
	 * is not finished
	 */
	for (index = 0; index < num_elem; index++) {
		if (MTP_PHONE_USB_DISCONNECTED == g_ph_status->usb_state ||
				TRUE == g_status->is_usb_discon) {
			/* seems usb is disconnected, stop */
			_entity_dealloc_obj_info(obj_info);
			resp = MTP_ERROR_GENERAL;
			goto ERROR_EXIT;
		}

		*err_idx = index;
		if (bytes_left < quad_sz) {
			/* seems invalid dataset received: Stops parsing */
			_entity_dealloc_obj_info(obj_info);
			resp = MTP_ERROR_INVALID_DATASET;
			goto ERROR_EXIT;
		}

		/* Get ObjectHandle & validate */
		memcpy(&obj_handle, temp, sizeof(mtp_uint32));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(&obj_handle, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
		temp += sizeof(mtp_uint32);
		bytes_left -= sizeof(mtp_uint32);
		if (obj_handle != 0x00000000) {
			_entity_dealloc_obj_info(obj_info);
			resp = MTP_ERROR_INVALID_OBJECTHANDLE;
			goto ERROR_EXIT;
		}

		/* Get PropCode & Validate */
		memcpy(&prop_code, temp, sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(&prop_code, sizeof(mtp_uint16));
#endif /* __BIG_ENDIAN__ */
		temp += sizeof(mtp_uint16);
		bytes_left -= sizeof(mtp_uint16);
		prop_desc = _prop_get_obj_prop_desc(obj_info->obj_fmt, prop_code);
		if (prop_desc == NULL) {
			_entity_dealloc_obj_info(obj_info);
			ERR("property may be unsupported!!\n");
			resp = MTP_ERROR_INVALID_OBJ_PROP_CODE;
			goto ERROR_EXIT;
		}

		/* Verify that properties already present in parameters
		 * don't get repeated in the list
		 */
		if ((prop_code == MTP_OBJ_PROPERTYCODE_STORAGEID) ||
				(prop_code == MTP_OBJ_PROPERTYCODE_PARENT) ||
				(prop_code == MTP_OBJ_PROPERTYCODE_OBJECTFORMAT) ||
				(prop_code == MTP_OBJ_PROPERTYCODE_OBJECTSIZE)) {
			_entity_dealloc_obj_info(obj_info);
			resp = MTP_ERROR_INVALID_DATASET;
			goto ERROR_EXIT;
		}

		/* Get DataType */
		memcpy(&data_type, temp, sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(&data_type, sizeof(mtp_uint16));
#endif /* __BIG_ENDIAN__ */
		temp += sizeof(mtp_uint16);
		bytes_left -= sizeof(mtp_uint16);
		if (data_type != prop_desc->propinfo.data_type) {
			_entity_dealloc_obj_info(obj_info);
			resp = MTP_ERROR_INVALID_OBJECT_PROP_FORMAT;
			goto ERROR_EXIT;
		}

		/* Acquire object information related data. */
		prop_val = _prop_alloc_obj_propval(prop_desc);
		if (prop_val == NULL)
			continue;

		_prop_set_current_array_val(prop_val, temp, bytes_left);
		switch (prop_code) {
		case MTP_OBJ_PROPERTYCODE_WIDTH:
			// TODO: find mechanism to save (integer)
			break;
		case MTP_OBJ_PROPERTYCODE_TRACK:
			// TODO: find mechanism to save (integer)
			break;
		case MTP_OBJ_PROPERTYCODE_GENRE:
			// TODO: find mechanism to save (string)
			break;
		case MTP_OBJ_PROPERTYCODE_HEIGHT:
			// TODO: find mechanism to save (integer)
			break;
		case MTP_OBJ_PROPERTYCODE_ARTIST:
			// TODO: find mechanism to save (string)
			break;
		case MTP_OBJ_PROPERTYCODE_DURATION:
			// TODO: find mechanism to save (integer)
			break;
		case MTP_OBJ_PROPERTYCODE_COMPOSER:
			// TODO: find mechanism to save (string)
			break;
		case MTP_OBJ_PROPERTYCODE_ALBUMNAME:
			// TODO: find mechanism to save (string)
			break;
		case MTP_OBJ_PROPERTYCODE_SAMPLERATE:
			// TODO: find mechanism to save (integer)
			break;
		case MTP_OBJ_PROPERTYCODE_DATECREATED:
			// TODO: find mechanism to save (string)
			break;
		case MTP_OBJ_PROPERTYCODE_DATEMODIFIED:
			// TODO: find mechanism to save (string)
			break;
		case MTP_OBJ_PROPERTYCODE_AUDIOBITRATE:
			// TODO: find mechanism to save (integer)
			break;
		case MTP_OBJ_PROPERTYCODE_VIDEOBITRATE:
			// TODO: find mechanism to save (integer)
			break;
		case  MTP_OBJ_PROPERTYCODE_OBJECTFILENAME:
			/* empty metadata folder problem
			 * emtpy file name
			 */
			if (prop_val->current_val.str->num_chars == 0) {
				g_strlcpy(file_name, MTP_UNKNOWN_METADATA, sizeof(file_name));
			} else {
				_util_utf16_to_utf8(file_name, sizeof(file_name),
						prop_val->current_val.str->str);
			}
			break;

		case MTP_OBJ_PROPERTYCODE_PROTECTIONSTATUS:
#ifdef MTP_SUPPORT_SET_PROTECTION
			memcpy(&obj_info->protcn_status, prop_val->current_val.integer,
					sizeof(mtp_uint16));
#else /* MTP_SUPPORT_SET_PROTECTION */
			obj_info->protcn_status = PTP_PROTECTIONSTATUS_NOPROTECTION;
#endif /*MTP_SUPPORT_SET_PROTECTION*/
			break;

		case MTP_OBJ_PROPERTYCODE_ASSOCIATIONTYPE:
			memcpy(&obj_info->association_type, prop_val->current_val.integer,
					sizeof(mtp_uint16));
			break;

		case MTP_OBJ_PROPERTYCODE_NUMBEROFCHANNELS:
			// TODO: find mechanism to save (integer)
			break;
		case MTP_OBJ_PROPERTYCODE_FRAMESPER1KSECONDS:
			// TODO: find mechanism to save (integer)
			break;

		default:
			DBG("Unsupported Property [0x%x]\n", prop_code);
			break;
		}

		temp += _prop_size_obj_propval(prop_val);
		bytes_left -= _prop_size_obj_propval(prop_val);
		_prop_destroy_obj_propval(prop_val);
	}

	obj_info->store_id = store_id;
	obj_info->h_parent = h_parent;

	if ((resp = _hutil_add_object_entry(obj_info, file_name, &obj)) !=
			MTP_ERROR_NONE) {
		goto ERROR_EXIT;
	}

	*obj_ptr = obj;

	return MTP_ERROR_NONE;

ERROR_EXIT:
	if (obj_info != NULL)
		_entity_dealloc_obj_info(obj_info);
	/* LCOV_EXCL_STOP */
	return resp;
}

mtp_err_t _hutil_get_object_prop_value(mtp_uint32 obj_handle,
		mtp_uint32 prop_code, obj_prop_val_t *prop_val, mtp_obj_t **obj)
{
	obj_prop_val_t *tprop = NULL;
	mtp_obj_t *tobj = NULL;

	tobj = _device_get_object_with_handle(obj_handle);
	retvm_if(!tobj, MTP_ERROR_INVALID_OBJECTHANDLE,
		"requested handle does not exist[0x%x]\n", obj_handle);

	/* LCOV_EXCL_START */
	tprop = _prop_get_prop_val(tobj, prop_code);
	if (tprop != NULL) {
		memcpy(prop_val, tprop, sizeof(obj_prop_val_t));
		*obj = tobj;
		return MTP_ERROR_NONE;
	}

	ERR("can not get the prop value for propcode [0x%x]\n", prop_code);
	return MTP_ERROR_GENERAL;
	/* LCOV_EXCL_STOP */
}

mtp_err_t _hutil_update_object_property(mtp_uint32 obj_handle,
		mtp_uint32 prop_code, mtp_uint16 *data_type, void *buf,
		mtp_uint32 buf_sz, mtp_uint32 *prop_sz)
{
	mtp_int32 error = 0;
	mtp_uint32 p_size = 0;
	mtp_obj_t *obj = NULL;
	obj_info_t *obj_info = NULL;
	obj_prop_desc_t *prp_dev = NULL;
	mtp_char temp_buf[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char orig_pfpath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_wchar mov_fpath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char orig_fpath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char dest_fpath[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	retv_if(NULL == buf, MTP_ERROR_INVALID_PARAM);

	obj = _device_get_object_with_handle(obj_handle);
	retvm_if(!obj || !obj->obj_info, MTP_ERROR_INVALID_OBJECTHANDLE, "Object not found\n");

	/* LCOV_EXCL_START */
	obj_info = obj->obj_info;
	/* Avoid to rename file/folder during file operating by phone side. */
	retvm_if(_util_is_file_opened(obj->file_path), MTP_ERROR_GENERAL,
		"Object [%s] is already opened\n", obj->file_path);

	prp_dev = _prop_get_obj_prop_desc(obj_info->obj_fmt, prop_code);
	retvm_if(!prp_dev, MTP_ERROR_INVALID_OBJ_PROP_CODE, "_prop_get_obj_prop_desc Fail\n");

#ifdef MTP_SUPPORT_SET_PROTECTION
	retvm_if(obj_info->protcn_status == PTP_PROTECTIONSTATUS_READONLY,
		MTP_ERROR_ACCESS_DENIED, "protection is PTP_PROTECTIONSTATUS_READONLY\n");

#endif /* MTP_SUPPORT_SET_PROTECTION */

	retvm_if(prp_dev->propinfo.get_set == PTP_PROPGETSET_GETONLY, MTP_ERROR_ACCESS_DENIED,
		"property type is GETONLY\n");

	retvm_if(data_type && *data_type != prp_dev->propinfo.data_type,
		MTP_ERROR_INVALID_OBJECT_PROP_FORMAT,
		"Not matched data type [%d][%d]\n",
				*data_type, prp_dev->propinfo.data_type);

	/* Set up needed object info fields */
	if (prop_code == MTP_OBJ_PROPERTYCODE_OBJECTFILENAME) {
		ptp_string_t fname = { 0 };

		fname.num_chars = 0;
		_prop_parse_rawstring(&fname, buf, buf_sz);

		_util_utf16_to_utf8(temp_buf, sizeof(temp_buf),
				fname.str);
		g_strlcpy(orig_fpath, obj->file_path,
				MTP_MAX_PATHNAME_SIZE + 1);
		_util_get_parent_path(orig_fpath, orig_pfpath);

		retvm_if(!_util_create_path(dest_fpath, sizeof(dest_fpath),
			orig_pfpath, temp_buf), MTP_ERROR_ACCESS_DENIED, "Path is too long\n");

		_util_utf8_to_utf16(mov_fpath,
				sizeof(mov_fpath) / WCHAR_SIZ, dest_fpath);

		/* when changed name is different */
		if (strcasecmp(orig_fpath, dest_fpath)) {
			/* Extension change is not permitted */
			mtp_char orig_extn[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
			mtp_char dest_extn[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

			_util_get_file_extn(orig_fpath, orig_extn);
			_util_get_file_extn(dest_fpath, dest_extn);

			retvm_if(strcasecmp(orig_extn, dest_extn), MTP_ERROR_INVALID_OBJECT_PROP_FORMAT,
				"file extension is different with original\
						one [%s]:[%s]\n", orig_extn, dest_extn);

			/* FILE RENAME */
			retvm_if(!_entity_check_child_obj_path(obj, orig_fpath,
				dest_fpath), MTP_ERROR_GENERAL,
				"_entity_check_child_obj_path FALSE.\n");

			g_snprintf(g_last_moved, MTP_MAX_PATHNAME_SIZE + 1,
					"%s", orig_fpath);
			if (FALSE == _util_file_move(orig_fpath, dest_fpath,
						&error)) {
				if (EACCES == error)
					return MTP_ERROR_ACCESS_DENIED;
				return MTP_ERROR_GENERAL;
			}

			/* Finally assign new handle and update full path */
			_entity_set_object_file_path(obj, dest_fpath,
					CHAR_TYPE);

			/* FILE RENAME */
			retvm_if(!_entity_set_child_object_path(obj, orig_fpath, dest_fpath),
				MTP_ERROR_INVALID_OBJECT_PROP_FORMAT,
				"failed to set the full path!!\n");

			DBG("File moved to [%s]\n", dest_fpath);
		} else {
			ERR_SECURE("changed name is same with original one. [%s]\n",
					dest_fpath);
		}
		p_size = _prop_size_ptpstring(&fname);
	} else if (prop_code == MTP_OBJ_PROPERTYCODE_ASSOCIATIONTYPE) {
		memcpy(&obj_info->association_type, buf, sizeof(mtp_uint16));
		p_size = sizeof(mtp_uint16);
	} else {
		ERR("Propert [0x%x] is GETONLY\n", prop_code);
	}

	if (prop_sz != NULL)
		*prop_sz = p_size;
	/* LCOV_EXCL_STOP */
	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_get_prop_desc(mtp_uint32 format, mtp_uint32 prop_code,
		void *data)
{
	obj_prop_desc_t *prop = NULL;

	prop = _prop_get_obj_prop_desc(format, prop_code);
	retvm_if(!prop, MTP_ERROR_GENERAL, "pProperty is NULL\n");

	memcpy(data, prop, sizeof(obj_prop_desc_t));
	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_get_object_prop_supported(mtp_uint32 format,
		ptp_array_t	*prop_arr)
{
	_prop_get_supp_obj_props(format, prop_arr);
	return MTP_ERROR_NONE;
}

#ifndef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
mtp_err_t _hutil_get_object_prop_list(mtp_uint32 obj_handle, mtp_uint32 format,
		mtp_uint32 prop_code, mtp_uint32 group_code, mtp_uint32 depth,
		obj_proplist_t *prop_list)
#else /* MTP_USE_RUNTIME_GETOBJECTPROPVALUE */
mtp_err_t _hutil_get_object_prop_list(mtp_uint32 obj_handle, mtp_uint32 format,
		mtp_uint32 prop_code, mtp_uint32 group_code, mtp_uint32 depth,
		obj_proplist_t *prop_list, ptp_array_t *obj_arr)
#endif /* MTP_USE_RUNTIME_GETOBJECTPROPVALUE */
{
#ifndef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	ptp_array_t obj_arr = { 0 };
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/
	mtp_obj_t *obj = NULL;
	mtp_uint32 i = 0;
	mtp_uint32 ii = 0;
	mtp_store_t *store = NULL;

	if ((obj_handle != PTP_OBJECTHANDLE_UNDEFINED) &&
			(obj_handle != PTP_OBJECTHANDLE_ALL)) {
		/* Is this object handle valid? */
		/* LCOV_EXCL_START */
		store = _device_get_store_containing_obj(obj_handle);
		retvm_if(!store, MTP_ERROR_INVALID_OBJECTHANDLE, "invalid object handle\n");
		/* LCOV_EXCL_STOP */
	}

	if (prop_code == PTP_PROPERTY_UNDEFINED) {
		/* PropGroupCode should be used if Property code
		 * is not specified.
		 * */
		retvm_if(group_code == 0x0, MTP_ERROR_INVALID_PARAM, "PropGroupCode is zero\n");
	}

	/* LCOV_EXCL_START */
	retvm_if(!(obj_handle == PTP_OBJECTHANDLE_ALL ||
				obj_handle == PTP_OBJECTHANDLE_UNDEFINED) &&
			!(format == PTP_FORMATCODE_NOTUSED ||
				format == PTP_FORMATCODE_ALL), MTP_ERROR_NO_SPEC_BY_FORMAT,
				"both object handle and format code is specified!\
				return nospecification by format\n");

	if (prop_list == NULL)
		return MTP_ERROR_INVALID_PARAM;

	_util_init_list(&(prop_list->prop_quad_list));
	_prop_init_ptparray(obj_arr, UINT32_TYPE);

	if (store != NULL) {
		_entity_get_objects_from_store_till_depth(store, obj_handle,
				format, depth, obj_arr);
	} else {
		for (ii = 0; ii < g_device->num_stores; ii++) {
			store = _device_get_store_at_index(ii);
			_entity_get_objects_from_store_till_depth(store,
					obj_handle, format, depth, obj_arr);
		}
	}

	if (obj_arr->num_ele != 0) {
		mtp_uint32 *obj_handles = obj_arr->array_entry;

		for (i = 0; i < obj_arr->num_ele; i++) {
			obj = _entity_get_object_from_store(store, obj_handles[i]);
			if (!obj)
				continue;

			if (_prop_get_obj_proplist(obj, prop_code, group_code,
						prop_list) == FALSE) {
				ERR("Fail to create Proplist\n");
#ifndef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
				_prop_deinit_ptparray(&obj_arr);
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/
				return MTP_ERROR_GENERAL;
			}
		}
	/* LCOV_EXCL_STOP */
	}

#ifndef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	prop_deinit_ptparray(&obj_arr);
#endif /*MTP_USE_RUNTIME_GETOBJECTPROPVALUE*/

	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_remove_object_reference(mtp_uint32 obj_handle,
		mtp_uint32 ref_handle)
{
	mtp_obj_t *obj = NULL;

	obj = _device_get_object_with_handle(obj_handle);
	retvm_if (!obj, MTP_ERROR_NONE, "No object for handle[%d]\n", obj_handle);

	if (_entity_remove_reference_child_array(obj, ref_handle) == FALSE)
		ERR("_entity_remove_reference_child_array Fail\n");

	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_add_object_references_enhanced(mtp_uint32 obj_handle,
		mtp_uchar *buffer, mtp_uint32 buf_sz)
{
	mtp_obj_t *obj = NULL;

	obj = _device_get_object_with_handle(obj_handle);
	if (obj == NULL) {
		DBG("No object for handle[0x%x]\n", obj_handle);
		return MTP_ERROR_NONE;
	}

	if (_entity_set_reference_child_array(obj, buffer, buf_sz) == FALSE) {
		ERR("_entity_set_reference_child_array Fail\n");
		return MTP_ERROR_GENERAL;
	}

	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_get_object_references(mtp_uint32 obj_handle,
		ptp_array_t *parray, mtp_uint32 *num_ele)
{
	mtp_obj_t *obj = NULL;
	mtp_obj_t *ref_obj = NULL;
	mtp_uint16 idx = 0;
	mtp_uint32 *ref_ptr = NULL;
	ptp_array_t ref_arr = { 0 };

	obj = _device_get_object_with_handle(obj_handle);
	if (NULL == obj || NULL == obj->obj_info) {
		*num_ele = 0;
		return MTP_ERROR_INVALID_OBJECTHANDLE;
	}

	/* LCOV_EXCL_START */
	if (parray == NULL) {
		*num_ele = 0;
		return MTP_ERROR_GENERAL;
	}

	if (obj->child_array.num_ele == 0) {
		*num_ele = 0;
		return MTP_ERROR_NONE;
	}

	_prop_init_ptparray(&ref_arr, obj->child_array.type);
	_prop_copy_ptparray(&ref_arr, &(obj->child_array));
	ref_ptr = (mtp_uint32 *)(ref_arr.array_entry);

	for (idx = 0; idx < ref_arr.num_ele; idx++) {
		ref_obj = _device_get_object_with_handle(ref_ptr[idx]);
		if (ref_obj == NULL)
			_entity_remove_reference_child_array(obj, ref_ptr[idx]);
	}

	*num_ele = obj->child_array.num_ele;
	if (*num_ele) {
		_prop_init_ptparray(parray, obj->child_array.type);
		_prop_grow_ptparray(parray, *num_ele);
		_prop_copy_ptparray(parray, &(obj->child_array));
	}

	_prop_deinit_ptparray(&ref_arr);
	/* LCOV_EXCL_STOP */
	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_get_interdep_prop_config_list_size(mtp_uint32 *list_sz,
		mtp_uint32 format)
{
	*list_sz = _prop_get_size_interdep_proplist(&interdep_proplist,
			format);
	return MTP_ERROR_NONE;
}

mtp_err_t _hutil_get_interdep_prop_config_list_data(void *data,
		mtp_uint32 list_sz, mtp_uint32 format)
{
	if (list_sz == _prop_pack_interdep_proplist(&interdep_proplist,
				format, data, list_sz)) {
		return MTP_ERROR_NONE;
	}

	ERR("packet and requested size do not match\n");
	return MTP_ERROR_GENERAL;
}

/* LCOV_EXCL_STOP */

mtp_uint32 _hutil_get_storage_info_size(store_info_t *store_info)
{
	mtp_uint32 size = 0;

	size += sizeof(store_info->access);
	size += sizeof(store_info->fs_type);
	size += sizeof(store_info->free_space);
	size += sizeof(store_info->free_space_in_objs);
	size += sizeof(store_info->capacity);
	size += sizeof(store_info->store_type);
	size += _prop_size_ptpstring(&(store_info->store_desc));
	size += _prop_size_ptpstring(&(store_info->vol_label));

	return size;
}
