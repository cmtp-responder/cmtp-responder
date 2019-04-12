#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

mtp_prepare_tests_linux_gvfs()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local BASE=${__local_cookie[2]}
	find "$BASE"/"$STORE"/ -type f -print0 | xargs -0 gio remove -f
	find "$BASE"/"$STORE"/ -type d -print0 | sort -r -z | xargs -0 gio remove -f
	return 0
}

mtp_list_files_linux_gvfs()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local BASE=${__local_cookie[2]}
	/usr/bin/find "$BASE"/"$STORE"/"$PARENT" -type f -printf "%P\n"
	return 0
}

mtp_list_folders_linux_gvfs()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local BASE=${__local_cookie[2]}
	/usr/bin/find "$BASE"/"$STORE"/"$PARENT" -type d -printf "%P\n"
	return 0
}

mtp_copy_to_store_linux_gvfs()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local NAME="${4}"
	local BASE=${__local_cookie[2]}
	[ -d "$BASE"/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	gio copy "$NAME" "$BASE"/"$STORE"/"$PARENT"
	return $?
}

mtp_retrieve_from_store_linux_gvfs()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local REMOTE_NAME="${4}"
	local LOCAL_NAME="${5}"
	local BASE=${__local_cookie[2]}
	[ -d "$BASE"/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	[ -f "$BASE"/"$STORE"/"$PARENT"/"$REMOTE_NAME" ] || { echo "$REMOTE_NAME does not exist" return 1; }
	[ -r "$BASE"/"$STORE"/"$PARENT"/"$REMOTE_NAME" ] || { echo "$REMOTE_NAME is not readable"; return 1; }
	gio copy "$BASE"/"$STORE"/"$PARENT"/"$REMOTE_NAME" "$LOCAL_NAME"
	return $?
}

mtp_remove_from_store_linux_gvfs()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local NAME="${4}"
	local BASE=${__local_cookie[2]}
	gio remove -f "$BASE"/"$STORE"/"$PARENT"/"$NAME"
	return 0
}

mtp_create_folder_linux_gvfs()
{
	local -n __local_cookie=${1}
	local STORE="${2#s:}"
	local PARENT="${3#p:}"
	local NAME="${4#n:}"
	local BASE=${__local_cookie[2]}
	[ -d "$BASE"/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	gio mkdir "$BASE"/"$STORE"/"$PARENT"/"$NAME"
	return 0
}

mtp_remove_folder_linux_gvfs()
{
	local -n __local_cookie=${1}
	local STORE="${2#s:}"
	local PARENT="${3#p:}"
	local NAME="${4#n:}"
	local BASE=${__local_cookie[2]}
	find "$BASE"/"$STORE"/"$PARENT"/"$NAME" -type f -print0 | xargs -0 gio remove -f
	find "$BASE"/"$STORE"/"$PARENT"/"$NAME" -type d -print0 | sort -r -z | xargs -0 gio remove -f
	return 0
}

mtp_finish_tests_linux_gvfs()
{
	return 0
}
