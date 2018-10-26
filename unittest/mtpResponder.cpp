/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mtpResponder.h"

MtpResponder::MtpResponder(void)
{

}

MtpResponder::~MtpResponder(void)
{
	_mtp_deinit();
}


int MtpResponder::mtp_init(add_rem_store_t sel)
{
	_mtp_init(sel);

	return ERROR_NONE;
}

int MtpResponder::mtp_deinit(void)
{
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	_mtp_deinit();

	return ERROR_NONE;
}

int MtpResponder::hutil_get_storage_entry(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_get_storage_entry(0, NULL);

	return ret;
}

int MtpResponder::hutil_get_storage_ids(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ptp_array_t store_ids = {UINT32_TYPE, 0, 0, NULL};

	ret = _hutil_get_storage_ids(&store_ids);

	return ret;
}

int MtpResponder::hutil_get_device_property(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_get_device_property(0, NULL);

	return ret;
}

int MtpResponder::hutil_set_device_property(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_set_device_property(0, NULL, 0);

	return ret;
}

int MtpResponder::hutil_reset_device_entry(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_reset_device_entry(0);

	return ret;
}

int MtpResponder::hutil_add_object_entry(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_add_object_entry(NULL, NULL, NULL);

	return ret;
}

int MtpResponder::hutil_remove_object_entry(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_remove_object_entry(0, 0);

	return ret;
}

int MtpResponder::hutil_get_object_entry(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_obj_t *ptr_mtp_obj = NULL;
	ret = _hutil_get_object_entry(0, &ptr_mtp_obj);

	return ret;
}

int MtpResponder::hutil_copy_object_entries(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint32 new_hobj = 0;
	ret = _hutil_copy_object_entries(0, 0, 0, 0, &new_hobj, 0);

	return ret;
}

int MtpResponder::hutil_move_object_entry(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_move_object_entry(0, 0, 0);

	return ret;
}

int MtpResponder::hutil_duplicate_object_entry(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint32 new_handle = 0;
	ret = _hutil_duplicate_object_entry(0, 0, 0, &new_handle);

	return ret;
}

int MtpResponder::hutil_read_file_data_from_offset(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint32 data_sz = 0;
	mtp_char data[] = "aaaaaaaaaa";
	ret = _hutil_read_file_data_from_offset(0, (off_t)0, (void*)data, &data_sz);

	return ret;
}

int MtpResponder::hutil_write_file_data(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_char fpath[] = "/Phone/DCIM/abc.txt";
	ret = _hutil_write_file_data(0, NULL, fpath);

	return ret;
}

int MtpResponder::hutil_get_object_entry_size(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint64 obj_sz = 0;
	ret = _hutil_get_object_entry_size(0, &obj_sz);

	return ret;
}

#ifdef MTP_SUPPORT_SET_PROTECTION
int MtpResponder::hutil_set_protection(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_set_protection(0, 0);

	return ret;
}
#endif /* MTP_SUPPORT_SET_PROTECTION */

int MtpResponder::hutil_get_num_objects(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint32 num_obj = 0;
	ret = _hutil_get_num_objects(0, 0, 0, &num_obj);

	return ret;
}

int MtpResponder::hutil_get_object_handles(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ptp_array_t handle_arr = {UINT32_TYPE, 0, 0, NULL};
	ret = _hutil_get_object_handles(0, 0, 0, &handle_arr);

	return ret;
}

int MtpResponder::hutil_construct_object_entry(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	obj_data_t obj_data = {0, 0, NULL};
	mtp_obj_t *mtp_obj = NULL;
	mtp_char data[] = "aaaaaaaaa";
	ret = _hutil_construct_object_entry(0, 0, &obj_data, &mtp_obj, (void*)data, 0);

	return ret;
}

//========================================

int MtpResponder::hutil_construct_object_entry_prop_list(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	obj_data_t obj_data = {0, 0, NULL};
	mtp_obj_t *mtp_obj = NULL;
	mtp_char data[] = "aaaaaaaaa";
	mtp_uint32 err_idx = 0;
	ret = _hutil_construct_object_entry_prop_list(0, 0, 0, 0, &obj_data, &mtp_obj, (void*)data, 0, &err_idx);

	return ret;
}

