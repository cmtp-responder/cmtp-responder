#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

mtp_prepare_tests_bash_dummy()
{
	local -n __local_cookie=${1}
	local STORE="${2}"

	rm -rf /tmp/"$STORE"
	mkdir -p /tmp/"$STORE"
}

mtp_list_files_bash_dummy()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"

	mkdir -p /tmp/"$STORE"
	/usr/bin/find /tmp/"$STORE"/"$PARENT" -type f -printf "%P\n"
	return 0
}

mtp_list_folders_bash_dummy()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"

	mkdir -p /tmp/"$STORE"
	/usr/bin/find /tmp/"$STORE"/"$PARENT" -type d -printf "%P\n"
	return 0
}

mtp_copy_to_store_bash_dummy()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local NAME="${4}"

	mkdir -p /tmp/"$STORE"
	[ -d /tmp/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	cp "$NAME" /tmp/"$STORE"/"$PARENT"
	return $?
}

mtp_retrieve_from_store_bash_dummy()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local REMOTE_NAME="${4}"
	local LOCAL_NAME="${5}"

	mkdir -p /tmp/"$STORE"
	[ -d /tmp/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	[ -f /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" ] || { echo "$REMOTE_NAME does not exist" return 1; }
	[ -r /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" ] || { echo "$REMOTE_NAME is not readable"; return 1; }
	/bin/cp /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" "$LOCAL_NAME"

	return $?
}

mtp_remove_from_store_bash_dummy()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local NAME="${4}"

	mkdir -p /tmp/"$STORE"
	/bin/rm -rf /tmp/"$STORE"/"$PARENT"/"$NAME"

	return 0
}

mtp_create_folder_bash_dummy()
{
	local -n __local_cookie=${1}
	local STORE="${2#s:}"
	local PARENT="${3#p:}"
	local NAME="${4#n:}"

	/bin/mkdir -p /tmp/"$STORE"
	[ -d /tmp/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	mkdir /tmp/"$STORE"/"$PARENT"/"$NAME"
}

mtp_remove_folder_bash_dummy()
{
	local -n __local_cookie=${1}
	local STORE="${2#s:}"
	local PARENT="${3#p:}"
	local NAME="${4#n:}"
	mkdir -p /tmp/"$STORE"

	/bin/rm -rf /tmp/"$STORE"/"$PARENT"/"$NAME"
}

mtp_finish_tests_bash_dummy()
{
	return 0
}
