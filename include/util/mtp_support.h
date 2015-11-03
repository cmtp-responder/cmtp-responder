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

#ifndef _MTP_SUPPORT_H_
#define	_MTP_SUPPORT_H_

#include "mtp_datatype.h"
#include "mtp_config.h"

typedef struct {
	mtp_char *extn;
	mtp_uint16 fmt_code;
} fmt_code_t;

/*
 * Convert byte order for big endian architecture
 */
void _util_conv_byte_order(void *data, mtp_int32 size);

/*
 * Convert byte order for WCHAR string
 */
void _util_conv_byte_order_wstring(mtp_uint16 *wstr, mtp_int32 size);
void _util_conv_byte_order_gen_str(void *str, mtp_int32 size, mtp_int32 elem_sz);
void _util_wchar_cpy(mtp_wchar *dest, const mtp_wchar *src);
void _util_wchar_ncpy(mtp_wchar *dest, const mtp_wchar *src, unsigned long n);
size_t _util_wchar_len(const mtp_wchar *s);
mtp_wchar *mtp_wcscat_charstr(mtp_wchar *str1, const mtp_char *char_str);
mtp_err_t _util_wchar_swprintf(mtp_wchar *mtp_wstr, mtp_int32 size,
		mtp_char *format, ...);
mtp_int32 _util_utf16_to_utf8(char *dest, mtp_int32 size, const mtp_wchar *src);
mtp_int32 _util_utf8_to_utf16(mtp_wchar *dest, mtp_int32 no_of_item, const char *src);
mtp_uint16 _util_get_fmtcode(const mtp_char *extn);
mtp_bool _util_get_file_extn(const mtp_char *f_name, mtp_char *f_extn);
mtp_bool _util_get_file_name(const mtp_char *fullpath, mtp_char *f_name);
mtp_bool _util_get_file_name_wo_extn(const mtp_char *fullpath, mtp_char *f_name);
mtp_bool _util_is_path_len_valid(const mtp_char *path);
mtp_bool _util_create_path(mtp_char *path, mtp_uint32 size, const mtp_char *dir,
		const mtp_char *filename);
void _util_get_parent_path(const mtp_char *fullpath, mtp_char *p_path);
void _util_conv_wstr_to_guid(mtp_wchar *wstr, mtp_uint64 *guid);
mtp_bool _util_get_unique_dir_path(const mtp_char *exist_path, mtp_char *new_path,
		mtp_uint32 new_path_buf_len);

#endif /* _MTP_SUPPORT_H_ */