int MtpResponder::hutil_get_object_prop_value(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_obj_t *mtp_obj = NULL;
	ret = _hutil_get_object_prop_value(0, 0, NULL, &mtp_obj);

	return ret;
}

int MtpResponder::hutil_update_object_property(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint16 data_type = 0;
	mtp_char buf[] = "aaaaaa";
	mtp_uint32 prop_sz = 0;
	ret = _hutil_update_object_property(0, 0, &data_type, (void*)buf, 0, &prop_sz);

	return ret;
}

int MtpResponder::hutil_get_prop_desc(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_char data[] = "aaaaaa";
	ret = _hutil_get_prop_desc(0, 0, &data);

	return ret;
}

int MtpResponder::hutil_get_object_prop_supported(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ptp_array_t prop_arr = {UINT32_TYPE, 0, 0, NULL};

	ret = _hutil_get_object_prop_supported(0, &prop_arr);

	return ret;
}

int MtpResponder::hutil_get_object_prop_list(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ptp_array_t obj_arr = {UINT32_TYPE, 0, 0, NULL};

	#ifdef MTP_USE_RUNTIME_GETOBJECTPROPVALUE
	ret = _hutil_get_object_prop_list(0, 0, 0, 0, 0, NULL, &obj_arr);
	#else /* MTP_USE_RUNTIME_GETOBJECTPROPVALUE */
	ret = _hutil_get_object_prop_list(0, 0, 0, 0, 0, NULL);
	#endif /* MTP_USE_RUNTIME_GETOBJECTPROPVALUE */

	return ret;
}

int MtpResponder::hutil_add_object_references_enhanced(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uchar buffer[] = "aaaaaaaa";
	ret = _hutil_add_object_references_enhanced(0, buffer, 0);

	return ret;
}

int MtpResponder::hutil_remove_object_reference(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_remove_object_reference(0, 0);

	return ret;
}

int MtpResponder::hutil_get_object_references(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ptp_array_t parray = {UINT32_TYPE, 0, 0, NULL};
	mtp_uint32 num_ele = 0;
	ret = _hutil_get_object_references(0, &parray, &num_ele);

	return ret;
}

int MtpResponder::hutil_get_number_of_objects(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint32 num_obj = 0;
	ret = _hutil_get_number_of_objects(0, &num_obj);

	return ret;
}

int MtpResponder::hutil_get_interdep_prop_config_list_size(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint32 list_sz = 0;
	ret = _hutil_get_interdep_prop_config_list_size(&list_sz, 0);

	return ret;
}

int MtpResponder::hutil_get_interdep_prop_config_list_data(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_char data[] = "aaaaaaaa";
	ret = _hutil_get_interdep_prop_config_list_data(data, 0, 0);

	return ret;
}

int MtpResponder::hutil_get_playback_skip(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_get_playback_skip(0);

	return ret;
}

int MtpResponder::hutil_format_storage(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _hutil_format_storage(0, 0);

	return ret;
}

int MtpResponder::hutil_get_storage_info_size(void)
{
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	int ret_size = -1;
	store_info_t store_info;
	ret_size = _hutil_get_storage_info_size(&store_info);

	if (ret_size >= 0)
		return ERROR_NONE;
	return ERROR_OPERATION_FAILED;
}

//================== _MTP_CMD_HANDLER_H_ ======================

int MtpResponder::cmd_hdlr_send_response(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	mtp_uint32 params = 0;
	ret = _cmd_hdlr_send_response(NULL, 0, 0, &params);

	return ret;
}

int MtpResponder::cmd_hdlr_send_response_code(void)
{
	int ret = 0;
	add_rem_store_t sel = MTP_ADDREM_AUTO;
	_mtp_init(sel);

	ret = _cmd_hdlr_send_response_code(NULL, 0);

	return ret;
}

//================== _MTP_EVENT_HANDLER_H_ ======================

