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

#include <glib.h>
#include "mtp_property.h"
#include "mtp_media_info.h"
#include "mtp_support.h"
#include "mtp_transport.h"

/*
 * EXTERN AND GLOBAL VARIABLES
 */
obj_interdep_proplist_t interdep_proplist;
/*
 * STATIC VARIABLES
 */
static obj_prop_desc_t props_list_mp3[NUM_OBJECT_PROP_DESC_MP3];
static obj_prop_desc_t props_list_wma[NUM_OBJECT_PROP_DESC_WMA];
static obj_prop_desc_t props_list_wmv[NUM_OBJECT_PROP_DESC_WMV];
static obj_prop_desc_t props_list_album[NUM_OBJECT_PROP_DESC_ALBUM];
static obj_prop_desc_t props_list_default[NUM_OBJECT_PROP_DESC_DEFAULT];

/*
 * STATIC FUNCTIONS
 */
static mtp_uint16 __get_ptp_array_elem_size(data_type_t type);
static mtp_bool __check_object_propcode(obj_prop_desc_t *prop,
		mtp_uint32 propcode, mtp_uint32 group_code);
static mtp_bool __create_prop_integer(mtp_obj_t *obj,
		mtp_uint16 propcode, mtp_uint64 value);
static mtp_bool __create_prop_string(mtp_obj_t *obj, mtp_uint16 propcode,
		mtp_wchar *value);
static mtp_bool __create_prop_array(mtp_obj_t *obj, mtp_uint16 propcode,
		mtp_char *arr, mtp_uint32 size);
#ifdef MTP_SUPPORT_PROPERTY_SAMPLE
static mtp_bool __create_prop_sample(mtp_obj_t *obj);
static void __build_supported_sample_props(mtp_uchar *count,
		obj_prop_desc_t *prop);
#endif /*MTP_SUPPORT_PROPERTY_SAMPLE*/
static mtp_bool __update_prop_values_audio(mtp_obj_t *obj);
static mtp_bool __update_prop_values_video(mtp_obj_t *obj);
static mtp_bool __update_prop_values_image(mtp_obj_t *obj);
static mtp_bool __prop_common_metadata(mtp_obj_t *obj,
		common_meta_t *p_metata);
static void __build_supported_common_props(mtp_uchar *count,
		obj_prop_desc_t *prop);
/* PtpString Functions */
#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
static ptp_string_t *__alloc_ptpstring(void);
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
static ptp_string_t *__alloc_ptpstring(mtp_uint32 size);
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
static void __init_obj_propval(obj_prop_val_t *val, obj_prop_desc_t *prop);
static void __init_ptptimestring(ptp_time_string_t *pstring);
static mtp_uint32 __size_curval_device_prop(device_prop_desc_t *prop);
static void __init_obj_prop_desc(obj_prop_desc_t *prop, mtp_uint16 propcode,
		mtp_uint16 data_type, mtp_uchar get_set, mtp_uchar form_flag,
		mtp_uint32 group_code);
static mtp_uint32 __get_size_default_val_obj_prop_desc(obj_prop_desc_t *prop);
static void __destroy_obj_prop_desc(obj_prop_desc_t *prop);
static mtp_uint32 __count_obj_proplist(obj_proplist_t *plist);
static mtp_bool __append_obj_proplist(obj_proplist_t *prop_list, mtp_uint32 obj_handle,
		mtp_uint16 prop_code, mtp_uint32 data_type, mtp_uchar *val);
#ifdef MTP_SUPPORT_INTERDEPENDENTPROP
static mtp_bool __append_interdep_prop(interdep_prop_config_t *config,
		obj_prop_desc_t *prop);
#endif /* MTP_SUPPORT_INTERDEPENDENTPROP */
static mtp_uint32 __count_interdep_proplist(obj_interdep_proplist_t *config_list,
		mtp_uint32 format_code);
/*
 * FUNCTIONS
 */
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
	if (NULL == prop) {
		ERR("Create property Fail.. Prop = [0x%X]\n", propcode);
		return FALSE;
	}

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
	if (NULL == prop) {
		ERR("Create property Fail.. Prop = [0x%X]\n", propcode);
		return FALSE;
	}

	propvalue_alloc_and_check(prop_val);
	_prop_copy_char_to_ptpstring(&ptp_str, value, WCHAR_TYPE);
	_prop_set_current_string_val(prop_val, &ptp_str);
	node_alloc_and_append();

	return TRUE;
}

static mtp_bool __create_prop_timestring(mtp_obj_t *obj,
	mtp_uint32 propcode, ptp_time_string_t *value)
{
	obj_prop_desc_t *prop  = NULL;
	obj_prop_val_t *prop_val= NULL;
	mtp_uint32 fmt_code = obj->obj_info->obj_fmt;

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	prop = _prop_get_obj_prop_desc(fmt_code, propcode);
	if (NULL == prop) {
		ERR("Create property Fail.. Prop = [0x%X]\n", propcode);
		return FALSE;
	}

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
	if (NULL == prop) {
		ERR("Create property Fail.. Prop = [0x%X]\n", propcode);
		return FALSE;
	}

	propvalue_alloc_and_check(prop_val);
	_prop_set_current_array_val(prop_val, (mtp_uchar *)arr, size);
	node_alloc_and_append();

	return TRUE;
}

static mtp_bool __update_prop_values_audio(mtp_obj_t *obj)
{
	mtp_bool success = TRUE;
	mtp_int32 converted_rating = 0;
	comp_audio_meta_t audio_data = {{0}, {0}};

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	if (_util_get_audio_metadata(obj->file_path, &audio_data) == FALSE) {
		if (_util_get_audio_meta_from_extractor(obj->file_path,
					&audio_data) == FALSE) {
			success = FALSE;
			goto DONE;
		}
	}

	/*Update common metadata information*/
	if (FALSE == __prop_common_metadata(obj,
				&(audio_data.commonmeta))) {
		ERR("Common  metadata update Fail");
		success = FALSE;
		goto DONE;
	}


	/*--------------TRACK--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_TRACK, audio_data.audiometa.track)) {
		success = FALSE;
		goto DONE;
	}

	/*--------------USER RATING--------------*/
	/* DCM rating -> MTP (WMP) rating */
	switch (audio_data.commonmeta.rating) {
	case 1:
		converted_rating = 1;	/* 1-12 */
		break;
	case 2:
		converted_rating = 25;	/* 13-37 */
		break;
	case 3:
		converted_rating = 50;	/* 37-62 */
		break;
	case 4:
		converted_rating = 75;	/* 63-86 */
		break;
	case 5:
		converted_rating = 99;	/* 87-100 */
		break;
	default:
		converted_rating = 0;	/* otherwise */
		break;
	}

	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_USERRATING, converted_rating)) {
		success = FALSE;
		goto DONE;

	}

	/*-------------AUDIOWAVECODEC--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_AUDIOWAVECODEC,
				MTP_WAVEFORMAT_UNKNOWN)) {
		success = FALSE;
		goto DONE;
	}

DONE:
	_util_free_common_meta(&(audio_data.commonmeta));
	return success;
}

static mtp_bool __update_prop_values_video(mtp_obj_t *obj)
{
	mtp_bool success = TRUE;
	mtp_int32 converted_rating = 0;
	comp_video_meta_t video_data = { {0,}, {0,} };
	video_meta_t *viddata = &(video_data.videometa);
	mtp_wchar buf[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);
	retv_if(obj->file_path == NULL, FALSE);

	if (_util_get_video_metadata(obj->file_path, &video_data) == FALSE) {
		if (_util_get_video_meta_from_extractor(obj->file_path,
					&video_data) == FALSE) {
			success = FALSE;
			goto DONE;
		}
	}

	/*Update common metadata information*/
	if (FALSE == __prop_common_metadata(obj,
				&(video_data.commonmeta))) {
		ERR("Common  metadata update Fail");
		success = FALSE;
		goto DONE;
	}

	/*--------------TRACK--------------*/
	if ((viddata->track != NULL) && strlen(viddata->track) > 0) {
		if (FALSE == __create_prop_integer(obj,
					MTP_OBJ_PROPERTYCODE_TRACK,
					atoi(viddata->track))) {
			success = FALSE;
			goto DONE;
		}
	}

	/*--------------AUDIO WAVE CODEC--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_AUDIOWAVECODEC, 0)) {
		success = FALSE;
		goto DONE;
	}

	/*--------------VIDEO CODEC--------------*/
	/*
	 * Conversion needs to be decided from audio_codec to
	 * exact format by spec
	 */
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_VIDEOFOURCCCODEC, 0)) {
		success = FALSE;
		goto DONE;
	}

	/*--------------VIDEO FRAMES PER 1K SECONDS--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_FRAMESPER1KSECONDS,
				viddata->video_fps * 1000)) {
		success = FALSE;
		goto DONE;
	}

	/*--------------VIDEO BITRATE--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_VIDEOBITRATE,
				viddata->video_br)) {
		success = FALSE;
		goto DONE;
	}

	/*--------------VIDEO WIDTH--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_WIDTH, viddata->video_w)) {
		success = FALSE;
		goto DONE;
	}

	/*--------------VIDEO HEIGHT--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_HEIGHT, viddata->video_h)) {
		success = FALSE;
		goto DONE;
	}

	/*--------------USER RATING--------------*/
	switch (video_data.commonmeta.rating) {
	case 1:
		converted_rating = 1;	/* 1-12 */
		break;
	case 2:
		converted_rating = 25;	/* 13-37 */
		break;
	case 3:
		converted_rating = 50;	/* 37-62 */
		break;
	case 4:
		converted_rating = 75;	/* 63-86 */
		break;
	case 5:
		converted_rating = 99;	/* 87-100 */
		break;
	default:
		converted_rating = 0;	/* otherwise */
		break;
	}

	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_USERRATING, converted_rating)) {
		success = FALSE;
		goto DONE;
	}

	/*----------ENCODING PROFILE----------*/
	_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ, "");
	if (FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_COPYRIGHTINFO, buf)) {
		success = FALSE;
		goto DONE;
	}

	/*----------METAGENRE-----------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_METAGENRE, 0x00)) {
		success = FALSE;
		goto DONE;
	}

	/*---------SCAN TYPE---------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_SCANTYPE, 0x00)) {
		success = FALSE;
		goto DONE;
	}

DONE:
	_util_free_common_meta(&(video_data.commonmeta));
	_util_free_video_meta(&(video_data.videometa));
	return success;
}

static mtp_bool __update_prop_values_image(mtp_obj_t *obj)
{
	image_meta_t image_data;

	retv_if(obj == NULL, FALSE);
	retv_if(obj->obj_info == NULL, FALSE);

	if (_util_get_image_ht_wt(obj->file_path, &image_data) == FALSE)
		return FALSE;

	/*--------------IMAGE WIDTH--------------*/
	if (FALSE == __create_prop_integer(obj, MTP_OBJ_PROPERTYCODE_WIDTH,
				image_data.wt))
		return FALSE;

	/*--------------IMAGE HEIGHT--------------*/
	if (FALSE == __create_prop_integer(obj, MTP_OBJ_PROPERTYCODE_HEIGHT,
				image_data.ht))
		return FALSE;

	return TRUE;
}

static mtp_bool __prop_common_metadata(mtp_obj_t *obj,
		common_meta_t *p_metata)
{
	mtp_wchar buf[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	/*--------------ARTIST--------------*/
	if (p_metata->artist != NULL && strlen(p_metata->artist) > 0) {
		_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ,
				p_metata->artist);
	}
#ifdef MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN
	else {
		DBG("got null artist");
		_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ,
				MTP_UNKNOWN_METADATA);
	}
#endif /*MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN*/

	if (FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_ARTIST, buf)) {
		return FALSE;
	}

	/*--------------ALBUM--------------*/
	if (p_metata->album != NULL && strlen(p_metata->album) > 0) {
		_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ,
				p_metata->album);
	}
#ifdef MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN
	else {
		DBG("got null album");
		_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ,
				MTP_UNKNOWN_METADATA);
	}
#endif /*MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN*/

	if (FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_ALBUMNAME, buf)) {
		return FALSE;
	}

	/*--------------GENRE--------------*/
	if (p_metata->genre != NULL && strlen(p_metata->genre) > 0) {
		_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ,
				p_metata->genre);
	}
