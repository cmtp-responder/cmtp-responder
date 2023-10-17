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

#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "mtp_fs.h"
#include "mtp_util.h"
#include "mtp_support.h"
#include "ptp_datacodes.h"
#include "mtp_device.h"

extern mtp_uint32 g_next_obj_handle;
extern mtp_config_t g_conf;

/*
 * FUNCTIONS
 */

/*
 * mtp_uint32 _util_file_open(const mtp_char *filename,
 *	file_mode_t mode, mtp_int32 *error)
 * This function opens the file in specific mode.
 *
 * @param[in]	filename	Specifies the name of file to open.
 * @param[in]	mode		Specifies the mode of file to open.
 * @param[out]	error		Specifies the type of error
 * @return	This function returns the file handle  on success,
 or INVALID_FILE on failure
 */
FILE* _util_file_open(const mtp_char *filename, file_mode_t mode,
		mtp_int32 *error)
{
	FILE *fhandle = NULL;
	char *fmode = NULL;

	switch ((int)mode) {
	case MTP_FILE_READ:
		fmode = "rm";
		break;

	case MTP_FILE_WRITE:
		fmode = "w";
		break;

	default:
		ERR("Invalid mode : %d\n", mode);
		*error = EINVAL;
		return NULL;
	}

	fhandle = fopen(filename, fmode);
	if (fhandle == NULL) {
		ERR("File open Fail:mode[0x%x], errno [%d]\n", mode, errno);
		ERR_SECURE("filename[%s]\n", filename);
		*error = errno;
		return NULL;
	}

	fcntl(fileno(fhandle), F_SETFL, O_NOATIME);

	return fhandle;
}

/*
 * void _util_file_read(mtp_uint32 handle, void *bufptr, mtp_uint32 size,
 *	mtp_uint32 *preadcount)
 *
 * This function reads data from the file handle into the data buffer.
 *
 * @param[in]	handle		Specifies the handle of file to read.
 * @param[out]	bufptr		Points to buff where data is to be read.
 * @param[in]	size		Specifies the num bytes to be read.
 * @param[out]	preadcount	Will store the actual num bytes read.
 * @return	None
 */
void _util_file_read(FILE* fhandle, void *bufptr, mtp_uint32 size,
		mtp_uint32 *read_count)
{
	*read_count = fread_unlocked(bufptr, sizeof(mtp_char), size, fhandle);
}
/**
 * mtp_uint32 _util_file_write(mtp_uint32 fhandle, void *bufptr, mtp_uint32 size)
 *
 * This function writes data to the file using the data buffer passed.
 *
 * @param[in]	handle	Specifies the handle of file to write.
 * @param[in]	bufptr	Points the buffer which holds the data.
 * @param[in]	size	Specifies num bytes to be written.
 * @return	This function returns num bytes written.
 */

mtp_uint32 _util_file_write(FILE* fhandle, void *bufptr, mtp_uint32 size)
{
	return fwrite_unlocked(bufptr, sizeof(mtp_char), size, fhandle);
}

/**
 * mtp_int32 _util_file_close(mtp_uint32 fhandle)
 * This function closes the file.
 *
 * @param[in]	handle	Specifies the handle of file to close.
 * @return	0 in case of success or EOF on failure.
 */
mtp_int32 _util_file_close(FILE* fhandle)
{
	return fclose(fhandle);
}

/*
 * This function seeks to a particular location in a file.
 *
 * @param[in]	handle		Specifies the handle of file to seek.
 * @param[in]	offset		Specifies the starting point.
 * @param[in]	whence		Specifies the setting value
 * @return	Returns TRUE in case of success or FALSE on Failure.
 */
mtp_bool _util_file_seek(FILE* handle, mtp_uint64 offset, mtp_int32 whence)
{
	mtp_int64 ret_val = 0;

	ret_val = fseek(handle, offset, whence);
	retvm_if(ret_val < 0, FALSE, " _util_file_seek error errno [%d]\n", errno);

	return TRUE;
}

/*
 * This function truncates a file.
 *
 * @param[in]	handle		Specifies the handle of file to seek.
 * @param[in]	length		Specifies the new file length.
 * @return	Returns TRUE in case of success or FALSE on Failure.
 */
