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

#include <glib.h>
#include "mtp_property.h"
#include "mtp_support.h"
#include "mtp_transport.h"

/*
 * EXTERN AND GLOBAL VARIABLES
 */
obj_interdep_proplist_t interdep_proplist;
/*
 * STATIC VARIABLES
 */
static obj_prop_desc_t props_list_default[NUM_OBJECT_PROP_DESC_DEFAULT];

/*
 * FUNCTIONS
 */
mtp_uint16 __get_ptp_array_elem_size(data_type_t type)
{
	switch (type) {
	case UINT8_TYPE:
		return 1;
	case UINT16_TYPE:
		return 2;
	case PTR_TYPE:
	case UINT32_TYPE:
		return 4;
	case UINT128_TYPE:
		return 16;
	default:
		;
	}

	return 0;
}

static mtp_bool __check_object_propcode(obj_prop_desc_t *prop,
		mtp_uint32 propcode, mtp_uint32 group_code)
{
	if ((prop->propinfo.prop_code == propcode) ||
			((propcode == PTP_PROPERTY_ALL) &&
			 !(prop->group_code & GROUP_CODE_SLOW)) ||
			((propcode == PTP_PROPERTY_UNDEFINED) &&
			 (prop->group_code & group_code))) {
		return TRUE;
	}

	return FALSE;
}

static mtp_bool __create_prop_integer(mtp_obj_t *obj,
		mtp_uint16 propcode, mtp_uint64 value)
{
	obj_prop_desc_t *prop = NULL;
	obj_prop_val_t *prop_val = NULL;
	mtp_uint32 fmt_code = obj->obj_info->obj_fmt;

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	prop = _prop_get_obj_prop_desc(fmt_code, propcode);
	retvm_if(!prop, FALSE, "Create property Fail.. Prop = [0x%X]\n", propcode);

	propvalue_alloc_and_check(prop_val);
	_prop_set_current_integer_val(prop_val, value);
	node_alloc_and_append();

	return TRUE;
}

static mtp_bool __create_prop_string(mtp_obj_t *obj, mtp_uint16 propcode,
		mtp_wchar *value)
{
	ptp_string_t ptp_str = {0};
	obj_prop_desc_t *prop = NULL;
	obj_prop_val_t *prop_val = NULL;
	mtp_uint32 fmt_code = obj->obj_info->obj_fmt;

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	prop = _prop_get_obj_prop_desc(fmt_code, propcode);
	retvm_if(!prop, FALSE, "Create property Fail.. Prop = [0x%X]\n", propcode);

	propvalue_alloc_and_check(prop_val);
	_prop_copy_char_to_ptpstring(&ptp_str, value, WCHAR_TYPE);
	_prop_set_current_string_val(prop_val, &ptp_str);
	node_alloc_and_append();

	return TRUE;
}

static mtp_bool __create_prop_timestring(mtp_obj_t *obj,
	mtp_uint32 propcode, ptp_time_string_t *value)
{
	obj_prop_desc_t *prop = NULL;
	obj_prop_val_t *prop_val = NULL;
	mtp_uint32 fmt_code = obj->obj_info->obj_fmt;

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	prop = _prop_get_obj_prop_desc(fmt_code, propcode);
	retvm_if(!prop, FALSE, "Create property Fail.. Prop = [0x%X]\n", propcode);

	propvalue_alloc_and_check(prop_val);
	_prop_set_current_string_val(prop_val, (ptp_string_t *)value);
	node_alloc_and_append();

	return TRUE;
}

static mtp_bool __create_prop_array(mtp_obj_t *obj, mtp_uint16 propcode,
		mtp_char *arr, mtp_uint32 size)
{
	obj_prop_desc_t *prop = NULL;
	obj_prop_val_t *prop_val = NULL;
	mtp_uint32 fmt_code = obj->obj_info->obj_fmt;

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	prop = _prop_get_obj_prop_desc(fmt_code, propcode);
	retvm_if(!prop, FALSE, "Create property Fail.. Prop = [0x%X]\n", propcode);

	propvalue_alloc_and_check(prop_val);
	_prop_set_current_array_val(prop_val, (mtp_uchar *)arr, size);
	node_alloc_and_append();

	return TRUE;
}
/* LCOV_EXCL_STOP */

/* PTP Array Functions */
void _prop_init_ptparray(ptp_array_t *parray, data_type_t type)
{
	mtp_uint16 size = 0;
	parray->num_ele = 0;
	parray->type = type;

	size = __get_ptp_array_elem_size(type);
	if (size == 0)
		return;

	parray->array_entry = g_malloc(size * INITIAL_ARRAY_SIZE);
	if (parray->array_entry == NULL) {
		parray->arr_size = 0;
	} else {
		parray->arr_size = INITIAL_ARRAY_SIZE;
		memset(parray->array_entry, 0, size * INITIAL_ARRAY_SIZE);
	}
}

ptp_array_t *_prop_alloc_ptparray(data_type_t type)
{
	ptp_array_t *parray;
	parray = (ptp_array_t *)g_malloc(sizeof(ptp_array_t));
	if (parray != NULL)
		_prop_init_ptparray(parray, type);

	return (parray);
}

/* LCOV_EXCL_START */
mtp_uint32 _prop_get_size_ptparray(ptp_array_t *parray)
{
	mtp_uint16 size = 0;

	retvm_if(!parray, 0, "ptp_array_t is NULL\n");

	size = __get_ptp_array_elem_size(parray->type);
	if (size == 0)
		return 0;

	return (sizeof(mtp_uint32) + (size *parray->num_ele));
}

/* LCOV_EXCL_STOP */

mtp_bool _prop_grow_ptparray(ptp_array_t *parray, mtp_uint32 new_size)
{
	mtp_uint16 size = 0;

	size = __get_ptp_array_elem_size(parray->type);
	if (size == 0)
		return FALSE;

	if (parray->arr_size == 0)
		_prop_init_ptparray(parray, parray->type);

	if (new_size < parray->arr_size)
		return TRUE;

	/* LCOV_EXCL_START */
	parray->array_entry =
		g_realloc(parray->array_entry, size * new_size);
	if (parray->array_entry == NULL) {
		parray->arr_size = 0;
		return FALSE;
	}
	parray->arr_size = new_size;

	return TRUE;
	/* LCOV_EXCL_STOP */
}

mtp_int32 _prop_find_ele_ptparray(ptp_array_t *parray, mtp_uint32 element)
{
	mtp_uchar *ptr8 = NULL;
	mtp_uint16 *ptr16 = NULL;
	mtp_uint32 *ptr32 = NULL;
	mtp_int32 ii;

	retv_if(parray->array_entry == NULL, ELEMENT_NOT_FOUND);

	switch (parray->type) {
	/* LCOV_EXCL_START */
	case UINT8_TYPE:
		ptr8 = parray->array_entry;
		for (ii = 0; ii < parray->num_ele; ii++) {
			if (ptr8[ii] == (mtp_uchar) element)
				return ii;
		}
		break;

	case UINT16_TYPE:
		ptr16 = parray->array_entry;
		for (ii = 0; ii < parray->num_ele; ii++) {
			if (ptr16[ii] == (mtp_uint16) element)
				return ii;
		}
		break;
	/* LCOV_EXCL_STOP */
	case PTR_TYPE:
	case UINT32_TYPE:
		ptr32 = parray->array_entry;
		for (ii = 0; ii < parray->num_ele; ii++) {
			if (ptr32[ii] == (mtp_uint32)element)
				return ii;
		}
		break;

	default:
		break;
	}
	return ELEMENT_NOT_FOUND;
}

mtp_bool _prop_append_ele_ptparray(ptp_array_t *parray, mtp_uint32 element)
{

	mtp_uchar *ptr8 = NULL;
	mtp_uint16 *ptr16 = NULL;
	mtp_uint32 *ptr32 = NULL;

	if (parray->num_ele >= parray->arr_size) {
		ERR("parray->num_ele [%d] is bigger than parray->arr_size [%d]\n",
				parray->num_ele, parray->arr_size);
		if (FALSE == _prop_grow_ptparray(parray,
					((parray->arr_size * 3) >> 1) + 2))
			return FALSE;
	}

	switch (parray->type) {
	/* LCOV_EXCL_START */
	case UINT8_TYPE:
		ptr8 = parray->array_entry;
		ptr8[parray->num_ele++] = (mtp_uchar)element;
		break;

	case UINT16_TYPE:
		ptr16 = parray->array_entry;
		ptr16[parray->num_ele++] = (mtp_uint16) element;
		break;
	/* LCOV_EXCL_STOP */
	case PTR_TYPE:
	case UINT32_TYPE:
		ptr32 = parray->array_entry;
		ptr32[parray->num_ele++] = (mtp_uint32)element;
		break;

	default:
		break;
	}

	return TRUE;
}