#ifdef MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN
	else {
		DBG("got null genre");
		_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ,
				MTP_UNKNOWN_METADATA);
	}
#endif /*MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN*/

	if (FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_GENRE, buf)) {
		return FALSE;
	}

	/*--------------AUTHOR--------------*/
	if (p_metata->author != NULL && strlen(p_metata->author) > 0) {
		_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ,
				p_metata->author);
	}
#ifdef MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN
	else {
		DBG("got null author");
		_util_utf8_to_utf16(buf, sizeof(buf) / WCHAR_SIZ,
				MTP_UNKNOWN_METADATA);
	}
#endif /*MTP_USE_FILL_EMPTYMETADATA_WITH_UNKNOWN*/

	if (FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_COMPOSER, buf)) {
		return FALSE;
	}

	/*--------------COPYRIGHT--------------*/
	if ((p_metata->copyright != NULL) && strlen(p_metata->copyright) > 0) {
		_util_utf8_to_utf16(buf,
				sizeof(buf) / WCHAR_SIZ,
				p_metata->copyright);
	} else {
		_util_utf8_to_utf16(buf,
				sizeof(buf) / WCHAR_SIZ, "");
	}

	if (FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_COPYRIGHTINFO, buf)) {
		return FALSE;
	}

	/*--------------DURATION--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_DURATION, p_metata->duration)) {
		return FALSE;
	}

	/*--------------AUDIO BITRATE--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_AUDIOBITRATE, p_metata->audio_bitrate)) {
		return FALSE;
	}

	/*--------------RELEASE YEAR--------------*/
	if ((p_metata->year != NULL) && strlen(p_metata->year) > 0) {
		_util_utf8_to_utf16(buf,
				sizeof(buf) / WCHAR_SIZ,
				p_metata->year);
	} else {
		_util_utf8_to_utf16(buf,
				sizeof(buf) / WCHAR_SIZ, "");
	}

	if (FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_ORIGINALRELEASEDATE, buf)) {
		return FALSE;
	}

	/*--------------DESCRIPTION--------------*/
	if ((p_metata->description != NULL) &&
			strlen(p_metata->description) > 0) {
		_util_utf8_to_utf16(buf,
				sizeof(buf) / WCHAR_SIZ,
				p_metata->description);
	} else {
		_util_utf8_to_utf16(buf,
				sizeof(buf) / WCHAR_SIZ, "");
	}

	if (FALSE == __create_prop_string(obj,
				MTP_OBJ_PROPERTYCODE_DESCRIPTION, buf)) {
		return FALSE;
	}

	/*--------------SAMPLE RATE--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_SAMPLERATE, p_metata->sample_rate)) {
		return FALSE;
	}

	/*-------------NUMBEROFCHANNELS--------------*/
	if (FALSE == __create_prop_integer(obj,
				MTP_OBJ_PROPERTYCODE_NUMBEROFCHANNELS, p_metata->num_channel)) {
		return FALSE;
	}

	return TRUE;
}

