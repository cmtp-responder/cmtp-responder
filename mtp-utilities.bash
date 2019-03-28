#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

mtp_list_files()
{
	local STORE="${1}"
	local PARENT="${2}"
	# TODO: dummy
	mkdir -p /tmp/"$STORE"
	find /tmp/"$STORE"/"$PARENT" -type f
	return $?
}

mtp_list_folders()
{
	local STORE="${1}"
	local PARENT="${2}"
	# TODO: dummy
	mkdir -p /tmp/"$STORE"
	find /tmp/"$STORE"/"$PARENT" -type d
	return $?
}

mtp_copy_to_store()
{
	local STORE="${1}"
	local PARENT="${2}"
	local NAME="${3}"
	# TODO: dummy
	mkdir -p /tmp/"$STORE"
	[ -d /tmp/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	cp "$NAME" /tmp/"$STORE"/"$PARENT"
	return $?
}

mtp_retrieve_from_store()
{
	local STORE="${1}"
	local PARENT="${2}"
	local REMOTE_NAME="${3}"
	local LOCAL_NAME="${4}"
	# TODO: dummy
	mkdir -p /tmp/"$STORE"
	[ -d /tmp/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	[ -f /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" ] || { echo "$REMOTE_NAME does not exist" return 1; }
	[ -r /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" ] || { echo "$REMOTE_NAME is not readable"; return 1; }
	/bin/cp /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" "$LOCAL_NAME"
	return $?
}

mtp_remove_from_store()
{
	local STORE="${1}"
	local PARENT="${2}"
	local NAME="${3}"
	# TODO: dummy
	mkdir -p /tmp/"$STORE"
	/bin/rm -rf /tmp/"$STORE"/"$PARENT"/"$NAME"
	return 0
}
