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

#ifndef _MTP_STORE_H_
#define _MTP_STORE_H_

#include "mtp_object.h"


/* First six members of SStorageInfo structure */
#define FIXED_LENGTH_MEMBERS_MTPSTORE_SIZE \
	(sizeof(mtp_uint16) * 3 + sizeof(mtp_uint64) * 2 + sizeof(mtp_uint32))

typedef enum {
	MTPSTORE_WMPINFO_PHONE = 1,
	MTPSTORE_WMPINFO_CARD
} store_special_obj_t;

/*
 * store_info
 * This structure stores store information for each store
 */
typedef struct {
	mtp_uint16 store_type;	/*storage type*/
	mtp_uint16 fs_type;	/*file system type*/
	mtp_uint16 access;	/*access capability(read/write)*/
	mtp_uint64 capacity;	/*maximum capacity in bytes*/
	mtp_uint64 free_space;	/*free space in bytes*/
	mtp_uint32 free_space_in_objs;	/*free space in objects*/
	ptp_string_t store_desc;
	ptp_string_t vol_label;	/*optional volume label: variable length*/
} store_info_t;

/*
 * MTP store structure.
 * This structure is instantiated for each store within the MTP device.
 */
typedef struct {
	mtp_char *root_path;	/* Root path of the store */
	mtp_uint32 store_id;
	store_info_t store_info;
	slist_t obj_list;
	mtp_bool is_hidden;	/*for hidden storage*/
} mtp_store_t;

typedef struct {
	mtp_uint32 h_parent;
	mtp_store_t *store;
	ptp_array_t child_data;
} missing_child_data_t;

typedef enum {
	MTP_INTERNAL_STORE_ID = 0x10001,
	MTP_EXTERNAL_STORE_ID = 0x20001
} mtp_store_id_t;

typedef enum {
	MTP_ADDREM_AUTO,
	MTP_ADDREM_INTERNAL,
	MTP_ADDREM_EXTERNAL,
	MTP_ADDREM_ALL
} add_rem_store_t;

void _entity_update_store_info_run_time(store_info_t *info,
		mtp_char *root_path);
mtp_bool _entity_get_store_path_by_id(mtp_uint32 store_id, mtp_char *path);
mtp_uint32 _entity_get_store_info_size(store_info_t *info);
mtp_uint32 _entity_pack_store_info(store_info_t *info, mtp_uchar *buf,
		mtp_uint32 buf_sz);
mtp_uint32 _entity_get_store_id_by_path(const mtp_char *path_name);
mtp_bool _entity_init_mtp_store(mtp_store_t *store, mtp_uint32 store_id,
		mtp_char *store_path);
mtp_obj_t *_entity_add_file_to_store(mtp_store_t *store, mtp_uint32 h_parent,
		mtp_char *file_path, mtp_char *file_name, dir_entry_t *file_info);
mtp_obj_t *_entity_add_folder_to_store(mtp_store_t *store, mtp_uint32 h_parent,
		mtp_char *file_path, mtp_char *file_name, dir_entry_t *file_info);
mtp_bool _entity_add_object_to_store(mtp_store_t *store, mtp_obj_t *obj);
mtp_obj_t *_entity_get_object_from_store(mtp_store_t *store, mtp_uint32 handle);
mtp_obj_t *_entity_get_last_object_from_store(mtp_store_t *store,
		mtp_uint32 handle);
mtp_obj_t *_entity_get_object_from_store_by_path(mtp_store_t *store,
		mtp_char *file_path);
mtp_uint32 _entity_get_objects_from_store(mtp_store_t *store,
		mtp_uint32 obj_handle, mtp_uint32 fmt, ptp_array_t *obj_arr);
mtp_uint32 _entity_get_objects_from_store_till_depth(mtp_store_t *store,
		mtp_uint32 obj_handle, mtp_uint32 fmt_code, mtp_uint32 depth,
		ptp_array_t *obj_arr);
mtp_uint32 _entity_get_objects_from_store_by_format(mtp_store_t *store,
		mtp_uint32 format, ptp_array_t *obj_arr);
mtp_uint32 _entity_get_num_object_with_same_format(mtp_store_t *store,
		mtp_uint32 format);
mtp_uint32 _entity_get_num_children(mtp_store_t *store, mtp_uint32 h_parent,
		mtp_uint32 format);
mtp_uint32 _entity_get_child_handles(mtp_store_t *store, mtp_uint32 h_parent,
		ptp_array_t *child_arr);
mtp_uint32 _entity_get_child_handles_with_same_format(mtp_store_t *store,
		mtp_uint32 h_parent, mtp_uint32 format, ptp_array_t *child_arr);
mtp_bool _entity_remove_object_mtp_store(mtp_store_t *store, mtp_obj_t *obj,
		mtp_uint32 format, mtp_uint16 *response, mtp_bool *atleast_one,
		mtp_bool read_only);
mtp_uint16 _entity_delete_obj_mtp_store(mtp_store_t *store,
		mtp_uint32 obj_handle, mtp_uint32 fmt, mtp_bool read_only);
mtp_uint32 _entity_get_object_tree_size(mtp_store_t *store, mtp_obj_t *obj);
mtp_bool _entity_check_if_B_parent_of_A(mtp_store_t *store,
		mtp_uint32 handleA, mtp_uint32 handleB);
mtp_uint32 _entity_generate_next_obj_handle(void);
mtp_uint16 _entity_format_store(mtp_store_t *store, mtp_uint32 fs_format);
void _entity_destroy_mtp_store(mtp_store_t *store);
void _entity_store_recursive_enum_folder_objects(mtp_store_t *store,
		mtp_obj_t *pobj);
void _entity_list_modified_files(mtp_uint32 minutes);
void _entity_copy_store_data(mtp_store_t *dst, mtp_store_t *src);

#endif /* _MTP_STORE_H_ */
