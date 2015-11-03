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

#ifndef _MTP_PROPERTY_H_
#define _MTP_PROPERTY_H_

#include "mtp_list.h"
#include "mtp_object.h"

#define MAX_SIZE_IN_BYTES_OF_PROP_VALUE		16000
#define ELEMENT_NOT_FOUND			-1
#define INITIAL_ARRAY_SIZE			100
#define NUM_OBJECT_PROP_DESC			38
#define NUM_OBJECT_PROP_DESC_DEFAULT		13
#define NUM_OBJECT_PROP_DESC_MP3		14
#define NUM_OBJECT_PROP_DESC_WMA		14
#define MTP_HEAP_CAL_INTERVAL			100

#ifdef MTP_SUPPORT_PROPERTY_SAMPLE
#define NUM_OBJECT_PROP_DESC_WMV		28
#else /* MTP_SUPPORT_PROPERTY_SAMPLE */
#define NUM_OBJECT_PROP_DESC_WMV		22
#endif /* MTP_SUPPORT_PROPERTY_SAMPLE */

#ifdef MTP_SUPPORT_PROPERTY_SAMPLE
#define NUM_OBJECT_PROP_DESC_ALBUM		8
#else /* MTP_SUPPORT_PROPERTY_SAMPLE */
#define NUM_OBJECT_PROP_DESC_ALBUM		2
#endif /* MTP_SUPPORT_PROPERTY_SAMPLE */

typedef struct _interdep_prop_config {
	mtp_uint32 format_code;
	slist_t propdesc_list;
} interdep_prop_config_t;

/*
 * brief The property form flag.
 * This enumerated type defines what kind of data the structure contains.
 */
typedef enum {
	NONE = 0x00,
	RANGE_FORM = 0x01,
	ENUM_FORM = 0x02,
	DATE_TIME_FORM = 0x03,
	REGULAR_EXPRESSION_FORM = 0x04,
	BYTE_ARRAY_FORM = 0x05,
	LONG_STRING_FORM = 0xFF,
} form_flag_t;

enum {
	GROUP_CODE_NONE = 0x00000000,
	GROUP_CODE_SYNC_PROPS = 0x00000001,
	GROUP_CODE_UI_PROPS = 0x00000002,
	GROUP_CODE_OBJ_Info = 0x00000004,
	GROUP_CODE_OFTEN_USED = 0x00000008,
	GROUP_CODE_SUPPLEMENTAL = 0x00000100,
	GROUP_CODE_UNKNOWN_PROP = 0x00010000,
	GROUP_CODE_SLOW = 0x00FF0000,
	GROUP_CODE_ALL = 0xFFFFFFFF
};

#define MTP_PROP_GROUPCODE_GENERAL	GROUP_CODE_OFTEN_USED
#define MTP_PROP_GROUPCODE_OBJECT	GROUP_CODE_OFTEN_USED
#define MTP_PROP_GROUPCODE_ALBUMART	GROUP_CODE_OFTEN_USED

#define node_alloc_and_append()\
	do {\
		mtp_bool ret;\
		ret = _util_add_node(&(obj->propval_list), (void *)prop_val);\
		if (FALSE == ret) {\
			_prop_destroy_obj_propval(prop_val);\
			ERR("_util_add_node() Fail");\
			return (FALSE);\
		}\
	} while (0);

#define propvalue_alloc_and_check(prop_val)\
	do {\
		prop_val = _prop_alloc_obj_propval(prop);\
		if (prop_val == NULL) {\
			ERR("prop_val == NULL");\
			return (FALSE);\
		} \
	} while (0);

#define init_default_value(value)\
	do {\
		memset(&default_val, 0, sizeof(default_val));\
		default_val = value;\
	} while (0);

typedef struct {
	mtp_uint32 min_val;	/* Minimum value */
	mtp_uint32 max_val;	/* Maximum value */
	mtp_uint32 step_size;	/* Step value */
} range_form_t;

/* This structure contains property info */
typedef struct {
	mtp_uint16 prop_code;	/* Property code */
	mtp_uint16 data_type;	/* type of the data */
	mtp_uint32 dts_size;	/* Count of Byte Size of the
							   Data-Type-Specific value (DTS) */
	mtp_uchar get_set;	/* Set possible or not */
	mtp_uchar form_flag;	/* Indicates the form of the valid values */
	range_form_t range;	/* Range Values */
	slist_t supp_value_list; /*Enum Values */
	union {
		mtp_uchar integer[16];	/* Default value for any integer value
								   type (UINT8, mtp_uint16, mtp_uint32) */
		ptp_string_t *str;	/* Default value for String type */
		ptp_array_t *array;	/* Default array */
	} default_val;		/* Default value */
} prop_info_t;

/* This structure contains device property description dataset */
typedef struct {
	prop_info_t propinfo;
	union {
		mtp_uchar integer[16];	/* Current value for any integer value
								   type (UINT8, mtp_uint16, mtp_uint32) */
		ptp_string_t *str;	/* Current value for String type */
		ptp_array_t *array;	/* Current value for array data type
							   (AINT8, AUINT8) */
	} current_val;                      /*Current value */
} device_prop_desc_t;

