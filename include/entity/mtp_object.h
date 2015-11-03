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

#ifndef _MTP_OBJECT_H_
#define _MTP_OBJECT_H_

#include "mtp_list.h"
#include "ptp_datacodes.h"
#include "mtp_fs.h"
#include "mtp_media_info.h"

#define FIXED_LENGTH_MEMBERS_SIZE \
	(4 * sizeof(mtp_uint16) + 11 * sizeof(mtp_uint32))

/*
 * MAX size for sendObjectInfo
 * SIZE_IN_BYTES_OF_FIXED_LENGTH_MEMBERS + 4 * (sizeof(ptp_string_t)) = 2096
 */
#define MAX_SIZE_IN_BYTES_OF_OBJECT_INFO	(4000*8)

/*
 * obj_info_t structure : Contains object metadata related information
 * Size = 26 Bytes
 */
typedef struct {
	mtp_uint32 store_id;		/* The storage the object resides */
	mtp_uint16 obj_fmt;		/* object format code */
	mtp_uint16 protcn_status;	/* object protection status */
	mtp_uint64 file_size;		/* object compressed size */
	mtp_uint32 thumb_file_size;	/* thumbnail compressedsize */
	mtp_uint32 h_parent;		/* parent object handle */
	mtp_uint16 association_type;	/* association type */
} obj_info_t;

/*
 * mtp_obj_t structure : Contains object related information
 * Gets created for each enumerated file/dir on the device
 * Size : 42 Bytes
 */
typedef struct {
	mtp_uint32 obj_handle;
	obj_info_t *obj_info;
	mtp_char *file_path;
	ptp_array_t child_array;	/* Include all the renferences */
	slist_t propval_list;	/* Object Properties implemented */
} mtp_obj_t;

mtp_bool _entity_get_file_times(mtp_obj_t *obj, ptp_time_string_t *create_tm,
		ptp_time_string_t *modify_tm);
obj_info_t *_entity_alloc_object_info(void);
void _entity_init_object_info(obj_info_t *info);
mtp_uint32 _entity_get_object_info_size(mtp_obj_t *obj, ptp_string_t *file_name);
void _entity_init_object_info_params(obj_info_t *obj_info, mtp_uint32 dwStore,
		mtp_uint32 hParent, mtp_char *pszFilename, dir_entry_t *fileInfo);
mtp_uint32 _entity_parse_raw_obj_info(mtp_uchar *buf, mtp_uint32 buf_sz,
		obj_info_t *info, mtp_char *file_name, mtp_uint16 fname_len);
void _entity_copy_obj_info(obj_info_t *dst, obj_info_t *src);
mtp_uint32 _entity_pack_obj_info(mtp_obj_t *obj, ptp_string_t *file_name,
		mtp_uchar *buf, mtp_uint32 buf_sz);
void _entity_dealloc_obj_info(obj_info_t *info);
mtp_obj_t *_entity_alloc_mtp_object(void);
mtp_bool _entity_init_mtp_object_params(
		mtp_obj_t *obj,
		mtp_uint32 store_id,
		mtp_uint32 h_parent,
		mtp_char *file_path,
		mtp_char *file_name,
		dir_entry_t *file_info);
mtp_bool _entity_set_object_file_path(mtp_obj_t *obj, void *file_path,
		char_mode_t char_type);
mtp_bool _entity_check_child_obj_path(mtp_obj_t *obj, mtp_char *src_path,
		mtp_char *dest_path);
mtp_bool _entity_set_child_object_path(mtp_obj_t *obj, mtp_char *src_path,
		mtp_char *dest_path);
mtp_bool _entity_add_reference_child_array(mtp_obj_t *obj, mtp_uint32 handle);
ptp_array_t* _entity_get_reference_child_array(mtp_obj_t *obj);
mtp_bool _entity_set_reference_child_array(mtp_obj_t *obj, mtp_uchar *buf,
		mtp_uint32 buf_sz);
void _entity_copy_mtp_object(mtp_obj_t *dst, mtp_obj_t *src);
mtp_bool _entity_remove_reference_child_array(mtp_obj_t *obj, mtp_uint32 handle);
void _entity_dealloc_mtp_obj(mtp_obj_t *obj);

#endif /* _MTP_OBJECT_H_ */
