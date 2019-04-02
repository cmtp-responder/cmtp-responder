#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

. mtp-utilities-bash-dummy.bash
. mtp-utilities-linux-gvfs.bash

mtp_prepare_tests_vtbl=(mtp_prepare_tests_bash_dummy mtp_prepare_tests_linux_gvfs)

mtp_prepare_tests()
{
	${mtp_prepare_tests_vtbl[$HOST]} $@
}

mtp_list_files_vtbl=(mtp_list_files_bash_dummy mtp_list_files_linux_gvfs)

mtp_list_files()
{
	${mtp_list_files_vtbl[$HOST]} $@
}

mtp_list_folders_vtbl=(mtp_list_folders_bash_dummy mtp_list_folders_linux_gvfs)

mtp_list_folders()
{
	${mtp_list_folders_vtbl[$HOST]} $@
}

mtp_copy_to_store_vtbl=(mtp_copy_to_store_bash_dummy mtp_copy_to_store_linux_gvfs)

mtp_copy_to_store()
{
	${mtp_copy_to_store_vtbl[$HOST]} $@
}

mtp_retrieve_from_store_vtbl=(mtp_retrieve_from_store_bash_dummy mtp_retrieve_from_store_linux_gvfs)

mtp_retrieve_from_store()
{
	${mtp_retrieve_from_store_vtbl[$HOST]} $@
}

mtp_remove_from_store_vtbl=(mtp_remove_from_store_bash_dummy mtp_remove_from_store_linux_gvfs)

mtp_remove_from_store()
{
	${mtp_remove_from_store_vtbl[$HOST]} $@
}

mtp_create_folder_vtbl=(mtp_create_folder_bash_dummy mtp_create_folder_linux_gvfs)

mtp_create_folder()
{
	${mtp_create_folder_vtbl[0]} $@
}

mtp_remove_folder_vtbl=(mtp_remove_folder_bash_dummy mtp_remove_folder_linux_gvfs)

mtp_remove_folder()
{
	${mtp_remove_folder_vtbl[0]} $@
}
