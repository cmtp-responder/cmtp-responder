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

#ifndef _MTP_CONFIG_H_
#define _MTP_CONFIG_H_

#include <stdbool.h>

/* Function Features */
#define MTP_USE_INFORMATION_REGISTRY		/* for get and set some information value */

/* Set write-protection for read-only files */
/*#define MTP_SUPPORT_SET_PROTECTION*/

/*MtpObject.c, for unknown metadata */
#define MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN

/*define MTP_USE_VARIABLE_PTP_STRING_MALLOC*/ 	/*allocPtpString in ptpstring.c*/
#define MTP_USE_RUNTIME_GETOBJECTPROPVALUE	/*use runtime get object property list*/

/*keywords has many space. not support*/
/*#define MTP_USE_OBJPROPERTY_KEYWORDS*/
/*#define MTP_USE_SELFMAKE_ABSTRACTION*/

/*after db loading(mass files), there are some duplicate open session packet*/
/*#define MTP_USE_SKIP_CONTINUOUS_OPENSESSION*/

/* Support Features */

/* cancel transactio, device reset, mainly used in mtpmain.c */
#define MTP_SUPPORT_CONTROL_REQUEST

#define MTP_SUPPORT_ALBUM_ART		/* album art image file */
/*#define MTP_SUPPORT_DELETE_MEDIA_ALBUM_FILE*/

/*support battery level using mtp_pal_phone_get_status(MTP_PAL_BATTLEVEL) */
#define MTP_SUPPORT_DEVICEPROP_BATTERYLEVEL

/*support get wmpinfo.xml from hidden file list */
/*#define MTP_SUPPORT_HIDE_WMPINFO_XML*/

/*support mtp command GetInterdependentPropDesc (0x9807) */
/*#define MTP_SUPPORT_INTERDEPENDENTPROP*/

/*mobile class. this should be set with ms os descriptor. see portable device installation consideration */
#define MTP_SUPPORT_DEVICE_CLASS

/*#define MTP_SUPPORT_WMV_ENCODINGPROFILE */		/* for WMV encoding profile */

/*Representive Sample Properties are not supported by either music player or libmm-fileinfo */
#ifdef MTP_SUPPORT_ALBUM_ART
/*#define MTP_SUPPORT_PROPERTY_SAMPLE*/
#endif/*MTP_SUPPORT_ALBUM_ART*/

/* external features */
#define MTP_SUPPORT_OBJECTADDDELETE_EVENT

/*For printing Commands sent by initiator
 *This needs to be diabled later before commercial binary release
 */
#define MTP_SUPPORT_PRINT_COMMAND

/*
 * Transport related configuration
 */
/* Internal Storage */
#define MTP_STORE_PATH_CHAR		"/opt/usr/media"

/* External Storage */
#define MTP_EXTERNAL_PATH_CHAR		"/opt/storage/sdcard"
#define MTP_DEVICE_ICON			"/usr/share/mtp/device_icon.ico"

/* File For WMP extesions */
#define MTP_FILES_MODIFIED_FILES	"/tmp/mtp_mod_files.log"

/* Base MTP USER Directory */
#define MTP_USER_DIRECTORY		"/opt/usr/media/user"

/* Hidden Root directory */
#define MTP_HIDDEN_PHONE		".HiddenPhone"
#define MTP_HIDDEN_CARD			".HiddenCard"

/* Windows Media Player xml file */
#define MTP_FILE_NAME_WMPINFO_XML	"WMPInfo.xml"

/* MTP extension name */
#define MTP_EXT_NAME_REGISTRY		"ini"

/* special extension */
#define MTP_ALBUMFILE_EXTENSION		"alb"

/* Reference Key name */
#define MTP_REF_KEYNAME_CHAR		"Reference"