/* LCOV_EXCL_START */
mtp_bool _prop_copy_ptparray(ptp_array_t *dst, ptp_array_t *src)
{
	mtp_uchar *ptr8src = NULL;
	mtp_uint16 *ptr16src = NULL;
	mtp_uint32 *ptr32src = NULL;
	mtp_uint32 ii;

	dst->type = src->type;

	switch (src->type) {
	case UINT8_TYPE:
		ptr8src = src->array_entry;
		for (ii = 0; ii < src->num_ele; ii++)
			_prop_append_ele_ptparray(dst, ptr8src[ii]);
		break;

	case UINT16_TYPE:
		ptr16src = src->array_entry;
		for (ii = 0; ii < src->num_ele; ii++)
			_prop_append_ele_ptparray(dst, ptr16src[ii]);
		break;

	case PTR_TYPE:
	case UINT32_TYPE:
		ptr32src = src->array_entry;
		for (ii = 0; ii < src->num_ele; ii++)
			_prop_append_ele_ptparray(dst, ptr32src[ii]);
		break;

	default:
		return 0;
	}
	return TRUE;
}

mtp_uint32 _prop_pack_ptparray(ptp_array_t *parray, mtp_uchar *buf,
		mtp_uint32 bufsize)
{
	mtp_uint16 size = 1;

	retvm_if(!parray || !buf, 0, "pArray or buf is NULL\n");

	size = __get_ptp_array_elem_size(parray->type);
	if (size == 0)
		return 0;

	if ((buf == NULL) || (bufsize < (sizeof(mtp_uint32) +
					parray->num_ele * size)))
		return 0;

	memcpy(buf, &(parray->num_ele), sizeof(mtp_uint32));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */

	if (parray->num_ele != 0) {
#ifdef __BIG_ENDIAN__
		mtp_uint32 ii;
		mtp_uchar *temp = buf + sizeof(mtp_uint32);
		mtp_uchar *ptr_entry = parray->array_entry;

		for (ii = 0; ii < parray->num_ele; ii++) {
			memcpy(temp, ptr_entry, size);
			_util_conv_byte_order(temp, size);
			temp += size;
			ptr_entry += size;
		}
#else /* __BIG_ENDIAN__ */
		memcpy(buf + sizeof(mtp_uint32), parray->array_entry,
				parray->num_ele * size);
#endif /* __BIG_ENDIAN__ */
	}
	return (sizeof(mtp_uint32) + parray->num_ele * size);
}

mtp_bool _prop_rem_elem_ptparray(ptp_array_t *parray, mtp_uint32 element)
{
	mtp_uchar *ptr8 = NULL;
	mtp_uint16 *ptr16 = NULL;
	mtp_uint32 *ptr32 = NULL;
	mtp_int32 ii;

	ii = _prop_find_ele_ptparray(parray, element);

	if (ii == ELEMENT_NOT_FOUND)
		return FALSE;

	switch (parray->type) {
	case UINT8_TYPE:
		ptr8 = parray->array_entry;
		for (; ii < (parray->num_ele - 1); ii++)
			ptr8[ii] = ptr8[ii + 1];
		break;

	case UINT16_TYPE:
		ptr16 = parray->array_entry;
		for (; ii < (parray->num_ele - 1); ii++)
			ptr16[ii] = ptr16[ii + 1];
		break;

	case UINT32_TYPE:
		ptr32 = parray->array_entry;
		for (; ii < (parray->num_ele - 1); ii++)
			ptr32[ii] = ptr32[ii + 1];
		break;

	case PTR_TYPE:
		ptr32 = parray->array_entry;
		for (; ii < (parray->num_ele - 1); ii++)
			ptr32[ii] = ptr32[ii + 1];
		break;

	default:
		break;
	}

	parray->num_ele--;

	return TRUE;

}

void _prop_deinit_ptparray(ptp_array_t *parray)
{
	parray->num_ele = 0;
	parray->arr_size = 0;
	g_free(parray->array_entry);
	parray->array_entry = NULL;
}

void _prop_destroy_ptparray(ptp_array_t *parray)
{
	if (parray == NULL)
		return;

	g_free(parray->array_entry);
	g_free(parray);
}
/* LCOV_EXCL_STOP */

/* PtpString Functions */
static ptp_string_t *__alloc_ptpstring(void)
{
	ptp_string_t *pstring = NULL;

	pstring = (ptp_string_t *)g_malloc(sizeof(ptp_string_t));
	if (pstring != NULL)
		pstring->num_chars = 0;

	return (pstring);
}

void _prop_copy_char_to_ptpstring(ptp_string_t *pstring, void *str,
		char_mode_t cmode)
{
	if (pstring == NULL)
		return;

	mtp_char *pchar = NULL;
	mtp_wchar *pwchar = NULL;
	mtp_uchar i = 0;

	pchar = (mtp_char *)str;
	pwchar = (mtp_wchar *)str;

	if (str == NULL) {
		/* LCOV_EXCL_START */
		pstring->num_chars = 0;
		return;
	}

	if (cmode == CHAR_TYPE) {
		if (pchar[0] == 0) {
			pstring->num_chars = 0;
			return;
		}
		for (i = 0; i < MAX_PTP_STRING_CHARS && pchar[i]; i++)
			pstring->str[i] = (mtp_wchar)pchar[i];
	} else if (cmode == WCHAR_TYPE) {
		if (pwchar[0] == 0) {
			pstring->num_chars = 0;
			return;
		}
		for (i = 0; i < MAX_PTP_STRING_CHARS && pwchar[i]; i++)
			pstring->str[i] = pwchar[i];
	} else {
		ERR("Unknown character mode : %d\n", cmode);
		pstring->num_chars = 0;
		return;
		/* LCOV_EXCL_STOP */
	}

	if (i == MAX_PTP_STRING_CHARS)
		pstring->num_chars = i;
	else
		pstring->num_chars = i + 1;

	pstring->str[pstring->num_chars - 1] = (mtp_wchar)0;
}

/* LCOV_EXCL_START */
void _prop_copy_time_to_ptptimestring(ptp_time_string_t *pstring,
		system_time_t *sys_time)
{
	char time[17] = { 0 };

	if (sys_time == NULL) {
		pstring->num_chars = 0;
	} else {
#if defined(NEED_TO_PORT)
		_util_wchar_swprintf(pstring->str, sizeof(pstring->str) / WCHAR_SIZ,
				"%04d%02d%02dT%02d%02d%02d.%01d",
				sys_time->year, sys_time->month,
				sys_time->day, sys_time->hour,
				sys_time->minute, sys_time->second,
				(sys_time->millisecond) / 100);
#else
		g_snprintf(time, sizeof(time), "%04d%02d%02dT%02d%02d%02d.%01d",
				sys_time->year, sys_time->month, sys_time->day,
				sys_time->hour, sys_time->minute,
				sys_time->second, (sys_time->millisecond) / 100);

		_util_utf8_to_utf16(pstring->str, sizeof(pstring->str) / WCHAR_SIZ, time);
#endif
		pstring->num_chars = 17;
		pstring->str[17] = '\0';
	}
}
/* LCOV_EXCL_STOP */

void _prop_copy_ptpstring(ptp_string_t *dst, ptp_string_t *src)
{
	mtp_uint16 ii;

	dst->num_chars = src->num_chars;
	for (ii = 0; ii < src->num_chars; ii++)
		dst->str[ii] = src->str[ii];
}

/* LCOV_EXCL_START */
mtp_bool _prop_is_equal_ptpstring(ptp_string_t *dst, ptp_string_t *src)
{
	mtp_uint16 ii;

	if (dst->num_chars != src->num_chars)
		return FALSE;

	for (ii = 0; ii < dst->num_chars; ii++) {
		if (dst->str[ii] != src->str[ii])
			return FALSE;
	}
	return TRUE;
}
/* LCOV_EXCL_STOP */

