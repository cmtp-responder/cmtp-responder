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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include "mtp_fs.h"
#include "mtp_support.h"
#include "mtp_util.h"
#include "mtp_device.h"


extern mtp_bool g_is_full_enum;


mtp_bool _entity_get_file_times(mtp_obj_t *obj, ptp_time_string_t *create_tm,
		ptp_time_string_t *modify_tm)
{
	file_attr_t attrs = {0};
	system_time_t local_time = {0};
	struct tm new_time = {0};

	if (FALSE == _util_get_file_attrs(obj->file_path, &attrs)) {
		ERR("_util_get_file_attrs Fail");
		return FALSE;
	}

	if (NULL != localtime_r((time_t*)&attrs.ctime, &new_time)) {
		local_time.year = new_time.tm_year + 1900;
		local_time.month = new_time.tm_mon + 1;
		local_time.day = new_time.tm_mday;
		local_time.hour = new_time.tm_hour;
		local_time.minute = new_time.tm_min;
		local_time.second = new_time.tm_sec;
		local_time.day_of_week = 0;
		local_time.millisecond = 0;
	} else {
		ERR("localtime_r returned NULL");
		_util_print_error();
		return FALSE;
	}
	_prop_copy_time_to_ptptimestring(create_tm, &local_time);

	if (NULL != localtime_r((time_t*)&attrs.mtime, &new_time)) {
		local_time.year = new_time.tm_year + 1900;
		local_time.month = new_time.tm_mon + 1;
		local_time.day = new_time.tm_mday;
		local_time.hour = new_time.tm_hour;
		local_time.minute = new_time.tm_min;
		local_time.second = new_time.tm_sec;
		local_time.day_of_week = 0;
		local_time.millisecond = 0;
	} else {
		ERR("localtime_r returned NULL");
		_util_print_error();
		return FALSE;
	}
	_prop_copy_time_to_ptptimestring(modify_tm, &local_time);

	return TRUE;
}

obj_info_t *_entity_alloc_object_info(void)
{
	obj_info_t *info = NULL;

	info = (obj_info_t *)g_malloc(sizeof(obj_info_t));
	if (NULL == info) {
		ERR("Memory allocation Fail");
		return NULL;
	}

	_entity_init_object_info(info);

	return info;
}

void _entity_init_object_info(obj_info_t *info)
{
	ret_if(info == NULL);

	memset(info, 0, sizeof(obj_info_t));
}

mtp_uint32 _entity_get_object_info_size(mtp_obj_t *obj, ptp_string_t *file_name)
{
	int ret;
	ptp_string_t keywords;
	ptp_time_string_t create_time_str = {0};
	ptp_time_string_t modify_time_str = {0};
	mtp_uint32 size = FIXED_LENGTH_MEMBERS_SIZE;

	retv_if(obj == NULL, 0);

	ret = _entity_get_file_times(obj, &create_time_str, &modify_time_str);
	if (FALSE == ret) {
		ERR("_entity_get_file_times() Fail");
		return 0;
	}

	_prop_copy_char_to_ptpstring(&keywords, (mtp_wchar *)"", WCHAR_TYPE);

	size += _prop_size_ptpstring(file_name);
	size += _prop_size_ptptimestring(&create_time_str);
	size += _prop_size_ptptimestring(&modify_time_str);
	size += _prop_size_ptpstring(&keywords);

	return (size);
}

