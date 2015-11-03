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
#include <glib/gprintf.h>
#include <unistd.h>
#include "mtp_support.h"
#include "ptp_datacodes.h"
#include "mtp_util.h"

/*
 * STATIC FUNCTIONS
 */
static mtp_char *__util_conv_int_to_hex_str(mtp_int32 int_val, mtp_char *str);

/*
 * FUNCTIONS
 */
void _util_conv_byte_order(void *data, mtp_int32 size)
{
	mtp_int32 idx;
	mtp_uchar temp;
	mtp_int32 h_size;
	mtp_uchar *l_data = (mtp_uchar *)data;

	ret_if(data == NULL);
	retm_if(size <= 1, "size(%d) is invalid", size);

	h_size = size / 2;
	for (idx = 0; idx < h_size; idx++) {
		temp = l_data[idx];
		l_data[idx] = l_data[size - idx - 1];
		l_data[size - idx - 1] = temp;
	}

	return;
}

void _util_conv_byte_order_wstring(mtp_uint16 *wstr, mtp_int32 size)
{
	_util_conv_byte_order_gen_str(wstr, size, sizeof(mtp_uint16));
	return;
}

void _util_conv_byte_order_gen_str(void *str, mtp_int32 size, mtp_int32 elem_sz)
{
	mtp_int32 idx;
	mtp_int32 f_size = size * elem_sz;
	mtp_uchar *l_str = (mtp_uchar *)str;

	ret_if(str == NULL);
	retm_if(elem_sz <= 1, "elem_sz(%d) is invalid", elem_sz);

	for (idx = 0; idx < f_size; idx += elem_sz) {
		_util_conv_byte_order(&(l_str[idx]), elem_sz);
	}

	return;
}

/*
 * If items_written is greater than or equal to dest_size,
 * src is truncated when it is copied to dest.
 */
mtp_int32 _util_utf16_to_utf8(char *dest, mtp_int32 dest_size,
		const mtp_wchar *src)
{
	gchar *utf8 = NULL;
	GError *error = NULL;
	glong items_read = 0;
	glong items_written = 0;
	const gunichar2 *utf16 = (const gunichar2 *)src;

	retv_if(src == NULL, 0);
	retv_if(dest == NULL, 0);

	utf8 = g_utf16_to_utf8(utf16, -1, &items_read, &items_written, &error);
	if (utf8 == NULL) {
		ERR("%s\n", error->message);
		g_error_free(error);

		dest[0] = '\0';
		items_written = 0;
	} else {
		g_strlcpy(dest, (char *)utf8, dest_size);
		g_free(utf8);
	}

	return (mtp_int32)items_written;
}

/*
 * If items_written is greater than or equal to dest_items,
 * src is truncated when it is copied to dest.
 */
mtp_int32 _util_utf8_to_utf16(mtp_wchar *dest, mtp_int32 dest_items,
		const char *src)
{
	GError *error = NULL;
	glong items_read = 0;
	gunichar2 *utf16 = NULL;
	glong items_written = 0;

	retv_if(src == NULL, 0);
	retv_if(dest == NULL, 0);

	utf16 = g_utf8_to_utf16(src, -1, &items_read, &items_written, &error);
	if (utf16 == NULL) {
		ERR("%s\n", error->message);
		g_error_free(error);
		error = NULL;

		dest[0] = (mtp_wchar)'\0';
		items_written = 0;
	} else {
		_util_wchar_ncpy(dest, utf16, dest_items);
		g_free(utf16);
	}

	return (mtp_int32)items_written;
}


/*
 * Copies a unicode string.
 * @param[in]	src	Null-terminated source string
 * @param[out]	dest	Destination buffer
 * @return	None
 */
