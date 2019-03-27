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
	echo ""
	return 0
}

mtp_list_folders()
{
	local STORE="${1}"
	local PARENT="${2}"
	# TODO: dummy
	echo ""
	return 0
}

mtp_store_empty()
{
	local FILES=`mtp_list_files "$STORE_NAME" /`
	[ x"$FILES" == "x" ] || return 1

	local FOLDERS=`mtp_list_folders "$STORE_NAME" /`
	[ x"$FOLDERS" == "x" ] || return 1

	return 0
}

mtp_copy_to_store()
{
	local STORE="${1}"
	local PATH="${2}"
	local NAME="${3}"
	# TODO: dummy
	return 0
}

mtp_retrieve_from_store()
{
	local STORE="${1}"
	local PATH="${2}"
	local REMOTE_NAME="${3}"
	local LOCAL_NAME="${4}"
	# TODO: dummy
	[ -f "$REMOTE_NAME" ] || return 1
	[ -r "$REMOTE_NAME" ] || return 1
	/bin/cp "$REMOTE_NAME" "$LOCAL_NAME"
	return $?
}

mtp_remove_from_store()
{
	local STORE="${1}"
	local PATH="${2}"
	local NAME="${3}"
	# TODO: dummy
	/bin/rm -rf "$NAME"
	return 0
}