static void __build_supported_common_props(mtp_uchar *count,
		obj_prop_desc_t *prop)
{
	mtp_uchar i = 0;
	mtp_wchar str[MTP_MAX_REG_STRING + 1] = { 0 };
	mtp_uint32 default_val;

	_util_utf8_to_utf16(str, sizeof(str) / WCHAR_SIZ, "");

	/*
	 * MTP_OBJ_PROPERTYCODE_ARTIST (1)
	 */
	__init_obj_prop_desc((prop + i),
			MTP_OBJ_PROPERTYCODE_ARTIST,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_OBJECT);
	_prop_set_default_string(&(prop[i].propinfo), str);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_DURATION (2)
	 */
	__init_obj_prop_desc((prop + i),
			MTP_OBJ_PROPERTYCODE_DURATION,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = 0x0;
	_prop_set_range_integer(&(prop[i].propinfo), 0, 0xffffffff, 1L);
	_prop_set_default_integer(&((prop[i].propinfo)), (mtp_uchar *) &default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_USERRATING (3)
	 */
	__init_obj_prop_desc((prop + i),
			MTP_OBJ_PROPERTYCODE_USERRATING,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = 0x0;
	_prop_set_range_integer(&(prop[i].propinfo), 0, 100, 1L);
	_prop_set_default_integer(&(prop[i].propinfo),
			(mtp_uchar *) &default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_TRACK (4)
	 */
	__init_obj_prop_desc((prop + i),
			MTP_OBJ_PROPERTYCODE_TRACK,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = 0x0;
	_prop_set_default_integer(&(prop[i].propinfo),
			(mtp_uchar *) &default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_GENRE (5)
	 */
	__init_obj_prop_desc((prop + i),
			MTP_OBJ_PROPERTYCODE_GENRE,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_OBJECT);
	_prop_set_default_string(&(prop[i].propinfo), str);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_ORIGINALRELEASEDATE (6)
	 */
	__init_obj_prop_desc((prop + i),
			MTP_OBJ_PROPERTYCODE_ORIGINALRELEASEDATE,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			DATE_TIME_FORM,
			MTP_PROP_GROUPCODE_OBJECT);
	_prop_set_default_string(&(prop[i].propinfo), str);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_ALBUMNAME (7)
	 */
	__init_obj_prop_desc((prop + i),
			MTP_OBJ_PROPERTYCODE_ALBUMNAME,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_OBJECT);
	_prop_set_default_string(&(prop[i].propinfo), str);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_COMPOSER (8)
	 */
	__init_obj_prop_desc((prop + i),
			MTP_OBJ_PROPERTYCODE_COMPOSER,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			NONE,
			MTP_PROP_GROUPCODE_OBJECT);
	_prop_set_default_string(&(prop[i].propinfo), str);
	i++;

	*count += i;

	return;
}

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
	return;
}

ptp_array_t *_prop_alloc_ptparray(data_type_t type)
{
	ptp_array_t *parray;
	parray = (ptp_array_t *)g_malloc(sizeof(ptp_array_t));
	if (parray != NULL) {
		_prop_init_ptparray(parray, type);
	}

	return (parray);
}

mtp_uint32 _prop_get_size_ptparray(ptp_array_t *parray)
{
	mtp_uint16 size = 0;

	if (parray == NULL) {
		ERR("ptp_array_t is NULL");
		return 0;
	}

	size = __get_ptp_array_elem_size(parray->type);
	if (size == 0)
		return 0;

	return (sizeof(mtp_uint32) + (size *parray->num_ele));
}

mtp_uint32 _prop_get_size_ptparray_without_elemsize(ptp_array_t *parray)
{
	mtp_uint16 size = 0;

	size = __get_ptp_array_elem_size(parray->type);
	if (size == 0)
		return 0;

	return (size * parray->num_ele);
}

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

	parray->array_entry =
		g_realloc(parray->array_entry, size * new_size);
	if (parray->array_entry == NULL) {
		parray->arr_size = 0;
		return FALSE;
	}
	parray->arr_size = new_size;

	return TRUE;
}

mtp_int32 _prop_find_ele_ptparray(ptp_array_t *parray, mtp_uint32 element)
{
	mtp_uchar *ptr8 = NULL;
	mtp_uint16 *ptr16 = NULL;
	mtp_uint32 *ptr32 = NULL;
	mtp_int32 ii;

	retv_if(parray->array_entry == NULL, ELEMENT_NOT_FOUND);

	switch (parray->type) {
	case UINT8_TYPE:
		ptr8 = parray->array_entry;
		for (ii = 0; ii < parray->num_ele; ii++) {
			if (ptr8[ii] == (mtp_uchar) element) {
				return ii;
			}
		}
		break;

	case UINT16_TYPE:
		ptr16 = parray->array_entry;
		for (ii = 0; ii < parray->num_ele; ii++) {
			if (ptr16[ii] == (mtp_uint16) element) {
				return ii;
			}
		}
		break;

	case PTR_TYPE:
	case UINT32_TYPE:
		ptr32 = parray->array_entry;
		for (ii = 0; ii < parray->num_ele; ii++) {
			if (ptr32[ii] == (mtp_uint32)element) {
				return ii;
			}
		}
		break;

	default:
		break;
	}
	return ELEMENT_NOT_FOUND;
}

mtp_bool _prop_get_ele_ptparray(ptp_array_t *parray, mtp_uint32 index, void *ele)
{
	mtp_uchar *ptr8 = NULL;
	mtp_uint16 *ptr16 = NULL;
	mtp_uint32 *ptr32 = NULL;

	retv_if(parray->array_entry == NULL, FALSE);

	if (index >= parray->num_ele)
		return FALSE;

	switch (parray->type) {
	case UINT8_TYPE:
		ptr8 = parray->array_entry;
		*((mtp_uchar *)ele) = ptr8[index];
		break;
	case UINT16_TYPE:
		ptr16 = parray->array_entry;
		*((mtp_uint16 *)ele) = ptr16[index];
		break;

	case PTR_TYPE:
	case UINT32_TYPE:
		ptr32 = parray->array_entry;
		*((mtp_uint32 *)ele) = ptr32[index];
		break;
	default:
		return FALSE;
	}
	return TRUE;
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
	case UINT8_TYPE:
		ptr8 = parray->array_entry;
		ptr8[parray->num_ele++] = (mtp_uchar)element;
		break;

	case UINT16_TYPE:
		ptr16 = parray->array_entry;
		ptr16[parray->num_ele++] = (mtp_uint16) element;
		break;

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

mtp_bool _prop_append_ele128_ptparray(ptp_array_t *parray, mtp_uint64 *element)
{
	mtp_uchar *ptr = NULL;
	mtp_bool ret = FALSE;
	if (parray->num_ele >= parray->arr_size) {
		if (FALSE == _prop_grow_ptparray(parray,
					((parray->arr_size * 3) >> 1) + 2))
			return FALSE;
	}

	switch (parray->type) {
	case UINT128_TYPE:
		ptr = parray->array_entry;
		memcpy(&(ptr[(parray->num_ele * 16)]), element,
				sizeof(mtp_uint64) * 2);
		parray->num_ele++;
		ret = TRUE;
		break;

	default:
		break;
	}

	return ret;
}

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
	if (parray == NULL || buf == NULL) {
		ERR("pArray or buf is NULL");
		return 0;
	}

	mtp_uint16 size = 1;

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

mtp_uint32 _prop_pack_ptparray_without_elemsize(ptp_array_t *parray,
		mtp_uchar *buf, mtp_uint32 bufsize)
{
	mtp_uint16 size = 1;
#ifdef __BIG_ENDIAN__
	mtp_uchar *temp;
	mtp_uint32 ii;
#endif /* __BIG_ENDIAN__ */

	size = __get_ptp_array_elem_size(parray->type);
	if (size == 0)
		return 0;

	if ((buf == NULL) || (bufsize < (parray->num_ele * size)))
		return 0;

	if (parray->num_ele != 0)
		memcpy(buf, parray->array_entry, parray->num_ele * size);

#ifdef __BIG_ENDIAN__
	/* Swap all the elements */
	temp = buf;
	for (ii = 0; ii < parray->num_ele; ii++) {
		_util_conv_byte_order(temp, size);
		temp += size;
	}
#endif /* __BIG_ENDIAN__ */

	return (parray->num_ele * size);
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
	if (parray->array_entry) {
		g_free(parray->array_entry);
	}
	parray->array_entry = NULL;
	return;
}

void _prop_destroy_ptparray(ptp_array_t *parray)
{
	if (parray == NULL)
		return;

	if (parray->array_entry != NULL) {
		g_free(parray->array_entry);
	}
	parray->arr_size = 0;
	parray->num_ele = 0;
	g_free(parray);
	return;
}

mtp_uint16 __get_ptp_array_elem_size(data_type_t type)
{
	mtp_uint16 size = 0;

	switch (type) {
	case UINT8_TYPE:
		size = 1;
		break;
	case UINT16_TYPE:
		size = 2;
		break;
	case PTR_TYPE:
	case UINT32_TYPE:
		size = 4;
		break;
	case UINT128_TYPE:
		size = 16;
		break;
	default:
		size = 0;
		break;
	}

	return size;
}

/* PtpString Functions */
#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
static ptp_string_t *__alloc_ptpstring(void)
{
	ptp_string_t *pstring = NULL;

	pstring = (ptp_string_t *)g_malloc(sizeof(ptp_string_t));
	if (pstring != NULL) {
		_prop_init_ptpstring(pstring);
	}

	return (pstring);
}
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
static ptp_string_t *__alloc_ptpstring(mtp_uint32 size)
{
	ptp_string_t *pstring = NULL;
	mtp_int32 size_tmp = 0;
	mtp_int32 alloc_size = 0;

	size_tmp = sizeof(wchar_t) * size + sizeof(wchar_t) * 2;
	alloc_size = ((size_tmp >> 5) + 1) << 5;	/* multiple of 32 */

	pstring = (ptp_string_t *)g_malloc(alloc_size);	/* for margin */
	if (pstring != NULL) {
		_prop_init_ptpstring(pstring);
	}

	return (pstring);
}
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */

void _prop_init_ptpstring(ptp_string_t *pstring)
{
	pstring->num_chars = 0;
	return;
}

static void __init_ptptimestring(ptp_time_string_t *pstring)
{
	pstring->num_chars = 0;
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
		pstring->num_chars = 0;
		return;
	}

	if (cmode == CHAR_TYPE) {
		if (pchar[0] == 0) {
			pstring->num_chars = 0;
			return;
		}
		for (i = 0; i < MAX_PTP_STRING_CHARS && pchar[i]; i++) {
			pstring->str[i] = (mtp_wchar)pchar[i];
		}
	} else if (cmode == WCHAR_TYPE) {
		if (pwchar[0] == 0) {
			pstring->num_chars = 0;
			return;
		}
		for (i = 0; i < MAX_PTP_STRING_CHARS && pwchar[i]; i++) {
			pstring->str[i] = pwchar[i];
		}
	} else {
		ERR("Unknown character mode : %d\n", cmode);
		pstring->num_chars = 0;
		return;
	}

	if (i == MAX_PTP_STRING_CHARS)
		pstring->num_chars = i;
	else
		pstring->num_chars = i + 1;

	pstring->str[pstring->num_chars - 1] = (mtp_wchar)0;

	return;
}

void _prop_copy_time_to_ptptimestring(ptp_time_string_t *pstring,
		system_time_t *sys_time)
{
	char time[17] = { 0 };

	if (sys_time == NULL) {
		__init_ptptimestring(pstring);
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
	return;
}

void _prop_copy_ptpstring(ptp_string_t *dst, ptp_string_t *src)
{
	mtp_uint16 ii;

	dst->num_chars = src->num_chars;
	for (ii = 0; ii < src->num_chars; ii++) {
		dst->str[ii] = src->str[ii];
	}
	return;
}

void _prop_copy_ptptimestring(ptp_time_string_t *dst, ptp_time_string_t *src)
{
	mtp_uint16 ii;

	dst->num_chars = src->num_chars;
	for (ii = 0; ii < src->num_chars; ii++) {
		dst->str[ii] = src->str[ii];
	}
	return;
}

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

mtp_uint32 _prop_size_ptpstring(ptp_string_t *pstring)
{
	if (pstring == NULL)
		return 0;

	return (pstring->num_chars * sizeof(mtp_wchar) + 1);
}

mtp_uint32 _prop_size_ptptimestring(ptp_time_string_t *pstring)
{
	if (pstring == NULL)
		return 0;

	return (pstring->num_chars * sizeof(mtp_wchar) + 1);
}

mtp_uint32 _prop_pack_ptpstring(ptp_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size)
{
	mtp_uint32 bytes_written = 0;
	mtp_uint32 ii;
	mtp_uchar *pchar = NULL;
#ifdef __BIG_ENDIAN__
	mtp_wchar conv_str[MAX_PTP_STRING_CHARS];
#endif /* __BIG_ENDIAN__ */

	if ((buf == NULL) || (pstring == NULL) || (size == 0) ||
			(size < _prop_size_ptpstring(pstring))) {
		return bytes_written;
	}

	if (pstring->num_chars == 0) {
		buf[0] = 0;
		bytes_written = 1;
	} else {
#ifdef __BIG_ENDIAN__
		memcpy(conv_str, pstring->str,
				pstring->num_chars * sizeof(mtp_wchar));
		_util_conv_byte_orderForWString(conv_str, pstring->num_chars);
		pchar = (mtp_uchar *) conv_str;
#else /* __BIG_ENDIAN__ */
		pchar = (mtp_uchar *) pstring->str;
#endif /* __BIG_ENDIAN__ */
		buf[0] = pstring->num_chars;

		bytes_written = _prop_size_ptpstring(pstring);
		for (ii = 0; ii < (bytes_written - 1); ii++) {
			buf[ii + 1] = pchar[ii];
		}
	}
	return bytes_written;
}

mtp_uint32 _prop_pack_ptptimestring(ptp_time_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size)
{
	mtp_uint32 bytes_written = 0;
	mtp_uchar *pchar = NULL;
#ifdef __BIG_ENDIAN__
	mtp_wchar conv_str[MAX_PTP_STRING_CHARS];
#endif /* __BIG_ENDIAN__ */

	if ((buf == NULL) || (pstring == NULL) || (size == 0) ||
			(size < _prop_size_ptptimestring(pstring))) {
		return bytes_written;
	}

	if (pstring->num_chars == 0) {
		buf[0] = 0;
		bytes_written = 1;
	} else {
#ifdef __BIG_ENDIAN__
		memcpy(conv_str, pstring->str,
				pstring->num_chars * sizeof(mtp_wchar));
		_util_conv_byte_order_wstring(conv_str, pstring->num_chars);
		pchar = (mtp_uchar *)conv_str;
#else /* __BIG_ENDIAN__ */
		pchar = (mtp_uchar *)pstring->str;
#endif /* __BIG_ENDIAN__ */
		buf[0] = pstring->num_chars;

		bytes_written = _prop_size_ptptimestring(pstring);

		memcpy(&buf[1], pchar, bytes_written - 1);

	}
	return bytes_written;
}

mtp_uint32 _prop_parse_rawstring(ptp_string_t *pstring, mtp_uchar *buf,
		mtp_uint32 size)
{
	mtp_uint16 ii;

	if (buf == NULL) {
		return 0;
	}

	if (buf[0] == 0) {
		pstring->num_chars = 0;
		return 1;
	} else {
		pstring->num_chars = buf[0];
		ii = (mtp_uint16) ((size - 1) / sizeof(mtp_wchar));
		if (pstring->num_chars > ii) {
			pstring->num_chars = (mtp_uchar)ii;
		}

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

void _prop_destroy_ptpstring(ptp_string_t *pstring)
{
	if (pstring != NULL) {
		g_free(pstring);
	}
	return;
}

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
		slist_node_t *node = prop_info->supp_value_list.start;
		mtp_uint32 ii;
		for (ii = 0; ii < prop_info->supp_value_list.nnodes;
				ii++, node = node->link) {
			if (value == (mtp_uint32) node->value) {
				return TRUE;
			}
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
	if ((prop_info->data_type != PTP_DATATYPE_STRING) || (pstring == NULL)) {
		return FALSE;
	}

	if (prop_info->form_flag == ENUM_FORM)
	{
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
		if ((date_time[8] != L'T') && (pstring->num_chars > 9)) {
			ERR("invalid data time format");
			return FALSE;
		}
		return TRUE;
	} else if (prop_info->form_flag == REGULAR_EXPRESSION_FORM) {
		return TRUE;
	}

	return TRUE;
}

mtp_bool _prop_set_default_string(prop_info_t *prop_info, mtp_wchar *val)
{
	if (prop_info->data_type == PTP_DATATYPE_STRING) {
		_prop_destroy_ptpstring(prop_info->default_val.str);
#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
		prop_info->default_val.str = __alloc_ptpstring();
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		prop_info->default_val.str = __alloc_ptpstring(_util_wchar_len(val));
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		if (NULL == prop_info->default_val.str)
			return FALSE;

		_prop_copy_char_to_ptpstring(prop_info->default_val.str,
				val, WCHAR_TYPE);
		return TRUE;
	}
	else {
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

mtp_bool _prop_set_default_array(prop_info_t *prop_info, mtp_uchar *parray,
		mtp_uint32 num_ele)
{
	/* Allocate memory for the PTP array */
	if ((prop_info->data_type == PTP_DATATYPE_AUINT8) ||
			(prop_info->data_type == PTP_DATATYPE_AINT8))
		prop_info->default_val.array = _prop_alloc_ptparray(UINT8_TYPE);
	else if ((prop_info->data_type == PTP_DATATYPE_AUINT16) ||
			(prop_info->data_type == PTP_DATATYPE_AINT16))
		prop_info->default_val.array = _prop_alloc_ptparray(UINT16_TYPE);
	else if ((prop_info->data_type == PTP_DATATYPE_AUINT32) ||
			(prop_info->data_type == PTP_DATATYPE_AINT32))
		prop_info->default_val.array = _prop_alloc_ptparray(UINT32_TYPE);
	else
		return FALSE;

	if (prop_info->default_val.array == NULL)
		return FALSE;

	/* Copies the data into the PTP array */
	if ((prop_info->default_val.array != NULL) && (num_ele != 0))
	{
		mtp_uchar *ptr8 = NULL;
		mtp_uint16 *ptr16 = NULL;
		mtp_uint32 *ptr32 = NULL;
		mtp_uint32 ii;

		_prop_grow_ptparray(prop_info->default_val.array, num_ele);

		if ((prop_info->data_type == PTP_DATATYPE_AUINT8) ||
				(prop_info->data_type == PTP_DATATYPE_AINT8))
		{
			ptr8 = (mtp_uchar *) parray;
			for (ii = 0; ii < num_ele; ii++)
				_prop_append_ele_ptparray(prop_info->default_val.array,
						ptr8[ii]);

		} else if ((prop_info->data_type == PTP_DATATYPE_AUINT16) ||
				(prop_info->data_type == PTP_DATATYPE_AINT16)) {

			ptr16 = (mtp_uint16 *) parray;
			for (ii = 0; ii < num_ele; ii++)
				_prop_append_ele_ptparray(prop_info->default_val.array,
						ptr16[ii]);

		} else if ((prop_info->data_type == PTP_DATATYPE_AUINT32) ||
				(prop_info->data_type == PTP_DATATYPE_AINT32)) {

			ptr32 = (mtp_uint32 *)parray;
			for (ii = 0; ii < num_ele; ii++)
				_prop_append_ele_ptparray(prop_info->default_val.array,
						ptr32[ii]);
		}
		return TRUE;
	}
	return FALSE;
}

mtp_bool _prop_set_current_integer(device_prop_desc_t *prop, mtp_uint32 val)
{
	if (_prop_is_valid_integer(&(prop->propinfo), val)) {
		mtp_int32 ii;
		mtp_uchar *ptr;

		ptr = (mtp_uchar *) &val;

		for (ii = 0; ii < sizeof(mtp_uint32); ii++) {
			prop->current_val.integer[ii] = ptr[ii];
		}

		return TRUE;
	} else {
		/* setting invalid value */
		return FALSE;
	}
}

mtp_bool _prop_set_current_string(device_prop_desc_t *prop, ptp_string_t *str)
{
	if (_prop_is_valid_string(&(prop->propinfo), str))
	{
		_prop_destroy_ptpstring(prop->current_val.str);
#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
		prop->current_val.str = __alloc_ptpstring();
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		prop->current_val.str = __alloc_ptpstring(str->num_chars);
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		if (prop->current_val.str != NULL) {
			_prop_copy_ptpstring(prop->current_val.str, str);
			return TRUE;
		}
		else {
			_prop_destroy_ptpstring(prop->current_val.str);
			return FALSE;
		}
	} else {
		/* setting invalid value */
		return FALSE;
	}
}

mtp_bool _prop_set_current_array(device_prop_desc_t *prop, mtp_uchar *arr)
{
	mtp_uint32 num_ele = 0;
	mtp_uchar *pval = NULL;
	memcpy(&num_ele, arr, sizeof(mtp_uint32));
	pval = arr + sizeof(mtp_uint32);

#ifdef __BIG_ENDIAN__
	/* Byte swap the number of elements */
	_util_conv_byte_order(&num_ele, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */

	/* Allocate memory for the PTP array */
	if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT8) ||
			(prop->propinfo.data_type == PTP_DATATYPE_AINT8)) {
		_prop_destroy_ptparray(prop->current_val.array);
		prop->current_val.array = _prop_alloc_ptparray(UINT8_TYPE);

	} else if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT16) ||
			(prop->propinfo.data_type == PTP_DATATYPE_AINT16)) {

		_prop_destroy_ptparray(prop->current_val.array);
		prop->current_val.array = _prop_alloc_ptparray(UINT16_TYPE);

	} else if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT32) ||
			(prop->propinfo.data_type == PTP_DATATYPE_AINT32)) {

		_prop_destroy_ptparray(prop->current_val.array);
		prop->current_val.array = _prop_alloc_ptparray(UINT32_TYPE);

	} else
		return FALSE;

	/* Copies the data into the PTP array */
	if ((prop->current_val.array != NULL) && (num_ele != 0)) {
		mtp_uchar *ptr8 = NULL;
		mtp_uint16 *ptr16 = NULL;
		mtp_uint32 *ptr32 = NULL;
		mtp_uint32 ii;
#ifdef __BIG_ENDIAN__
		/* Some temporary variables for swapping the bytes */
		mtp_uint16 swap16;
		mtp_uint32 swap32;
#endif /* __BIG_ENDIAN__ */

		_prop_grow_ptparray(prop->current_val.array, num_ele);

		if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT8) ||
				(prop->propinfo.data_type == PTP_DATATYPE_AINT8)) {

			ptr8 = (mtp_uchar *) pval;
			for (ii = 0; ii < num_ele; ii++)
				_prop_append_ele_ptparray(prop->current_val.array,
						ptr8[ii]);

		} else if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT16) ||
				(prop->propinfo.data_type == PTP_DATATYPE_AINT16)) {

			ptr16 = (mtp_uint16 *) pval;
			for (ii = 0; ii < num_ele; ii++) {
#ifdef __BIG_ENDIAN__
				swap16 = ptr16[ii];
				_util_conv_byte_order(&swap16, sizeof(mtp_uint16));
				_prop_append_ele_ptparray(prop->current_val.array,
						swap16);
#else /* __BIG_ENDIAN__ */
				_prop_append_ele_ptparray(prop->current_val.array,
						ptr16[ii]);
#endif /* __BIG_ENDIAN__ */
			}
		} else if ((prop->propinfo.data_type == PTP_DATATYPE_AUINT32) ||
				(prop->propinfo.data_type == PTP_DATATYPE_AINT32)) {

			ptr32 = (mtp_uint32 *) pval;
			for (ii = 0; ii < num_ele; ii++) {
#ifdef __BIG_ENDIAN__
				swap32 = ptr32[ii];
				_util_conv_byte_order(&swap32, sizeof(mtp_uint32));
				_prop_append_ele_ptparray(prop->current_val.array,
						swap32);
#else /* __BIG_ENDIAN__ */
				_prop_append_ele_ptparray(prop->current_val.array,
						ptr32[ii]);
#endif /* __BIG_ENDIAN__ */
			}
		}
		return TRUE;
	}

	return FALSE;
}