#define _prop_size_ptp_string_body(pstring)	\
	if ((pstring) == NULL)			\
		return 0;			\
	return ((pstring)->num_chars * sizeof(mtp_wchar) + 1);

mtp_uint32 _prop_size_ptpstring(ptp_string_t *pstring)
{
	_prop_size_ptp_string_body(pstring);
}

/* LCOV_EXCL_START */
mtp_uint32 _prop_size_ptptimestring(ptp_time_string_t *pstring)
{
	_prop_size_ptp_string_body(pstring);
}

static mtp_uint32 _prop_pack_ptpstring_body(mtp_wchar *str, mtp_uchar *buf,
		mtp_uint32 size, mtp_uint32 pstring_size, mtp_uint32 num_chars)
{
	mtp_uint32 bytes_written = 0;
	mtp_uchar *pchar = NULL;
#ifdef __BIG_ENDIAN__
	mtp_wchar conv_str[MAX_PTP_STRING_CHARS];
#endif /* __BIG_ENDIAN__ */

	if ((buf == NULL) || (size == 0) || (size < pstring_size))
		return bytes_written;

	if (num_chars == 0) {
		buf[0] = 0;
		bytes_written = 1;
	} else {
#ifdef __BIG_ENDIAN__
		memcpy(conv_str, str, num_chars * sizeof(mtp_wchar));
		_util_conv_byte_order_wstring(conv_str, num_chars);
		pchar = (mtp_uchar *)conv_str;
#else /* __BIG_ENDIAN__ */
		pchar = (mtp_uchar *)str;
#endif /* __BIG_ENDIAN__ */
		buf[0] = num_chars;

		bytes_written = pstring_size;

		memcpy(&buf[1], pchar, bytes_written - 1);

	}
	return bytes_written;
}

mtp_uint32 _prop_pack_ptpstring(ptp_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size)
{
	if (pstring == NULL)
		return 0;

	return _prop_pack_ptpstring_body(pstring->str, buf, size,
			_prop_size_ptpstring(pstring), pstring->num_chars);
}

mtp_uint32 _prop_pack_ptptimestring(ptp_time_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size)
{
	if (pstring == NULL)
		return 0;

	return _prop_pack_ptpstring_body(pstring->str, buf, size,
			_prop_size_ptptimestring(pstring), pstring->num_chars);
}

mtp_uint32 _prop_parse_rawstring(ptp_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size)
{
	mtp_uint16 ii;

	if (buf == NULL)
		return 0;

	if (buf[0] == 0) {
		pstring->num_chars = 0;
		return 1;
	} else {
		pstring->num_chars = buf[0];
		ii = (mtp_uint16) ((size - 1) / sizeof(mtp_wchar));
		if (pstring->num_chars > ii)
			pstring->num_chars = (mtp_uchar)ii;

		for (ii = 1; ii <= pstring->num_chars; ii++) {
#ifdef __BIG_ENDIAN__
			pstring->str[ii - 1] =
				buf[2 * ii] | (buf[2 * ii - 1] << 8);
#else /* __BIG_ENDIAN__ */
			pstring->str[ii - 1] =
				buf[2 * ii - 1] | (buf[2 * ii] << 8);
#endif /* __BIG_ENDIAN__ */
		}
		pstring->str[pstring->num_chars - 1] = (mtp_wchar) 0;
		return _prop_size_ptpstring(pstring);
	}
}
/* LCOV_EXCL_STOP */

mtp_bool _prop_is_valid_integer(prop_info_t *prop_info, mtp_uint64 value)
{
	if ((prop_info->data_type & PTP_DATATYPE_VALUEMASK) !=
			PTP_DATATYPE_VALUE) {
		return FALSE;
	}

	if (prop_info->form_flag == RANGE_FORM) {
		if ((value >= prop_info->range.min_val) &&
				(value <= prop_info->range.max_val)) {
			return TRUE;
		} else {
			/* invalid value */
			return FALSE;
		}
	} else if (prop_info->form_flag == ENUM_FORM) {
		/* LCOV_EXCL_START */
		slist_node_t *node = prop_info->supp_value_list.start;
		mtp_uint32 ii;
		for (ii = 0; ii < prop_info->supp_value_list.nnodes;
				ii++, node = node->link) {
			if (value == (mtp_uint32) node->value)
				return TRUE;
		/* LCOV_EXCL_STOP */
		}

		/* if it hits here, must be an invalid value */
		return FALSE;
	} else if (prop_info->form_flag == NONE) {
		/* No restrictions */
		return TRUE;
	}

	/* shouldn't be here */
	return FALSE;
}

mtp_bool _prop_is_valid_string(prop_info_t *prop_info, ptp_string_t *pstring)
{
	if ((prop_info->data_type != PTP_DATATYPE_STRING) || (pstring == NULL))
		return FALSE;

	if (prop_info->form_flag == ENUM_FORM) {
		/* LCOV_EXCL_START */
		slist_node_t *node = NULL;
		mtp_uint32 ii;
		ptp_string_t *ele_str = NULL;

		node = prop_info->supp_value_list.start;
		for (ii = 0; ii < prop_info->supp_value_list.nnodes;
				ii++, node = node->link) {
			ele_str = (ptp_string_t *) node->value;
			if (ele_str != NULL) {
				if (_prop_is_equal_ptpstring(pstring, ele_str)) {
					/* value found in the list of supported values */
					return TRUE;
				}
			}
		}
		/* if it hits here, must be an invalid value */
		return FALSE;
	} else if (prop_info->form_flag == NONE) {
		/* No restrictions */
		return TRUE;
	} else if (prop_info->form_flag == DATE_TIME_FORM) {
		mtp_wchar *date_time = pstring->str;
		retvm_if((date_time[8] != L'T') && (pstring->num_chars > 9), FALSE,
			"invalid data time format\n");

		/* LCOV_EXCL_STOP */
		return TRUE;
	} else if (prop_info->form_flag == REGULAR_EXPRESSION_FORM) {
		return TRUE;
	}

	return TRUE;
}

mtp_bool _prop_set_default_string(prop_info_t *prop_info, mtp_wchar *val)
{
	if (prop_info->data_type == PTP_DATATYPE_STRING) {
		g_free(prop_info->default_val.str);
		prop_info->default_val.str = __alloc_ptpstring();
		if (NULL == prop_info->default_val.str)
			return FALSE;

		_prop_copy_char_to_ptpstring(prop_info->default_val.str,
				val, WCHAR_TYPE);
		return TRUE;
	} else {
		return FALSE;
	}
}