void _util_wchar_cpy(mtp_wchar *dest, const mtp_wchar *src)
{
	ret_if(src == NULL);
	ret_if(dest == NULL);

	if (!((int)dest & 0x1) && !((int)src & 0x1)){
		/* 2-byte aligned */
		mtp_wchar *temp = dest;

		while ((*temp++ = *src++) != '\0') {
			;	/* DO NOTHING  */
		}
	} else {
		/* not-aligned, byte to byte approach - slow */
		mtp_char *pc1 = (mtp_char *)dest;
		mtp_char *pc2 = (mtp_char *)src;

		while (*pc2 || *(pc2 + 1)) {
			*pc1 = *pc2;
			*(pc1 + 1) = *(pc2 + 1);

			pc1 += 2;
			pc2 += 2;
		}
	}

	return;
}

/*
 * Copies a unicode string a given numbers of character.
 * @param[in]	src	Null-terminated source string
 * @param[out]	dest	Destination buffer
 * @return	None
 */
void _util_wchar_ncpy(mtp_wchar *dest, const mtp_wchar *src, unsigned long n)
{
	char *pc1 = NULL;
	char *pc2 = NULL;
	mtp_wchar *temp = NULL;

	ret_if(src == NULL);
	ret_if(dest == NULL);

	if (!((int)dest & 0x1) && !((int)src & 0x1)) {	/* 2-byte aligned */
		temp = dest;

		while (n && (*temp++ = *src++)) {
			n--;
		}

		if (n) {
			while (--n) {
				*temp++ = 0;
			}
		}
	} else {		/* not-aligned, byte to byte approach - slow */

		pc1 = (char *)dest;
		pc2 = (char *)src;

		while (n && (*pc2 || *(pc2 + 1))) {
			--n;
			*pc1++ = *pc2++;
			*pc1++ = *pc2++;
		}

		if (n) {
			while (--n) {
				*pc1++ = 0;
				*pc1++ = 0;
			}
		}
	}

	return;
}

/*
 * Returns the length of wide character string
 * @param[in]	src	Wide char string
 * @return	length
 */
size_t _util_wchar_len(const mtp_wchar *s)
{
	if (!((int)s & 0x1)) {	/* 2-byte aligned */
		mtp_wchar *temp = (mtp_wchar *)s;

		while (*temp++) {
			/* DO NOTHING */ ;
		}

		DBG("Length : %d\n", temp - s - 1);
		return ((size_t)(temp - s - 1));
	} else {		/* not-aligned, byte to byte approach - slow */

		unsigned char *temp = (unsigned char *)s;

		while (*temp || *(temp + 1)) {
			temp += 2;
		}

		DBG("Length : %d\n", (temp - (unsigned char *)s) / 2);
		return ((size_t) (temp - (unsigned char *)s) / 2);
	}
}

static mtp_char* __util_conv_int_to_hex_str(mtp_int32 int_val, mtp_char *str)
{
	mtp_char *nstr = str;
	mtp_int32 val = int_val;
	mtp_char hex[] = { "0123456789ABCDEF" };

	retv_if(str == NULL, NULL);

	*nstr++ = '0';
	*nstr++ = 'x';

	for (val = int_val; val; val <<= 4) {
		*nstr++ = hex[(val >> (sizeof(int) * 8 - 4)) & 0xF];
	}
	*nstr = '\0';
	return str;
}

/*
 * This is very minimal implementation.
 * Be cautious using this function.
 */
