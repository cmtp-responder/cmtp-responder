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
#include <unistd.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <glib.h>
#include <glib-object.h>
#include <malloc.h>
#include <vconf.h>

#include "mtpResponder.h"

using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;

#ifdef TIZEN_TEST_GCOV
extern "C" void __gcov_flush(void);
#endif

TEST(MtpResponder_t, mtp_init_p0)
{
	int ret = 0;
	MtpResponder mtp;
	add_rem_store_t sel = MTP_ADDREM_AUTO;

	ret = mtp.mtp_init(sel);

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, mtp_init_p1)
{
	int ret = 0;
	MtpResponder mtp;
	add_rem_store_t sel = MTP_ADDREM_INTERNAL;

	ret = mtp.mtp_init(sel);

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, mtp_init_p2)
{
	int ret = 0;
	MtpResponder mtp;
	add_rem_store_t sel = MTP_ADDREM_EXTERNAL;

	ret = mtp.mtp_init(sel);

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, mtp_init_p3)
{
	int ret = 0;
	MtpResponder mtp;
	add_rem_store_t sel = MTP_ADDREM_ALL;

	ret = mtp.mtp_init(sel);

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, mtp_deinit_p0)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.mtp_deinit();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_storage_entry_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_storage_entry();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_storage_ids_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_storage_ids();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_device_property_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_device_property();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_set_device_property_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_set_device_property();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_reset_device_entry_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_reset_device_entry();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_add_object_entry_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_add_object_entry();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_remove_object_entry_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_remove_object_entry();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_object_entry_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_object_entry();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_copy_object_entries_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_copy_object_entries();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_move_object_entry_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_move_object_entry();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_duplicate_object_entry_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_duplicate_object_entry();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_read_file_data_from_offset_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_read_file_data_from_offset();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_write_file_data_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_write_file_data();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_object_entry_size_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_object_entry_size();

	EXPECT_NE(ERROR_NONE, ret);
}

#ifdef MTP_SUPPORT_SET_PROTECTION
TEST(MtpResponder_t, hutil_set_protection_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_set_protection();

	EXPECT_NE(ERROR_NONE, ret);
}
#endif /* MTP_SUPPORT_SET_PROTECTION */

TEST(MtpResponder_t, hutil_get_num_objects_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_num_objects();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_object_handles_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_object_handles();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_construct_object_entry_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_construct_object_entry();

	EXPECT_EQ(ERROR_NONE, ret);
}

//========================================

TEST(MtpResponder_t, hutil_construct_object_entry_prop_list_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_construct_object_entry_prop_list();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_object_prop_value_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_object_prop_value();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_update_object_property_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_update_object_property();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_prop_desc_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_prop_desc();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_object_prop_supported_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_object_prop_supported();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_object_prop_list_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_object_prop_list();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_add_object_references_enhanced_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_add_object_references_enhanced();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_remove_object_reference_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_remove_object_reference();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_object_references_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_object_references();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_number_of_objects_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_number_of_objects();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_interdep_prop_config_list_size_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_interdep_prop_config_list_size();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_interdep_prop_config_list_data_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_interdep_prop_config_list_data();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_playback_skip_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_playback_skip();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_format_storage_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_format_storage();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hutil_get_storage_info_size_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hutil_get_storage_info_size();

	EXPECT_EQ(ERROR_NONE, ret);
}

//================== _MTP_CMD_HANDLER_H_ ======================

TEST(MtpResponder_t, cmd_hdlr_send_response_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.cmd_hdlr_send_response();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, cmd_hdlr_send_response_code_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.cmd_hdlr_send_response_code();

	EXPECT_EQ(ERROR_NONE, ret);
}

//================== _MTP_EVENT_HANDLER_H_ ======================

TEST(MtpResponder_t, hdlr_get_param_cmd_container_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_get_param_cmd_container();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_add_param_resp_container_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_add_param_resp_container();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_validate_cmd_container_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_validate_cmd_container();

	EXPECT_NE(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_alloc_buf_data_container_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_alloc_buf_data_container();

	EXPECT_EQ(ERROR_NONE, ret);
}

/*
TEST(MtpResponder_t, hdlr_send_data_container_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_send_data_container();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_send_bulk_data_n)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_send_bulk_data();

	EXPECT_EQ(ERROR_NONE, ret);
}
*/
TEST(MtpResponder_t, hdlr_rcv_data_container_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_rcv_data_container();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_get_payload_size_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_get_payload_size();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_get_payload_data_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_get_payload_data();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_resp_container_init_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_resp_container_init();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_init_event_container_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_init_event_container();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_init_event_container_with_param_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_init_event_container_with_param();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_conv_cmd_container_byte_order_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_conv_cmd_container_byte_order();

	EXPECT_EQ(ERROR_NONE, ret);
}

TEST(MtpResponder_t, hdlr_conv_data_container_byte_order_p)
{
	int ret = 0;
	MtpResponder mtp;

	ret = mtp.hdlr_conv_data_container_byte_order();

	EXPECT_EQ(ERROR_NONE, ret);
}

//mtp_bool _hdlr_rcv_file_in_data_container(data_container_t *dst, mtp_char *filepath, mtp_uint32 path_len);



//========================================

int main(int argc, char **argv)
{
#ifdef TIZEN_TEST_GCOV
	setenv("GCOV_PREFIX", "/tmp", 1);
#endif

	InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();

#ifdef TIZEN_TEST_GCOV
	__gcov_flush();
#endif
	return ret;
}