/*STORAGE*/
#define MTP_MAX_STORAGE			(30*1024*1024)	/*30MB */
#define MTP_MAX_STORAGE_IN_OBJTS	0xFFFFFFFF
#define MTP_MAX_INT_OBJECT_NUM		10000000
#define MTP_MIN_EXT_OBJECT_NUM		5000000

#define MTP_BUF_SIZE_FOR_INT		11      /* 2^32 - 1 = 4294967295 (10 digits) */

/*To determine whether command is for Samsung mobile*/
#define MTP_SAMSUNG_MOBILE		0x1000
#define MTP_STORAGE_DESC_INT		"Phone"
#define MTP_STORAGE_DESC_EXT		"Card"

/*Devices Property*/
#define MTP_DEFAULT_MODEL_NAME			"SAMSUNG Mobile"
#define MTP_DEFAULT_DEVICE_VERSION		""
#define MTP_DEV_PROPERTY_SYNCPARTNER		"Longhorn Sync Engine"
#define MTP_DEV_PROPERTY_FRIENDLYNAME		"SAMSUNG Mobile"
#define MTP_DEV_PROPERTY_NULL_SYNCPARTNER	"{00000000-0000-0000-0000-000000000000}"

/*temporary file*/
#define MTP_TEMP_FILE			".mtptemp.tmp"
#define MTP_TEMP_FILE_DEFAULT		"/tmp/.mtptemp.tmp"

/*Unknown Metadata default name*/
#define MTP_UNKNOWN_METADATA		"Unknown"

/*2006.10.20 empty metadata folder problem*/
#define MTP_UNKNOWN_METADATAW		L"Unknown"

/* strlen(/opt/usr/share/crash/) + MTP path len limit */
#define MTP_MAX_PATHNAME_SIZE		(21 + 255)	/* except \0 */
#define MTP_MAX_FILENAME_SIZE		(254)	/* except \0 */

#define MTP_MAX_CMD_BLOCK_SIZE		36	/* Bytes */

#define MTP_MAX_PACKET_SIZE_SEND_HS	512	/* High speed */
#define MTP_MAX_PACKET_SIZE_SEND_FS	64	/* Full speed */
#define MTP_FILESIZE_4GB 		4294967296L

/* approximately 3 times of media files. consider album*/
#define MTP_MAX_REFDB_ROWCNT		4500
#define MTP_MAX_EXTENSION_LENGTH	11
#define MTP_MAX_REG_STRING		128

#define MTP_SERIAL_LEN_MAX		32
#define MD5_HASH_LEN			16
#define MTP_MODEL_NAME_LEN_MAX		32
#define MTP_DEVICE_NAME_LEN_MAX		64

/*
 *  User defined Configureations
 */
/*#define MTP_USE_DEPEND_DEFAULT_MEMORY*/
#define MTP_SUPPORT_OMADRM_EXTENSION

/* Image Height/Width */
#define MTP_MAX_IMG_WIDTH		32672
#define MTP_MAX_IMG_HEIGHT		32672

#define MTP_MAX_VIDEO_WIDTH		1920
#define MTP_MAX_VIDEO_HEIGHT		1080

#define MTP_MIN_VIDEO_WIDTH		0
#define MTP_MIN_VIDEO_HEIGHT		0

/* about 976kbytes for object property value like sample data*/
#define MTP_MAX_PROP_DATASIZE		1000000

#define MTP_MAX_METADATA		204800	/* 200KB */

#define MTP_VENDOR_EXTENSIONDESC_CHAR	\
	"microsoft.com/WMDRMPD:10.1;microsoft.com/playready:1.10; "

#define MTP_MANUFACTURER_CHAR		"Tizen"
#define HAVE_OWN_WCHAR_CONVERSION

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

#define MTP_CONFIG_FILE_PATH		"/opt/var/lib/misc/mtp-responder.conf"

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
	/* MTP Features (End) */

	/* Vendor Features */
	/* Features (End) */

	bool is_init;
} mtp_config_t;

#endif	/* _MTP_CONFIG_H_ */
