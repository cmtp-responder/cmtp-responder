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
#ifndef __MTP_RESPONDER_H__
#define __MTP_RESPONDER_H__

#include <glib.h>
#include <gio/gio.h>

#include "mtpconf.h"
#include "mtp_init.h"

#include "mtp_thread.h"
#include "mtp_support.h"
#include "mtp_device.h"
#include "mtp_event_handler.h"
#include "mtp_cmd_handler.h"
#include "mtp_cmd_handler_util.h"
#include "mtp_inoti_handler.h"
#include "mtp_transport.h"
#include "mtp_util.h"
#include "mtp_media_info.h"
#include "mtp_usb_driver.h"
#include "mtp_descs_strings.h"

class MtpResponder {
	private:
	public:
		MtpResponder(void);
		~MtpResponder(void);

		int mtp_init(add_rem_store_t sel);
		int mtp_deinit(void);
		int hutil_get_storage_entry(void);
		int hutil_get_storage_ids(void);
		int hutil_get_device_property(void);
		int hutil_set_device_property(void);
		int hutil_reset_device_entry(void);
		int hutil_add_object_entry(void);
		int hutil_remove_object_entry(void);
		int hutil_get_object_entry(void);
		int hutil_copy_object_entries(void);
		int hutil_move_object_entry(void);
		int hutil_duplicate_object_entry(void);
		int hutil_read_file_data_from_offset(void);
		int hutil_write_file_data(void);
		int hutil_get_object_entry_size(void);
		int hutil_set_protection(void);
		int hutil_get_num_objects(void);
		int hutil_get_object_handles(void);
		int hutil_construct_object_entry(void);

//========================================

		int hutil_construct_object_entry_prop_list(void);
		int hutil_get_object_prop_value(void);
		int hutil_update_object_property(void);
		int hutil_get_prop_desc(void);
		int hutil_get_object_prop_supported(void);
		int hutil_get_object_prop_list(void);
		int hutil_add_object_references_enhanced(void);
		int hutil_remove_object_reference(void);
		int hutil_get_object_references(void);
		int hutil_get_number_of_objects(void);
		int hutil_get_interdep_prop_config_list_size(void);
		int hutil_get_interdep_prop_config_list_data(void);
		int hutil_get_playback_skip(void);
		int hutil_format_storage(void);
		int hutil_get_storage_info_size(void);

//================== _MTP_CMD_HANDLER_H_ ======================

		int cmd_hdlr_send_response(void);
		int cmd_hdlr_send_response_code(void);

//================== _MTP_EVENT_HANDLER_H_ ======================

		int hdlr_get_param_cmd_container(void);
		int hdlr_add_param_resp_container(void);
		int hdlr_validate_cmd_container(void);
		int hdlr_alloc_buf_data_container(void);
		int hdlr_send_data_container(void);
		int hdlr_send_bulk_data(void);
		int hdlr_rcv_data_container(void);
		int hdlr_get_payload_size(void);
		int hdlr_get_payload_data(void);
		int hdlr_resp_container_init(void);
		int hdlr_init_event_container(void);
		int hdlr_init_event_container_with_param(void);
		int hdlr_conv_cmd_container_byte_order(void);
		int hdlr_conv_data_container_byte_order(void);





};
#endif /* __MTP_RESPONDER_H__ */