mtp_bool _prop_set_default_integer(prop_info_t *prop_info, mtp_uchar *value)
{
	if ((prop_info->data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {
		memcpy(prop_info->default_val.integer, value,
				prop_info->dts_size);
		return TRUE;
	} else {
		return FALSE;
	}
}

mtp_bool _prop_set_current_integer(device_prop_desc_t *prop, mtp_uint32 val)
{
	if (_prop_is_valid_integer(&(prop->propinfo), val)) {
		mtp_int32 ii;
		mtp_uchar *ptr;

		ptr = (mtp_uchar *) &val;

		for (ii = 0; ii < sizeof(mtp_uint32); ii++)
			prop->current_val.integer[ii] = ptr[ii];

		return TRUE;
	} else {
		/* setting invalid value */
		return FALSE;
	}
}

mtp_bool _prop_set_current_string(device_prop_desc_t *prop, ptp_string_t *str)
{
	if (_prop_is_valid_string(&(prop->propinfo), str)) {
		g_free(prop->current_val.str);
		prop->current_val.str = __alloc_ptpstring();
		if (prop->current_val.str != NULL) {
			_prop_copy_ptpstring(prop->current_val.str, str);
			return TRUE;
		} else {
			g_free(prop->current_val.str);
			return FALSE;
		}
	} else {
		/* setting invalid value */
		return FALSE;
	}
}

mtp_bool _prop_set_current_integer_val(obj_prop_val_t *pval, mtp_uint64 val)
{
	if (_prop_is_valid_integer(&(pval->prop->propinfo), val)) {
		memset(pval->current_val.integer, 0,
				pval->prop->propinfo.dts_size * sizeof(mtp_byte));
		memcpy(pval->current_val.integer, &val,
				pval->prop->propinfo.dts_size * sizeof(mtp_byte));
		return TRUE;
	} else {
		/* setting invalid value */
		return FALSE;
	}
}

mtp_bool _prop_set_current_string_val(obj_prop_val_t *pval, ptp_string_t *str)
{
	if (_prop_is_valid_string(&(pval->prop->propinfo), str)) {
		g_free(pval->current_val.str);
		pval->current_val.str = __alloc_ptpstring();
		if (pval->current_val.str != NULL) {
			_prop_copy_ptpstring(pval->current_val.str, str);
			return TRUE;
		} else
			return FALSE;
	} else {
		/* setting invalid value */
		return FALSE;
	}
}

mtp_bool _prop_set_current_array_val(obj_prop_val_t *pval, mtp_uchar *arr,
		mtp_uint32 size)
{
	mtp_uint32 num_ele = 0;
	mtp_uchar *value = NULL;
	prop_info_t *propinfo = NULL;

	propinfo = &(pval->prop->propinfo);

	if (propinfo->data_type == PTP_DATATYPE_STRING) {
		ptp_string_t str;
		str.num_chars = 0;
		_prop_parse_rawstring(&str, arr, size);
		return _prop_set_current_string_val(pval, &str);

	} else if ((propinfo->data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		if (size < sizeof(mtp_uint32))
			return FALSE;

		memcpy(&num_ele, arr, sizeof(mtp_uint32));
		DBG("parsed array num [%d]\n", num_ele);

		retvm_if(size < sizeof(mtp_uint32) + num_ele * (propinfo->dts_size),
			FALSE, "buffer size is not enough [%d]\n", size);

		value = arr + sizeof(mtp_uint32);

		if ((propinfo->data_type == PTP_DATATYPE_AUINT8) ||
				(propinfo->data_type == PTP_DATATYPE_AINT8)) {

			_prop_destroy_ptparray(pval->current_val.array);
			pval->current_val.array = _prop_alloc_ptparray(UINT8_TYPE);

		} else if ((propinfo->data_type == PTP_DATATYPE_AUINT16) ||
				(propinfo->data_type == PTP_DATATYPE_AINT16)) {

			_prop_destroy_ptparray(pval->current_val.array);
			pval->current_val.array = _prop_alloc_ptparray(UINT16_TYPE);

		} else if ((propinfo->data_type == PTP_DATATYPE_AUINT32) ||
				(propinfo->data_type == PTP_DATATYPE_AINT32)) {

			_prop_destroy_ptparray(pval->current_val.array);
			pval->current_val.array = _prop_alloc_ptparray(UINT32_TYPE);
		}

		/* Copies the data into the PTP array */
		if ((pval->current_val.array != NULL) && (num_ele != 0)) {
			mtp_uchar *ptr8 = NULL;
			mtp_uint16 *ptr16 = NULL;
			mtp_uint32 *ptr32 = NULL;
			mtp_uint32 ii;

			_prop_grow_ptparray(pval->current_val.array, num_ele);

			if ((propinfo->data_type == PTP_DATATYPE_AUINT8) ||
					(propinfo->data_type == PTP_DATATYPE_AINT8)) {

				ptr8 = (mtp_uchar *) value;
				for (ii = 0; ii < num_ele; ii++)
					_prop_append_ele_ptparray(pval->current_val.array,
							ptr8[ii]);

			} else if ((propinfo->data_type == PTP_DATATYPE_AUINT16) ||
					(propinfo->data_type == PTP_DATATYPE_AINT16)) {
				ptr16 = (mtp_uint16 *) value;
				for (ii = 0; ii < num_ele; ii++)
					_prop_append_ele_ptparray(pval->current_val.array,
							ptr16[ii]);

			} else if ((propinfo->data_type == PTP_DATATYPE_AUINT32) ||
					(propinfo->data_type == PTP_DATATYPE_AINT32)) {

				ptr32 = (mtp_uint32 *)value;
				for (ii = 0; ii < num_ele; ii++)
					_prop_append_ele_ptparray(pval->current_val.array,
							ptr32[ii]);
			}
		}
		return TRUE;
	} else if ((propinfo->data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		if (propinfo->dts_size > size)
			return FALSE;

		if ((propinfo->data_type == PTP_DATATYPE_INT64) ||
				(propinfo->data_type == PTP_DATATYPE_UINT64) ||
				(propinfo->data_type == PTP_DATATYPE_INT128) ||
				(propinfo->data_type == PTP_DATATYPE_UINT128)) {

			if (propinfo->data_type == PTP_DATATYPE_UINT64) {
				memcpy(pval->current_val.integer, arr,
						propinfo->dts_size);
			}
			memcpy(pval->current_val.integer, arr,
					propinfo->dts_size);
			return TRUE;
		} else {

			mtp_uint32 new_val = 0;
			memcpy(&new_val, arr, propinfo->dts_size);
			return _prop_set_current_integer_val(pval,
					new_val);
		}
	}
	return FALSE;
}
/* LCOV_EXCL_STOP */

mtp_bool _prop_set_regexp(obj_prop_desc_t *prop, mtp_wchar *regex)
{
	ptp_string_t *str;

	if ((prop->propinfo.data_type != PTP_DATATYPE_STRING) ||
			(prop->propinfo.form_flag != REGULAR_EXPRESSION_FORM)) {
		return FALSE;
	}
	str = __alloc_ptpstring();
	if (str == NULL)
		return FALSE;

	_prop_copy_char_to_ptpstring(str, regex, WCHAR_TYPE);
	prop->prop_forms.reg_exp = str;

	return TRUE;
}

/* DeviceObjectPropDesc Functions */

/* ObjectPropVal Functions */
static void __init_obj_propval(obj_prop_val_t *pval, obj_prop_desc_t *prop)
{
	mtp_int32 ii;

	pval->prop = prop;

	for (ii = 0; ii < 16; ii++)
		pval->current_val.integer[ii] = 0;

	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		pval->current_val.str = __alloc_ptpstring();
		if (NULL == pval->current_val.str)
			return;
		_prop_copy_ptpstring(pval->current_val.str,
				prop->propinfo.default_val.str);
	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		memcpy(pval->current_val.integer,
				prop->propinfo.default_val.integer,
				prop->propinfo.dts_size);
	} else {
		if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT8) ||
				(prop->propinfo.data_type == PTP_DATATYPE_AINT8)) {

			pval->current_val.array = _prop_alloc_ptparray(UINT8_TYPE);
			if (NULL == pval->current_val.array)
				return;

			_prop_copy_ptparray(pval->current_val.array,
					prop->propinfo.default_val.array);

		} else if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT16) ||
				(prop->propinfo.data_type == PTP_DATATYPE_AINT16)) {

			pval->current_val.array = _prop_alloc_ptparray(UINT16_TYPE);
			if (NULL == pval->current_val.array)
				return;

			_prop_copy_ptparray(pval->current_val.array,
					prop->propinfo.default_val.array);

		} else if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT32) ||
				(prop->propinfo.data_type == PTP_DATATYPE_AINT32)) {

			pval->current_val.array = _prop_alloc_ptparray(UINT32_TYPE);
			if (NULL == pval->current_val.array)
				return;

			_prop_copy_ptparray(pval->current_val.array,
					prop->propinfo.default_val.array);
		}

		/* Add support for other array data types */
	}
}

obj_prop_val_t * _prop_alloc_obj_propval(obj_prop_desc_t *prop)
{
	obj_prop_val_t *pval = NULL;
	pval = (obj_prop_val_t *)g_malloc(sizeof(obj_prop_val_t));

	if (pval != NULL)
		__init_obj_propval(pval, prop);

	return pval;
}

obj_prop_val_t *_prop_get_prop_val(mtp_obj_t *obj, mtp_uint32 propcode)
{
	obj_prop_val_t *prop_val = NULL;
	slist_node_t *node = NULL;
	mtp_uint32 ii = 0;

	/*Update the properties if count is zero*/
	if (obj->propval_list.nnodes == 0)
		_prop_update_property_values_list(obj);

	for (ii = 0, node = obj->propval_list.start;
			ii < obj->propval_list.nnodes; ii++, node = node->link) {
		if (node == NULL || node->value == NULL)
			goto ERROR_CATCH;

		prop_val = (obj_prop_val_t *)node->value;
		if (prop_val) {
			if (prop_val->prop->propinfo.prop_code == propcode)
				return prop_val;
		}
	}

	return NULL;

ERROR_CATCH:
	/* update property and try again */
	_prop_update_property_values_list(obj);

	for (ii = 0, node = obj->propval_list.start;
			ii < obj->propval_list.nnodes;
			ii++, node = node->link) {
		if (node == NULL || node->value == NULL)
			break;

		prop_val = (obj_prop_val_t *)node->value;
		if ((prop_val) && (prop_val->prop->propinfo.prop_code ==
					propcode)) {
			return prop_val;
		}
	}
	ERR("node or node->value is null. try again but not found\n");
	return NULL;
}

