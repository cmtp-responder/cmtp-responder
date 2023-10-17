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

#ifndef _MTP_CONFIG_H_
#define _MTP_CONFIG_H_

#include <stdbool.h>
#include <glib.h>

/* Set write-protection for read-only files */
/*#define MTP_SUPPORT_SET_PROTECTION*/

#define MTP_USE_RUNTIME_GETOBJECTPROPVALUE	/*use runtime get object property list*/
/* Support Features */

/* cancel transactio, device reset, mainly used in mtpmain.c */
#define MTP_SUPPORT_CONTROL_REQUEST

/*support get wmpinfo.xml from hidden file list */

/* external features */
#define MTP_SUPPORT_OBJECTADDDELETE_EVENT

/*For printing Commands sent by initiator
 *This needs to be diabled later before commercial binary release
 */
#define MTP_SUPPORT_PRINT_COMMAND

/* External Storage */
#define MTP_EXTERNAL_PATH_CHAR		"/media/card"

/*STORAGE*/
#define MTP_MAX_STORAGE			(30*1024*1024)	/*30MB */
#define MTP_MAX_STORAGE_IN_OBJTS	0xFFFFFFFF

#define MTP_BUF_SIZE_FOR_INT		11      /* 2^32 - 1 = 4294967295 (10 digits) */

#define MTP_STORAGE_DESC_EXT		"Card Storage"

/* about 976kbytes for object property value like sample data*/
#define MTP_MAX_PROP_DATASIZE          1000000

/*temporary file*/
#define MTP_TEMP_FILE			".mtptemp.tmp"

/*Unknown Metadata default name*/
#define MTP_UNKNOWN_METADATA		"Unknown"

/* strlen(/opt/usr/share/crash/) + MTP path len limit */
#define MTP_MAX_PATHNAME_SIZE		(21 + 1024)	/* except \0 */
#define MTP_MAX_FILENAME_SIZE		(1024)	/* except \0 */

#define MTP_MAX_CMD_BLOCK_SIZE		36	/* Bytes */

#define MTP_MAX_PACKET_SIZE_SEND_HS	512	/* High speed */
#define MTP_MAX_PACKET_SIZE_SEND_FS	64	/* Full speed */
#define MTP_FILESIZE_4GB			4294967296L

#define MTP_MAX_REG_STRING		128

#define MTP_SERIAL_LEN_MAX		32

/*
 *  User defined Configureations
 */
#define MTP_SUPPORT_OMADRM_EXTENSION

#define MTP_VENDOR_EXTENSIONDESC_CHAR	\
	"microsoft.com:1.0;android.com:1.0; "

#define MTP_MANUFACTURER_CHAR		"Collabora"

#define MTP_MMAP_THRESHOLD	524288
#define MTP_READ_USB_SIZE	4096
#define MTP_WRITE_USB_SIZE	4096
#define MTP_READ_FILE_SIZE	524288
#define MTP_WRITE_FILE_SIZE	524288
#define MTP_INIT_RX_IPC_SIZE	32768
#define MTP_INIT_TX_IPC_SIZE	262144
#define MTP_MAX_RX_IPC_SIZE	32768
#define MTP_MAX_TX_IPC_SIZE	262144
#define MTP_MAX_IO_BUF_SIZE	10485760	/* 10MB */
#define MTP_READ_FILE_DELAY	0		/* us */

#define MTP_SUPPORT_PTHREAD_SCHED	false
#define MTP_INHERITSCHED		'i'
#define MTP_SCHEDPOLICY			'o'
#define MTP_FILE_SCHEDPARAM		0
#define MTP_USB_SCHEDPARAM		0

#define MTP_CONFIG_FILE_PATH		"/etc/cmtp-responder.conf"

typedef struct {
	/* Speed related config */
	int mmap_threshold;	/* Max. 512KB. If requested memory is lesser than this, malloc is used. Otherwise, mmap is used */

	int read_usb_size;	/* USB read request size */
	int write_usb_size;	/* USB write request size */

	int read_file_size;	/* File read request size */
	int write_file_size;	/* File write request size */

	int init_rx_ipc_size;	/* Init. Rx(PC -> Phone) IPC size between USB and File threads */
	int init_tx_ipc_size;	/* Init. Tx(Phone -> PC) IPC size between USB and File threads */
	int max_rx_ipc_size;	/* Max. Rx(PC -> Phone) IPC size between USB and File threads */
	int max_tx_ipc_size;	/* Max. Tx(Phone -> PC) IPC size between USB and File threads */

	int max_io_buf_size;	/* Max. Heap memory size for buffer between USB and File threads */

	int read_file_delay;

	/* Experimental */
	bool support_pthread_sched;
	char inheritsched;	/* i : Inherit, e : Explicit */
	char schedpolicy;	/* f : FIFO, r : Round Robin, o : Other */
	int file_schedparam;	/* File I/O thread's priority for scheduling */
	int usb_schedparam;	/* USB I/O thread's priority for scheduling */

	/* Experimental (End) */
	/* Speed related config (End) */

	/* MTP Features */
	GString *external_path;
	GString *device_info_vendor_extension_desc;
	GString *device_info_manufacturer;
	GString *device_info_model;
	GString *device_info_device_version;
	GString *device_info_serial_number;
	/* MTP Features (End) */

	/* Vendor Features */
	/* Features (End) */

	bool is_init;
} mtp_config_t;

#endif	/* _MTP_CONFIG_H_ */