mtp_bool _util_file_truncate(const mtp_char *filename, mtp_uint64 length)
{
	mtp_int32 ret_val;
	struct stat st;

	ret_val = stat(filename, &st);
	retvm_if(ret_val != 0, FALSE, "Error while stat [%d]\n", errno);
	retvm_if(!(S_IWUSR & st.st_mode), FALSE, "Wrong privilege, missing S_IWUSR\n");

	ret_val = truncate(filename, length);
	retvm_if(ret_val != 0, FALSE, "Truncate error [%d]\n", errno);

	return TRUE;
}

mtp_bool _util_file_append(const mtp_char *srcpath, const mtp_char *dstpath,
			   mtp_int64 ofs, mtp_int32 *error)
{
	int oflags;
	int src, dst;
	mtp_int32 ret;
	struct stat sb;
	ssize_t written_bytes;
	off_t bytes_to_send;

	if ((src = open(srcpath, O_RDONLY)) < 0) {
		ERR("In-file open Fail errno [%d]\n", errno);
		*error = errno;
		return FALSE;
	}

	ret = fstat(src, &sb);
	if (ret) {
		ERR("In-file stat failed errno [%d]\n", errno);
		close(src);
		return FALSE;
	}

	oflags = O_WRONLY | O_CREAT;
	if (ofs == 0)
		oflags |= O_TRUNC;

	if ((dst = open(dstpath, oflags, sb.st_mode)) < 0) {
		ERR("Out-file open Fail errno [%d]\n", errno);
		*error = errno;
		close(src);
		return FALSE;
	}

	if (ofs > 0) {
		if (lseek(dst, ofs, SEEK_SET) != ofs) {
			ERR("Failed to set file offset to 0x%x\n", ofs);
			*error = errno;
			close(dst);
			close(src);
			return FALSE;
		}
	}

	bytes_to_send = sb.st_size;
	do {
		written_bytes = sendfile(dst, src, NULL, bytes_to_send);
		if (written_bytes < 0) {
			ERR("Failed to copy file %s to %s errno[%d]",
			    srcpath, dstpath, errno);
			close(dst);
			close(src);
			remove(dstpath);
			return FALSE;
		}
		bytes_to_send -= written_bytes;
	} while (written_bytes != sb.st_size);

	close(dst);
	close(src);

	return TRUE;
}

mtp_bool _util_file_copy(const mtp_char *origpath, const mtp_char *newpath,
			 mtp_int32 *error)
{
	return _util_file_append(origpath, newpath, 0, error);
}

/*
 * A temporary wrapper to localize the warning about readdir_r usage.
 * To be replaced by readdir_r emulated with readdir.
 */
static inline int do_readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
	return readdir_r(dirp, entry, result);
}

