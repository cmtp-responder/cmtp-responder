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

#ifndef _MTP_FS_H_
#define _MTP_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <dirent.h>
#include "mtp_datatype.h"
#include "mtp_config.h"

#define MTP_FILE_ATTR_INVALID		0xFFFFFFFF
#define MTP_LOG_FILE			"/var/log/mtp.log"
#define MTP_LOG_MAX_SIZE		5 * 1024 * 1024 /*5MB*/

typedef enum {
	MTP_FILE_TYPE = 0,
	MTP_DIR_TYPE,
	MTP_ALL_TYPE
} file_type_t;

typedef enum {
	MTP_FILE_ATTR_MODE_NONE = 0x00000000,
	MTP_FILE_ATTR_MODE_REG  = 0x00000100,
	MTP_FILE_ATTR_MODE_DIR = 0x00000200,
	MTP_FILE_ATTR_MODE_READ_ONLY = 0x00000400,
	MTP_FILE_ATTR_MODE_SYSTEM = 0x00001000
} file_attr_mode_t;

typedef struct {
	file_attr_mode_t attribute;
	mtp_uint64 fsize;
	time_t ctime;  /* created time */
	time_t mtime;	/* modified time */
} file_attr_t;

typedef struct {
	mtp_char filename[MTP_MAX_PATHNAME_SIZE + 1];
	file_type_t type;
	file_attr_t attrs;
} dir_entry_t;

typedef enum {
	MTP_FILE_READ = 0x1,
	MTP_FILE_WRITE = 0x2,
} file_mode_t;

typedef struct {
	mtp_uint64 disk_size;
	mtp_uint64 avail_size;
	mtp_uint64 reserved_size;
} fs_info_t;

FILE* _util_file_open(const mtp_char *filename, file_mode_t mode,
		mtp_int32 *error);
void _util_file_read(FILE* fhandle, void *bufptr, mtp_uint32 size,
		mtp_uint32 *read_count);
mtp_uint32 _util_file_write(FILE* fhandle, void *bufptr, mtp_uint32 size);
mtp_int32 _util_file_close(FILE* fhandle);
mtp_bool _util_file_seek(FILE* fhandle, mtp_uint64 offset, mtp_int32 whence);
mtp_bool _util_file_truncate(const mtp_char *filename, mtp_uint64 length);
mtp_bool _util_file_copy(const mtp_char *origpath, const mtp_char *newpath,
		mtp_int32 *error);
mtp_bool _util_file_append(const mtp_char *srcpath, const mtp_char *dstpath,
			   mtp_int64 ofs, mtp_int32 *error);
mtp_bool _util_copy_dir_children_recursive(const mtp_char *origpath,
		const mtp_char *newpath, mtp_uint32 store_id, mtp_int32 *error);
mtp_bool _util_file_move(const mtp_char *origpath, const mtp_char *newpath,
		mtp_int32 *error);
mtp_bool _util_get_file_attrs(const mtp_char *filename, file_attr_t *attrs);
mtp_bool _util_set_file_attrs(const mtp_char *filename, mtp_dword attrs);
mtp_bool _util_dir_create(const mtp_char *dirname, mtp_int32 *error);
mtp_int32 _util_remove_dir_children_recursive(const mtp_char *dirname,
		mtp_uint32 *num_of_deleted_file, mtp_uint32 *num_of_file, mtp_bool readonly);
mtp_bool _util_ifind_next(char *dir_name, DIR *dirp, dir_entry_t *dir_info);
mtp_bool _util_ifind_first(char *dir_name, DIR **dirp, dir_entry_t *dir_info);
mtp_bool _util_is_file_opened(const mtp_char *fullpath);
mtp_bool _util_get_filesystem_info(mtp_char *storepath, fs_info_t *fs_info);
void _FLOGD(const char *file, const char *fmt, ...);
#define FLOGD(arg...) _FLOGD(__FILE__, ##arg)

#ifdef __cplusplus
}
#endif

#endif /* _MTP_FS_H_ */