/* This structure contains object property description dataset */
typedef struct {
	prop_info_t propinfo;
	mtp_uint32 group_code;	/* Identifies the group of this property */
	union {
		ptp_string_t *reg_exp;  /* Regular Expression Form */
		mtp_uint32 max_len; /* LongString Form */
	} prop_forms;            /*Object property-specific forms */
} obj_prop_desc_t;

/* This structure contains current value of a object property */
typedef struct {
	obj_prop_desc_t *prop;
	union {
		mtp_uchar integer[16];	/* Current value for any integer */
		ptp_string_t *str;	/* Current value for String type */
		ptp_array_t *array;
	} current_val;
} obj_prop_val_t;

/* This structure defines ObjectPropQuad for GetObjectPropList */
typedef struct {
	mtp_uint32 obj_handle;
	mtp_uint16 prop_code;
	mtp_uint16 data_type;
	mtp_uchar *pval;
	mtp_uint32 val_size;	/* a useful helper,
							   not part of the Object Property Quadruple */
} prop_quad_t;

/* This structure contains a list of MTP Object properties */
typedef struct {
	slist_t prop_quad_list;
} obj_proplist_t;

/* This structure contains a list of InterdependentProperties  */
typedef struct {
	slist_t plist;
} obj_interdep_proplist_t ;

/*
 * Ptparray functions
 */
void _prop_init_ptparray(ptp_array_t *parray, data_type_t type);
ptp_array_t *_prop_alloc_ptparray(data_type_t type);
mtp_uint32 _prop_get_size_ptparray(ptp_array_t *parray);
mtp_uint32 _prop_get_size_ptparray_without_elemsize(ptp_array_t *parray);
mtp_bool _prop_grow_ptparray(ptp_array_t *parray, mtp_uint32 new_size);
mtp_int32 _prop_find_ele_ptparray(ptp_array_t *parray, mtp_uint32 element);
mtp_bool _prop_get_ele_ptparray(ptp_array_t *parray, mtp_uint32 index, void *ele);
mtp_bool _prop_append_ele_ptparray(ptp_array_t *parray, mtp_uint32 element);
mtp_bool _prop_append_ele128_ptparray(ptp_array_t *parray, mtp_uint64 *element);
mtp_bool _prop_copy_ptparray(ptp_array_t *dst, ptp_array_t *src);
mtp_uint32 _prop_pack_ptparray(ptp_array_t *parray, mtp_uchar *buf,
		mtp_uint32 bufsize);
mtp_uint32 _prop_pack_ptparray_without_elemsize(ptp_array_t *parray,
		mtp_uchar *buf, mtp_uint32 bufsize);
mtp_bool _prop_rem_elem_ptparray(ptp_array_t *parray, mtp_uint32 element);
void _prop_deinit_ptparray(ptp_array_t *parray);
void _prop_destroy_ptparray(ptp_array_t *parray);

/*
 * PtpString Functions
 */
void _prop_init_ptpstring(ptp_string_t *pstring);
void _prop_init_ptptimestring(ptp_time_string_t *pstring);
void _prop_copy_time_to_ptptimestring(ptp_time_string_t *pString,
		system_time_t *sys_time);
void _prop_copy_ptpstring(ptp_string_t *dst, ptp_string_t *src);
void _prop_copy_ptptimestring(ptp_time_string_t *dst, ptp_time_string_t *src);
void _prop_copy_char_to_ptpstring(ptp_string_t *pstring, void *str,
		char_mode_t cmode);
mtp_uint32 _prop_size_ptpstring(ptp_string_t *pstring);
mtp_uint32 _prop_size_ptptimestring(ptp_time_string_t *pstring);
mtp_uint32 _prop_pack_ptpstring(ptp_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size);
mtp_uint32 _prop_pack_ptptimestring(ptp_time_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size);
mtp_uint32 _prop_parse_rawstring(ptp_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size);
void _prop_destroy_ptpstring(ptp_string_t *pstring);

/*
 * DevicePropDesc Functions
 */
void _prop_init_device_property_desc(device_prop_desc_t *prop,
		mtp_uint16 propcode, mtp_uint16 data_type, mtp_uchar get_set,
		mtp_uchar form_flag);
mtp_uint32 _prop_size_device_prop_desc(device_prop_desc_t *prop);
mtp_uint32 _prop_pack_device_prop_desc(device_prop_desc_t *prop,
		mtp_uchar *buf, mtp_uint32 size);
mtp_uint32 _prop_pack_curval_device_prop_desc(device_prop_desc_t *prop,
		mtp_uchar *buf, mtp_uint32 size);
void _prop_reset_device_prop_desc(device_prop_desc_t *prop);

/*
 * ObjectPropVal Functions
 */
obj_prop_val_t *_prop_alloc_obj_propval(obj_prop_desc_t *prop);
obj_prop_val_t *_prop_get_prop_val(mtp_obj_t *obj, mtp_uint32 prop_code);
mtp_uint32 _prop_pack_obj_propval(obj_prop_val_t *val, mtp_uchar *buf,
		mtp_uint32 size);
