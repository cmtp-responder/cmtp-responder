#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

mtp_prepare_tests_bash_dummy()
{
	local STORE="${1}"
	rm -rf /tmp/"$STORE"
	mkdir -p /tmp/"$STORE"
}

mtp_prepare_tests_vtbl=(mtp_prepare_tests_bash_dummy)

mtp_prepare_tests()
{
	${mtp_prepare_tests_vtbl[$HOST]} $@
}

mtp_list_files_bash_dummy()
{
	local STORE="${1}"
	local PARENT="${2}"
	mkdir -p /tmp/"$STORE"
	/usr/bin/find /tmp/"$STORE"/"$PARENT" -type f -printf "%P\n"
	return 0
}

mtp_list_files_vtbl=(mtp_list_files_bash_dummy)

mtp_list_files()
{
	${mtp_list_files_vtbl[$HOST]} $@
}

mtp_list_folders_bash_dummy()
{
	local STORE="${1}"
	local PARENT="${2}"
	mkdir -p /tmp/"$STORE"
	/usr/bin/find /tmp/"$STORE"/"$PARENT" -type d -printf "%P\n"
	return 0
}

mtp_list_folders_vtbl=(mtp_list_folders_bash_dummy)

mtp_list_folders()
{
	${mtp_list_folders_vtbl[$HOST]} $@
}

mtp_copy_to_store_bash_dummy()
{
	local STORE="${1}"
	local PARENT="${2}"
	local NAME="${3}"
	mkdir -p /tmp/"$STORE"
	[ -d /tmp/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	cp "$NAME" /tmp/"$STORE"/"$PARENT"
	return $?
}

mtp_copy_to_store_vtbl=(mtp_copy_to_store_bash_dummy)

mtp_copy_to_store()
{
	${mtp_copy_to_store_vtbl[$HOST]} $@
}

mtp_retrieve_from_store_bash_dummy()
{
	local STORE="${1}"
	local PARENT="${2}"
	local REMOTE_NAME="${3}"
	local LOCAL_NAME="${4}"
	mkdir -p /tmp/"$STORE"
	[ -d /tmp/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	[ -f /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" ] || { echo "$REMOTE_NAME does not exist" return 1; }
	[ -r /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" ] || { echo "$REMOTE_NAME is not readable"; return 1; }
	/bin/cp /tmp/"$STORE"/"$PARENT"/"$REMOTE_NAME" "$LOCAL_NAME"
	return $?
}

mtp_retrieve_from_store_vtbl=(mtp_retrieve_from_store_bash_dummy)

mtp_retrieve_from_store()
{
	${mtp_retrieve_from_store_vtbl[$HOST]} $@
}

mtp_remove_from_store_bash_dummy()
{
	local STORE="${1}"
	local PARENT="${2}"
	local NAME="${3}"
	mkdir -p /tmp/"$STORE"
	/bin/rm -rf /tmp/"$STORE"/"$PARENT"/"$NAME"
	return 0
}

mtp_remove_from_store_vtbl=(mtp_remove_from_store_bash_dummy)

mtp_remove_from_store()
{
	${mtp_remove_from_store_vtbl[$HOST]} $@
}

mtp_create_folder_bash_dummy()
{
	local STORE="${1#s:}"
	local PARENT="${2#p:}"
	local NAME="${3#n:}"
	/bin/mkdir -p /tmp/"$STORE"
	[ -d /tmp/"$STORE"/"$PARENT" ] || { echo "$PARENT does not exist"; return 1; }
	mkdir /tmp/"$STORE"/"$PARENT"/"$NAME"
}

mtp_create_folder_vtbl=(mtp_create_folder_bash_dummy)

mtp_create_folder()
{
	${mtp_create_folder_vtbl[0]} $@
}

mtp_remove_folder_bash_dummy()
{
	local STORE="${1#s:}"
	local PARENT="${2#p:}"
	local NAME="${3#n:}"
	mkdir -p /tmp/"$STORE"
	/bin/rm -rf /tmp/"$STORE"/"$PARENT"/"$NAME"
}

mtp_remove_folder_vtbl=(mtp_remove_folder_bash_dummy)

mtp_remove_folder()
{
	${mtp_remove_folder_vtbl[0]} $@
}