mtp_bool _prop_set_current_device_prop(device_prop_desc_t *prop, mtp_uchar *val,
		mtp_uint32 size)
{
	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {
		ptp_string_t str;
		_prop_init_ptpstring(&str);
		_prop_parse_rawstring(&str, val, size);
		return _prop_set_current_string(prop, &str);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		mtp_uint32 *ptr = (mtp_uint32 *) val;
		if (size < sizeof(mtp_uint32)) {
			return FALSE;
		}
		if (size < sizeof(mtp_uint32) + ptr[0] * prop->propinfo.dts_size) {
			return FALSE;
		}
		return _prop_set_current_array(prop, val);

	}
	else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		if (prop->propinfo.dts_size > size) {
			return FALSE;
		}

		if ((prop->propinfo.data_type == PTP_DATATYPE_INT64) ||
				(prop->propinfo.data_type == PTP_DATATYPE_UINT64) ||
				(prop->propinfo.data_type == PTP_DATATYPE_INT128) ||
				(prop->propinfo.data_type == PTP_DATATYPE_UINT128)) {

			/* No validation at this time */
			memcpy(prop->current_val.integer, val,
					prop->propinfo.dts_size);
#ifdef __BIG_ENDIAN__
			_util_conv_byte_order(prop->current_val.integer,
					prop->propinfo.dts_size);
#endif /* __BIG_ENDIAN__ */
			return TRUE;
		} else {
			/* avoid using new_val = *(ddword*)val; */
			mtp_uint32 new_val = (mtp_uint32)0;
			memcpy(&new_val, val, size);
#ifdef __BIG_ENDIAN__
			_util_conv_byte_order(&new_val, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
			return _prop_set_current_integer(prop, new_val);
		}
	}

	return FALSE;
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
		_prop_destroy_ptpstring(pval->current_val.str);
#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
		pval->current_val.str = __alloc_ptpstring();
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		pval->current_val.str = __alloc_ptpstring(str->num_chars);
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		if (pval->current_val.str != NULL) {
			_prop_copy_ptpstring (pval->current_val.str, str);
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
		_prop_init_ptpstring(&str);
		_prop_parse_rawstring(&str, arr, size);
		return _prop_set_current_string_val(pval, &str);

	} else if ((propinfo->data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		if (size < sizeof(mtp_uint32))
			return FALSE;

		memcpy(&num_ele, arr, sizeof(mtp_uint32));
		DBG("parsed array num [%d]\n", num_ele);

		if (size < sizeof(mtp_uint32) +
				num_ele * (propinfo->dts_size)) {

			ERR("buffer size is not enough [%d]\n", size);
			return FALSE;
		}

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

#ifdef __BIG_ENDIAN__
mtp_bool _prop_set_current_array_val_usbrawdata(obj_prop_val_t *pval,
		mtp_uchar *arr, mtp_uint32 size)
{
	mtp_uint32 num_ele = 0;
	mtp_uchar *value = NULL;
	prop_info_t *propinfo = NULL;

	propinfo = &(pval->prop->propinfo);

	if (propinfo->data_type == PTP_DATATYPE_STRING) {
		ptp_string_t str;
		_prop_init_ptpstring(&str);
		_prop_parse_rawstring(&str, arr, size);
		return _prop_set_current_string_val(pval, &str);

	} else if ((propinfo->data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		if (size < sizeof(mtp_uint32))
			return FALSE;

		memcpy(&num_ele, arr, sizeof(mtp_uint32));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(&num_ele, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
		if (size < sizeof(mtp_uint32) + num_ele * propinfo->dts_size)
			return FALSE;

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

				ptr8 = (mtp_uchar *) arr;
				for (ii = 0; ii < num_ele; ii++)
					_prop_append_ele_ptparray(pval->current_val.array,
							ptr8[ii]);

			} else if ((propinfo->data_type == PTP_DATATYPE_AUINT16) ||
					(propinfo->data_type ==
					 PTP_DATATYPE_AINT16)) {

				ptr16 = (mtp_uint16 *) arr;
#ifdef __BIG_ENDIAN__
				_util_conv_byte_order_gen_str(ptr16,
						num_ele, sizeof(mtp_uint16));
#endif /* __BIG_ENDIAN__ */
				for (ii = 0; ii < num_ele; ii++)
					_prop_append_ele_ptparray(pval->current_val.array,
							ptr16[ii]);

			} else if ((propinfo->data_type == PTP_DATATYPE_AUINT32) ||
					(propinfo->data_type ==
					 PTP_DATATYPE_AINT32)) {

				ptr32 = (mtp_uint32 *) arr;
#ifdef __BIG_ENDIAN__
				_util_conv_byte_order_gen_str(ptr32, num_ele,
						sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
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

			memcpy(pval->current_val.integer, arr,
					propinfo->dts_size);
#ifdef __BIG_ENDIAN__
			_util_conv_byte_order(pval->current_val.integer,
					propinfo->dts_size);
#endif /* __BIG_ENDIAN__ */
			return TRUE;
		} else {
			mtp_uint32 new_val = 0;
			memcpy(&new_val, arr, propinfo->dts_size);
			return _prop_set_current_integer_val(pval, new_val);
		}
	}
	return FALSE;
}
#endif /* __BIG_ENDIAN__ */

mtp_bool _prop_set_range_integer(prop_info_t *prop_info, mtp_uint32 min,
		mtp_uint32 max, mtp_uint32 step)
{
	if (((prop_info->data_type & PTP_DATATYPE_VALUEMASK) !=
				PTP_DATATYPE_VALUE) || (prop_info->form_flag != RANGE_FORM)) {
		return FALSE;

	} else {
		prop_info->range.min_val = min;
		prop_info->range.max_val = max;
		prop_info->range.step_size = step;
		return TRUE;
	}
}

mtp_bool _prop_set_regexp(obj_prop_desc_t *prop, mtp_wchar *regex)
{
	ptp_string_t *str;

	if ((prop->propinfo.data_type != PTP_DATATYPE_STRING) ||
			(prop->propinfo.form_flag != REGULAR_EXPRESSION_FORM)) {
		return FALSE;
	}
#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
	str = __alloc_ptpstring();
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
	str = __alloc_ptpstring(_util_wchar_len(regex));
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
	if (str == NULL)
		return FALSE;

	_prop_copy_char_to_ptpstring(str, regex, WCHAR_TYPE);
	prop->prop_forms.reg_exp = str;

	return TRUE;
}

mtp_bool _prop_set_maxlen(obj_prop_desc_t *prop, mtp_uint32 max)
{
	if ((prop->propinfo.form_flag != BYTE_ARRAY_FORM) &&
			(prop->propinfo.form_flag != LONG_STRING_FORM)) {
		return FALSE;
	}

	if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) !=
			PTP_DATATYPE_ARRAY) {
		return FALSE;
	}

	prop->prop_forms.max_len = max;
	return TRUE;

}

/* DeviceObjectPropDesc Functions */
void _prop_init_device_property_desc(device_prop_desc_t *prop,
		mtp_uint16 propcode, mtp_uint16 data_type, mtp_uchar get_set,
		mtp_uchar form_flag)
{
	mtp_int32 ii;

	prop->propinfo.prop_code = propcode;
	prop->propinfo.data_type = data_type;
	prop->propinfo.get_set = get_set;
	prop->propinfo.default_val.str = NULL;
	prop->current_val.str = NULL;
	prop->propinfo.form_flag = form_flag;

	for (ii = 0; ii < 16; ii++) {
		prop->current_val.integer[ii] = 0;
		prop->propinfo.default_val.integer[ii] = 0;
	}

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
		/* don't know how to handle at this point (including PTP_DATATYPE_STRING) */
		prop->propinfo.dts_size = 0;
		break;
	}

	_util_init_list(&(prop->propinfo.supp_value_list));

	return;
}

mtp_uint32 _prop_size_device_prop_desc(device_prop_desc_t *prop)
{
	/* size :PropCode,Datatype,Getset,formflag */
	mtp_uint32 size = sizeof(mtp_uint16) + sizeof(mtp_uint16) +
		sizeof(mtp_uchar) + sizeof(mtp_uchar);

	/* size of default value: DTS */
	/* size of current value: DTS */
	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		size += _prop_size_ptpstring(prop->propinfo.default_val.str);
		size += _prop_size_ptpstring(prop->current_val.str);


	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		size += _prop_get_size_ptparray(prop->propinfo.default_val.array);
		size += _prop_get_size_ptparray(prop->current_val.array);

	} else {
		size += 2 * prop->propinfo.dts_size;
	}

	/* Add the size of the Form followed */
	switch (prop->propinfo.form_flag) {
	case NONE:
		break;

	case RANGE_FORM:
		size += 3 * prop->propinfo.dts_size;
		break;

	case ENUM_FORM:
		/* Number of Values */
		size += sizeof(mtp_uint16);
		if (prop->propinfo.data_type != PTP_DATATYPE_STRING) {

			size += prop->propinfo.supp_value_list.nnodes *
				prop->propinfo.dts_size;

		} else {
			slist_node_t *node = NULL;
			mtp_uint16 ii;

			for (ii = 0, node = prop->propinfo.supp_value_list.start;
					ii < prop->propinfo.supp_value_list.nnodes;
					ii++, node = node->link) {

				size += _prop_size_ptpstring((ptp_string_t *) node->value);
			}
		}
		break;

	default:
		/* don't know how to handle */
		break;

	}

	return size;
}

static mtp_uint32 __size_curval_device_prop(device_prop_desc_t *prop)
{
	mtp_uint32 size = 0;

	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		size = _prop_size_ptpstring(prop->current_val.str);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		size = _prop_get_size_ptparray(prop->current_val.array);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		size = prop->propinfo.dts_size;
	}
	return size;
}