mtp_err_t _util_wchar_swprintf(mtp_wchar *mtp_wstr, mtp_int32 size,
		mtp_char *format, ...)
{
	mtp_char *ptr;
	mtp_wchar wsHex[24];
	mtp_int32 count = 0;
	mtp_wchar *wbuf = mtp_wstr;
	mtp_char *bptr_val = NULL;
	mtp_wchar *wstr_val = NULL;
	mtp_int32 int_val = 0, len = 0;
	mtp_char buf[MTP_MAX_PATHNAME_SIZE + 1];

	va_list arg_list;
	va_start(arg_list, format);
	for (ptr = format; *ptr && count < (size - 1); ptr++) {
		if (*ptr == '%') {
			switch (*(ptr + 1)) {
			case 'd':
				int_val = va_arg(arg_list, int);
				wbuf[count++] = (mtp_wchar) (int_val + '0');
				ptr++;
				break;
			case 's':
				bptr_val = va_arg(arg_list, char *);
				wstr_val = (mtp_wchar *) bptr_val;
				len = _util_wchar_len(wstr_val);
				if (len + count > size - 1) {
					len = size - 1 - count;
					_util_wchar_ncpy(&wbuf[count], wstr_val,
							len);
				} else {
					_util_wchar_cpy(&wbuf[count], wstr_val);
				}
				count += len;
				ptr++;
				break;
			case 'x':
				int_val = va_arg(arg_list, int);
				__util_conv_int_to_hex_str(int_val, buf);
				_util_utf8_to_utf16(wsHex,
						sizeof(wsHex) / WCHAR_SIZ, buf);
				len = strlen(buf);
				if (len + count > size - 1) {
					len = size - 1 - count;
					_util_wchar_ncpy(&wbuf[count], wsHex,
							len);
				} else {
					_util_wchar_cpy(&wbuf[count], wsHex);
				}
				count += len;
				ptr++;
				break;

			default:
				DBG("swprintf not handling: %c", *(ptr + 1));
				break;
			}
		} else {
			wbuf[count++] = (mtp_wchar)(*ptr);
		}
	}

	va_end(arg_list);
	wbuf[count] = (mtp_wchar)'\0';
	_util_utf16_to_utf8(buf, sizeof(buf), wbuf);

	return MTP_ERROR_NONE;
}

mtp_uint16 _util_get_fmtcode(const mtp_char *extn)
{
	static fmt_code_t fmt_code_table[] = {
		{"ALB", MTP_FMT_ABSTRACT_AUDIO_ALBUM},
		{"MP3", PTP_FMT_MP3},
		{"WMA", MTP_FMT_WMA},
		{"WMV", MTP_FMT_WMV},
		{"JPG", PTP_FMT_IMG_EXIF},
		{"GIF", PTP_FMT_IMG_GIF},
		{"BMP", PTP_FMT_IMG_BMP},
		{"PNG", PTP_FMT_IMG_PNG},
		{"ASF", PTP_FMT_ASF},
		{"WAV", PTP_FMT_WAVE},
		{"AVI", PTP_FMT_AVI},
		{"MPG", PTP_FMT_MPEG},
		{"TXT", PTP_FMT_TEXT},
		{"3GP", MTP_FMT_3GP},
		{"ODF", MTP_FMT_3GP},
		{"O4A", MTP_FMT_3GP},
		{"O4V", MTP_FMT_3GP},
		{"MP4", MTP_FMT_MP4},
		{"FLAC", MTP_FMT_FLAC},
		{"", PTP_FMT_UNDEF}
	};

	fmt_code_t *p = NULL;
	p = fmt_code_table;

	while (p->fmt_code != PTP_FMT_UNDEF) {
		if (!strncasecmp(extn, p->extn, strlen(p->extn)))
			break;

		p++;
	}

	/* will return FormatCode or PTP_FORMATCODE_UNDEFINED
	 * if we hit end of list.
	 */
	return p->fmt_code;
}

/*
 * This function gets the file extension.
 * @param[in]		fileName		Specifies the file name.
 * @param[out]		file_extn		holds the extension of the file.
 * @return		Returns TRUE on success and FALSE on failure.
 */
mtp_bool _util_get_file_extn(const mtp_char *f_name, mtp_char *f_extn)
{
	char *ptr;

	retv_if(NULL == f_name, FALSE);
	retv_if(NULL == f_extn, FALSE);

	ptr = strrchr(f_name, '.');

	if (ptr != NULL) {
		g_strlcpy(f_extn, ptr + 1, MTP_MAX_PATHNAME_SIZE);
		return TRUE;
	}

	return FALSE;
}