mtp_uint32 _prop_size_obj_propval(obj_prop_val_t *val);
void _prop_destroy_obj_propval(obj_prop_val_t *pval);
mtp_bool _prop_set_default_integer(prop_info_t *prop_info, mtp_uchar *value);
mtp_bool _prop_set_default_string(prop_info_t *prop_info, mtp_wchar *val);
mtp_bool _prop_set_range_integer(prop_info_t *prop_info, mtp_uint32 min,
		mtp_uint32 max, mtp_uint32 step);
mtp_bool _prop_set_maxlen(obj_prop_desc_t *prop, mtp_uint32 max);
mtp_bool _prop_set_default_array(prop_info_t *prop_info, mtp_uchar *parray,
		mtp_uint32 num_ele);
mtp_bool _prop_add_supp_integer_val(prop_info_t *prop_info, mtp_uint32 val);
mtp_bool _prop_add_supp_string_val(prop_info_t *prop_info, mtp_wchar *val);
mtp_bool _prop_is_valid_integer(prop_info_t *prop_info, mtp_uint64 value);
mtp_bool _prop_is_valid_string(prop_info_t *prop, ptp_string_t *str);
mtp_bool _prop_is_equal_ptpstring(ptp_string_t *dst, ptp_string_t *src);
mtp_bool _prop_set_current_integer(device_prop_desc_t *prop, mtp_uint32 val);
mtp_bool _prop_set_current_string(device_prop_desc_t *prop, ptp_string_t *str);
mtp_bool _prop_set_current_array(device_prop_desc_t *prop, mtp_uchar *arr);
mtp_bool _prop_set_current_device_prop(device_prop_desc_t *prop, mtp_uchar *val,
		mtp_uint32 size);
mtp_bool _prop_set_current_integer_val(obj_prop_val_t *propval, mtp_uint64 val);
mtp_bool _prop_set_current_string_val(obj_prop_val_t *propval, ptp_string_t *str);
mtp_bool _prop_set_current_array_val(obj_prop_val_t *propval, mtp_uchar *arr,
		mtp_uint32 size);
#ifdef __BIG_ENDIAN__
mtp_bool _prop_set_current_array_val_usbrawdata(obj_prop_val_t *val,
		mtp_uchar *arr, mtp_uint32 size);
#endif /* __BIG_ENDIAN__ */
mtp_bool _prop_set_regexp(obj_prop_desc_t *prop, mtp_wchar *regex);

/*
 * ObjectPropDesc Functions
 */
mtp_uint32 _prop_size_obj_prop_desc(obj_prop_desc_t *prop);
mtp_uint32 _prop_pack_obj_prop_desc(obj_prop_desc_t *prop, mtp_uchar *buf,
		mtp_uint32 size);
mtp_uint32 _prop_pack_default_val_obj_prop_desc(obj_prop_desc_t *prop,
		mtp_uchar *buf, mtp_uint32 size);
obj_prop_desc_t *_prop_get_obj_prop_desc(mtp_uint32 format_code,
		mtp_uint32 prop_code);

/*
 * ObjectProplist Functions
 */
mtp_bool _prop_update_property_values_list(mtp_obj_t *obj);
mtp_uint32 _prop_size_obj_proplist(obj_proplist_t *prop_list);
mtp_uint32 _prop_get_obj_proplist(mtp_obj_t *pObject, mtp_uint32 prop_code,
		mtp_uint32 group_code, obj_proplist_t *prop_list);
mtp_uint32 _prop_pack_obj_proplist(obj_proplist_t *prop_list, mtp_uchar *buf,
		mtp_uint32 size);
void _prop_destroy_obj_proplist(obj_proplist_t *prop_list);

/*
 * ObjectProp Functions
 */
mtp_uint32 _prop_get_supp_obj_props(mtp_uint32 format_code,
		ptp_array_t *supp_props);
mtp_bool _prop_build_supp_props_mp3(void);
mtp_bool _prop_build_supp_props_wma(void);
mtp_bool _prop_build_supp_props_wmv(void);
mtp_bool _prop_build_supp_props_album(void);
mtp_bool _prop_build_supp_props_default(void);
void _prop_destroy_supp_obj_props(void);

/*
 *Interdependent Prop Functions
 */
mtp_uint32 _prop_get_size_interdep_prop(interdep_prop_config_t *prop);
mtp_uint32 _prop_pack_interdep_prop(interdep_prop_config_t *prop, mtp_uchar *buf,
		mtp_uint32 size);
mtp_uint32 _prop_get_size_interdep_proplist(obj_interdep_proplist_t *config_list,
		mtp_uint32 format_code);
mtp_uint32 _prop_pack_interdep_proplist(obj_interdep_proplist_t *config_list,
		mtp_uint32 format_code, mtp_uchar *buf, mtp_uint32 size);
mtp_bool _get_oma_drm_status(void);

#endif /* _MTP_PROPERTY_H_ */