mtp_uint32 _prop_pack_device_prop_desc(device_prop_desc_t *prop,
		mtp_uchar *buf, mtp_uint32 size)
{
	mtp_uchar *temp = buf;
	mtp_uint32 count = 0;
	mtp_uint32 bytes_to_write = 0;
	slist_node_t *node = NULL;
	mtp_uint32 ii;

	if (!buf || size < _prop_size_device_prop_desc(prop)) {
		return 0;
	}

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

	/* Pack Default/Current Value: DTS */
	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		bytes_to_write = _prop_size_ptpstring(prop->propinfo.default_val.str);
		if (bytes_to_write !=
				_prop_pack_ptpstring(prop->propinfo.default_val.str,
					temp, bytes_to_write)) {

			return (mtp_uint32)(temp - buf);
		}
		temp += bytes_to_write;

		bytes_to_write = _prop_size_ptpstring(prop->current_val.str);
		if (bytes_to_write !=
				_prop_pack_ptpstring(prop->current_val.str,
					temp, bytes_to_write)) {

			return (mtp_uint32)(temp - buf);
		}
		temp += bytes_to_write;

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		bytes_to_write = _prop_get_size_ptparray(prop->propinfo.default_val.array);
		if (bytes_to_write !=
				_prop_pack_ptparray(prop->propinfo.default_val.array,
					temp, bytes_to_write)) {

			return (mtp_uint32)(temp - buf);
		}
		temp += bytes_to_write;

		bytes_to_write = _prop_get_size_ptparray(prop->current_val.array);
		if (bytes_to_write !=
				_prop_pack_ptparray(prop->current_val.array,
					temp, bytes_to_write)) {

			return (mtp_uint32)(temp - buf);
		}
		temp += bytes_to_write;

		/* Add support for other array data types */
	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		bytes_to_write = prop->propinfo.dts_size;
		memcpy(temp, prop->propinfo.default_val.integer, bytes_to_write);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
		temp += bytes_to_write;

		memcpy(temp, prop->current_val.integer, bytes_to_write);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
		temp += bytes_to_write;
	}


	/* Now pack the FormFlag */
	memcpy(temp, &(prop->propinfo.form_flag), sizeof(mtp_uchar));
	temp += sizeof(mtp_uchar);

	/* Finally pack the Form followed */
	switch (prop->propinfo.form_flag) {
	case NONE:
		break;

	case RANGE_FORM:
		/* Min, Max, & Step */
		memcpy(temp, &(prop->propinfo.range.min_val),
				prop->propinfo.dts_size);
		temp += prop->propinfo.dts_size;
		memcpy(temp, &(prop->propinfo.range.max_val),
				prop->propinfo.dts_size);
		temp += prop->propinfo.dts_size;
		memcpy(temp, &(prop->propinfo.range.step_size),
				prop->propinfo.dts_size);
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

					return (mtp_uint32)(temp - buf);
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

	default:
		/*don't know how to handle */
		break;

	}

	return (mtp_uint32)(temp - buf);
}

