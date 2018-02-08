/*
 * Copyright (c) 2012, 2013, 2018 Samsung Electronics Co., Ltd.
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

#include "mtp_descs_strings.h"

struct mtp_usb_descs descriptors = {
	.header = {
		.magic = cpu_to_le32(FUNCTIONFS_DESCRIPTORS_MAGIC_V2),
		.length = cpu_to_le32(sizeof(descriptors)),
		.flags = FUNCTIONFS_HAS_FS_DESC | FUNCTIONFS_HAS_HS_DESC, // | FUNCTIONFS_HAS_MS_OS_DESC,
		.fs_count = 4,
		.hs_count = 4,
		// .os_count = 0;
	},
	.fs_descs = {
		// drivers/usb/gadget/f_mtp_slp.c:207
		.intf = {
			.bLength = sizeof(descriptors.fs_descs.intf),
			.bDescriptorType = USB_DT_INTERFACE,
			.bNumEndpoints = 3,
			.bInterfaceClass = USB_CLASS_STILL_IMAGE,
			.bInterfaceSubClass = 1,
			.bInterfaceProtocol = 1,
			.iInterface = 1,
		},
		.bulk_in = {
			.bLength = USB_DT_ENDPOINT_SIZE,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 1 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = __constant_cpu_to_le16(64),
		},
		.bulk_out = {
			.bLength = USB_DT_ENDPOINT_SIZE,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 2 | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = __constant_cpu_to_le16(64),
		},
		.int_in = {
			.bLength = USB_DT_ENDPOINT_SIZE,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 3 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_INT,
			.wMaxPacketSize = __constant_cpu_to_le16(64),
			.bInterval = 6,
		},
	},
	.hs_descs = {
		.intf = {
			.bLength = sizeof(descriptors.fs_descs.intf),
			.bDescriptorType = USB_DT_INTERFACE,
			.bNumEndpoints = 3,
			.bInterfaceClass = USB_CLASS_STILL_IMAGE,
			.bInterfaceSubClass = 1,
			.bInterfaceProtocol = 1,
			.iInterface = 1,
		},
		.bulk_in = {
			.bLength = USB_DT_ENDPOINT_SIZE,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 1 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = __constant_cpu_to_le16(512),
		},
		.bulk_out = {
			.bLength = USB_DT_ENDPOINT_SIZE,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 2 | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = __constant_cpu_to_le16(512),
		},
		.int_in = {
			.bLength = USB_DT_ENDPOINT_SIZE,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 3 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_INT,
			.wMaxPacketSize = __constant_cpu_to_le16(64),
			.bInterval = 6,
		},
	},
};

struct mtp_usb_strs strings = {
	.header = {
		.magic = cpu_to_le32(FUNCTIONFS_STRINGS_MAGIC),
		.length = cpu_to_le32(sizeof(strings)),
		.str_count = cpu_to_le32(1),
		.lang_count = cpu_to_le32(1),
	},
	.lang0 = {
		cpu_to_le16(0x0409), /* en-us */
		STR_INTERFACE,
	},
};
