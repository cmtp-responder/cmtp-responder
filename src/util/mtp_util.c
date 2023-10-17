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


#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include "mtp_util.h"
#include "mtp_support.h"
#include "mtp_fs.h"
#include "mtp_property.h"
#include <sys/stat.h>
#include <systemd/sd-login.h>
#include <sys/types.h>
//#include <grp.h>
#include <glib.h>
#include <pwd.h>
#include <poll.h>


static phone_state_t _g_ph_status = { 0 };
phone_state_t *g_ph_status = &_g_ph_status;
extern mtp_config_t g_conf;

/* LCOV_EXCL_START */
void _util_print_error()
{
	/*In glibc-2.7, the longest error message string is 50 characters
	  ("Invalid or incomplete multibyte or wide character"). */
	mtp_char buff[100] = {0};

	strerror_r(errno, buff, sizeof(buff));
	ERR("Error: [%d]:[%s]\n", errno, buff);
}

void _util_get_external_path(char *external_path)
{
/* LCOV_EXCL_START */
	strncpy(external_path, g_conf.external_path->str, g_conf.external_path->len);
	external_path[g_conf.external_path->len + 1] = 0;
}
/* LCOV_EXCL_STOP */

void _util_ptp_pack_string(ptp_string_t *string, char *data)
{
	mtp_wchar wtemp[MAX_PTP_STRING_CHARS + 1] = { 0 };

	_util_utf8_to_utf16(wtemp, sizeof(wtemp) / WCHAR_SIZ, data);
	_prop_copy_char_to_ptpstring(string, wtemp, WCHAR_TYPE);
}