mtp_uint32 _prop_pack_curval_device_prop_desc(device_prop_desc_t *prop,
		mtp_uchar *buf, mtp_uint32 size)
{
	mtp_uint32 bytes_to_write;

	bytes_to_write = __size_curval_device_prop(prop);

	if ((!bytes_to_write) || (buf == NULL) || (size < bytes_to_write)) {
		return 0;
	}

	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {
		if (bytes_to_write != _prop_pack_ptpstring(prop->current_val.str,
					buf, bytes_to_write)) {
			return 0;
		}
	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		if (bytes_to_write != _prop_pack_ptparray(prop->current_val.array,
					buf, bytes_to_write)) {
			return 0;
		}
	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {
		/* this property is of type UINT8, ... */
		memcpy(buf, prop->current_val.integer, bytes_to_write);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(buf, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
	} else {
		return 0;
	}

	return bytes_to_write;
}

void _prop_reset_device_prop_desc(device_prop_desc_t *prop)
{
	ret_if(prop == NULL);

	if (prop->propinfo.get_set == PTP_PROPGETSET_GETONLY)
		return;

	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		_prop_destroy_ptpstring(prop->current_val.str);
#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
		prop->current_val.str = __alloc_ptpstring();
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		prop->current_val.str = __alloc_ptpstring(prop->propinfo.default_val.str->num_chars);
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		if (NULL == prop->current_val.str)
			return;

		_prop_copy_ptpstring (prop->current_val.str,
				prop->propinfo.default_val.str);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		_prop_destroy_ptparray(prop->current_val.array);
		prop->current_val.array = _prop_alloc_ptparray(UINT32_TYPE);
		if (NULL == prop->current_val.array)
			return;

		_prop_copy_ptparray(prop->current_val.array,
				prop->propinfo.default_val.array);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		mtp_int32 ii;
		for (ii = 0; ii < 16; ii++)
			prop->current_val.integer[ii] =
				prop->propinfo.default_val.integer[ii];
	}
}

/* ObjectPropVal Functions */
obj_prop_val_t * _prop_alloc_obj_propval(obj_prop_desc_t *prop)
{
	obj_prop_val_t *pval = NULL;
	pval = (obj_prop_val_t *)g_malloc(sizeof(obj_prop_val_t));

	if (pval != NULL) {
		__init_obj_propval(pval, prop);
	}

	return pval;
}

static void __init_obj_propval(obj_prop_val_t *pval, obj_prop_desc_t *prop)
{
	mtp_int32 ii;

	pval->prop = prop;

	for (ii = 0; ii < 16; ii++) {
		pval->current_val.integer[ii] = 0;
	}

	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
		pval->current_val.str = __alloc_ptpstring();
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		pval->current_val.str = __alloc_ptpstring(prop->propinfo.default_val.str->num_chars);
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
		if (NULL == pval->current_val.str)
			return;
		_prop_copy_ptpstring (pval->current_val.str,
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
	return;
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
			if (prop_val->prop->propinfo.prop_code == propcode) {
				return prop_val;
			}
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
	ERR("node or node->value is null. try again but not found");
	return NULL;
}

mtp_uint32 _prop_pack_obj_propval(obj_prop_val_t *pval, mtp_uchar *buf,
		mtp_uint32 size)
{
	mtp_uint32 bytes_to_write = _prop_size_obj_propval(pval);

	if ((!bytes_to_write) || (buf == NULL) || (size < bytes_to_write)) {
		return 0;
	}

	if (pval->prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		if (bytes_to_write != _prop_pack_ptpstring(pval->current_val.str,
					buf, bytes_to_write)) {
			return 0;
		}
	} else if ((pval->prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		if (bytes_to_write != _prop_pack_ptparray(pval->current_val.array,
					buf, size)) {
			return 0;
		}

	} else if ((pval->prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		/* this property is of type UINT8, ... */
		memcpy(buf, pval->current_val.integer, bytes_to_write);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(buf, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
	} else {
		return 0;
	}

	return bytes_to_write;
}


mtp_uint32 _prop_size_obj_propval(obj_prop_val_t *pval)
{
	mtp_uint32 size = 0;

	if (pval == NULL || pval->prop == NULL)
		return size;

	if (pval->prop->propinfo.data_type == PTP_DATATYPE_STRING) {
		if (pval->current_val.str == NULL) {
			size = 0;
		} else {
			size = _prop_size_ptpstring(pval->current_val.str);
		}

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
	if (pval == NULL) {
		return;
	}

	if (pval->prop == NULL) {
		g_free(pval);
		pval = NULL;
		return;
	}

	if (pval->prop->propinfo.data_type == PTP_DATATYPE_STRING) {
		if (pval->current_val.str) {
			_prop_destroy_ptpstring(pval->current_val.str);
			pval->current_val.str = NULL;
		}
	} else if ((pval->prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {
		_prop_destroy_ptparray(pval->current_val.array);
		pval->current_val.array = NULL;
	}

	if (pval != NULL) {
		g_free(pval);
		pval = NULL;
	}

	return;
}

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

	if (prop->propinfo.form_flag == BYTE_ARRAY_FORM) {
		prop->propinfo.data_type = PTP_DATATYPE_AUINT8;
	} else if (prop->propinfo.form_flag == LONG_STRING_FORM) {
		prop->propinfo.data_type = PTP_DATATYPE_AUINT16;
	}

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

	return;
}

static mtp_uint32 __get_size_default_val_obj_prop_desc(obj_prop_desc_t *prop)
{
	mtp_uint32 size = 0;

	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		size = _prop_size_ptpstring(prop->propinfo.default_val.str);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		size = _prop_get_size_ptparray(prop->propinfo.default_val.array);

	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {

		size = prop->propinfo.dts_size;
	}

	return size;
}

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
		if (prop->propinfo.data_type != PTP_DATATYPE_STRING) {
			size += 3 * prop->propinfo.dts_size;/* Min,Max,Step */
		}
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

	if (!buf || size < _prop_size_obj_prop_desc(prop)) {
		return 0;
	}

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

mtp_uint32 _prop_pack_default_val_obj_prop_desc(obj_prop_desc_t *prop,
		mtp_uchar *buf, mtp_uint32 size)
{
	mtp_uint32 bytes_to_write;

	bytes_to_write = __get_size_default_val_obj_prop_desc(prop);

	if ((!bytes_to_write) || (buf == NULL) || (size < bytes_to_write)) {
		return 0;
	}

	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {
		if (bytes_to_write !=
				_prop_pack_ptpstring(prop->propinfo.default_val.str,
					buf, bytes_to_write)) {
			return 0;
		}
	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {
		if (bytes_to_write !=
				_prop_pack_ptparray(prop->propinfo.default_val.array,
					buf, bytes_to_write)) {
			return 0;
		}
	} else if ((prop->propinfo.data_type & PTP_DATATYPE_VALUEMASK) ==
			PTP_DATATYPE_VALUE) {
		/* this property is of type UINT8, ... */
		memcpy(buf, prop->propinfo.default_val.integer, bytes_to_write);
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(buf, bytes_to_write);
#endif /* __BIG_ENDIAN__ */
	} else {
		return 0;
	}

	return bytes_to_write;
}

obj_prop_desc_t *_prop_get_obj_prop_desc(mtp_uint32 format_code,
		mtp_uint32 propcode)
{
	mtp_uint32 i = 0;
	int num_default_obj_props = 0;

	/*Default*/
	if (_get_oma_drm_status() == TRUE) {
		num_default_obj_props = NUM_OBJECT_PROP_DESC_DEFAULT;
	} else {
		num_default_obj_props = NUM_OBJECT_PROP_DESC_DEFAULT - 1;
	}

	for (i = 0; i < num_default_obj_props; i++) {
		if (props_list_default[i].propinfo.prop_code == propcode) {
			return &(props_list_default[i]);
		}
	}

	switch (format_code) {
	case PTP_FMT_MP3:
	case PTP_FMT_WAVE:
		for (i = 0; i < NUM_OBJECT_PROP_DESC_MP3; i++) {
			if (props_list_mp3[i].propinfo.prop_code == propcode) {
				return &(props_list_mp3[i]);
			}
		}
		break;
	case MTP_FMT_WMA:
		for (i = 0; i < NUM_OBJECT_PROP_DESC_WMA; i++) {
			if (props_list_wma[i].propinfo.prop_code == propcode) {
				return &(props_list_wma[i]);
			}
		}
		break;
	case MTP_FMT_WMV:
	case PTP_FMT_ASF:
	case MTP_FMT_MP4:
	case PTP_FMT_AVI:
	case PTP_FMT_MPEG:
	case MTP_FMT_3GP:
		for (i = 0; i < NUM_OBJECT_PROP_DESC_WMV; i++) {
			if (props_list_wmv[i].propinfo.prop_code == propcode) {
				return &(props_list_wmv[i]);
			}
		}
		break;
	case MTP_FMT_ABSTRACT_AUDIO_ALBUM:
	case PTP_FMT_IMG_EXIF:
	case PTP_FMT_IMG_GIF:
	case PTP_FMT_IMG_BMP:
	case PTP_FMT_IMG_PNG:
		for (i = 0; i < NUM_OBJECT_PROP_DESC_ALBUM; i++) {
			if (props_list_album[i].propinfo.prop_code == propcode) {
				return &(props_list_album[i]);
			}
		}
		break;

	default:
		break;
	}

	ERR("No matched property[0x%x], format[0x%x]!!\n", propcode,
			format_code);
	return NULL;
}

static void __destroy_obj_prop_desc(obj_prop_desc_t *prop)
{
	slist_node_t *node = NULL;
	slist_node_t *next_node = NULL;
	mtp_uint32 ii;

	if (prop->propinfo.data_type == PTP_DATATYPE_STRING) {

		if (prop->propinfo.default_val.str) {

			_prop_destroy_ptpstring(prop->propinfo.default_val.str);
			prop->propinfo.default_val.str = NULL;
		}

		if (prop->propinfo.form_flag == ENUM_FORM) {

			for (node = prop->propinfo.supp_value_list.start, ii = 0;
					ii < prop->propinfo.supp_value_list.nnodes;
					node = node->link, ii++) {
				_prop_destroy_ptpstring((ptp_string_t *) node->value);
			}
		}

		if (prop->propinfo.form_flag == REGULAR_EXPRESSION_FORM) {

			if (prop->prop_forms.reg_exp != NULL) {
				_prop_destroy_ptpstring(prop->prop_forms.reg_exp);
				prop->prop_forms.reg_exp = NULL;
			}
		}
	} else if ((prop->propinfo.data_type & PTP_DATATYPE_ARRAYMASK) ==
			PTP_DATATYPE_ARRAY) {

		if (prop->propinfo.default_val.array) {

			_prop_destroy_ptparray(prop->propinfo.default_val.array);
			prop->propinfo.default_val.array = NULL;
		}
	}

	/* deallocate memory consumed by list elements  */
	next_node = prop->propinfo.supp_value_list.start;
	for (ii = 0; ii < prop->propinfo.supp_value_list.nnodes; ii++) {
		node = next_node;
		next_node = next_node->link;
		g_free(node);
	}
	return;
}

/* Objectproplist functions */
static mtp_uint32 __count_obj_proplist(obj_proplist_t *prop_list)
{
	return prop_list->prop_quad_list.nnodes;
}

mtp_uint32 _prop_size_obj_proplist(obj_proplist_t *prop_list)
{
	prop_quad_t *quad = NULL;
	slist_node_t *node = NULL;
	mtp_uint32 ii;
	mtp_uint32 size;

	/* for the NumberOfElements field in objpropvalList Dataset */
	size = sizeof(mtp_uint32);

	/* add all the fixed length members of the list */
	size += prop_list->prop_quad_list.nnodes * (sizeof(mtp_uint32) +
			sizeof(mtp_uint16) + sizeof(mtp_uint16));
	node = prop_list->prop_quad_list.start;
	for (ii = 0; ii < prop_list->prop_quad_list.nnodes; ii++) {
		quad = (prop_quad_t *) node->value;
		if (quad) {
			size += quad->val_size;
		}
		node = node->link;
	}
	return size;
}

mtp_uint32 _prop_get_obj_proplist(mtp_obj_t *obj, mtp_uint32 propcode,
		mtp_uint32 group_code, obj_proplist_t *prop_list)
{
	obj_prop_val_t *propval = NULL;
	slist_node_t *node = NULL;
	mtp_uint32 ii = 0;

	if (obj->propval_list.nnodes == 0) {
		if (FALSE == _prop_update_property_values_list(obj)) {
			ERR("update Property Values FAIL!!");
			return 0;
		}
	}
	for (ii = 0, node = obj->propval_list.start;
			ii < obj->propval_list.nnodes;
			ii++, node = node->link) {
		propval = (obj_prop_val_t *)node->value;

		if (NULL == propval) {
			continue;
		}

		if (FALSE == __check_object_propcode(propval->prop,
					propcode, group_code)) {
			continue;
		}
		__append_obj_proplist(prop_list, obj->obj_handle,
				propval->prop->propinfo.prop_code,
				propval->prop->propinfo.data_type,
				(mtp_uchar *)propval->current_val.integer);
	}

	return __count_obj_proplist(prop_list);
}

mtp_bool _prop_update_property_values_list(mtp_obj_t *obj)
{
	mtp_uint32 ii = 0;
	mtp_char guid[16] = { 0 };
	slist_node_t *node = NULL;
	slist_node_t *next_node = NULL;
	ptp_time_string_t create_tm, modify_tm;
	mtp_uint32 fmt_code = obj->obj_info->obj_fmt;
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
	if (obj->file_path == NULL || obj->file_path[0] != '/') {
		ERR_SECURE("Path is not valid.. path = [%s]\n",
				obj->file_path);
		return FALSE;
	}

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

	/*Get DCM data*/
	if (fmt_code == PTP_FMT_MP3 ||
			fmt_code == MTP_FMT_WMA ||
			fmt_code == PTP_FMT_WAVE ||
			fmt_code == MTP_FMT_FLAC) {
		__update_prop_values_audio(obj);

	} else if (fmt_code == MTP_FMT_WMV ||
			fmt_code == PTP_FMT_ASF ||
			fmt_code == MTP_FMT_MP4 ||
			fmt_code == PTP_FMT_AVI ||
			fmt_code == PTP_FMT_MPEG) {
		__update_prop_values_video(obj);

	} else if (fmt_code == PTP_FMT_IMG_EXIF ||
			fmt_code == PTP_FMT_IMG_BMP ||
			fmt_code == PTP_FMT_IMG_GIF ||
			fmt_code == PTP_FMT_IMG_PNG) {
		__update_prop_values_image(obj);
	}

	return TRUE;
}

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

mtp_uint32 _prop_pack_obj_proplist(obj_proplist_t *prop_list, mtp_uchar *buf,
		mtp_uint32 size)
{
	mtp_uchar *temp = buf;
	ptp_string_t *str = NULL;
	prop_quad_t *quad = NULL;
	mtp_uint32 ii;
	slist_node_t *node = NULL;

	if (!buf || size < _prop_size_obj_proplist(prop_list)) {
		return 0;
	}

	*(mtp_uint32 *) buf = prop_list->prop_quad_list.nnodes;
#ifdef __BIG_ENDIAN__
	_util_conv_byte_order(temp, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
	temp += sizeof(mtp_uint32);

	/* Pack the array elements */
	node = prop_list->prop_quad_list.start;
	for (ii = 0; ii < prop_list->prop_quad_list.nnodes; ii++) {
		quad = (prop_quad_t *) node->value;
		node = node->link;
		/* pack the fixed length members */
		memcpy(temp, &quad->obj_handle, sizeof(mtp_uint32));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
		temp += sizeof(mtp_uint32);

		memcpy(temp, &quad->prop_code, sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
		temp += sizeof(mtp_uint16);

		memcpy(temp, &quad->data_type, sizeof(mtp_uint16));
#ifdef __BIG_ENDIAN__
		_util_conv_byte_order(temp, sizeof(mtp_uint32));
#endif /* __BIG_ENDIAN__ */
		temp += sizeof(mtp_uint16);

		/* Pack property value */
		if ((quad->data_type & PTP_DATATYPE_VALUEMASK) ==
				PTP_DATATYPE_VALUE) {

			memcpy(temp, quad->pval, quad->val_size);
#ifdef __BIG_ENDIAN__
			_util_conv_byte_order(temp, quad->val_size);
#endif /* __BIG_ENDIAN__ */
			temp += quad->val_size;

		} else if (quad->data_type == PTP_DATATYPE_STRING) {

			str = (ptp_string_t *) quad->pval;
			if (str) {
				temp += _prop_pack_ptpstring(str, temp,
						_prop_size_ptpstring (str));
			} else {
				/* Put in an empty string: NumOfChars = 0; */
				*temp++ = 0;
			}
		} else if ((quad->data_type & PTP_DATATYPE_ARRAYMASK) ==
				PTP_DATATYPE_ARRAY) {

			if (quad->pval != NULL) {

				if (quad->val_size !=
						_prop_pack_ptparray((ptp_array_t *)quad->pval,
							temp, quad->val_size)) {
					return (mtp_uint32) (temp - buf);
				}
				temp += quad->val_size;
			} else {
				/* Fill in an empty array: mtp_uint32 */
				mtp_uint32 zero = 0;
				memcpy(temp, &zero, sizeof(mtp_uint32));
				temp += sizeof(mtp_uint32);
			}
		}
	}

	return (mtp_uint32)(temp - buf);
}

void _prop_destroy_obj_proplist(obj_proplist_t *prop_list)
{
	slist_node_t *node = NULL;
	slist_node_t *next_node = NULL;
	mtp_uint32 ii;

	for (ii = 0, node = prop_list->prop_quad_list.start;
			ii < prop_list->prop_quad_list.nnodes; ii++, node = node->link) {
		g_free(node->value);
	}

	next_node = prop_list->prop_quad_list.start;

	for (ii = 0; ii < prop_list->prop_quad_list.nnodes; ii++) {
		node = next_node;
		next_node = node->link;
		g_free(node);
	}
	return;
}

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
#ifndef MTP_USE_VARIABLE_PTP_STRING_MALLOC
	str = __alloc_ptpstring();
#else /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */
	str = __alloc_ptpstring(_util_wchar_len(val));
#endif /* MTP_USE_VARIABLE_PTP_STRING_MALLOC */

	if (str != NULL) {
		_prop_copy_char_to_ptpstring(str, val, WCHAR_TYPE);
		ret = _util_add_node(&(prop_info->supp_value_list), (void *)str);
		if (ret == FALSE) {
			ERR("List add Fail");
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
	if (_get_oma_drm_status() == TRUE) {
		num_default_obj_props = NUM_OBJECT_PROP_DESC_DEFAULT;
	} else {
		num_default_obj_props = NUM_OBJECT_PROP_DESC_DEFAULT - 1;
	}

	for (i = 0; i < num_default_obj_props; i++) {
		_prop_append_ele_ptparray(supp_props,
				props_list_default[i].propinfo.prop_code);
	}

	switch (format_code) {
	case PTP_FMT_MP3:
	case PTP_FMT_WAVE:
		for (i = 0; i < NUM_OBJECT_PROP_DESC_MP3; i++) {
			_prop_append_ele_ptparray(supp_props,
					props_list_mp3[i].propinfo.prop_code);
		}
		break;
	case MTP_FMT_WMA:
		for (i = 0; i < NUM_OBJECT_PROP_DESC_WMA; i++) {
			_prop_append_ele_ptparray(supp_props,
					props_list_wma[i].propinfo.prop_code);
		}
		break;
	case MTP_FMT_WMV:
	case PTP_FMT_ASF:
	case MTP_FMT_MP4:
	case PTP_FMT_AVI:
	case PTP_FMT_MPEG:
	case MTP_FMT_3GP:
		for (i = 0; i < NUM_OBJECT_PROP_DESC_WMV; i++) {
			_prop_append_ele_ptparray(supp_props,
					props_list_wmv[i].propinfo.prop_code);
		}
		break;
	case MTP_FMT_ABSTRACT_AUDIO_ALBUM:
	case PTP_FMT_IMG_EXIF:
	case PTP_FMT_IMG_GIF:
	case PTP_FMT_IMG_BMP:
	case PTP_FMT_IMG_PNG:
		for (i = 0; i < NUM_OBJECT_PROP_DESC_ALBUM; i++) {
			_prop_append_ele_ptparray(supp_props,
					props_list_album[i].propinfo.prop_code);
		}
		break;
	default:
		break;
	}
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

	if (initialized == TRUE) {
		DBG("already supported list is in there. just return!!");
		return TRUE;
	}

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

	default_val = PTP_FMT_UNDEF;
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

mtp_bool _prop_build_supp_props_mp3(void)
{
	static mtp_bool initialized = FALSE;
	mtp_uchar i = 0;
	mtp_uint32 default_val;

	if (initialized == TRUE) {
		DBG("already supported list is in there. just return!!");
		return TRUE;
	}

	/*Common properties 1 - 8 */
	__build_supported_common_props(&i, &props_list_mp3[i]);

	/*
	 * MTP_OBJ_PROPERTYCODE_AUDIOBITRATE (9)
	 */
	__init_obj_prop_desc(&(props_list_mp3[i]),
			MTP_OBJ_PROPERTYCODE_AUDIOBITRATE,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_AUDIO_BITRATE_UNKNOWN;
	_prop_set_range_integer(&(props_list_mp3[i].propinfo),
			MTP_AUDIO_BITRATE_UNKNOWN, MTP_AUDIO_BITRATE_BLUERAY, 1L);
	_prop_set_default_integer(&(props_list_mp3[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_SAMPLERATE (10)
	 */
	__init_obj_prop_desc(&(props_list_mp3[i]),
			MTP_OBJ_PROPERTYCODE_SAMPLERATE,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_AUDIO_SAMPLERATE_UNKNOWN;
	_prop_set_range_integer(&(props_list_mp3[i].propinfo),
			MTP_AUDIO_SAMPLERATE_8K, MTP_AUDIO_SAMPLERATE_48K, 1L);
	_prop_set_default_integer(&(props_list_mp3[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_NUMBEROFCHANNELS (11)
	 */
	__init_obj_prop_desc(&(props_list_mp3[i]),
			MTP_OBJ_PROPERTYCODE_NUMBEROFCHANNELS,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_CHANNELS_MONO;
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_CHANNELS_NOT_USED);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_CHANNELS_MONO);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_CHANNELS_STEREO);
	_prop_set_default_integer(&(props_list_mp3[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_AUDIOWAVECODEC (12)
	 */
	__init_obj_prop_desc(&(props_list_mp3[i]),
			MTP_OBJ_PROPERTYCODE_AUDIOWAVECODEC,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_WAVEFORMAT_UNKNOWN;
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_UNKNOWN);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_PCM);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_ADPCM);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_IEEEFLOAT);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_DTS);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_DRM);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_WMSP2);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_GSM610);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_MSNAUDIO);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_MPEG);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_MPEGLAYER3);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO1);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO2);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO3);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_WMAUDIOLOSSLESS);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_WMASPDIF);
	_prop_add_supp_integer_val(&(props_list_mp3[i].propinfo),
			MTP_WAVEFORMAT_AAC);
	_prop_set_default_integer(&(props_list_mp3[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_DESCRIPTION (13)
	 */
	__init_obj_prop_desc(&(props_list_mp3[i]),
			MTP_OBJ_PROPERTYCODE_DESCRIPTION,
			PTP_DATATYPE_AUINT16,
			PTP_PROPGETSET_GETONLY,
			LONG_STRING_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_maxlen(&(props_list_mp3[i]), MAX_PTP_STRING_CHARS);
	_prop_set_default_array(&(props_list_mp3[i].propinfo), NULL, 0);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_COPYRIGHTINFO (14)
	 */
	__init_obj_prop_desc(&(props_list_mp3[i]),
			MTP_OBJ_PROPERTYCODE_COPYRIGHTINFO,
			PTP_DATATYPE_AUINT16,
			PTP_PROPGETSET_GETONLY,
			LONG_STRING_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_maxlen(&(props_list_mp3[i]), MAX_PTP_STRING_CHARS);
	_prop_set_default_array(&(props_list_mp3[i].propinfo), NULL, 0);

	initialized = TRUE;
	return TRUE;
}

mtp_bool _prop_build_supp_props_wma(void)
{
	static mtp_bool initialized = FALSE;
	mtp_uchar i = 0;
	mtp_uint32 default_val;

	if (initialized == TRUE) {
		DBG("already supported list is in there. just return!!");
		return TRUE;
	}

	/*Common properties 1 - 8 */
	__build_supported_common_props(&i, &props_list_wma[i]);
	/*
	 * MTP_OBJ_PROPERTYCODE_AUDIOBITRATE (9)
	 */
	__init_obj_prop_desc(&(props_list_wma[i]),
			MTP_OBJ_PROPERTYCODE_AUDIOBITRATE,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_AUDIO_BITRATE_UNKNOWN;
	_prop_set_range_integer(&(props_list_wma[i].propinfo),
			MTP_AUDIO_BITRATE_UNKNOWN, MTP_AUDIO_BITRATE_BLUERAY, 1L);
	_prop_set_default_integer(&(props_list_wma[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_SAMPLERATE (10)
	 */
	__init_obj_prop_desc(&(props_list_wma[i]),
			MTP_OBJ_PROPERTYCODE_SAMPLERATE,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_AUDIO_SAMPLERATE_UNKNOWN;
	_prop_set_range_integer(&(props_list_wma[i].propinfo),
			MTP_AUDIO_SAMPLERATE_8K, MTP_AUDIO_SAMPLERATE_48K, 1L);
	_prop_set_default_integer(&(props_list_wma[i]).propinfo,
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_NUMBEROFCHANNELS (11)
	 */
	__init_obj_prop_desc(&(props_list_wma[i]),
			MTP_OBJ_PROPERTYCODE_NUMBEROFCHANNELS,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_CHANNELS_MONO;
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_CHANNELS_NOT_USED);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_CHANNELS_MONO);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_CHANNELS_STEREO);
	_prop_set_default_integer(&(props_list_wma[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_AUDIOWAVECODEC (12)
	 */
	__init_obj_prop_desc(&(props_list_wma[i]),
			MTP_OBJ_PROPERTYCODE_AUDIOWAVECODEC,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM,
			MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_WAVEFORMAT_UNKNOWN;
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_UNKNOWN);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_PCM);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_ADPCM);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_IEEEFLOAT);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_DTS);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_DRM);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_WMSP2);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_GSM610);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_MSNAUDIO);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_MPEG);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_MPEGLAYER3);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO1);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO2);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO3);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_WMAUDIOLOSSLESS);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_WMASPDIF);
	_prop_add_supp_integer_val(&(props_list_wma[i].propinfo),
			MTP_WAVEFORMAT_AAC);
	_prop_set_default_integer(&(props_list_wma[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_DESCRIPTION (13)
	 */
	__init_obj_prop_desc(&(props_list_wma[i]),
			MTP_OBJ_PROPERTYCODE_DESCRIPTION,
			PTP_DATATYPE_AUINT16,
			PTP_PROPGETSET_GETONLY,
			LONG_STRING_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_maxlen(&(props_list_wma[i]), MAX_PTP_STRING_CHARS);
	_prop_set_default_array(&(props_list_wma[i].propinfo), NULL, 0);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_COPYRIGHTINFO (14)
	 */
	__init_obj_prop_desc(&(props_list_wma[i]),
			MTP_OBJ_PROPERTYCODE_COPYRIGHTINFO,
			PTP_DATATYPE_AUINT16,
			PTP_PROPGETSET_GETONLY,
			LONG_STRING_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_maxlen(&(props_list_wma[i]), MAX_PTP_STRING_CHARS);
	_prop_set_default_array(&(props_list_wma[i].propinfo), NULL, 0);

	/*-------------------------------------------------------------
	 * Valid Configurations for interdependent Object properties
	 *-------------------------------------------------------------
	 */
#ifdef MTP_SUPPORT_INTERDEPENDENTPROP
	{
		interdep_prop_config_t *ptr_interdep_prop_cfg = NULL;
		obj_prop_desc_t *prop = NULL;
		mtp_uint32 default_val;

		_util_init_list(&(interdep_proplist.plist));

		prop = (obj_prop_desc_t *)g_malloc(sizeof(obj_prop_desc_t) * 4);
		if (!prop) {
			ERR("prop g_malloc fail");
			return FALSE;
		}
		/* Valid config. 1 for Bit Rate and Sample Rate */
		ptr_interdep_prop_cfg =
			(interdep_prop_config_t *)g_malloc(sizeof(interdep_prop_config_t));
		if (!ptr_interdep_prop_cfg) {
			g_free(prop);
			ERR("ptr_interdep_prop_config g_malloc fail");
			return FALSE;
		}

		_util_init_list(&(ptr_interdep_prop_cfg->propdesc_list));
		if (!ptr_interdep_prop_cfg) {
			g_free(prop);
			return FALSE;
		}
		ptr_interdep_prop_cfg->format_code = MTP_FMT_WMA;
		i = 0;

		__init_obj_prop_desc(&(prop[i]),
				MTP_OBJ_PROPERTYCODE_TOTALBITRATE,
				PTP_DATATYPE_UINT32,
				PTP_PROPGETSET_GETONLY,
				RANGE_FORM, MTP_PROP_GROUPCODE_GENERAL);

		default_val = MTP_AUDIO_BITRATE_192K;
		_prop_set_range_integer(&(prop[i].propinfo),
				MTP_AUDIO_BITRATE_192K,
				MTP_AUDIO_BITRATE_256K, 1000L);
		_prop_set_default_integer(&(prop[i].propinfo),
				(mtp_uchar *)&default_val);
		__append_interdep_prop(ptr_interdep_prop_cfg, &(prop[i]));
		i++;

		__init_obj_prop_desc(&(prop[i]),
				MTP_OBJ_PROPERTYCODE_SAMPLERATE,
				PTP_DATATYPE_UINT32,
				PTP_PROPGETSET_GETONLY,
				ENUM_FORM, MTP_PROP_GROUPCODE_GENERAL);

		_prop_add_supp_integer_val(&(prop[i].propinfo),
				MTP_AUDIO_SAMPLERATE_DVD);
		_prop_set_default_integer(&(prop[i].propinfo),
				(mtp_uchar *)MTP_AUDIO_SAMPLERATE_DVD);
		__append_interdep_prop(ptr_interdep_prop_cfg, &(prop[i]));
		i++;

		/* Created one valid configuration */

		mtp_bool ret;
		ret = _util_add_node(&(interdep_proplist.plist),
				(void *)ptr_interdep_prop_cfg);
		if (ret == FALSE) {
			g_free(prop);
			ERR("List add Fail");
			return FALSE;
		}

		/* Valid config. 2 for Bit Rate and Sample Rate */
		ptr_interdep_prop_cfg =
			(interdep_prop_config_t *)g_malloc(sizeof(interdep_prop_config_t));
		if (!ptr_interdep_prop_cfg) {
			g_free(prop);
			ERR("ptr_interdep_prop_cfg g_malloc fail");
			return FALSE;
		}
		_util_init_list(&(ptr_interdep_prop_cfg->propdesc_list));

		if (!ptr_interdep_prop_cfg) {
			g_free(prop);
			return FALSE;
		}
		ptr_interdep_prop_cfg->format_code = MTP_FMT_WMA;

		__init_obj_prop_desc(&(prop[i]),
				MTP_OBJ_PROPERTYCODE_TOTALBITRATE,
				PTP_DATATYPE_UINT32,
				PTP_PROPGETSET_GETONLY,
				RANGE_FORM, MTP_PROP_GROUPCODE_GENERAL);

		default_val = 0x0;
		_prop_set_range_integer(&(prop[i].propinfo),
				MTP_AUDIO_BITRATE_GSM,
				MTP_AUDIO_BITRATE_BLUERAY, 1L);
		_prop_set_default_integer(&(prop[i].propinfo),
				(mtp_uchar *)&default_val);
		__append_interdep_prop(ptr_interdep_prop_cfg, &(prop[i]));
		i++;

		__init_obj_prop_desc(&(prop[i]),
				MTP_OBJ_PROPERTYCODE_SAMPLERATE,
				PTP_DATATYPE_UINT32,
				PTP_PROPGETSET_GETONLY,
				ENUM_FORM, MTP_PROP_GROUPCODE_GENERAL);

		default_val = MTP_AUDIO_SAMPLERATE_CD;
		_prop_add_supp_integer_val(&(prop[i].propinfo),
				MTP_AUDIO_SAMPLERATE_32K);
		_prop_add_supp_integer_val(&(prop[i].propinfo),
				MTP_AUDIO_SAMPLERATE_CD);
		_prop_set_default_integer(&(prop[i].propinfo),
				(mtp_uchar *)&default_val);
		__append_interdep_prop(ptr_interdep_prop_cfg, &(prop[i]));
		i++;

		/* Created one valid configuration */
		ret = _util_add_node(&(interdep_proplist.plist),
				(void *)ptr_interdep_prop_cfg);
		if (ret == FALSE) {
			g_free(prop);
			ERR("List add Fail");
			return FALSE;
		}
		g_free(prop);
	}
#endif /*MTP_SUPPORT_INTERDEPENDENTPROP*/

	initialized = TRUE;

	return TRUE;
}

mtp_bool _prop_build_supp_props_wmv(void)
{
	static mtp_bool initialized = FALSE;
	mtp_wchar buff[3] = { 0 };
	mtp_uchar i = 0;
	mtp_uint32 default_val;

	if (initialized == TRUE) {
		DBG("already supported list is in there. just return!!");
		return TRUE;
	}
	_util_utf8_to_utf16(buff, sizeof(buff) / WCHAR_SIZ, "");

	/*Common properties 1 - 8 */
	__build_supported_common_props(&i, &props_list_wmv[i]);

	/*
	 * MTP_OBJ_PROPERTYCODE_WIDTH (9)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_WIDTH,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = 0x0;
	_prop_set_range_integer(&(props_list_wmv[i].propinfo),
			MTP_MIN_VIDEO_WIDTH, MTP_MAX_VIDEO_WIDTH,
			MTP_VIDEO_HEIGHT_WIDTH_INTERVAL);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_HEIGHT (10)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_HEIGHT,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = 0x0;
	_prop_set_range_integer(&(props_list_wmv[i].propinfo),
			MTP_MIN_VIDEO_HEIGHT, MTP_MAX_VIDEO_HEIGHT,
			MTP_VIDEO_HEIGHT_WIDTH_INTERVAL);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);

	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_SAMPLERATE (11)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_SAMPLERATE,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_AUDIO_SAMPLERATE_8K;
	_prop_set_range_integer(&(props_list_wmv[i]).propinfo,
			MTP_AUDIO_SAMPLERATE_8K,
			MTP_AUDIO_SAMPLERATE_DVD, MTP_AUDIO_SAMPLE_RATE_INTERVAL);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_NUMBEROFCHANNELS (12)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_NUMBEROFCHANNELS,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_CHANNELS_MONO;
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_CHANNELS_NOT_USED);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_CHANNELS_MONO);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_CHANNELS_STEREO);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_AUDIOWAVECODEC (13)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_AUDIOWAVECODEC,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_WAVEFORMAT_UNKNOWN;
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_UNKNOWN);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_MPEGLAYER3);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO1);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO2);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_PCM);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_ADPCM);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_MSAUDIO3);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_RAW_AAC1);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_WAVEFORMAT_MPEG_HEAAC);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_AUDIOBITRATE (14)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_AUDIOBITRATE,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_AUDIO_BITRATE_UNKNOWN;
	_prop_set_range_integer(&(props_list_wmv[i].propinfo),
			MTP_AUDIO_BITRATE_UNKNOWN,
			MTP_AUDIO_BITRATE_BLUERAY, 1L);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_VIDEOFOURCCCODEC (15)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_VIDEOFOURCCCODEC,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_VIDEOFOURCC_MP42;
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_UNKNOWN);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_H263);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_H264);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_MP42);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_MP43);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_WMV1);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_WMV2);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_WMV3);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_DIVX);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_XVID);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_M4S2);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_MP4V);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_AVC1);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_h264);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_X264);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_VIDEOFOURCC_N264);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_VIDEOBITRATE (16)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_VIDEOBITRATE,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_MIN_VIDEO_BITRATE;
	_prop_set_range_integer(&(props_list_wmv[i].propinfo),
			MTP_MIN_VIDEO_BITRATE, MTP_MAX_VIDEO_BITRATE, 1L);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_FRAMESPER1KSECONDS (17)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_FRAMESPER1KSECONDS,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_MIN_VIDEO_FPS;
	_prop_set_range_integer(&(props_list_wmv[i].propinfo), MTP_MIN_VIDEO_FPS,
			MTP_MAX_VIDEO_FPS, 1L);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_DESCRIPTION (18)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_DESCRIPTION,
			PTP_DATATYPE_AUINT16,
			PTP_PROPGETSET_GETONLY,
			LONG_STRING_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_maxlen(&(props_list_wmv[i]), MAX_PTP_STRING_CHARS);
	_prop_set_default_array(&(props_list_wmv[i].propinfo), NULL, 0);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_COPYRIGHTINFO (19)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_COPYRIGHTINFO,
			PTP_DATATYPE_AUINT16,
			PTP_PROPGETSET_GETONLY,
			LONG_STRING_FORM,
			MTP_PROP_GROUPCODE_GENERAL);

	_prop_set_maxlen(&(props_list_wmv[i]), MAX_PTP_STRING_CHARS);
	_prop_set_default_array(&(props_list_wmv[i].propinfo), NULL, 0);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_ENCODINGPROFILE (20)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_ENCODINGPROFILE,
			PTP_DATATYPE_STRING,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM, MTP_PROP_GROUPCODE_OBJECT);

	_util_utf8_to_utf16(buff, sizeof(buff) / WCHAR_SIZ, "SP");
	_prop_add_supp_string_val(&(props_list_wmv[i].propinfo), buff);
	_util_utf8_to_utf16(buff, sizeof(buff) / WCHAR_SIZ, "MP");
	_prop_add_supp_string_val(&(props_list_wmv[i].propinfo), buff);
	_util_utf8_to_utf16(buff, sizeof(buff) / WCHAR_SIZ, "SP");
	_prop_set_default_string(&(props_list_wmv[i].propinfo), buff);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_METAGENRE (21)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_METAGENRE,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_METAGENRE_NOT_USED;
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_NOT_USED);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_GENERIC_MUSIC_AUDIO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_GENERIC_NONMUSIC_AUDIO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_SPOKEN_WORD_AUDIO_BOOK_FILES);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_SPOKEN_WORD_NONAUDIO_BOOK_FILES);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_SPOKEN_WORD_NEWS);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_GENERIC_VIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_NEWS_VIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_MUSIC_VIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_HOME_VIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_FEATURE_FILM_VIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_TV_SHOW_VIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_TRAINING_VIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_PHOTO_MONTAGE_VIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_GENERIC_NONAUDIO_NONVIDEO_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_AUDIO_MEDIA_CAST_FILE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_METAGENRE_VIDEO_MEDIA_CAST_FILE);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_SCANTYPE (22)
	 */
	__init_obj_prop_desc(&(props_list_wmv[i]),
			MTP_OBJ_PROPERTYCODE_SCANTYPE,
			PTP_DATATYPE_UINT16,
			PTP_PROPGETSET_GETONLY,
			ENUM_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = MTP_SCANTYPE_NOT_USED;
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_SCANTYPE_NOT_USED);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_SCANTYPE_PROGESSIVE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_SCANTYPE_FIELDINTERLEAVEDUPPERFIRST);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_SCANTYPE_FIELDINTERLEAVEDLOWERFIRST);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_SCANTYPE_FIELDSINGLEUPPERFIRST);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_SCANTYPE_FIELDSINGLELOWERFIRST);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_SCANTYPE_MIXEDINTERLACE);
	_prop_add_supp_integer_val(&(props_list_wmv[i].propinfo),
			MTP_SCANTYPE_MIXEDINTERLACEANDPROGRESSIVE);
	_prop_set_default_integer(&(props_list_wmv[i].propinfo),
			(mtp_uchar *)&default_val);

#ifdef MTP_SUPPORT_PROPERTY_SAMPLE
	__build_supported_sample_props(&i, &props_list_wmv[i]);
#endif /*MTP_SUPPORT_PROPERTY_SAMPLE*/

	initialized = TRUE;

	return TRUE;
}

mtp_bool _prop_build_supp_props_album(void)
{
	static mtp_bool initialized = FALSE;
	mtp_uchar i = 0;
	mtp_uint32 default_val;

	if (initialized == TRUE) {
		DBG("already supported list is in there. just return!!");
		return TRUE;
	}

	/*
	 * MTP_OBJ_PROPERTYCODE_WIDTH (1)
	 */
	__init_obj_prop_desc(&(props_list_album[i]),
			MTP_OBJ_PROPERTYCODE_WIDTH,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = 0x0;
	_prop_set_range_integer(&(props_list_album[i].propinfo), 0,
			MTP_MAX_IMG_WIDTH, 1L);
	_prop_set_default_integer(&(props_list_album[i].propinfo),
			(mtp_uchar *)&default_val);
	i++;

	/*
	 * MTP_OBJ_PROPERTYCODE_HEIGHT (2)
	 */
	__init_obj_prop_desc(&(props_list_album[i]),
			MTP_OBJ_PROPERTYCODE_HEIGHT,
			PTP_DATATYPE_UINT32,
			PTP_PROPGETSET_GETONLY,
			RANGE_FORM, MTP_PROP_GROUPCODE_OBJECT);

	default_val = 0x0;
	_prop_set_range_integer(&(props_list_album[i].propinfo), 0,
			MTP_MAX_IMG_HEIGHT, 1L);
	_prop_set_default_integer(&(props_list_album[i].propinfo),
			(mtp_uchar *)&default_val);

	/*
	 * SAMPLE PROPERTIES (3-8)
	 */
#ifdef MTP_SUPPORT_PROPERTY_SAMPLE
	i++
		__build_supported_sample_props(&i, &props_list_album[i]);
#endif /*MTP_SUPPORT_PROPERTY_SAMPLE*/

	initialized = TRUE;

	return TRUE;
}

void _prop_destroy_supp_obj_props(void)
{
	mtp_uint32 i = 0;
	int num_default_obj_prps = 0;

	for (i = 0; i < NUM_OBJECT_PROP_DESC_MP3; i++) {
		__destroy_obj_prop_desc(&(props_list_mp3[i]));
	}

	for (i = 0; i < NUM_OBJECT_PROP_DESC_WMA; i++) {
		__destroy_obj_prop_desc(&(props_list_wma[i]));
	}

	for (i = 0; i < NUM_OBJECT_PROP_DESC_WMV; i++) {
		__destroy_obj_prop_desc(&(props_list_wmv[i]));
	}

	for (i = 0; i < NUM_OBJECT_PROP_DESC_ALBUM; i++) {
		__destroy_obj_prop_desc(&(props_list_album[i]));
	}

	if (_get_oma_drm_status() == TRUE) {
		num_default_obj_prps = NUM_OBJECT_PROP_DESC_DEFAULT;
	} else {
		num_default_obj_prps = NUM_OBJECT_PROP_DESC_DEFAULT - 1;
	}

	for (i = 0; i < num_default_obj_prps; i++) {
		__destroy_obj_prop_desc(&(props_list_default[i]));
	}
	return;
}

#ifdef MTP_SUPPORT_INTERDEPENDENTPROP
static mtp_bool __append_interdep_prop(interdep_prop_config_t *prop_config,
		obj_prop_desc_t *prop)
{
	mtp_bool ret;
	if (prop != NULL) {
		ret = _util_add_node(&(prop_config->propdesc_list),
				(void*)prop);
		if (ret == FALSE) {
			ERR("list add Fail");
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
#endif /* MTP_SUPPORT_INTERDEPENDENTPROP */

mtp_uint32 _prop_get_size_interdep_prop(interdep_prop_config_t *prop_config)
{
	obj_prop_desc_t *prop = NULL;
	slist_node_t *node;
	mtp_int32 ii;
	mtp_uint32 size = sizeof(mtp_uint32);

	node = prop_config->propdesc_list.start;
	for (ii = 0; ii < prop_config->propdesc_list.nnodes; ii++) {
		prop = node->value;
		if (prop) {
			size += _prop_size_obj_prop_desc(prop);
		}
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

	if (!buf || size < _prop_get_size_interdep_prop(prop_config)) {
		return 0;
	}

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

mtp_uint32 _prop_get_size_interdep_proplist(obj_interdep_proplist_t *config_list,
		mtp_uint32 format_code)
{
	mtp_uint32 size = sizeof(mtp_uint32);
	interdep_prop_config_t *prop_config = NULL;
	slist_node_t *node;
	mtp_int32 ii;

	node = config_list->plist.start;

	for (ii = 0; ii < config_list->plist.nnodes; ii++) {
		prop_config = node->value;
		if ((prop_config->format_code == format_code) ||
				(prop_config->format_code == PTP_FORMATCODE_NOTUSED)) {

			size += _prop_get_size_interdep_prop(prop_config);
		}
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

mtp_bool _get_oma_drm_status(void)
{
#ifdef MTP_SUPPORT_OMADRM_EXTENSION
	return TRUE;
#else /* MTP_SUPPORT_OMADRM_EXTENSION */
	return FALSE;
#endif /* MTP_SUPPORT_OMADRM_EXTENSION */
}