mtp_uint32 _prop_size_obj_propval(obj_prop_val_t *pval)
{
	mtp_uint32 size = 0;

	if (pval == NULL || pval->prop == NULL)
		return size;

	if (pval->prop->propinfo.data_type == PTP_DATATYPE_STRING) {
		if (pval->current_val.str == NULL)
			size = 0;
		else
			size = _prop_size_ptpstring(pval->current_val.str);

	} else if ((pval->prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {
		size = _prop_get_size_ptparray(pval->current_val.array);

	} else if ((pval->prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {
		size = pval->prop->propinfo.dts_size;
	}

	return size;
}

void _prop_destroy_obj_propval(obj_prop_val_t *pval)
{
	if (pval == NULL)
		return;

	if (pval->prop == NULL) {
		g_free(pval);
		pval = NULL;
		return;
	}

	if (pval->prop->propinfo.data_type == PTP_DATATYPE_STRING) {
		if (pval->current_val.str) {
			g_free(pval->current_val.str);
			pval->current_val.str = NULL;
		}
	} else if ((pval->prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {
		_prop_destroy_ptparray(pval->current_val.array);
		pval->current_val.array = NULL;
	}

	g_free(pval);
	pval = NULL;
}
/* LCOV_EXCL_STOP */

static void __init_obj_prop_desc(obj_prop_desc_t *prop, mtp_uint16 propcode,
		mtp_uint16 data_type, mtp_uchar get_set, mtp_uchar form_flag,
		mtp_uint32 group_code)
{
	mtp_int32 ii;
	prop->propinfo.prop_code = propcode;/*Property code for this property*/
	prop->propinfo.data_type = data_type;	/* 2=byte, 4=word,
											 * 6=dword, 0xFFFF=String)
											 */
	prop->propinfo.get_set = get_set;	/* 0=get-only, 1=get-set */
	prop->propinfo.default_val.str = NULL; /* Default value for a String value type */

	prop->propinfo.form_flag = form_flag;

	if (prop->propinfo.form_flag == BYTE_ARRAY_FORM)
		prop->propinfo.data_type = PTP_DATATYPE_AUINT8;
	else if (prop->propinfo.form_flag == LONG_STRING_FORM)
		prop->propinfo.data_type = PTP_DATATYPE_AUINT16;

	prop->group_code = group_code;

	/* Zero out the integer byte array */
	for (ii = 0; ii < 16; ii++)
		prop->propinfo.default_val.integer[ii] = 0;

	/* size of default value: DTS */
	switch (prop->propinfo.data_type) {

	case PTP_DATATYPE_UINT8:
	case PTP_DATATYPE_INT8:
	case PTP_DATATYPE_AINT8:
	case PTP_DATATYPE_AUINT8:
		prop->propinfo.dts_size = sizeof(mtp_uchar);
		break;

	case PTP_DATATYPE_UINT16:
	case PTP_DATATYPE_INT16:
	case PTP_DATATYPE_AINT16:
	case PTP_DATATYPE_AUINT16:
		prop->propinfo.dts_size = sizeof(mtp_uint16);
		break;

	case PTP_DATATYPE_UINT32:
	case PTP_DATATYPE_INT32:
	case PTP_DATATYPE_AINT32:
	case PTP_DATATYPE_AUINT32:
		prop->propinfo.dts_size = sizeof(mtp_uint32);
		break;

	case PTP_DATATYPE_UINT64:
	case PTP_DATATYPE_INT64:
	case PTP_DATATYPE_AINT64:
	case PTP_DATATYPE_AUINT64:
		prop->propinfo.dts_size = sizeof(mtp_int64);
		break;

	case PTP_DATATYPE_UINT128:
	case PTP_DATATYPE_INT128:
	case PTP_DATATYPE_AINT128:
	case PTP_DATATYPE_AUINT128:
		prop->propinfo.dts_size = 2 * sizeof(mtp_int64);
		break;

	case PTP_DATATYPE_STRING:
	default:
		prop->propinfo.dts_size = 0;
		break;
	}

	_util_init_list(&(prop->propinfo.supp_value_list));

	prop->prop_forms.reg_exp = NULL;
	prop->prop_forms.max_len = 0;
}

/* LCOV_EXCL_START */
mtp_uint32 _prop_size_obj_prop_desc(obj_prop_desc_t *prop)
{
	mtp_uint32 size =
		sizeof(mtp_uint16) + sizeof(mtp_uint16) + sizeof(mtp_uchar) +
		sizeof(mtp_uint32) + sizeof(mtp_uchar);

	/* size of default value: DTS */
	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		size += _prop_size_ptpstring(prop->propinfo.default_val.str);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		size += _prop_get_size_ptparray(prop->propinfo.default_val.array);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		size += prop->propinfo.dts_size;
	}

	/* Add the size of the Form followed */
	switch (prop->propinfo.form_flag) {
	case NONE:
		break;

	case RANGE_FORM:
		if (prop->propinfo.data_type != PTP_DATATYPE_STRING)
			size += 3 * prop->propinfo.dts_size;/* Min,Max,Step */

		break;

	case ENUM_FORM:
		/* Number of Values */
		size += sizeof(mtp_uint16);
		if (prop->propinfo.data_type != PTP_DATATYPE_STRING) {
			size += prop->propinfo.supp_value_list.nnodes *
				prop->propinfo.dts_size;
		} else {
			slist_node_t *node = NULL;
			mtp_uint32 ii;

			for (ii = 0, node = prop->propinfo.supp_value_list.start;
					ii < prop->propinfo.supp_value_list.nnodes;
					ii++, node = node->link) {

				size += _prop_size_ptpstring((ptp_string_t *) node->value);
			}
		}
		break;

	case DATE_TIME_FORM:
		break;

	case REGULAR_EXPRESSION_FORM:
		size += _prop_size_ptpstring(prop->prop_forms.reg_exp);
		break;

	case BYTE_ARRAY_FORM:
	case LONG_STRING_FORM:
		size += sizeof(prop->prop_forms.max_len);
		break;

	default:
		/*don't know how to handle */
		break;

	}

	return size;
}

mtp_uint32 _prop_pack_obj_prop_desc(obj_prop_desc_t *prop, mtp_uchar *buf,
		mtp_uint32 size)
{
	mtp_uchar *temp = buf;
	mtp_uint32 count = 0;
	mtp_uint32 bytes_to_write = 0;
	slist_node_t *node = NULL;
	mtp_uint16 ii;

	if (!buf || size < _prop_size_obj_prop_desc(prop))
		return 0;

	/* Pack propcode, data_type, & get_set */
	bytes_to_write = sizeof(mtp_uint16);
	memcpy(temp, &(prop->propinfo.prop_code), bytes_to_write);
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(temp, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
	temp += bytes_to_write;

	bytes_to_write = sizeof(mtp_uint16);
	memcpy(temp, &(prop->propinfo.data_type), bytes_to_write);
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(temp, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
	temp += bytes_to_write;

	bytes_to_write = sizeof(mtp_uchar);
	memcpy(temp, &(prop->propinfo.get_set), bytes_to_write);
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(temp, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
	temp += bytes_to_write;

	/* Pack Default Value: DTS */
	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		bytes_to_write =
			_prop_size_ptpstring(prop->propinfo.default_val.str);
		if (bytes_to_write != _prop_pack_ptpstring(prop->propinfo.default_val.str,
					temp, bytes_to_write)) {
			return (mtp_uint32)(temp - buf);
		}
		temp += bytes_to_write;

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		bytes_to_write = _prop_get_size_ptparray(prop->propinfo.default_val.array);
		if (bytes_to_write != _prop_pack_ptparray(prop->propinfo.default_val.array,
					temp, bytes_to_write)) {
			return (mtp_uint32)(temp - buf);
		}
		temp += bytes_to_write;

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		bytes_to_write = prop->propinfo.dts_size;
		memcpy(temp, prop->propinfo.default_val.integer, bytes_to_write);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
		temp += bytes_to_write;
	}

	/* Pack group_code */
	memcpy(temp, &(prop->group_code), sizeof(mtp_uint32));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(temp, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
	temp += sizeof(mtp_uint32);

	/* Pack the FormFlag */
	memcpy(temp, &(prop->propinfo.form_flag), sizeof(mtp_uchar));
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(temp, sizeof(mtp_uchar));
#endif /* __BIG_ENDIAN__ */
	temp += sizeof(mtp_uchar);

	/* Pack the Form Flag values */
	switch (prop->propinfo.form_flag) {
	case NONE:
		break;

	case RANGE_FORM:
		/* Min, Max, & Step */
		memcpy(temp, &(prop->propinfo.range.min_val),
				prop->propinfo.dts_size);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, prop->propinfo.dts_size);
#endif /* __BIG_ENDIAN__ */
		temp += prop->propinfo.dts_size;
		memcpy(temp, &(prop->propinfo.range.max_val),
				prop->propinfo.dts_size);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, prop->propinfo.dts_size);
#endif /* __BIG_ENDIAN__ */
		temp += prop->propinfo.dts_size;
		memcpy(temp, &(prop->propinfo.range.step_size),
				prop->propinfo.dts_size);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, prop->propinfo.dts_size);
#endif /* __BIG_ENDIAN__ */
		temp += prop->propinfo.dts_size;
		break;

	case ENUM_FORM:

		/* Pack Number of Values in this enumeration */
		count = prop->propinfo.supp_value_list.nnodes;

		memcpy(temp, &count, sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, sizeof(mtp_uint16));
#endif /* __BIG_ENDIAN__ */
		temp += sizeof(mtp_uint16);

		if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

			for (ii = 0, node = prop->propinfo.supp_value_list.start;
					ii < prop->propinfo.supp_value_list.nnodes;
					ii++, node = node->link) {

				bytes_to_write =
					_prop_size_ptpstring((ptp_string_t *) node->value);
				if (bytes_to_write !=
						_prop_pack_ptpstring((ptp_string_t *) node->value,
							temp, bytes_to_write)) {
					return (mtp_uint32) (temp - buf);
				}
				temp += bytes_to_write;
			}
		} else {
			mtp_uint32 value = 0;

			for (ii = 0, node = prop->propinfo.supp_value_list.start;
					ii < prop->propinfo.supp_value_list.nnodes;
					ii++, node = node->link) {

				value = (mtp_uint32)node->value;
				memcpy(temp, &value, prop->propinfo.dts_size);
#ifdef __BIG_ENDIAN__
				_util_conv_byte_order(temp, prop->propinfo.dts_size);
#endif /* __BIG_ENDIAN__ */
				temp += prop->propinfo.dts_size;
			}
		}
		break;

	case DATE_TIME_FORM:
		break;

	case REGULAR_EXPRESSION_FORM:
		bytes_to_write = _prop_size_ptpstring(prop->prop_forms.reg_exp);
		if (bytes_to_write !=
				_prop_pack_ptpstring(prop->prop_forms.reg_exp,
					temp, bytes_to_write)) {

			return (mtp_uint32)(temp - buf);
		}
		temp += bytes_to_write;
		break;

	case BYTE_ARRAY_FORM:
	case LONG_STRING_FORM:
		memcpy(temp, &prop->prop_forms.max_len,
				sizeof(prop->prop_forms.max_len));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, sizeof(prop->prop_forms.max_len));
#endif /* __BIG_ENDIAN__ */
		temp += sizeof(prop->prop_forms.max_len);
		break;

	default:
		/*don't know how to handle */
		break;

	}

	return (mtp_uint32)(temp - buf);
}
/* LCOV_EXCL_STOP */

obj_prop_desc_t *_prop_get_obj_prop_desc(mtp_uint32 format_code,
		mtp_uint32 propcode)
{
	mtp_uint32 i = 0;
	int num_default_obj_props = 0;

	/*Default*/
	if (_get_oma_drm_status() == TRUE)
		num_default_obj_props = NUM_OBJECT_PROP_DESC_DEFAULT;
	else
		num_default_obj_props = NUM_OBJECT_PROP_DESC_DEFAULT - 1;

	for (i = 0; i < num_default_obj_props; i++) {
		if (props_list_default[i].propinfo.prop_code == propcode)
			return &(props_list_default[i]);
	}

	/* LCOV_EXCL_STOP */

	ERR("No matched property[0x%x], format[0x%x]!!\n", propcode,
			format_code);
	return NULL;
}

/* Objectproplist functions */
static mtp_bool __append_obj_proplist(obj_proplist_t *prop_list, mtp_uint32 obj_handle,
		mtp_uint16 propcode, mtp_uint32 data_type, mtp_uchar *val)
{
	ptp_string_t *str = NULL;
	prop_quad_t *quad = NULL;
	ptp_array_t *arr_uint8;
	ptp_array_t *arr_uint16;
	ptp_array_t *arr_uint32;

	quad = (prop_quad_t *)g_malloc(sizeof(prop_quad_t));
	if (NULL == quad)
		return FALSE;

	quad->obj_handle = obj_handle;
	quad->prop_code =  propcode;
	quad->data_type =  data_type;
	quad->pval = val;

	switch (data_type) {
	case PTP_DATATYPE_UINT8:
	case PTP_DATATYPE_INT8:
		quad->val_size = sizeof(mtp_uchar);
		break;

	case PTP_DATATYPE_UINT16:
	case PTP_DATATYPE_INT16:
		quad->val_size = sizeof(mtp_uint16);
		break;

	case PTP_DATATYPE_UINT32:
	case PTP_DATATYPE_INT32:
		quad->val_size = sizeof(mtp_uint32);
		break;

	case PTP_DATATYPE_UINT64:
	case PTP_DATATYPE_INT64:
		quad->val_size = sizeof(mtp_int64);
		break;

	case PTP_DATATYPE_UINT128:
	case PTP_DATATYPE_INT128:
		quad->val_size = 2 * sizeof(mtp_int64);
		break;

	case PTP_DATATYPE_AUINT8:
	case PTP_DATATYPE_AINT8:
		memcpy(&arr_uint8, val, sizeof(ptp_array_t *));
		quad->val_size = (arr_uint8 != NULL) ?
			_prop_get_size_ptparray(arr_uint8) : 0;
		quad->pval = (mtp_uchar *)arr_uint8;
		break;

	case PTP_DATATYPE_AUINT16:
	case PTP_DATATYPE_AINT16:
		memcpy(&arr_uint16, val, sizeof(ptp_array_t *));
		quad->val_size = (arr_uint16 != NULL) ?
			_prop_get_size_ptparray(arr_uint16) : 0;
		quad->pval = (mtp_uchar *)arr_uint16;
		break;

	case PTP_DATATYPE_AUINT32:
	case PTP_DATATYPE_AINT32:
		memcpy(&arr_uint32, val, sizeof(ptp_array_t *));
		quad->val_size = (arr_uint32 != NULL) ? _prop_get_size_ptparray(arr_uint32) : 0;
		quad->pval = (mtp_uchar *)arr_uint32;
		break;

	case PTP_DATATYPE_STRING:
		memcpy(&str, val, sizeof(ptp_string_t *));
		quad->val_size = (str != NULL) ? _prop_size_ptpstring(str) : 1;
		quad->pval = (mtp_uchar *)str;
		break;
	default:
		/* don't know  */
		quad->val_size = 0;
		break;
	}

	_util_add_node(&(prop_list->prop_quad_list), quad);
	return TRUE;
}
mtp_uint32 _prop_get_obj_proplist(mtp_obj_t *obj, mtp_uint32 propcode,
		mtp_uint32 group_code, obj_proplist_t *prop_list)
{
	obj_prop_val_t *propval = NULL;
	slist_node_t *node = NULL;
	mtp_uint32 ii = 0;

	if (obj->propval_list.nnodes == 0)
		retvm_if(!_prop_update_property_values_list(obj), 0,
			"update Property Values FAIL!!\n");

	for (ii = 0, node = obj->propval_list.start;
			ii < obj->propval_list.nnodes;
			ii++, node = node->link) {
		propval = (obj_prop_val_t *)node->value;

		if (NULL == propval)
			continue;

		if (FALSE == __check_object_propcode(propval->prop,
					propcode, group_code)) {
			continue;
		}
		__append_obj_proplist(prop_list, obj->obj_handle,
				propval->prop->propinfo.prop_code,
				propval->prop->propinfo.data_type,
				(mtp_uchar *)propval->current_val.integer);
	}

	return prop_list->prop_quad_list.nnodes;
}

mtp_bool _prop_update_property_values_list(mtp_obj_t *obj)
{
	mtp_uint32 ii = 0;
	mtp_char guid[16] = { 0 };
	slist_node_t *node = NULL;
	slist_node_t *next_node = NULL;
	ptp_time_string_t create_tm, modify_tm;
	mtp_wchar buf[MTP_MAX_PATHNAME_SIZE+1] = { 0 };
	mtp_char file_name[MTP_MAX_FILENAME_SIZE + 1] = { 0 };
	mtp_wchar w_file_name[MTP_MAX_FILENAME_SIZE + 1] = { 0 };
	char filename_wo_extn[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_wchar object_fullpath[MTP_MAX_PATHNAME_SIZE * 2 + 1] = { 0 };

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	if (obj->propval_list.nnodes > 0) {
		/*
		 * Remove all the old property value,
		 * and ready to set up new list
		 */
		for (ii = 0, next_node = obj->propval_list.start;
				ii < obj->propval_list.nnodes; ii++) {
			node = next_node;
			next_node = node->link;
			_prop_destroy_obj_propval((obj_prop_val_t *)
					node->value);
			g_free(node);
		}
		obj->propval_list.start = NULL;
		obj->propval_list.end = NULL;
		obj->propval_list.nnodes = 0;
		node = NULL;
	}

	/* Populate Object Info to Object properties */
	retvm_if(obj->file_path == NULL || obj->file_path[0] != '/', FALSE,
		"Path is not valid.. path = [%s]\n", obj->file_path);

	/*STORAGE ID*/
	retv_if(FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_STORAGEID,
				obj->obj_info->store_id), FALSE);

	/*OBJECT FORMAT*/
	retv_if(FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_OBJECTFORMAT,
				obj->obj_info->obj_fmt), FALSE);

	/*PROTECTION STATUS*/
	retv_if(FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_PROTECTIONSTATUS,
				obj->obj_info->protcn_status), FALSE);

	/*OBJECT SIZE*/
	retv_if(FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_OBJECTSIZE,
				obj->obj_info->file_size), FALSE);

	/*OBJECT FILENAME*/
	_util_get_file_name(obj->file_path, file_name);
	_util_utf8_to_utf16(w_file_name, sizeof(w_file_name) / WCHAR_SIZ, file_name);
	retv_if(FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_OBJECTFILENAME, w_file_name), FALSE);

	/*PARENT*/
	retv_if(FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_PARENT, obj->obj_info->h_parent), FALSE);

	/*PERSISTENT GUID*/
	_util_utf8_to_utf16(object_fullpath,
			sizeof(object_fullpath) / WCHAR_SIZ, obj->file_path);
	_util_conv_wstr_to_guid(object_fullpath, (mtp_uint64 *) guid);
	retv_if((FALSE == __create_prop_array(obj,
					MTP_OBJ_PROPERTYCODE_PERSISTENTGUID,
					guid, sizeof(guid))), FALSE);

	/*NON-CONSUMABLE*/
	retv_if(FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_NONCONSUMABLE, 0), FALSE);

	_entity_get_file_times(obj, &create_tm, &modify_tm);
	/*DATE MODIFIED*/
	retv_if(FALSE == __create_prop_timestring(obj, MTP_OBJ_PROPERTYCODE_DATEMODIFIED,
		&modify_tm), FALSE);

	/*DATE CREATED*/
	retv_if(FALSE == __create_prop_timestring(obj, MTP_OBJ_PROPERTYCODE_DATECREATED,
		&create_tm), FALSE);

	/* NAME */
	_util_get_file_name_wo_extn(obj->file_path,
			filename_wo_extn);
	_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ, filename_wo_extn);
	retv_if(FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_NAME, buf), FALSE);

	/*ASSOCIATION TYPE*/
	retv_if(FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_ASSOCIATIONTYPE,
				obj->obj_info->association_type), FALSE);

	/* OMADRM STATUS */
	if (_get_oma_drm_status() == TRUE) {
		retv_if(FALSE == __create_prop_integer(obj,
					MTP_OBJ_PROPERTYCODE_OMADRMSTATUS, 0), FALSE);
	}

	return TRUE;
}

