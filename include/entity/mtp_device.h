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

#ifndef _MTP_DEVICE_H_
#define _MTP_DEVICE_H_

#include "mtp_store.h"
#include "mtp_property.h"

#define MTP_NUM_DEVICE_DEFAULT		3

#define MTP_NUM_DEVICE_WMP11		1

#ifdef MTP_SUPPORT_DEVICEPROP_BATTERYLEVEL
#define MTP_NUM_DEVICE_BATTLEVEL	1
#else /*MTP_SUPPORT_DEVICEPROP_BATTERYLEVEL*/
#define MTP_NUM_DEVICE_BATTLEVEL	0
#endif /*MTP_SUPPORT_DEVICEPROP_BATTERYLEVEL*/

#ifdef MTP_SUPPORT_DEVICE_CLASS
#define MTP_NUM_DEVICE_DSC		1
#else /*MTP_SUPPORT_DEVICE_CLASS*/
#define MTP_NUM_DEVICE_DSC		0
#endif /*MTP_SUPPORT_DEVICE_CLASS*/

#define MTP_NUM_DEVICE_SVC		0

/* currently not supported */
#define MTP_NUM_DEVICE_PLAYBACK_2WIRE	4
#define MTP_NUM_DEVICE_OMADRM		1

#define NUM_DEVICE_PROPERTIES		(MTP_NUM_DEVICE_DEFAULT + \
		MTP_NUM_DEVICE_WMP11 + \
		MTP_NUM_DEVICE_BATTLEVEL + MTP_NUM_DEVICE_DSC + \
		MTP_NUM_DEVICE_SVC)

/*This number can be changed based on MAX number or stores allowed*/
#define	MAX_NUM_DEVICE_STORES		3

#define MTP_STANDARD_VERSION		0x64
#define MTP_VENDOR_EXTN_ID		0x06
#define MTP_VENDOR_EXTN_VERSION		0x64


/* This enumeration specifies the type of store. */
typedef enum {
	MTP_STORAGE_INTERNAL = 0,
	MTP_STORAGE_EXTERNAL,
	MTP_STORAGE_ALL
} store_type_t;


/* Defines the status of the Device */
typedef enum {
	DEVICE_STATUSOK = 0,		/* Device OK */
	DEVICE_DEVICEERROR = 4		/* Fatal device error, can't continue */
} device_status_t;

/* Defines the phase in which device is*/
typedef enum {
	DEVICE_PHASE_NOTREADY = 0,	/* busy state */
	DEVICE_PHASE_IDLE = 1,		/* idle state */
	DEVICE_PHASE_DATAIN = 3,	/* data-in phase */
	DEVICE_PHASE_DATAOUT = 4,	/* data out phase */
	DEVICE_PHASE_RESPONSE = 5	/* response phase */
} device_phase_t;

/*
 * device_info_t structure
 * brief Structure for MTP device information.
 */
typedef struct {
	mtp_uint16 std_version;		/* Version of the MTP spec supported */
	mtp_uint32 vendor_extn_id;	/* Vendor extension ID */
	mtp_uint16 vendor_extn_version;	/* Vendor extension version */
	ptp_string_t vendor_extn_desc;	/* Vendor extension description */
	mtp_uint16 functional_mode;	/* Funtional mode. */
	mtp_uint16 *ops_supported;	/* Array of operations supported */
	mtp_uint16 *events_supported;	/* Array of events supported */
	mtp_uint16 *device_prop_supported;/* Array of device prop supported */
	mtp_uint16 *capture_fmts;	/* Array of captured fmts supported */
	mtp_uint16 *object_fmts;	/* Array of file fmts supported */
	ptp_string_t manufacturer;	/* The manufacturer's name. */
	ptp_string_t model;		/* model name string */
	ptp_string_t device_version;	/* Device version string */
	ptp_string_t serial_no;		/* The serial number string. */
} device_info_t;

/*
 * MTP device structure.
 * A structure containing an instance of the MTP device.
 */
typedef struct {
	device_status_t status;		/* device status */
	device_phase_t phase;		/* device phase */
	device_info_t device_info;	/* Device information */
	mtp_store_t *store_list;	/* pointer to List of stores */
	mtp_uchar num_stores;		/* Number of valid stores */
	/* Used when SendObjectInfo does not specift store_id */
	mtp_uint32 default_store_id;
	/* Used when SendObjectInfo doesn't specify Parent Object Handle */
	mtp_uint32 default_hparent;
	/* A pointer to array of device property. */
	device_prop_desc_t *device_prop_list;
	mtp_char device_name[MTP_MAX_REG_STRING + 1];
	mtp_char sync_partner[MTP_MAX_REG_STRING + 1];
	mtp_bool is_mounted[MTP_STORAGE_ALL];
} mtp_device_t;

/*
 * void _init_mtp_device(void)
 * This function initializes mtp device structure.
 * @return	none.
 */
void _init_mtp_device(void);

/*
 * mtp_uint32 _get_device_info_size()
 * This function returns the size of device_info structure
 * @return	returns the size of this device_info.
 */
mtp_uint32 _get_device_info_size(void);

/*
 * mtp_uint32 _pack_device_info(mtp_uchar *buf, mtp_uint32 buf_sz)
 * This Functions Fills the device_info structure
 * @param[in]	buf	address of a buffer
 * @param[in]	buf_sz	size of the buffer
 * @return the length of useful data in the buffer.
 */
