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

#define __USE_STDIO__
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
mtp_uint32 _util_file_open(const mtp_char *filename, file_mode_t mode,
		mtp_int32 *error)
{
#ifdef __USE_STDIO__
	FILE *fhandle = NULL;
	char *fmode = NULL;

	switch ((int)mode) {
	case MTP_FILE_READ:
		fmode = "rm";
		break;

	case MTP_FILE_WRITE:
		fmode = "w";
		break;

	case MTP_FILE_APPEND:
		fmode = "a";
		break;

	case MTP_FILE_READ | MTP_FILE_WRITE:
		fmode = "r+";
		break;

	case MTP_FILE_READ | MTP_FILE_WRITE | MTP_FILE_APPEND:
		fmode = "a+";
		break;

	default:
		ERR("Invalid mode : %d\n", mode);
		*error = EINVAL;
		return INVALID_FILE;
	}

	fhandle = fopen(filename, fmode);
	if (fhandle == NULL) {
		ERR("File open Fail:mode[0x%x], errno [%d]\n", mode, errno);
		ERR_SECURE("filename[%s]\n", filename);
		*error = errno;
		return INVALID_FILE;
	}

	fcntl(fileno(fhandle), F_SETFL, O_NOATIME);

	return (mtp_uint32)fhandle;

#else /* __USE_STDIO__ */

	mtp_int32 fhandle = 0;
	mtp_int32 flags = 0;
	mode_t perm = 0;

	switch ((int)mode) {
	case MTP_FILE_READ:
		flags = O_RDONLY;
		break;

	case MTP_FILE_WRITE:
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		perm = 0644;
		break;

	case MTP_FILE_APPEND:
		flags = O_WRONLY | O_APPEND | O_CREAT;
		perm = 0644;
		break;

	case MTP_FILE_READ | MTP_FILE_WRITE:
		flags = O_RDWR;
		break;

	case MTP_FILE_READ | MTP_FILE_WRITE | MTP_FILE_APPEND:
		flags = O_RDWR | O_APPEND | O_CREAT;
		perm = 0644;
		break;

	default:
		ERR("Invalid mode : %d\n", mode);
		*error = EINVAL;
		return INVALID_FILE;
	}

	if (perm)
		fhandle = open(filename, flags, perm);
	else
		fhandle = open(filename, flags);

	if (fhandle < 0) {
		ERR("File open Fail:mode[0x%x], errno [%d]\n", mode, errno);
		ERR_SECURE("filename[%s]\n", filename);
		*error = errno;
		return INVALID_FILE;
	}

	return (mtp_uint32)fhandle;
#endif /* __USE_STDIO__ */
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
void _util_file_read(mtp_uint32 fhandle, void *bufptr, mtp_uint32 size,
		mtp_uint32 *read_count)
{
	mtp_uint32 bytes_read = 0;

#ifdef __USE_STDIO__
	bytes_read = fread_unlocked(bufptr, sizeof(mtp_char), size, (FILE *)fhandle);
#else /* __USE_STDIO__ */
	bytes_read = read(fhandle, bufptr, size);
#endif /* __USE_STDIO__ */

	*read_count = bytes_read;
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

mtp_uint32 _util_file_write(mtp_uint32 fhandle, void *bufptr, mtp_uint32 size)
{
	mtp_uint32 bytes_written = 0;

#ifdef __USE_STDIO__
	bytes_written = fwrite_unlocked(bufptr, sizeof(mtp_char), size, (FILE *)fhandle);
#else /* __USE_STDIO__ */
	mtp_int32 ret = 0;

	ret = write(fhandle, bufptr, size);
	if (ret < 0)
		ret = 0;

	bytes_written = ret;
#endif /* __USE_STDIO__ */

	return bytes_written;
}

/**
 * mtp_int32 _util_file_close(mtp_uint32 fhandle)
 * This function closes the file.
 *
 * @param[in]	handle	Specifies the handle of file to close.
 * @return	0 in case of success or EOF on failure.
 */
mtp_int32 _util_file_close(mtp_uint32 fhandle)
{
#ifdef __USE_STDIO__
	return fclose((FILE *)fhandle);
#else /* __USE_STDIO__ */
	return close(fhandle);
#endif /* __USE_STDIO__ */
}

/*
 * This function seeks to a particular location in a file.
 *
 * @param[in]	handle		Specifies the handle of file to seek.
 * @param[in]	offset		Specifies the starting point.
 * @param[in]	whence		Specifies the setting value
 * @return	Returns TRUE in case of success or FALSE on Failure.
 */
mtp_bool _util_file_seek(mtp_uint32 handle, off_t offset, mtp_int32 whence)
{
	mtp_int64 ret_val = 0;

#ifdef __USE_STDIO__
	ret_val = fseek((FILE *)handle, offset, whence);
#else /* __USE_STDIO__ */
	ret_val = lseek(handle, offset, whence);
	if (ret_val > 0)
		ret_val = 0;
#endif /* __USE_STDIO__ */
	if (ret_val < 0) {
		ERR(" _util_file_seek error errno [%d]\n", errno);
		return FALSE;
	}

	return TRUE;
}

mtp_bool _util_file_copy(const mtp_char *origpath, const mtp_char *newpath,
		mtp_int32 *error)
{
#ifdef __USE_STDIO__
	FILE *fold = NULL;
	FILE *fnew = NULL;
	size_t nmemb = 0;
	mtp_int32 ret = 0;
	mtp_char buf[BUFSIZ] = { 0 };

	if ((fold = fopen(origpath, "rb")) == NULL) {
		ERR("In-file open Fail errno [%d]\n", errno);
		*error = errno;
		return FALSE;
	}

	if ((fnew = fopen(newpath, "wb")) == NULL) {
		ERR("Out-file open Fail errno [%d]\n", errno);
		*error = errno;
		fclose(fold);
		return FALSE;
	}

	do {
		nmemb = fread(buf, sizeof(mtp_char), BUFSIZ, fold);
		if (nmemb < BUFSIZ && ferror(fold)) {
			ERR("fread Fail errno [%d] \n", errno);
			*error = errno;
			fclose(fnew);
			fclose(fold);
			if (remove(newpath) < 0)
				ERR("Remove Fail");
			return FALSE;
		}

		ret = fwrite(buf, sizeof(mtp_char), nmemb, fnew);
		if (ret < nmemb && ferror(fnew)) {
			ERR("fwrite Fail errno [%d]\n", errno);
			*error = errno;
			fclose(fnew);
			fclose(fold);
			if (remove(newpath) < 0)
				ERR("Remove Fail");
			return FALSE;
		}
	} while (!feof(fold));

	fclose(fnew);
	fclose(fold);
#else /* __USE_STDIO__ */
	mtp_int32 in_fd = 0;
	mtp_int32 out_fd = 0;
	mtp_int32 ret = 0;
	off_t offset = 0;

	if ((in_fd = open(origpath, O_RDONLY)) < 0) {
		ERR("In-file open Fail, errno [%d]\n", errno);
		*error = errno;
		return FALSE;
	}

	if ((out_fd = open(newpath, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		ERR("Out-file open Fail errno [%d] \n", errno);
		*error = errno;
		close(in_fd);
		return FALSE;
	}

	do {
		ret = sendfile(out_fd, in_fd, &offset, BUFSIZ);
		if (ret < 0) {
			ERR("sendfile Fail errno [%d]\n", errno);
			*error = errno;
			close(out_fd);
			close(in_fd);
			if (remove(newpath) < 0)
				ERR("Remove Fail");
			return FALSE;
		}
	} while (ret == BUFSIZ);

	close(out_fd);
	close(in_fd);
#endif /* __USE_STDIO__ */

	return TRUE;
}

mtp_bool _util_copy_dir_children_recursive(const mtp_char *origpath,
		const mtp_char *newpath, mtp_int32 *error)
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
		ERR("opendir(%s) Fail", origpath);
		_util_print_error();
		return FALSE;
	}

	retval = readdir_r(dir, &entry, &entryptr);

	while (retval == 0 && entryptr != NULL) {
		/* Skip the names "." and ".." as we don't want to recurse on them. */
		if (!g_strcmp0(entry.d_name, ".") ||
				!g_strcmp0(entry.d_name, "..")) {
			retval = readdir_r(dir, &entry, &entryptr);
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

		if (S_ISDIR(entryinfo.st_mode)) {
			if (FALSE == _util_dir_create(new_pathname, error)) {
				/* dir already exists
				   merge the contents */
				if (EEXIST != *error) {
					ERR("directory[%s] create Fail errno [%d]\n", new_pathname, errno);
					closedir(dir);
					return FALSE;
				}
			}
			if (FALSE == _util_copy_dir_children_recursive(old_pathname,
						new_pathname, error)) {
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
					closedir(dir);
					return FALSE;
				}
			}
#endif /* MTP_SUPPORT_SET_PROTECTION */
		}
DONE:
		retval = readdir_r(dir, &entry, &entryptr);
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
					mounted file system.");
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
		ERR("Open directory Fail[%s], errno [%d]", dirname, errno);
		return MTP_ERROR_GENERAL;
	}

	retval = readdir_r(dir, &entry, &entryptr);

	while (retval == 0 && entryptr != NULL) {
		/* Skip the names "." and ".."
		   as we don't want to recurse on them. */
		if (!g_strcmp0(entry.d_name, ".") ||
				!g_strcmp0(entry.d_name, "..")) {
			retval = readdir_r(dir, &entry, &entryptr);
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
						folder is not deleted\n",pathname);
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
					DBG("File [%s] is readOnly:Deletion Fail\n",pathname);
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
		retval = readdir_r(dir, &entry, &entryptr);
		if (retval != 0) {
			closedir(dir);
			return MTP_ERROR_GENERAL;
		}
	}

	closedir(dir);
	return ret;
}

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

	if (stat(filename, &fileinfo) < 0) {
		ERR_SECURE("%s : stat Fail errno [%d]\n", filename, errno);
		return FALSE;
	}

	memset(attrs, 0, sizeof(file_attr_t));
	attrs->fsize = fileinfo.st_size;
	attrs->ctime = fileinfo.st_ctime;
	attrs->mtime = fileinfo.st_mtime;

	/*Reset attribute mode */
	attrs->attribute = MTP_FILE_ATTR_MODE_NONE;
	if (S_ISREG(fileinfo.st_mode)) {
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
		DBG("entered here nothing");
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
		ERR("opendir(%s) Fail", dirname);
		_util_print_error();

		return FALSE;
	}

	if (_util_ifind_next(dirname, dir, dir_info) == FALSE) {
		DBG("Stop enumeration");
		_util_print_error();
		closedir(dir);
		return FALSE;
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
		ret = readdir_r(dirp, &entry, &result);
		if (ret != 0) {
			ERR("readdir_r Fail : %d\n", ret);
			return FALSE;
		} else if (result == NULL) {
			DBG("There is no more entry");
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
		if (!(stat_buf.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH))) {
			dir_info->attrs.attribute |= MTP_FILE_ATTR_MODE_READ_ONLY;
		}
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
		dir_info->attrs.attribute |= MTP_FILE_ATTR_MODE_SYSTEM;
		break;

	default:
		dir_info->attrs.attribute |= MTP_FILE_ATTR_MODE_SYSTEM;
		ERR_SECURE("%s has unknown type. mode[0x%x]\n",
				dir_info->filename, stat_buf.st_mode);
		break;
	}

	/* Directory Information */
	dir_info->attrs.mtime = stat_buf.st_mtime;
	dir_info->attrs.fsize = stat_buf.st_size;

	return TRUE;
}

mtp_bool _util_get_filesystem_info(mtp_char *storepath, fs_info_t *fs_info)
{
	struct statfs buf = { 0 };
	mtp_uint64 avail_size = 0;
	mtp_uint64 capacity = 0;
	mtp_uint64 used_size = 0;

	if (statfs(storepath, &buf) != 0) {
		ERR("statfs Fail");
		return FALSE;
	}

	capacity = used_size = avail_size = buf.f_bsize;
	DBG("Block size : %d\n", buf.f_bsize);
	capacity *= buf.f_blocks;
	used_size *= (buf.f_blocks - buf.f_bavail);
	avail_size *= buf.f_bavail;

	fs_info->disk_size = capacity;
	fs_info->reserved_size = used_size;
	fs_info->avail_size = avail_size;

	return TRUE;
}

void _util_count_num_lines(mtp_uint32 fhandle, mtp_uint32 *num_lines)
{
	if (fhandle == INVALID_FILE)
		return;

	mtp_uint32 line_count = 0;
	mtp_char *buffer = NULL;
	mtp_int32 read_bytes;
	mtp_uint32 line_max_length;

	line_max_length = LINUX_MAX_PATHNAME_LENGTH + 2;
	buffer = (mtp_char *)g_malloc(line_max_length);
	if (buffer == NULL) {
		ERR("Malloc Fail");
		return;
	}

#ifdef __USE_STDIO__
	while((read_bytes = getline(&buffer,
					&line_max_length, (FILE *)fhandle)) != -1) {
		if (read_bytes > MTP_MAX_PATHNAME_SIZE + 1)
			continue;
		line_count++;
	}
#else /* __USE_STDIO__ */

	mtp_uint16 ii = 0;
	mtp_uint32 prev_pos = -1;
	mtp_uint32 new_pos;
	mtp_uint32 filename_len = 0;
	while((read_bytes = read(fhandle, buffer, LINUX_MAX_PATHNAME_LENGTH)) > 0) {
		for (ii = 0; ii < read_bytes; ii++) {
			if (buffer[ii] != '\n')
				continue;
			new_pos = ii;
			filename_len = new_pos - prev_pos -1;
			prev_pos = new_pos;
			if (filename_len > MTP_MAX_PATHNAME_SIZE)
				continue;
			line_count++;
		}
		if (buffer[read_bytes - 1] != '\n') {
			_util_file_seek(fhandle, prev_pos + 1 - read_bytes, SEEK_CUR);
		}
		prev_pos = -1;
	}
#endif /* __USE_STDIO__ */

	*num_lines = line_count;
	g_free(buffer);
	return;
}

void _util_fill_guid_array(void *guidarray, mtp_uint32 start_index,
		mtp_uint32 fhandle, mtp_uint32 size)
{
	ptp_array_t *pguidarray = NULL;
	mtp_uint32 *guidptr = NULL;
	mtp_uint32 num_lines = 0;
	mtp_wchar objfullpath[MTP_MAX_PATHNAME_SIZE * 2 + 1] = { 0 };
	mtp_char guid[16] = { 0 };
	mtp_char *buffer = NULL;
	mtp_uint32 line_max_length;
	mtp_uint32 len = 0;

	line_max_length = LINUX_MAX_PATHNAME_LENGTH + 2;
	pguidarray = (ptp_array_t *)guidarray;
	guidptr = (mtp_uint32 *)(pguidarray->array_entry);

	buffer = (mtp_char *)g_malloc(line_max_length);
	if (buffer == NULL) {
		ERR("Malloc Fail");
		return;
	}

#ifdef __USE_STDIO__
	while ((len = getline(&buffer, &line_max_length, (FILE *)fhandle)) != -1 &&
			(num_lines - start_index) <= size) {
		if (len > MTP_MAX_PATHNAME_SIZE + 1)
			continue;
		num_lines++;
		if (num_lines < start_index)
			continue;
		buffer[len - 1] = '\0';
		_util_utf8_to_utf16(objfullpath,
				sizeof(objfullpath) / WCHAR_SIZ, buffer);
		_util_conv_wstr_to_guid(objfullpath, (mtp_uint64 *)guid);
		memcpy(&(guidptr[pguidarray->num_ele]),
				guid, sizeof(guid));
		pguidarray->num_ele += sizeof(mtp_uint32);
	}
#else /* __USE_STDIO__ */
	mtp_uint16 ii = 0;
	mtp_uint32 prev_pos = -1;
	mtp_uint32 new_pos;
	mtp_char file_name[MTP_MAX_PATHNAME_SIZE + 1];
	mtp_int32 read_bytes;

	while ((read_bytes = read(fHandle, buffer,
					LINUX_MAX_PATHNAME_LENGTH)) > 0 &&
			(num_lines - start_index) <= size) {

		for (ii = 0; ii < read_bytes; ii++) {
			if (buffer[ii] != '\n')
				continue;
			new_pos = ii;
			len = new_pos - prev_pos - 1;
			prev_pos = new_pos;
			if (len > MTP_MAX_PATHNAME_SIZE)
				continue;
			num_lines++;
			if (num_lines < start_index)
				continue;
			strncpy(file_name, &buffer[new_pos - len], len);
			file_name[len] = '\0';
			_util_utf8_to_utf16(objfullpath,
					sizeof(objfullpath) / WCHAR_SIZ, file_name);
			_util_conv_wstr_to_guid(objfullpath, (mtp_uint64 *)guid);
			memcpy(&(guidptr[pguidarray->num_elements]),
					guid, sizeof(guid));
			pguidarray->num_elements += sizeof(mtp_uint32);
		}

		if (buffer[read_bytes - 1] != '\n') {
			_util_file_seek(fhandle, prev_pos + 1 - read_bytes, SEEK_CUR);
		}
		prev_pos = -1;
	}
#endif /* __USE_STDIO__ */

	g_free(buffer);

	return;
}

/*
 * void FLOGD(const char *fmt, ...)
 * This function writes MTP debug message to MTP log file
 *
 * @param[in]		fmt Formatted debug message.
 * @param[out]		None.
 * @return		None.
 */
void FLOGD(const char *fmt, ...)
{
	static int written_bytes = 0;
	FILE *fp = NULL;
	va_list ap;

	if (written_bytes == 0 || written_bytes > MTP_LOG_MAX_SIZE) {
		fp = fopen(MTP_LOG_FILE, "w");
		written_bytes = 0;
	} else {
		fp = fopen(MTP_LOG_FILE, "a+");
	}

	if (fp == NULL) {
		return;
	}

	written_bytes += fprintf(fp, "%s ", __FILE__);
	va_start(ap, fmt);
	written_bytes += vfprintf(fp, fmt, ap);
	va_end(ap);

	fclose(fp);
	return;
}