void _entity_init_object_info_params(obj_info_t *info, mtp_uint32 store_id,
		mtp_uint32 parent_handle, mtp_char *file_name, dir_entry_t *dir)
{
	mtp_char extn[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	_entity_init_object_info(info);

	info->store_id = store_id;
	info->h_parent = parent_handle;

	_util_get_file_extn(file_name, extn);

	if (dir->attrs.attribute == MTP_FILE_ATTR_INVALID) {
		ERR("File attribute invalid");
		return;
	}
#ifndef MTP_SUPPORT_SET_PROTECTION
	info->protcn_status = PTP_PROTECTIONSTATUS_NOPROTECTION;
#else /* MTP_SUPPORT_SET_PROTECTION */
	info->protcn_status = (dir->attrs.attribute &
			MTP_FILE_ATTR_MODE_READ_ONLY) ?
		PTP_PROTECTIONSTATUS_READONLY :
		PTP_PROTECTIONSTATUS_NOPROTECTION;
#endif /* MTP_SUPPORT_SET_PROTECTION */

	if (dir->attrs.attribute & MTP_FILE_ATTR_MODE_DIR) {
		info->obj_fmt = PTP_FMT_ASSOCIATION;
		info->association_type = PTP_ASSOCIATIONTYPE_FOLDER;
		return;
	}

	info->file_size = dir->attrs.fsize;
	info->obj_fmt = _util_get_fmtcode(extn);

	return;
}

mtp_uint32 _entity_parse_raw_obj_info(mtp_uchar *buf, mtp_uint32 buf_sz,
		obj_info_t *info, mtp_char *file_name, mtp_uint16 fname_len)
{
	mtp_uchar *temp = buf;
	ptp_string_t str = {0};
	mtp_uint32 bytes_parsed = 0;

	retv_if(buf == NULL, 0);
	retvm_if(buf_sz < FIXED_LENGTH_MEMBERS_SIZE, 0, "buf_sz[%d] is less", buf_sz);

	/* Copy Obj Props from store_id till file_size */
	memcpy(&(info->store_id), temp, (sizeof(mtp_uint16) * 2 + sizeof(mtp_uint32) * 2));
	temp += (sizeof(mtp_uint16) * 2 + sizeof(mtp_uint32) * 2);

	/* Skip ObjProp:thumb format .No need to store ,the prop has
	 * a default value.
	 */
	temp += sizeof(mtp_uint16);

	memcpy(&(info->thumb_file_size), temp, sizeof(mtp_uint32));
	temp += sizeof(mtp_uint32);

	/* Skip ObjProps:ThumbPixWidth,ThumbPixHeight,ImagePixWidth,
	 * ImagePixHeight,ImageBitDepth.No need to store,they have default value
	 */
	temp += sizeof(mtp_uint32) * 5;

	memcpy(&(info->h_parent), temp, sizeof(mtp_uint32));
	temp += sizeof(mtp_uint32);

	memcpy(&(info->association_type), temp, sizeof(mtp_uint16));
	temp += sizeof(mtp_uint16);

	/* Skip ObjProps:association_desc, sequence_desc */
	temp += sizeof(mtp_uint32) * 2;

#ifdef __BIG_ENDIAN__
	/* Byte swap the structure elements if needed.
	 * This works since the data elements will have the same size,
	 * the only difference is the byte order.
	 */
	_util_conv_byte_order(&(info->store_id), sizeof(info->store_id));
	_util_conv_byte_order(&(info->obj_fmt), sizeof(info->obj_fmt));
	_util_conv_byte_order(&(info->protcn_status),
			sizeof(info->protcn_status));
	_util_conv_byte_order(&(info->file_size), sizeof(info->file_size));
	_util_conv_byte_order(&(info->thumb_file_size),
			sizeof(info->thumb_file_size));
	_util_conv_byte_order(&(info->h_parent), sizeof(info->h_parent));
	_util_conv_byte_order(&(info->association_type),
			sizeof(info->association_type));
#endif /*__BIG_ENDIAN__*/

	ptp_string_t fname = { 0 };
	bytes_parsed = _prop_parse_rawstring(&fname, temp, buf_sz);
	temp += bytes_parsed;

	_util_utf16_to_utf8(file_name, fname_len, fname.str);

	/* Skip ObjProps: datecreated/datemodified/keywords.
	 * The values are retrieved using stat.
	 */
	memcpy(&(str.num_chars), temp, sizeof(mtp_uchar));
	temp += _prop_size_ptpstring(&str);

	memcpy(&(str.num_chars), temp, sizeof(mtp_uchar));
	temp += _prop_size_ptpstring(&str);

	memcpy(&(str.num_chars), temp, sizeof(mtp_uchar));
	temp += _prop_size_ptpstring(&str);

	return (mtp_uint32)(temp - buf);
}

void _entity_copy_obj_info(obj_info_t *dst, obj_info_t *src)
{
	memcpy(&(dst->store_id), &(src->store_id), (2 * sizeof(mtp_uint16) +
				sizeof(mtp_uint32) + sizeof(mtp_uint64)));

	/* Copy Object Props:thumb_file_size,h_parent, association_type */
	memcpy(&(dst->thumb_file_size), &(src->thumb_file_size),
			(2 * sizeof(mtp_uint32) + sizeof(mtp_uint16)));

	return;
}

mtp_uint32 _entity_pack_obj_info(mtp_obj_t *obj, ptp_string_t *file_name,
		mtp_uchar *buf, mtp_uint32 buf_sz)
{
	int ret;
	mtp_uint32 num_bytes = 0;
	mtp_uint32 objSize = 0;
	mtp_uint16 thumb_fmt = 0;
	mtp_uint32 thumb_width = 0;
	mtp_uint32 thumb_height = 0;
	obj_info_t *info = obj->obj_info;
	ptp_time_string_t create_time_str = { 0 };
	ptp_time_string_t modify_time_str = { 0 };
	ptp_string_t keywords;

	retv_if(buf == NULL, 0);
	retv_if(obj == NULL, 0);

	ret = _entity_get_file_times(obj, &create_time_str, &modify_time_str);
	if (FALSE == ret) {
		ERR("_entity_get_file_times() Fail");
		return 0;
	}

	_prop_copy_char_to_ptpstring(&keywords, (mtp_wchar *)"", WCHAR_TYPE);

	if (buf_sz < _entity_get_object_info_size(obj, file_name)) {
		ERR("Buffer size is less than object info size");
		return 0;
	}

	/* As per Spec ObjectCompressedSize field in ObjectInfo dataset is
	 * 4 bytes. In case file size greater than 4Gb, value 0xFFFFFFFF is sent
	 */
	objSize = (info->file_size >= MTP_FILESIZE_4GB) ?
		0xFFFFFFFF : (mtp_uint32)info->file_size;

#ifdef __BIG_ENDIAN__
	memcpy(&(buf[num_bytes]), &(info->store_id), sizeof(mtp_uint32));
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	memcpy(&(buf[num_bytes]), &(info->obj_fmt), sizeof(mtp_uint16));
	_util_conv_byte_order(buf, sizeof(mtp_uint16));
	num_bytes += sizeof(mtp_uint16);

	memcpy(&(buf[num_bytes]), &(info->protcn_status), sizeof(mtp_uint16));
	_util_conv_byte_order(buf, sizeof(mtp_uint16));
	num_bytes += sizeof(mtp_uint16);

	memcpy(&(buf[num_bytes]), &objSize, sizeof(mtp_uint32));
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	/* Thumb format has a constant value of 2 bytes */
	memset(&(buf[num_bytes]), &thumb_fmt, sizeof(mtp_uint16));
	_util_conv_byte_order(buf, sizeof(mtp_uint16));
	num_bytes += sizeof(mtp_uint16);

	memcpy(&(buf[num_bytes]), &(info->thumb_file_size), sizeof(mtp_uint32));
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	memcpy(&(buf[num_bytes]), &thumb_width, sizeof(mtp_uint32));
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	memcpy(&(buf[num_bytes]), &thumb_height, sizeof(mtp_uint32));
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	/* ObjProps:image_width, image_height, image_bit_depth values
	 * currently are always 0 for any type of FILE/FOLDER
	 */
	memset(&(buf[num_bytes]), 0, sizeof(mtp_uint32) * 3);
	_util_conv_byte_order(buf, sizeof(mtp_uint32) * 3);
	num_bytes += (sizeof(mtp_uint32) * 3);

	memcpy(&(buf[num_bytes]), &(info->h_parent), sizeof(mtp_uint32));
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	memcpy(&(buf[num_bytes]),
			&(info->association_type), sizeof(mtp_uint16));
	_util_conv_byte_order(buf, sizeof(mtp_uint16));
	num_bytes += sizeof(mtp_uint16);

	/* ObjProps:association desc,sequence number values are always 0 */
	memset(&(buf[num_bytes]), 0, sizeof(mtp_uint32) * 2);
	_util_conv_byte_order(buf, sizeof(mtp_uint32) * 2);
	num_bytes += (sizeof(mtp_uint32) * 2);

#else /* __BIG_ENDIAN__ */

	/* ObjProps:store_id, obj_format, protection status */
	memcpy(buf, &(info->store_id),
			(sizeof(mtp_uint32) + sizeof(mtp_uint16) * 2));
	num_bytes += (sizeof(mtp_uint32) + sizeof(mtp_uint16) * 2);

	memcpy(&buf[num_bytes], &objSize, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	/* ObjProp:thumb format */
	memcpy(&buf[num_bytes], &thumb_fmt, sizeof(mtp_uint16));
	num_bytes += sizeof(mtp_uint16);

	/* ObjProp: thumb file size */
	memcpy(&buf[num_bytes], &(info->thumb_file_size), sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	/* ObjProp:thumb_width */
	memcpy(&buf[num_bytes], &thumb_width, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	/* ObjProp:thumb_height */
	memcpy(&buf[num_bytes], &thumb_height, sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	/* ObjProp:image_width, image_height,
	 * image_bit_depth values currently are always 0
	 */
	memset(&(buf[num_bytes]), 0, sizeof(mtp_uint32) * 3);
	num_bytes +=  (sizeof(mtp_uint32) * 3);

	/* ObjProp:parent_handle */
	memcpy(&buf[num_bytes], &(info->h_parent), sizeof(mtp_uint32));
	num_bytes += sizeof(mtp_uint32);

	/* ObjProp:association_type */
	memcpy(&buf[num_bytes], &(info->association_type), sizeof(mtp_uint16));
	num_bytes += sizeof(mtp_uint16);

	/* ObjProp:association_desc,sequence Number values are always 0 */
	memset(&buf[num_bytes], 0, sizeof(mtp_uint32) * 2);
	num_bytes += sizeof(mtp_uint32) * 2;
#endif /* __BIG_ENDIAN__ */

	num_bytes += _prop_pack_ptpstring(file_name, buf + num_bytes,
			_prop_size_ptpstring(file_name));

	num_bytes += _prop_pack_ptptimestring(&create_time_str,
			buf + num_bytes,
			_prop_size_ptptimestring(&create_time_str));

	num_bytes += _prop_pack_ptptimestring(&modify_time_str,
			buf + num_bytes,
			_prop_size_ptptimestring(&modify_time_str));

	num_bytes += _prop_pack_ptpstring(&keywords,
			buf + num_bytes,
			_prop_size_ptpstring(&keywords));

	DBG("number of bytes for objectinfo :[%d]\n", num_bytes);
	return num_bytes;
}

void _entity_dealloc_obj_info(obj_info_t *info)
{
	g_free(info);
	info = NULL;
	return;
}

mtp_obj_t *_entity_alloc_mtp_object(void)
{
	return ((mtp_obj_t *)g_malloc(sizeof(mtp_obj_t)));
}

mtp_bool _entity_init_mtp_object_params(
		mtp_obj_t *obj,
		mtp_uint32 store_id,
		mtp_uint32 h_parent,
		mtp_char *file_path,
		mtp_char *file_name,
		dir_entry_t *file_info)
{
	retv_if(obj == NULL, FALSE);

	obj->obj_handle = 0;
	obj->obj_info = NULL;
	obj->file_path = NULL;
	obj->obj_handle = _entity_generate_next_obj_handle();
	_entity_set_object_file_path(obj, file_path, CHAR_TYPE);
	obj->obj_info = _entity_alloc_object_info();

	if (NULL == obj->obj_info) {
		g_free(obj->file_path);
		return FALSE;
	}
	_entity_init_object_info_params(obj->obj_info, store_id, h_parent,
			file_name, file_info);

	_util_init_list(&(obj->propval_list));
	memset(&(obj->child_array), 0, sizeof(ptp_array_t));
	obj->child_array.type = UINT32_TYPE;

	return TRUE;
}

mtp_bool _entity_set_object_file_path(mtp_obj_t *obj, void *file_path,
		char_mode_t char_type)
{
	mtp_char temp[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	if (char_type == WCHAR_TYPE) {
		_util_utf16_to_utf8(temp, sizeof(temp), (mtp_wchar *)file_path);
		g_free(obj->file_path);
		obj->file_path = g_strdup(temp);
	} else {
		g_free(obj->file_path);
		obj->file_path = g_strdup((char *)file_path);
	}
	return TRUE;
}

mtp_bool _entity_check_child_obj_path(mtp_obj_t *obj,
		mtp_char *src_path, mtp_char *dest_path)
{
	mtp_uint16 idx = 0;
	mtp_char *ptr = NULL;
	mtp_obj_t *child_obj = NULL;
	ptp_array_t child_arr = { 0 };
	mtp_store_t *src_store = NULL;
	mtp_char dest_chld_path[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char temp_chld_path[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_wchar dest_chld_wpath[MTP_MAX_PATHNAME_SIZE * 2 + 1] = { 0 };
	mtp_wchar temp_chld_wpath[MTP_MAX_PATHNAME_SIZE * 2 + 1] = { 0 };

	retv_if(obj == NULL, FALSE);

	if (strlen(dest_path) > MTP_MAX_PATHNAME_SIZE - 1) {
		ERR("dest_path is too long[%d]\n", strlen(dest_path));
		return FALSE;
	}

	if (strlen(src_path) > MTP_MAX_PATHNAME_SIZE - 1) {
		ERR("src_path is too long[%d]\n", strlen(src_path));
		return FALSE;
	}

	src_store = _device_get_store_containing_obj(obj->obj_handle);
	if (NULL == src_store) {
		ERR("Object not present in store");
		return FALSE;
	}

	_prop_init_ptparray(&child_arr, UINT32_TYPE);
	_entity_get_child_handles(src_store, obj->obj_handle, &child_arr);

	DBG("obj_handle[%d], src_path[%s], dest_path[%s], num elements[%ld]\n",
			obj->obj_handle, src_path, dest_path, child_arr.num_ele);

	for (idx = 0; idx < child_arr.num_ele; idx++) {
		mtp_uint32 *ptr32 = child_arr.array_entry;

		child_obj = _entity_get_object_from_store(src_store, ptr32[idx]);
		if (NULL == child_obj) {
			continue;
		}

		if (_util_is_file_opened(child_obj->file_path) == TRUE) {
			ERR_SECURE("File [%s] is already opened\n",
					child_obj->file_path);
			return FALSE;
		}

		g_strlcpy(temp_chld_path, child_obj->file_path,
				MTP_MAX_PATHNAME_SIZE + 1);
		_util_utf8_to_utf16(temp_chld_wpath,
				sizeof(temp_chld_wpath) / WCHAR_SIZ, temp_chld_path);
		if (_util_wchar_len(temp_chld_wpath) >
				MTP_MAX_PATHNAME_SIZE - 1) {
			ERR("Child Object Full Path is too long[%d]\n",
					strlen(child_obj->file_path));
			_prop_deinit_ptparray(&child_arr);
			return FALSE;
		}

		ptr = strstr(child_obj->file_path, src_path);
		if (NULL == ptr) {
			continue;
		}

		_util_utf8_to_utf16(dest_chld_wpath,
				sizeof(dest_chld_wpath) / WCHAR_SIZ, src_path);

		if (_util_wchar_len(dest_chld_wpath) <
				MTP_MAX_PATHNAME_SIZE - 1) {
			g_strlcpy(dest_chld_path, dest_path,
					MTP_MAX_PATHNAME_SIZE + 1);
		} else {
			ERR("dest_chld_wpath is too long[%d]\n",
					strlen(dest_path));
			_prop_deinit_ptparray(&child_arr);
			return FALSE;
		}

		ptr += strlen(src_path);
		if ((strlen(dest_chld_path) + strlen(ptr)) <
				MTP_MAX_PATHNAME_SIZE) {
			g_strlcat(dest_chld_path, ptr,
					MTP_MAX_PATHNAME_SIZE + 1);
		} else {
			ERR("dest_chld_path + ptr is too long[%d]\n",
					(strlen(ptr) + strlen(ptr)));
			_prop_deinit_ptparray(&child_arr);
			return FALSE;
		}
		DBG("dest_chld_path[%s], ptr[%s]\n", dest_chld_path, ptr);

		if (_entity_check_child_obj_path(child_obj,
					temp_chld_path, dest_chld_path) == FALSE) {
			ERR("set full path Fail");
			_prop_deinit_ptparray(&child_arr);
			return FALSE;
		}
	}

	_prop_deinit_ptparray(&child_arr);
	return TRUE;
}

mtp_bool _entity_set_child_object_path(mtp_obj_t *obj, mtp_char *src_path,
		mtp_char *dest_path)
{
	mtp_uint16 idx = 0;
	mtp_char *ptr = NULL;
	ptp_array_t child_arr = {0};
	mtp_obj_t *child_obj = NULL;
	mtp_store_t *src_store = NULL;
	mtp_uint32 *child_handle_arr = NULL;
	mtp_char dest_child_path[MTP_MAX_PATHNAME_SIZE + 1] = {0};
	mtp_char temp_child_path[MTP_MAX_PATHNAME_SIZE + 1] = {0};

	retv_if(NULL == obj, FALSE);
	retv_if(NULL == src_path, FALSE);
	retv_if(NULL == dest_path, FALSE);

	src_store = _device_get_store_containing_obj(obj->obj_handle);
	if (NULL == src_store) {
		ERR("Object not present in store");
		return FALSE;
	}

	_prop_init_ptparray(&child_arr, UINT32_TYPE);
	_entity_get_child_handles(src_store, obj->obj_handle, &child_arr);
	DBG("Object handle[%ld], src_path[%s], dest_path[%s], Numchild[%ld]\n",
			obj->obj_handle, src_path, dest_path, child_arr.num_ele);

	for (idx = 0; idx < child_arr.num_ele; idx++) {
		child_handle_arr = child_arr.array_entry;
		child_obj = _entity_get_object_from_store(src_store, child_handle_arr[idx]);
		if (NULL == child_obj)
			continue;
		DBG_SECURE("obj_handle[%ld], full path[%s]\n", child_obj->obj_handle,
				child_obj->file_path);
		g_strlcpy(temp_child_path, child_obj->file_path,
				sizeof(temp_child_path));

		ptr = strstr(child_obj->file_path, src_path);
		if (NULL == ptr)
			continue;

		g_strlcpy(dest_child_path, dest_path, sizeof(dest_child_path));

		ptr += strlen(src_path);
		if (g_strlcat(dest_child_path, ptr, sizeof(dest_child_path)) >=
				sizeof(dest_child_path)) {
			ERR("g_strlcat truncation occured,failed to create\
					dest_child_path");
			_entity_remove_reference_child_array(obj,
					child_obj->obj_handle);
			_entity_dealloc_mtp_obj(child_obj);
			continue;
		}
		_util_delete_file_from_db(child_obj->file_path);

		if (_entity_set_object_file_path(child_obj, dest_child_path,
					CHAR_TYPE) == FALSE) {
			ERR("Failed to set full path!!");
			_entity_remove_reference_child_array(obj,
					child_obj->obj_handle);
			_entity_dealloc_mtp_obj(child_obj);
			continue;
		}

		if (child_obj->obj_info == NULL) {
			ERR("obj_info is NULL");
			continue;
		}

		if ((child_obj->obj_info->obj_fmt == PTP_FMT_ASSOCIATION)) {
			if (_entity_set_child_object_path(child_obj, temp_child_path,
						dest_child_path) == FALSE) {
				ERR("Fail to set the full path!!");
				_entity_remove_reference_child_array(obj,
						child_obj->obj_handle);
				_entity_dealloc_mtp_obj(child_obj);
				continue;
			}
		}
	}

	_prop_deinit_ptparray(&child_arr);
	return TRUE;
}

mtp_bool _entity_add_reference_child_array(mtp_obj_t *obj, mtp_uint32 handle)
{
	if (_prop_find_ele_ptparray(&(obj->child_array), handle) ==
			ELEMENT_NOT_FOUND) {
		return (_prop_append_ele_ptparray(&(obj->child_array), handle));
	}

	return TRUE;
}

ptp_array_t *_entity_get_reference_child_array(mtp_obj_t *obj)
{
	return &(obj->child_array);
}

mtp_bool _entity_set_reference_child_array(mtp_obj_t *obj, mtp_uchar *buf,
		mtp_uint32 buf_sz)
{
	mtp_uint32 i = 0;
	mtp_uint32 no_ref = 0;
	mtp_uint32 ref_handle = 0;

	retv_if(NULL == buf, FALSE);

	/*Retrieve the number of references*/
	no_ref = buf_sz;

	/*Clean up the reference array*/
	_entity_remove_reference_child_array(obj, PTP_OBJECTHANDLE_ALL);

	/* Grow the array to accommodate all references*/
	if (_prop_grow_ptparray(&(obj->child_array), no_ref) == FALSE) {
		ERR("grow ptp Array Fail");
		return FALSE;
	}

	ref_handle = 0;
	for (i = 0; i < no_ref; i++) {
		memcpy(&ref_handle, buf, sizeof(mtp_uint32));
		buf += sizeof(mtp_uint32);
		_prop_append_ele_ptparray(&(obj->child_array), ref_handle);
	}

	return TRUE;
}

void _entity_copy_mtp_object(mtp_obj_t *dst, mtp_obj_t *src)
{
	/*Copy same information*/
	dst->obj_info = _entity_alloc_object_info();
	if (dst->obj_info == NULL) {
		ERR("Object info allocation Fail.");
		return;
	}
	_entity_copy_obj_info(dst->obj_info, src->obj_info);
	dst->obj_handle = 0;
	dst->file_path = NULL;

#ifndef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	_prop_update_property_values_list(dst);
#endif /* MTP_USE_RUNTIME_GETOBJECTPROPVALUE */
	return;
}

mtp_bool _entity_remove_reference_child_array(mtp_obj_t *obj, mtp_uint32 handle)
{
	if (handle == PTP_OBJECTHANDLE_ALL) {
		_prop_deinit_ptparray(&(obj->child_array));
		return TRUE;
	}
	return _prop_rem_elem_ptparray(&(obj->child_array), handle);
}

void _entity_dealloc_mtp_obj(mtp_obj_t *obj)
{
	mtp_uint16 ii = 0;
	slist_node_t *node = NULL;
	slist_node_t *next_node = NULL;

	ret_if(NULL == obj);

	if (obj->obj_info) {
		_entity_dealloc_obj_info(obj->obj_info);
		obj->obj_info = NULL;
	}

	_entity_remove_reference_child_array(obj, PTP_OBJECTHANDLE_ALL);

	for (ii = 0, next_node = obj->propval_list.start;
			ii < obj->propval_list.nnodes; ii++) {
		node = next_node;
		next_node = node->link;
		_prop_destroy_obj_propval((obj_prop_val_t *)node->value);
		g_free(node);
	}

	g_free(obj->file_path);
	g_free(obj);
	obj = NULL;
	return;
}