mtp_bool _util_get_file_name(const mtp_char *fullpath, mtp_char *f_name)
{
	mtp_int32 i, j;

	retv_if(f_name == NULL, FALSE);
	retv_if(fullpath == NULL, FALSE);

	i = strlen(fullpath);

	for (j=0; i >= 0; i--) {
		if (fullpath[i] == '/') {
			g_strlcpy(f_name, &fullpath[i + 1], j);
			return TRUE;
		}
		j++;
	}
	g_strlcpy(f_name, fullpath, j);

	return TRUE;
}
/*
 * This function gives file name without extension.
 * @param[in]	fullpath	pointer to absolute file path
 * @param[out]	f_name	gets filled with filename w/o extension
 * @return	True or False based on success or failure
 */
mtp_bool _util_get_file_name_wo_extn(const mtp_char *fullpath, mtp_char *f_name)
{
	mtp_char *fname_ptr;
	mtp_uint32 fname_len;
	mtp_char *extn_ptr = NULL;

	retv_if(f_name == NULL, FALSE);
	retv_if(fullpath == NULL, FALSE);

	fname_ptr = strrchr(fullpath, '/');

	if (fname_ptr == NULL) {
		ERR("Invalid File Name");
		return FALSE;
	}

	fname_ptr = fname_ptr + sizeof(char);
	fname_len = strlen(fname_ptr);
	extn_ptr = strrchr(fname_ptr, '.');

	if (extn_ptr == NULL) {
		g_strlcpy(f_name, fname_ptr, fname_len + 1);
		return TRUE;
	}

	g_strlcpy(f_name, fname_ptr, ((mtp_uint32)(extn_ptr - fname_ptr) + 1));
	return TRUE;
}

mtp_bool _util_is_path_len_valid(const mtp_char *path)
{
	mtp_uint32 limit = 0;
	mtp_uint32 mtp_path_len = 0;
	mtp_uint32 root_path_len = 0;

	static mtp_uint32 max_store_len = 0;
	static mtp_bool is_initialized = FALSE;
	static mtp_uint32 internal_store_len = 0;
	static mtp_uint32 external_store_len = 0;

	retv_if(path == NULL, FALSE);

	if (!is_initialized) {
		is_initialized = TRUE;
		internal_store_len = strlen(MTP_STORE_PATH_CHAR);
		external_store_len = strlen(MTP_EXTERNAL_PATH_CHAR);

		max_store_len = internal_store_len > external_store_len ?
			internal_store_len : external_store_len;

		DBG("max store len : [%u]\n", max_store_len);
	}

	if (!strncmp(path, MTP_STORE_PATH_CHAR, internal_store_len)) {
		root_path_len = internal_store_len;
	} else if (!strncmp(path, MTP_EXTERNAL_PATH_CHAR, external_store_len)) {
		root_path_len = external_store_len;
	}
	else {
		ERR("Unknown store's path : %s\n", path);
		return FALSE;
	}

	/* Path len should be calculated except root path(eg. /opt/usr/media) */
	mtp_path_len = strlen(path) - root_path_len;

	/* MTP_MAX_PATHNAME_SIZE includes maximum length of root path */
	limit = MTP_MAX_PATHNAME_SIZE - max_store_len;

	if (mtp_path_len > limit) {
		ERR("Too long path : [%u] > [%u]\n", mtp_path_len, limit);
		return FALSE;
	}

	return TRUE;
}

mtp_bool _util_create_path(mtp_char *path, mtp_uint32 size, const mtp_char *dir,
		const mtp_char *filename)
{
	mtp_int32 ret = 0;
	mtp_uint32 len = 0;

	retv_if(dir == NULL, FALSE);
	retv_if(path == NULL, FALSE);
	retv_if(filename == NULL, FALSE);

	len = strlen(filename);
	if (len > MTP_MAX_FILENAME_SIZE) {
		ERR("filename is too long :[%u] > [%u]\n", len,
				MTP_MAX_FILENAME_SIZE);
		return FALSE;
	}

	ret = g_snprintf(path, size, "%s/%s", dir, filename);
	if (ret > size) {
		ERR("path is truncated");
		return FALSE;
	}

	if (_util_is_path_len_valid(path) == FALSE) {
		ERR("path length exceeds the limit");
		return FALSE;
	}

	return TRUE;
}