/* LCOV_EXCL_STOP */

mtp_bool _prop_add_supp_integer_val(prop_info_t *prop_info, mtp_uint32 value)
{
	if (((prop_info->data_type & PTP_DATATYPE_VALUEMASK) !=
				PTP_DATATYPE_VALUE) || (prop_info->form_flag != ENUM_FORM)) {
		return FALSE;
	}

	/* Create the node and append it. */
	_util_add_node(&(prop_info->supp_value_list), (void *)value);

	return TRUE;
}

mtp_bool _prop_add_supp_string_val(prop_info_t *prop_info, mtp_wchar *val)
{
	ptp_string_t *str = NULL;
	mtp_bool ret;

	if ((prop_info->data_type != PTP_DATATYPE_STRING) ||
			(prop_info->form_flag != ENUM_FORM)) {
		return FALSE;
	}
	str = __alloc_ptpstring();

	if (str != NULL) {
		_prop_copy_char_to_ptpstring(str, val, WCHAR_TYPE);
		ret = _util_add_node(&(prop_info->supp_value_list), (void *)str);
		if (ret == FALSE) {
			ERR("List add Fail\n");
			g_free(str);
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

/* ObjectProp Functions */
mtp_uint32 _prop_get_supp_obj_props(mtp_uint32 format_code,
		ptp_array_t *supp_props)
{
	mtp_uint32 i = 0;
	mtp_uint32 num_default_obj_props = 0;

	/*Default*/
	if (_get_oma_drm_status() == TRUE)
		num_default_obj_props = NUM_OBJECT_PROP_DESC_DEFAULT;
	else
		num_default_obj_props = NUM_OBJECT_PROP_DESC_DEFAULT - 1;

	for (i = 0; i < num_default_obj_props; i++) {
		_prop_append_ele_ptparray(supp_props,
				props_list_default[i].propinfo.prop_code);
	}

	/* LCOV_EXCL_STOP */
	DBG("getsupp_props : format [0x%x], supported list [%d]\n",
			format_code, supp_props->num_ele);
	return supp_props->num_ele;
}

mtp_bool _prop_build_supp_props_default(void)
{
	mtp_wchar temp[MTP_MAX_REG_STRING + 1] = { 0 };
	static mtp_bool initialized = FALSE;
	mtp_uchar i = 0;
	mtp_uint32 default_val;

	retvm_if(initialized, TRUE, "already supported list is in there. just return!!\n");

	/*
	 * MTP_OBJ_PROPERTYCODE_STORAGEID (1)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_STORAGEID,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_GENERAL);

	default_val = 0x0;
	_prop_set_default_integer(&(props_list_default[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_OBJECTFORMAT (2)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_OBJECTFORMAT,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_GENERAL);

	default_val = PTP_FMT_ASSOCIATION;
	_prop_set_default_integer(&(props_list_default[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_PROTECTIONSTATUS (3)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_PROTECTIONSTATUS,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	default_val = PTP_PROTECTIONSTATUS_NOPROTECTION;
	_prop_add_supp_integer_val(&(props_list_default[i].propinfo),
			PTP_PROTECTIONSTATUS_NOPROTECTION);
	_prop_add_supp_integer_val(&(props_list_default[i].propinfo),
			PTP_PROTECTIONSTATUS_READONLY);
	_prop_add_supp_integer_val(&(props_list_default[i].propinfo),
			MTP_PROTECTIONSTATUS_READONLY_DATA);
	_prop_add_supp_integer_val(&(props_list_default[i].propinfo),
			(mtp_uint32)MTP_PROTECTIONSTATUS_NONTRANSFERABLE_DATA);
	_prop_set_default_integer(&(props_list_default[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_OBJECTSIZE (4)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_OBJECTSIZE,
			PTP_DATATYPE_UINT64,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_add_supp_integer_val(&(props_list_default[i].propinfo), 0x0);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_OBJECTFILENAME (5)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_OBJECTFILENAME,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETSET,
			REGULAR_EXPRESSION_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_util_utf8_to_utf16(temp, sizeof(temp) / WCHAR_SIZ,
			"[a-zA-Z!#\\$%&`\\(\\)\\-0-9@\\^_\\'\\{\\}\\~]{1,8}\\.[[a-zA-Z!#\\$%&`\\(\\)\\-0-9@\\^_\\'\\{\\}\\~]{1,3}]");
	_prop_set_regexp(&(props_list_default[i]), temp);

	_util_utf8_to_utf16(temp, sizeof(temp) / WCHAR_SIZ, "");
	_prop_set_default_string(&(props_list_default[i].propinfo), temp);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_PARENT (6)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_PARENT,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_GENERAL);

	default_val = 0x0;
	_prop_set_default_integer(&(props_list_default[i].propinfo),
			(mtp_uchar *) &default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_PERSISTENTGUID (7)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_PERSISTENTGUID,
			PTP_DATATYPE_UINT128,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_GENERAL);

	{
		mtp_uchar guid[16] = { 0 };
		_prop_set_default_integer(&(props_list_default[i].propinfo),
				guid);
	}
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_NONCONSUMABLE (8)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_NONCONSUMABLE,
			PTP_DATATYPE_UINT8,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	default_val = 0x0;
	_prop_add_supp_integer_val(&(props_list_default[i].propinfo), 0x0);
	_prop_add_supp_integer_val(&(props_list_default[i].propinfo), 0x1);
	_prop_set_default_integer(&(props_list_default[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 *MTP_OBJ_PROPERTYCODE_DATEMODIFIED (9)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_DATEMODIFIED,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			DATE_TIME_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_default_string(&(props_list_default[i].propinfo), temp);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_DATECREATED (10)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_DATECREATED,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			DATE_TIME_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_default_string(&(props_list_default[i].propinfo), temp);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_NAME (11)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_NAME,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_default_string(&(props_list_default[i].propinfo), temp);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_ASSOCIATIONTYPE (12)
	 */
	__init_obj_prop_desc(&(props_list_default[i]),
			MTP_OBJ_PROPERTYCODE_ASSOCIATIONTYPE,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETSET,
			ENUM_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	default_val = PTP_ASSOCIATIONTYPE_UNDEFINED;
	_prop_add_supp_integer_val(&(props_list_default[i].propinfo),
			PTP_ASSOCIATIONTYPE_UNDEFINED);
	_prop_add_supp_integer_val(&(props_list_default[i].propinfo),
			PTP_ASSOCIATIONTYPE_FOLDER);
	_prop_set_default_integer(&(props_list_default[i].propinfo),
			(mtp_uchar *)&default_val);

	if (_get_oma_drm_status() == TRUE) {
		/*
		 * MTP_OBJ_PROPERTYCODE_OMADRMSTATUS (13)
		 */
		i++;
		__init_obj_prop_desc(&(props_list_default[i]),
				MTP_OBJ_PROPERTYCODE_OMADRMSTATUS,
				PTP_DATATYPE_UINT8,
				PTP_PROPGETSET_GETONLY,
				ENUM_FORM,
				MTP_PROP_GROUPCODE_GENERAL);

		default_val = 0;
		_prop_add_supp_integer_val(&(props_list_default[i].propinfo), 0);
		_prop_add_supp_integer_val(&(props_list_default[i].propinfo), 1);
		_prop_set_default_integer(&(props_list_default[i].propinfo),
				(mtp_uchar *)&default_val);
	}

	initialized = TRUE;

	return TRUE;
}

/* LCOV_EXCL_ST */
mtp_uint32 _prop_get_size_interdep_prop(interdep_prop_config_t *prop_config)
{
	obj_prop_desc_t *prop = NULL;
	slist_node_t *node;
	mtp_int32 ii;
	mtp_uint32 size = sizeof(mtp_uint32);

	node = prop_config->propdesc_list.start;
	for (ii = 0; ii < prop_config->propdesc_list.nnodes; ii++) {
		prop = node->value;
		if (prop)
			size += _prop_size_obj_prop_desc(prop);
	}
	return size;
}

mtp_uint32 _prop_pack_interdep_prop(interdep_prop_config_t *prop_config,
		mtp_uchar *buf, mtp_uint32 size)
{
	mtp_uchar *temp = buf;
	obj_prop_desc_t *prop = NULL;
	slist_node_t *node;
	mtp_uint32 ele_size = 0;
	mtp_int32 ii;

	if (!buf || size < _prop_get_size_interdep_prop(prop_config))
		return 0;

	*(mtp_uint32 *) buf = prop_config->propdesc_list.nnodes;
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
	temp += sizeof(mtp_uint32);

	node = prop_config->propdesc_list.start;
	for (ii = 0; ii < prop_config->propdesc_list.nnodes; ii++) {
		prop = node->value;

		if (prop) {
			ele_size = _prop_size_obj_prop_desc(prop);
			_prop_pack_obj_prop_desc(prop, temp, ele_size);
			temp += ele_size;
		}
	}

	return (mtp_uint32)(temp - buf);
}

static mtp_uint32 __count_interdep_proplist(obj_interdep_proplist_t *config_list,
		mtp_uint32 format_code)
{
	mtp_uint32 count = 0;
	interdep_prop_config_t *prop_config = NULL;
	slist_node_t *node;
	mtp_int32 ii;

	node = config_list->plist.start;

	for (ii = 0; ii < config_list->plist.nnodes; ii++) {
		prop_config = node->value;
		if ((prop_config->format_code == format_code) ||
				(prop_config->format_code == PTP_FORMATCODE_NOTUSED)) {
			count++;
		}
	}
	return count;
}
/* LCOV_EXCL_STOP */

mtp_uint32 _prop_get_size_interdep_proplist(obj_interdep_proplist_t *config_list,
		mtp_uint32 format_code)
{
	mtp_uint32 size = sizeof(mtp_uint32);
	interdep_prop_config_t *prop_config = NULL;
	slist_node_t *node;
	mtp_int32 ii;

	node = config_list->plist.start;

	for (ii = 0; ii < config_list->plist.nnodes; ii++) {
		/* LCOV_EXCL_START */
		prop_config = node->value;
		if ((prop_config->format_code == format_code) ||
				(prop_config->format_code == PTP_FORMATCODE_NOTUSED)) {

			size += _prop_get_size_interdep_prop(prop_config);
		}
		/* LCOV_EXCL_STOP */
	}
	return size;
}

mtp_uint32 _prop_pack_interdep_proplist(obj_interdep_proplist_t *config_list,
		mtp_uint32 format_code, mtp_uchar *buf, mtp_uint32 size)
{
	mtp_uchar *temp = buf;
	interdep_prop_config_t *prop_config = NULL;
	slist_node_t *node;
	mtp_int32 ii;
	mtp_uint32 ele_size = 0;

	if (!buf ||
			size < _prop_get_size_interdep_proplist(config_list, format_code)) {
		return 0;
	}

	/* LCOV_EXCL_START */
	*(mtp_uint32 *)buf = __count_interdep_proplist(config_list,
			format_code);
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(buf, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
	temp += sizeof(mtp_uint32);

	node = config_list->plist.start;

	for (ii = 0; ii < config_list->plist.nnodes; ii++) {

		prop_config = node->value;
		if ((prop_config->format_code == format_code) ||
				(prop_config->format_code == PTP_FORMATCODE_NOTUSED)) {

			ele_size = _prop_get_size_interdep_prop(prop_config);
			_prop_pack_interdep_prop(prop_config, temp, ele_size);
			temp += ele_size;
		}
	}

	return (mtp_uint32)(temp - buf);
}
/* LCOV_EXCL_STOP */

mtp_bool _get_oma_drm_status(void)
{
#ifdef MTP_SUPPORT_OMADRM_EXTENSION
	return TRUE;
#else /* MTP_SUPPORT_OMADRM_EXTENSION */
	return FALSE;
#endif /* MTP_SUPPORT_OMADRM_EXTENSION */
}
