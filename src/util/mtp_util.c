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


#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <tapi_common.h>
#include <ITapiModem.h>
#include <system_info.h>
#include <vconf.h>
#include <gcrypt.h>
#include "mtp_util.h"
#include "mtp_support.h"
#include "mtp_fs.h"

static phone_state_t g_ph_status = { 0 };


void _util_print_error()
{
	/*In glibc-2.7, the longest error message string is 50 characters
	  ("Invalid or incomplete multibyte or wide character"). */
	mtp_char buff[100] = {0};

	strerror_r(errno, buff, sizeof(buff));
	ERR("Error: [%d]:[%s]\n", errno, buff);
}

mtp_int32 _util_get_battery_level(void)
{
	mtp_int32 result = 0;
	mtp_int32 battery_level = 100;

	result = vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY,
			&battery_level);
	if (result != 0) {
		ERR("VCONFKEY_SYSMAN_BATTERY_CAPACITY Fail!");
	}
	return battery_level;
}

mtp_bool _util_get_serial(mtp_char *serial, mtp_uint32 len)
{
	TapiHandle *handle = NULL;
	mtp_char *imei_no = NULL;
	mtp_char *serial_no = NULL;
	mtp_uint16 i = 0;
	char hash_value[MD5_HASH_LEN] = { 0 };

	if (serial == NULL || len <= MD5_HASH_LEN * 2) {
		ERR("serial is null or length is less than (MD5_HASH_LEN * 2)");
		return FALSE;
	}

	serial_no = vconf_get_str(VCONFKEY_MTP_SERIAL_NUMBER_STR);
	if (serial_no == NULL) {
		ERR("vconf_get Fail for %s\n",
				VCONFKEY_MTP_SERIAL_NUMBER_STR);
		return FALSE;
	}

	if (strlen(serial_no) > 0) {
		g_strlcpy(serial, serial_no, len);
		g_free(serial_no);
		return TRUE;
	}
	g_free(serial_no);

	handle = tel_init(NULL);
	if (!handle) {
		ERR("tel_init Fail");
		return FALSE;
	}

	imei_no = tel_get_misc_me_imei_sync(handle);
	if (!imei_no) {
		ERR("tel_get_misc_me_imei_sync Fail");
		tel_deinit(handle);
		return FALSE;
	}

	tel_deinit(handle);

	gcry_md_hash_buffer(GCRY_MD_MD5, hash_value, imei_no, strlen(imei_no));

	for (i = 0; i < MD5_HASH_LEN; i++) {
		g_snprintf(&serial[i*2], 3, "%02X", hash_value[i]);
	}

	if (vconf_set_str(VCONFKEY_MTP_SERIAL_NUMBER_STR, serial) == -1) {
		ERR("vconf_set Fail for %s\n",
				VCONFKEY_MTP_SERIAL_NUMBER_STR);
		g_free(imei_no);
		return TRUE;
	}

	g_free(imei_no);
	return TRUE;
}

void _util_get_vendor_ext_desc(mtp_char *vendor_ext_desc, mtp_uint32 len)
{
	mtp_int32 ret = SYSTEM_INFO_ERROR_NONE;
	mtp_char *version = NULL;

	ret_if(len == 0);
	ret_if(vendor_ext_desc == NULL);

	ret = system_info_get_platform_string(
		"http://tizen.org/feature/platform.version", &version);

	if (ret != SYSTEM_INFO_ERROR_NONE) {
		ERR("system_info_get_value_string Fail : 0x%X\n", ret);
		g_strlcpy(vendor_ext_desc, MTP_VENDOR_EXTENSIONDESC_CHAR, len);
		return;
	}
	g_snprintf(vendor_ext_desc, len, "%stizen.org:%s; ",
			MTP_VENDOR_EXTENSIONDESC_CHAR, version);
	g_free(version);
	return;
}

void _util_get_model_name(mtp_char *model_name, mtp_uint32 len)
{
	mtp_int32 ret = SYSTEM_INFO_ERROR_NONE;
	mtp_char *model = NULL;

	ret_if(len == 0);
	ret_if(model_name == NULL);

	ret = system_info_get_platform_string(
		"http://tizen.org/system/model_name", &model);

	if (ret != SYSTEM_INFO_ERROR_NONE) {
		ERR("system_info_get_value_string Fail : 0x%X\n", ret);
		g_strlcpy(model_name, MTP_DEFAULT_MODEL_NAME, len);
		return;
	}
	g_strlcpy(model_name, model, len);
	g_free(model);
	return;
}