int MtpResponder::hdlr_get_param_cmd_container(void)
{
	int ret = 0;

	cmd_container_t cntr;
	mtp_uint32 index = 0;
	_hdlr_init_cmd_container(&cntr);

	ret = _hdlr_get_param_cmd_container(&cntr, index);

	return ret;
}

int MtpResponder::hdlr_add_param_resp_container(void)
{
	int ret = 0;

	cmd_container_t cntr;
	cmd_container_t dst;
	mtp_uint32 param = 0;
	_hdlr_init_cmd_container(&cntr);
	_hdlr_copy_cmd_container(&cntr, &dst);

	ret = _hdlr_add_param_resp_container(&dst, 0, &param);

	return (!ret);
}

int MtpResponder::hdlr_validate_cmd_container(void)
{
	int ret = 0;

	cmd_container_t cntr;
	_hdlr_init_cmd_container(&cntr);

	ret = _hdlr_validate_cmd_container((mtp_uchar*)&cntr, 0);

	return (!ret);
}

int MtpResponder::hdlr_alloc_buf_data_container(void)
{
	data_container_t dst;
	_hdlr_init_data_container(&dst, 0, 0);

	mtp_uchar *ptr = NULL;
	ptr = _hdlr_alloc_buf_data_container(&dst, 0, 0);

	if (ptr == NULL)
		return ERROR_OPERATION_FAILED;
	return ERROR_NONE;
}

int MtpResponder::hdlr_send_data_container(void)
{
	int ret = 0;

	data_container_t dst;
	_hdlr_init_data_container(&dst, 0, 0);
	_hdlr_alloc_buf_data_container(&dst, 0, 0);

	ret = _hdlr_send_data_container(&dst);

	return ret;
}

int MtpResponder::hdlr_send_bulk_data(void)
{
	int ret = 0;

	data_container_t dst;
	_hdlr_init_data_container(&dst, 0, 0);

	ret = _hdlr_send_bulk_data((mtp_uchar*)&dst, sizeof(data_container_t));

	return ret;
}

int MtpResponder::hdlr_rcv_data_container(void)
{
	int ret = 0;

	data_container_t dst;
	_hdlr_init_data_container(&dst, 0, 0);

	ret = _hdlr_rcv_data_container(&dst, 0);

	return ret;
}

int MtpResponder::hdlr_get_payload_size(void)
{
	int payload_size = -1;

	data_container_t dst;
	_hdlr_init_data_container(&dst, 0, 0);

	payload_size = _hdlr_get_payload_size(&dst);

	if (payload_size >= 0)
		return ERROR_NONE;
	return ERROR_OPERATION_FAILED;
}

int MtpResponder::hdlr_get_payload_data(void)
{
	mtp_uchar *ptr = NULL;

	data_container_t dst;
	_hdlr_init_data_container(&dst, 0, 0);
	_hdlr_alloc_buf_data_container(&dst, 0, 0);

	ptr = _hdlr_get_payload_data(&dst);

	if (ptr != NULL)
		return ERROR_NONE;
	return ERROR_OPERATION_FAILED;
}

int MtpResponder::hdlr_resp_container_init(void)
{
	cmd_container_t dst;
	_hdlr_resp_container_init(&dst, 0, 0);

	return ERROR_NONE;
}

int MtpResponder::hdlr_init_event_container(void)
{
	cmd_container_t dst;
	_hdlr_init_event_container(&dst, 0, 0, 0, 0);

	return ERROR_NONE;
}

int MtpResponder::hdlr_init_event_container_with_param(void)
{
	cmd_container_t dst;
	_hdlr_init_event_container_with_param(&dst, 0, 0, 0, 0);

	return ERROR_NONE;
}

int MtpResponder::hdlr_conv_cmd_container_byte_order(void)
{
	cmd_container_t dst;
	_hdlr_conv_cmd_container_byte_order(&dst);

	return ERROR_NONE;
}

int MtpResponder::hdlr_conv_data_container_byte_order(void)
{
	data_container_t dst;
	_hdlr_conv_data_container_byte_order(&dst);

	return ERROR_NONE;
}