mtp_bool _util_copy_dir_children_recursive(const mtp_char *origpath,
		const mtp_char *newpath, mtp_uint32 store_id, mtp_int32 *error)
{
	DIR *dir = NULL;
	struct dirent entry = { 0 };
	struct dirent *entryptr = NULL;
	mtp_int32 retval = 0;
	mtp_char old_pathname[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	mtp_char new_pathname[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	struct stat entryinfo;

	retv_if(origpath == NULL, FALSE);
	retv_if(newpath == NULL, FALSE);

	/* Open the given directory */
	dir = opendir(origpath);
	if (dir == NULL) {
		ERR("opendir(%s) Fail\n", origpath);
		_util_print_error();
		return FALSE;
	}

	retval = do_readdir_r(dir, &entry, &entryptr);

	while (retval == 0 && entryptr != NULL) {
		/* Skip the names "." and ".." as we don't want to recurse on them. */
		if (!g_strcmp0(entry.d_name, ".") ||
				!g_strcmp0(entry.d_name, "..")) {
			retval = do_readdir_r(dir, &entry, &entryptr);
			continue;
		}
		g_snprintf(old_pathname, MTP_MAX_PATHNAME_SIZE + 1,
				"%s/%s", origpath, entry.d_name);
		g_snprintf(new_pathname, MTP_MAX_PATHNAME_SIZE + 1,
				"%s/%s", newpath, entry.d_name);

		if (stat(old_pathname, &entryinfo) != 0) {
			ERR("Error statting [%s] errno [%d]\n", old_pathname, errno);
			closedir(dir);
			return FALSE;
		}

		/* Create new mtp object */
		mtp_store_t *store = _device_get_store(store_id);
		if (store == NULL) {
			ERR("store is NULL\n");
			closedir(dir);
			return FALSE;
		}

		mtp_obj_t *orig_obj = _entity_get_object_from_store_by_path(store, old_pathname);
		if (orig_obj == NULL) {
			ERR("orig_obj is NULL\n");
			closedir(dir);
			return FALSE;
		}

		mtp_obj_t *parent_obj = _entity_get_object_from_store_by_path(store, newpath);
		if (parent_obj == NULL) {
			ERR("orig_obj is NULL\n");
			closedir(dir);
			return FALSE;
		}

		mtp_obj_t *new_obj = _entity_alloc_mtp_object();
		if (new_obj == NULL) {
			ERR("_entity_alloc_mtp_object Fail\n");
			closedir(dir);
			return FALSE;
		}

		memset(new_obj, 0, sizeof(mtp_obj_t));

		new_obj->child_array.type = UINT32_TYPE;
		_entity_copy_mtp_object(new_obj, orig_obj);
		if (new_obj->obj_info == NULL) {
			_entity_dealloc_mtp_obj(new_obj);
			closedir(dir);
			return FALSE;
		}

		_entity_set_object_file_path(new_obj, new_pathname, CHAR_TYPE);
		new_obj->obj_handle = g_next_obj_handle++;
		new_obj->obj_info->h_parent = parent_obj->obj_handle;

		if (S_ISDIR(entryinfo.st_mode)) {
			if (FALSE == _util_dir_create(new_pathname, error)) {
				/* dir already exists
				   merge the contents */
				if (EEXIST != *error) {
					ERR("directory[%s] create Fail errno [%d]\n", new_pathname, errno);
					_entity_dealloc_mtp_obj(new_obj);
					closedir(dir);
					return FALSE;
				}
			}

			/* The directory is created. Add object to mtp store */
			_entity_add_object_to_store(store, new_obj);

			if (FALSE == _util_copy_dir_children_recursive(old_pathname,
						new_pathname, store_id, error)) {
				ERR("Recursive Copy of Children Fail\
						[%s]->[%s], errno [%d]\n", old_pathname, new_pathname, errno);
				closedir(dir);
				return FALSE;
			}
		} else {
			if (FALSE == _util_file_copy(old_pathname, new_pathname, error)) {
				ERR("file copy fail [%s]->[%s]\n",
						old_pathname, new_pathname);
				/* Cannot overwrite a read-only file,
				   Skip copy and retain the read-only file
				   on destination */
				if (EACCES == *error)
					goto DONE;
				_entity_dealloc_mtp_obj(new_obj);
				closedir(dir);
				return FALSE;
			}
#ifdef MTP_SUPPORT_SET_PROTECTION
			mtp_bool ret = FALSE;

			if (!((S_IWUSR & entryInfo.st_mode) ||
						(S_IWGRP & entryInfo.st_mode) ||
						(S_IWOTH & entryInfo.st_mode))) {
				ret = _util_set_file_attrs(newPathName,
						MTP_FILE_ATTR_MODE_REG |
						MTP_FILE_ATTR_MODE_READ_ONLY);
				if (!ret) {
					ERR("Failed to set directory attributes errno [%d]\n", errno);
					_entity_dealloc_mtp_obj(new_obj);
					closedir(dir);
					return FALSE;
				}
			}
#endif /* MTP_SUPPORT_SET_PROTECTION */
			/* The file is created. Add object to mtp store */
			_entity_add_object_to_store(store, new_obj);
		}
DONE:
		retval = do_readdir_r(dir, &entry, &entryptr);
	}

	closedir(dir);
	return (retval == 0) ? TRUE : FALSE;
}

mtp_bool _util_file_move(const mtp_char *origpath, const mtp_char *newpath,
		mtp_int32 *error)
{
	mtp_int32 ret = 0;

	ret = rename(origpath, newpath);
	if (ret < 0) {
		if (errno == EXDEV) {
			DBG("oldpath  and  newpath  are not on the same\
					mounted file system.\n");
			if (_util_file_copy(origpath, newpath, error) == FALSE) {
				ERR("_util_file_copy Fail errno [%d]\n", errno);
				return FALSE;
			}
			if (remove(origpath) < 0) {
				ERR("remove Fail : %d\n", errno);
				return FALSE;
			}
		} else {
			ERR("rename Fail : %d\n", errno);
			*error = errno;
			return FALSE;
		}
	}

	return TRUE;
}

mtp_bool _util_is_file_opened(const mtp_char *fullpath)
{
	mtp_int32 ret = 0;

	ret = rename(fullpath, fullpath);
	return (ret != 0);
}

mtp_bool _util_dir_create(const mtp_char *dirname, mtp_int32 *error)
{

	if (mkdir(dirname, S_IRWXU | S_IRGRP |
				S_IXGRP | S_IROTH | S_IXOTH) < 0) {
		*error = errno;
		return FALSE;
	}
	return TRUE;
}

mtp_int32 _util_remove_dir_children_recursive(const mtp_char *dirname,
		mtp_uint32 *num_of_deleted_file, mtp_uint32 *num_of_file, mtp_bool breadonly)
{
	retv_if(dirname == NULL, FALSE);

	DIR *dir = NULL;
	struct dirent entry = { 0 };
	struct dirent *entryptr = NULL;
	mtp_int32 retval = 0;
	mtp_char pathname[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };
	struct stat entryinfo;
	mtp_int32 ret = MTP_ERROR_NONE;

	/* Open the given directory */
	dir = opendir(dirname);
	if (dir == NULL) {
		ERR("Open directory Fail[%s], errno [%d]\n", dirname, errno);
		return MTP_ERROR_GENERAL;
	}

	retval = do_readdir_r(dir, &entry, &entryptr);

	while (retval == 0 && entryptr != NULL) {
		/* Skip the names "." and ".."
		   as we don't want to recurse on them. */
		if (!g_strcmp0(entry.d_name, ".") ||
				!g_strcmp0(entry.d_name, "..")) {
			retval = do_readdir_r(dir, &entry, &entryptr);
			continue;
		}
		g_snprintf(pathname, MTP_MAX_PATHNAME_SIZE + 1,
				"%s/%s", dirname, entry.d_name);
		if (stat(pathname, &entryinfo) != 0) {
			ERR("Error statting %s errno [%d]\n", pathname, errno);
			closedir(dir);
			return MTP_ERROR_GENERAL;
		}
		*num_of_file += 1;
		if (S_ISDIR(entryinfo.st_mode)) {
			ret = _util_remove_dir_children_recursive(pathname,
					num_of_deleted_file, num_of_file, breadonly);
			if (MTP_ERROR_GENERAL == ret || MTP_ERROR_ACCESS_DENIED == ret) {
				ERR("deletion fail [%s]\n", pathname);
				closedir(dir);
				return ret;
			}
			if (MTP_ERROR_OBJECT_WRITE_PROTECTED == ret) {
				DBG("Folder[%s] contains read-only files,hence\
						folder is not deleted\n", pathname);
				/* Read the next entry */
				goto DONE;
			}
			if (rmdir(pathname) < 0) {
				ERR("deletion fail [%s], errno [%d]\n", pathname, errno);
				closedir(dir);
				if (EACCES == errno)
					return MTP_ERROR_ACCESS_DENIED;
				return MTP_ERROR_GENERAL;
			}
			*num_of_deleted_file += 1;
		} else {
			/* Only during Deleteobject, bReadOnly(TRUE)
			   do not delete read-only files */
#ifdef MTP_SUPPORT_SET_PROTECTION
			if (breadonly) {
				/* check for file attributes */
				if (!((S_IWUSR & entryinfo.st_mode) ||
							(S_IWGRP & entryinfo.st_mode) ||
							(S_IWOTH & entryinfo.st_mode))) {
					ret = MTP_ERROR_OBJECT_WRITE_PROTECTED;
					DBG("File [%s] is readOnly:Deletion Fail\n", pathname);
					goto DONE;
				}
			}
#endif /* MTP_SUPPORT_SET_PROTECTION */
			if (unlink(pathname) < 0) {
				ERR("deletion fail [%s], errno [%d]\n", pathname, errno);
				closedir(dir);
				if (EACCES == errno)
					return MTP_ERROR_ACCESS_DENIED;
				return MTP_ERROR_GENERAL;
			}
			*num_of_deleted_file += 1;
		}
DONE:
		retval = do_readdir_r(dir, &entry, &entryptr);
		if (retval != 0) {
			closedir(dir);
			return MTP_ERROR_GENERAL;
		}
	}

	closedir(dir);
	return ret;
}
/* LCOV_EXCL_STOP */

/*
 * mtp_bool _util_get_file_attrs(const mtp_char *filename, file_attr_t *attrs)
 * This function gets the file attributes.
 *
 * @param[in]		filename	Specifies the name of file to find.
 * @param[out]		attrs		Points the file Attributes.
 * @return		This function returns TRUE if gets the attributes
 *			successfully, otherwise FALSE.
 */
mtp_bool _util_get_file_attrs(const mtp_char *filename, file_attr_t *attrs)
{
	struct stat fileinfo = { 0 };

	retvm_if(stat(filename, &fileinfo) < 0, FALSE,
		"%s : stat Fail errno [%d]\n", filename, errno);

	memset(attrs, 0, sizeof(file_attr_t));
	attrs->fsize = (mtp_uint64)fileinfo.st_size;
	attrs->ctime = fileinfo.st_ctime;
	attrs->mtime = fileinfo.st_mtime;

	/*Reset attribute mode */
	attrs->attribute = MTP_FILE_ATTR_MODE_NONE;
	if (S_ISREG(fileinfo.st_mode)) {
		/* LCOV_EXCL_START */
		attrs->attribute |= MTP_FILE_ATTR_MODE_REG;
		if (!((S_IWUSR & fileinfo.st_mode) ||
					(S_IWGRP & fileinfo.st_mode) ||
					(S_IWOTH & fileinfo.st_mode))) {
			attrs->attribute |= MTP_FILE_ATTR_MODE_READ_ONLY;
		}
	} else if (S_ISDIR(fileinfo.st_mode)) {
		attrs->attribute |= MTP_FILE_ATTR_MODE_DIR;
	} else if (S_ISBLK(fileinfo.st_mode) || S_ISCHR(fileinfo.st_mode) ||
			S_ISLNK(fileinfo.st_mode) || S_ISSOCK(fileinfo.st_mode)) {
		attrs->attribute |= MTP_FILE_ATTR_MODE_SYSTEM;
	}
	return TRUE;
}

mtp_bool _util_set_file_attrs(const mtp_char *filename, mtp_dword attrib)
{
	mtp_dword attrs = 0;

	if (MTP_FILE_ATTR_MODE_REG & attrib) {
		/*Reqular file */
		if (MTP_FILE_ATTR_MODE_READ_ONLY & attrib)
			attrs |= (S_IRUSR | S_IRGRP | S_IROTH);
		else
			attrs |= (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	} else {
		/* do nothing for files other than File/Folder */
		DBG("entered here nothing\n");
		return FALSE;
	}

	if (0 != chmod(filename, attrs)) {
		if (EPERM == errno)
			return TRUE;
		ERR_SECURE("Change mode of [File : %s] Fail\n", filename);
		_util_print_error();
		return FALSE;
	}
	return TRUE;
}
/* LCOV_EXCL_STOP */

/*
 * mtp_bool _util_ifind_first(mtp_char *dirname, DIR **dirp,
 *	dir_entry_t *dir_info)
 * This function finds the first file in the directory stream.
 *
 * @param[in]		dirname		specifies the name of directory.
 * @param[out]		dirp		pointer to the directory stream.
 * @param[in]		dir_info	pointer to the file information.
 * @return		This function returns TRUE on success, otherwise FALSE.
 */
mtp_bool _util_ifind_first(mtp_char *dirname, DIR **dirp, dir_entry_t *dir_info)
{
	DIR *dir;

	retv_if(dirp == NULL, FALSE);
	retv_if(dirname == NULL, FALSE);
	retv_if(dir_info == NULL, FALSE);

	dir = opendir(dirname);
	if (NULL == dir) {
		/* LCOV_EXCL_START */
		ERR("opendir(%s) Fail\n", dirname);
		_util_print_error();

		return FALSE;
	}

	if (_util_ifind_next(dirname, dir, dir_info) == FALSE) {
		DBG("Stop enumeration\n");
		_util_print_error();
		closedir(dir);
		return FALSE;
		/* LCOV_EXCL_STOP */
	}

	*dirp = dir;

	return TRUE;
}

/*
 * mtp_bool _util_ifind_next(mtp_char *dirname, DIR *dirp, dir_entry_t *dir_info)
 * This function finds the next successive file in the directory stream.
 *
 * @param[in]		dirname		name of the directory.
 * @param[in]		dirp		pointer to the directory stream.
 * @param[out]		dir_info	Points the file information.
 * @return		This function returns TRUE on success, otherwise FALSE.
 */
mtp_bool _util_ifind_next(mtp_char *dir_name, DIR *dirp, dir_entry_t *dir_info)
{
	mtp_int32 ret = 0;
	struct dirent entry = {0};
	struct stat stat_buf = {0};
	struct dirent *result = NULL;
	mtp_char path_name[MTP_MAX_PATHNAME_SIZE + 1] = { 0 };

	retv_if(dir_name == NULL, FALSE);
	retv_if(dir_info == NULL, FALSE);

	do {
		ret = do_readdir_r(dirp, &entry, &result);
		if (ret != 0) {
			ERR("do_readdir_r Fail : %d\n", ret);
			return FALSE;
		} else if (result == NULL) {
			DBG("There is no more entry\n");
			return FALSE;
		}

		if (_util_create_path(path_name, sizeof(path_name),
					dir_name, entry.d_name) == FALSE) {
			continue;
		}

		if (stat(path_name, &stat_buf) < 0) {
			ERR_SECURE("stat Fail, skip [%s]\n", path_name);
			continue;
		}
		break;
	} while (1);

	g_strlcpy(dir_info->filename, path_name, sizeof(dir_info->filename));
	dir_info->attrs.attribute = MTP_FILE_ATTR_MODE_NONE;

	switch (stat_buf.st_mode & S_IFMT) {
	case S_IFREG:
		dir_info->type = MTP_FILE_TYPE;
		if (!(stat_buf.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
			dir_info->attrs.attribute |= MTP_FILE_ATTR_MODE_READ_ONLY;
		break;

	case S_IFDIR:
		dir_info->type = MTP_DIR_TYPE;
		dir_info->attrs.attribute = MTP_FILE_ATTR_MODE_DIR;
		break;

	case S_IFBLK:
	case S_IFCHR:
	case S_IFIFO:
	case S_IFLNK:
	case S_IFSOCK:
	/* LCOV_EXCL_START */
		dir_info->attrs.attribute |= MTP_FILE_ATTR_MODE_SYSTEM;
		break;

	default:
		dir_info->attrs.attribute |= MTP_FILE_ATTR_MODE_SYSTEM;
		ERR_SECURE("%s has unknown type. mode[0x%x]\n",
				dir_info->filename, stat_buf.st_mode);
		break;
		/* LCOV_EXCL_STOP */
	}

	/* Directory Information */
	dir_info->attrs.mtime = stat_buf.st_mtime;
	dir_info->attrs.fsize = (mtp_uint64)stat_buf.st_size;

	return TRUE;
}

mtp_bool _util_get_filesystem_info(mtp_char *storepath,
	fs_info_t *fs_info)
{
	GString *path = g_string_new(storepath);
	mtp_bool ret = FALSE;

	if (g_string_equal(g_conf.external_path, path)) {
		struct statfs buf = { 0 };
		mtp_uint64 avail_size = 0;
		mtp_uint64 capacity = 0;
		mtp_uint64 used_size = 0;

		retvm_if(statfs(storepath, &buf) != 0, FALSE, "statfs is failed\n");

		capacity = used_size = avail_size = (mtp_uint64)buf.f_bsize;
		DBG("Block size : %lu\n", (unsigned long)buf.f_bsize);
		capacity *= buf.f_blocks;
		used_size *= (buf.f_blocks - buf.f_bavail);
		avail_size *= buf.f_bavail;

		fs_info->disk_size = capacity;
		fs_info->reserved_size = used_size;
		fs_info->avail_size = avail_size;

		ret = TRUE;
	}

	g_string_free(path, TRUE);
	return ret;
}

/* LCOV_EXCL_START */
/*
 * void _FLOGD(const char *fmt, ...)
 * This function writes MTP debug message to MTP log file
 *
 * @param[in]		fmt Formatted debug message.
 * @param[out]		None.
 * @return		None.
 */
void _FLOGD(const char *file, const char *fmt, ...)
{
	static mtp_int64 written_bytes = 0;
	FILE *fp = NULL;
	va_list ap;

	if (written_bytes == 0 || written_bytes > MTP_LOG_MAX_SIZE) {
		fp = fopen(MTP_LOG_FILE, "w");
		written_bytes = 0;
	} else {
		fp = fopen(MTP_LOG_FILE, "a+");
	}

	if (fp == NULL)
		return;

	written_bytes += fprintf(fp, "%s ", file + SRC_PATH_LEN) - SRC_PATH_LEN;
	va_start(ap, fmt);
	written_bytes += vfprintf(fp, fmt, ap);
	va_end(ap);

	fclose(fp);
}
/* LCOV_EXCL_STOP */