void _util_get_device_version(mtp_char *device_version, mtp_uint32 len)
{
	mtp_int32 ret = SYSTEM_INFO_ERROR_NONE;
	mtp_char *version = NULL;
	mtp_char *build_info = NULL;

	ret_if(len == 0);
	ret_if(device_version == NULL);

	ret = system_info_get_platform_string(
		"http://tizen.org/feature/platform.version", &version);

	if (ret != SYSTEM_INFO_ERROR_NONE) {
		ERR("system_info_get_value_string Fail : 0x%X\n", ret);
		g_strlcpy(device_version, MTP_DEFAULT_DEVICE_VERSION, len);
		return;
	}

	ret = system_info_get_platform_string(
		"http://tizen.org/system/build.string", &build_info);

	if (ret != SYSTEM_INFO_ERROR_NONE) {
		ERR("system_info_get_value_string Fail : 0x%X\n", ret);
		g_strlcpy(device_version, MTP_DEFAULT_DEVICE_VERSION, len);
		g_free(version);
		return;
	}
	g_snprintf(device_version, len, "TIZEN %s (%s)", version, build_info);
	g_free(version);
	g_free(build_info);
	return;
}

void _util_gen_alt_serial(mtp_char *serial, mtp_uint32 len)
{
	struct timeval st;
	mtp_char model_name[MTP_MODEL_NAME_LEN_MAX + 1] = { 0 };

	ret_if(len == 0);
	ret_if(serial == NULL);

	if (gettimeofday(&st, NULL) < 0) {
		ERR("gettimeofday Fail");
		_util_print_error();
		return;
	}
	_util_get_model_name(model_name, sizeof(model_name));
	g_snprintf(serial, len, "%s-%010ld-%011ld", model_name,
			st.tv_sec, st.tv_usec);

	if (vconf_set_str(VCONFKEY_MTP_SERIAL_NUMBER_STR, serial) == -1) {
		ERR("vconf_set Fail %s\n", VCONFKEY_MTP_SERIAL_NUMBER_STR);
	}

	return;
}

void _util_get_usb_status(phone_status_t *val)
{
	mtp_int32 ret = 0;
	mtp_int32 state = 0;

	ret = vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS, &state);
	if (ret == -1 || state == VCONFKEY_SYSMAN_USB_DISCONNECTED) {
		*val = MTP_PHONE_USB_DISCONNECTED;
		return;
	}

	*val = MTP_PHONE_USB_CONNECTED;
	return;
}

phone_status_t _util_get_local_usb_status(void)
{
	return g_ph_status.usb_state;
}

void _util_set_local_usb_status(const phone_status_t val)
{
	g_ph_status.usb_state = val;
	return;
}

void _util_get_mmc_status(phone_status_t *val)
{
	mtp_int32 ret = 0;
	mtp_int32 state = 0;

	ret = vconf_get_int(VCONFKEY_SYSMAN_MMC_STATUS, &state);
	if (ret == -1 || state !=
			VCONFKEY_SYSMAN_MMC_MOUNTED) {
		*val = MTP_PHONE_MMC_NONE;
		return;
	}

	*val = MTP_PHONE_MMC_INSERTED;
	return;
}

phone_status_t _util_get_local_mmc_status(void)
{
	return g_ph_status.mmc_state;
}

void _util_set_local_mmc_status(const phone_status_t val)
{
	g_ph_status.mmc_state = val;
	return;
}

void _util_get_usbmode_status(phone_status_t *val)
{
	mtp_int32 ret = 0;
	mtp_int32 state = 0;

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT,
			&state);
	if (ret < 0)
		*val = MTP_PHONE_USB_MODE_OTHER;
	else
		*val = state;
	return;
}

phone_status_t _util_get_local_usbmode_status(void)
{
	return g_ph_status.usb_mode_state;
}

void _util_set_local_usbmode_status(const phone_status_t val)
{
	g_ph_status.usb_mode_state = val;
	return;
}