/*
 * This function gets the parent path.
 * @param[in]	fullpath	Pointer to a buffer containing full file path.
 * @param[out]	p_path	Points the buffer to hold parent path.
 * @return	None
 */
void _util_get_parent_path(const mtp_char *fullpath, mtp_char *p_path)
{
	mtp_char *ptr = NULL;

	ret_if(NULL == p_path);
	ret_if(NULL == fullpath);

	ptr = strrchr(fullpath, '/');
	if (!ptr) {
		ERR("Path does not have parent path");
		return;
	}

	g_strlcpy(p_path, fullpath, (mtp_uint32)(ptr - fullpath) + 1);
	return;
}

void _util_conv_wstr_to_guid(mtp_wchar *wstr, mtp_uint64 *guid)
{
	mtp_uint32 skip_idx;
	mtp_uint32 cur_idx;
	mtp_uint64 temp[2];
	mtp_int32 count = 0;
	mtp_int32 cpy_sz = 0;

	ret_if(wstr == NULL);
	ret_if(guid == NULL);

	while (wstr[count] != 0) {
		count++;
	}

	memset(guid, 0, sizeof(temp));
	skip_idx = sizeof(temp) / sizeof(mtp_wchar);

	for (cur_idx = 0; cur_idx < count; cur_idx += skip_idx) {

		memset(temp, 0, sizeof(temp));
		cpy_sz = (count - cur_idx) * sizeof(mtp_wchar);
		if (cpy_sz > sizeof(temp)) {
			cpy_sz = sizeof(temp);
		}
		memcpy(temp, &(wstr[cur_idx]), cpy_sz);
		guid[0] += temp[0];
		guid[1] += temp[1];
	}

	return;
}

mtp_bool _util_get_unique_dir_path(const mtp_char *exist_path,
		mtp_char *new_path, mtp_uint32 new_path_buf_len)
{
	mtp_uint32 num_bytes = 0;
	mtp_uint32 max_value = 1;
	mtp_uint32 count = 1;
	mtp_uint32 val = 1;
	mtp_char *buf = NULL;
	mtp_uint32 len = strlen(exist_path);

	retv_if(new_path == NULL, FALSE);
	retv_if(exist_path == NULL, FALSE);

	/* Excluding '_' */
	num_bytes = new_path_buf_len - len - 1;
	if (num_bytes <= 0) {
		ERR("No space to append data[%d]\n", num_bytes);
		return FALSE;
	}

	if (num_bytes >= MTP_BUF_SIZE_FOR_INT - 1) {
		max_value = UINT_MAX;
	} else {
		while (count <= num_bytes) {
			max_value *= 10;
			count++;
		}
		DBG("max_value[%u]\n", max_value);
	}

	buf = (mtp_char *)g_malloc(new_path_buf_len);
	if (buf == NULL) {
		ERR("g_malloc Fail");
		return FALSE;
	}
	g_strlcpy(buf, exist_path, new_path_buf_len);
	while (val < max_value) {
		/* Including NUL and '_' */
		g_snprintf(&buf[len], num_bytes + 2, "_%u", val++);
		if (access(buf, F_OK) < 0) {
			goto SUCCESS;
		}
	}

	g_free(buf);
	ERR("Unable to generate Unique Dir Name");
	return FALSE;

SUCCESS:
	g_strlcpy(new_path, buf, new_path_buf_len);
	g_free(buf);
	DBG_SECURE("Unique dir name[%s]\n", new_path);
	return TRUE;
}
