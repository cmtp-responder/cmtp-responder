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
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <vconf.h>
#include "mtp_util.h"
#include "mtp_support.h"
#include "mtp_fs.h"
#include <storage/storage.h>
#include <sys/stat.h>
#include <systemd/sd-login.h>
#include <sys/types.h>
//#include <grp.h>
#include <glib.h>
#include <media_content_internal.h>
#include <pwd.h>
#include <poll.h>

/* time to wait for user session creation, in ms */
#define WAIT_FOR_USER_TIMEOUT 10000

static phone_state_t g_ph_status = { 0 };


/* LCOV_EXCL_START */
void _util_print_error()
{
	/*In glibc-2.7, the longest error message string is 50 characters
	  ("Invalid or incomplete multibyte or wide character"). */
	mtp_char buff[100] = {0};

	strerror_r(errno, buff, sizeof(buff));
	ERR("Error: [%d]:[%s]\n", errno, buff);
}
/* LCOV_EXCL_STOP */

mtp_bool _util_get_serial(mtp_char *serial, mtp_uint32 len)
{
	mtp_uint16 i = 0;
	char hash_value[MD5_HASH_LEN] = HASH_VALUE;

	if (serial == NULL || len <= MD5_HASH_LEN * 2) {
		ERR("serial is null or length is less than (MD5_HASH_LEN * 2)");
		return FALSE;
	}

/* LCOV_EXCL_START */

	for (i = 0; i < MD5_HASH_LEN; i++)
		g_snprintf(&serial[i*2], 3, "%02X", hash_value[i]);

	return TRUE;
}
/* LCOV_EXCL_STOP */

void _util_get_vendor_ext_desc(mtp_char *vendor_ext_desc, mtp_uint32 len)
{
	mtp_char *version = VENDOR_EXT_VERSION;
	ret_if(len == 0);
	ret_if(vendor_ext_desc == NULL);

	g_snprintf(vendor_ext_desc, len, "%s",
			MTP_VENDOR_EXTENSIONDESC_CHAR);
	return;
}

void _util_get_model_name(mtp_char *model_name, mtp_uint32 len)
{
	mtp_char *model = MODEL;

	ret_if(len == 0);
	ret_if(model_name == NULL);

	g_strlcpy(model_name, model, len);
	return;
}

void _util_get_device_version(mtp_char *device_version, mtp_uint32 len)
{
	mtp_char *version = DEVICE_VERSION;
	mtp_char *build_info = BUILD_INFO;

	ret_if(len == 0);
	ret_if(device_version == NULL);

	g_snprintf(device_version, len, "cmtp-responder %s (%s)", version, build_info);
	return;
}

/* LCOV_EXCL_START */
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
/* LCOV_EXCL_STOP */

phone_status_t _util_get_local_mmc_status(void)
{
	return g_ph_status.mmc_state;
}

/* LCOV_EXCL_START */
void _util_set_local_mmc_status(const phone_status_t val)
{
	g_ph_status.mmc_state = val;
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

/* LCOV_EXCL_STOP */

static bool _util_device_external_supported_cb(int storage_id, storage_type_e type,
	storage_state_e state, const char *path, void *user_data)
{
	char *storage_path = (char *)user_data;

	//DBG("storage id: %d, path: %s", storage_id, path);

	if (type == STORAGE_TYPE_EXTERNAL && path != NULL) {
		strncpy(storage_path, path, strlen(path)+1);
		storage_path[strlen(path)] = 0;
		//DBG("external storage path : %s", storage_path);
	}

	return TRUE;
}
/* LCOV_EXCL_STOP */

void _util_get_external_path(char *external_path)
{
	int error = STORAGE_ERROR_NONE;

	error = storage_foreach_device_supported(_util_device_external_supported_cb, external_path);

	if (error != STORAGE_ERROR_NONE) {
/* LCOV_EXCL_START */
		ERR("get external storage path Fail");
		if (external_path != NULL) {
			strncpy(external_path, MTP_EXTERNAL_PATH_CHAR, sizeof(MTP_EXTERNAL_PATH_CHAR));
			external_path[sizeof(MTP_EXTERNAL_PATH_CHAR) - 1] = 0;
		}
	}
}

int _util_wait_for_user()
{
	__attribute__((cleanup(sd_login_monitor_unrefp))) sd_login_monitor *monitor = NULL;
	int ret;
	struct pollfd fds;

	ret = sd_login_monitor_new("uid", &monitor);
	if (ret < 0) {
		char buf[256] = {0,};
		strerror_r(-ret, buf, sizeof(buf));
		ERR("Failed to allocate login monitor object: [%d]:[%s]", ret, buf);
		return ret;
	}

	fds.fd = sd_login_monitor_get_fd(monitor);
	fds.events = sd_login_monitor_get_events(monitor);

	ret = poll(&fds, 1, WAIT_FOR_USER_TIMEOUT);
	if (ret < 0) {
		ERR("Error polling: %m");
		return -1;
	}

	return 0;
}
/* LCOV_EXCL_STOP */

void _util_get_internal_path(char *internal_path)
{
	struct passwd *pwd;
	uid_t active_user = ACTIVE_USER;
	char *active_name = NULL;

	pwd = getpwuid(active_user);
	active_name = pwd->pw_name;

	if (active_name == NULL) {
		/* LCOV_EXCL_START */
		ERR("active_name is NULL");
		strncpy(internal_path, MTP_USER_DIRECTORY, sizeof(MTP_USER_DIRECTORY));
		internal_path[sizeof(MTP_USER_DIRECTORY) - 1] = 0;
		return;
		/* LCOV_EXCL_STOP */
	}

	if (internal_path != NULL) {
		strncpy(internal_path, MTP_INTERNAL_PATH_CHAR, sizeof(MTP_INTERNAL_PATH_CHAR));
		strncat(internal_path, active_name, strlen(active_name) + 1);
		strncat(internal_path, "/media", 7);
		internal_path[strlen(internal_path)] = 0;
	}

	ERR("internal path is %s", internal_path);
}

/* LCOV_EXCL_START */
mtp_bool _util_media_content_connect()
{
	mtp_int32 ret = 0;
	uid_t active_user = ACTIVE_USER;

	ret = media_content_connect_with_uid(active_user);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		ERR("media_content_connect() failed : %d", ret);
		return FALSE;
	}

	return TRUE;
}

void _util_media_content_disconnect()
{
	media_content_disconnect();
}
/* LCOV_EXCL_STOP */