mtp_uint32 _pack_device_info(mtp_uchar *buf, mtp_uint32 buf_sz);

/*
 * void _reset_mtp_device()
 * This functions resets device state to IDLE/Command Ready
 * @return	none
 */
void _reset_mtp_device(void);

/*
 * device_prop_desc_t *_device_get_device_property(mtp_uint32 prop_code)
 * This function will get the property with the input property code
 * @param[in]	prop_code	Property code
 * @return	pointer to the property if success else NULL.
 */
device_prop_desc_t *_device_get_device_property(mtp_uint32 prop_code);

mtp_bool _device_is_store_mounted(mtp_int32 store_type);

/*
 * mtp_bool _device_install_storage(mtp_int32 type)
 * This function add the storage.
 * @param[in]	type	Specifies the Storage : AUTO, INTERNAL, EXTERNAL, ALL
 * @return	If success, returns TRUE. Otherwise returns FALSE.
 */
mtp_bool _device_install_storage(mtp_int32 type);

/* mtp_bool _device_uninstall_storage(mtp_int32 type)
 * This function removes the storage.
 * @param[in]	type	Specifies the Storage : AUTO, INTERNAL, EXTERNAL, ALL
 * @return	If success, returns TRUE. Otherwise returns FALSE.
 */
mtp_bool _device_uninstall_storage(mtp_int32 type);

/*
 * mtp_store_t *_device_get_store(mtp_uint32 store_id)
 * This function will get the store with store_id.
 * @param[in]	store_id	ID of the store
 * @return	the pointer to the store with store ID, otherwise NULL.
 */
mtp_store_t *_device_get_store(mtp_uint32 store_id);

/*
 * mtp_uint32 _device_get_store_ids(ptp_array_t *store_ids)
 * This function returns a list of storage ID's of all stores in the device.
 * @param[out]	store_ids		reference to the store ID list
 * @return	the number of Storage IDs in the list.
 */
mtp_uint32 _device_get_store_ids(ptp_array_t *store_ids);

/*
 * mtp_uint32 _device_get_num_objects(mtp_uint32 store_id)
 * This functions will get the number of objects in the store
 * @param[in]	store_id	storage id
 * @return	the number of objects.
 */
mtp_uint32 _device_get_num_objects(mtp_uint32 store_id);

/*
 * mtp_uint32 _device_get_num_objects_with_format(mtp_uint32 store_id,
 * mtp_uint32 format)
 * This function will get the number of objects with certain format code in the
 * specified store
 * @param[in]	store_id	storage id
 * @param[in]	format	format code of the objects
 * @return	the number of objects with the formatcode.
 */
mtp_uint32 _device_get_num_objects_with_format(mtp_uint32 store_id,
		mtp_uint32 format);

/*
 * mtp_obj_t *_device_get_object_with_handle(mtp_uint32 obj_handle)
 * This fuctions returns the object with the obj_handle
 * @param[in]	obj_handle	object handle of the desired object
 * @return	the object pointer if find; otherwise NULL;
 */
mtp_obj_t *_device_get_object_with_handle(mtp_uint32 obj_handle);

/*
 * mtp_obj_t *_device_get_object_with_path(mtp_char *full_path)
 * This function return the pointer to the object if it exists.
 * @param[in]	full_path		points to the object full path
 * @return	the object pointer if found; otherwise NULL;
 */
mtp_obj_t *_device_get_object_with_path(mtp_char *full_path);

/*
 * mtp_uint16 _device_delete_object(mtp_uint32 obj_handle, mtp_uint32 fmt)
 * This function will delete the objects entry corresponding to the
 * object handle and the formatcode.
 * @param[in]	obj_handle	Object handle number
 * @param[in]	fmt		Object formatCode
 * @return	the response of the delete.
 */
mtp_uint16 _device_delete_object(mtp_uint32 obj_handle, mtp_uint32 fmt);

/*
 * mtp_store_t *_device_get_store_containing_obj(mtp_uint32 obj_handle)
 * This function will return the pointer to the store where the the object is.
 * @param[in]	obj_handle	Object handle number
 * @return	the store pointer if find, otherwise NULL.
 */
mtp_store_t *_device_get_store_containing_obj(mtp_uint32 obj_handle);
mtp_store_t *_device_get_store_at_index(mtp_uint32 index);
device_prop_desc_t *_device_get_ref_prop_list(void);

/*
 * Get current playback object from device property
 */
mtp_bool _device_get_playback_obj(mtp_uint32 *playback_obj);

/*
 * Set current playback object in device property,
 * and send out property change event.
 */
mtp_bool _device_set_playback_obj(mtp_uint32 playback_obj);
void _device_get_serial(mtp_char *serial_no, mtp_uint32 len);
void _device_set_phase(device_phase_t phase);
device_phase_t _device_get_phase(void);
mtp_uint32 _device_get_default_store_id(void);
mtp_uint32 _device_get_default_parent_handle(void);
mtp_uint32 _device_get_num_stores(void);
device_status_t _device_get_status(void);
void _device_set_device_name(mtp_char *dev_name);
mtp_char *_device_get_device_name(void);
mtp_char *_device_get_sync_partner(void);
void _device_set_sync_partner(mtp_char *sync_ptr);

#endif /* _MTP_DEVICE_H_ */
