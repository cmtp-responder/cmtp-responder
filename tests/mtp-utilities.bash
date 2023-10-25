#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

. mtp-utilities-bash-dummy.bash
. mtp-utilities-linux-gvfs.bash
. mtp-utilities-windows-power-shell.bash

mtp_prepare_tests_vtbl=(mtp_prepare_tests_bash_dummy mtp_prepare_tests_linux_gvfs mtp_prepare_tests_windows_power_shell)

mtp_prepare_tests()
{
	${mtp_prepare_tests_vtbl[$MTP_VARIANT]} "$@"
}

mtp_list_files_vtbl=(mtp_list_files_bash_dummy mtp_list_files_linux_gvfs mtp_list_files_windows_power_shell)

mtp_list_files()
{
	${mtp_list_files_vtbl[$MTP_VARIANT]} "$@"
}

mtp_list_folders_vtbl=(mtp_list_folders_bash_dummy mtp_list_folders_linux_gvfs mtp_list_folders_windows_power_shell)

mtp_list_folders()
{
	${mtp_list_folders_vtbl[$MTP_VARIANT]} "$@"
}

mtp_copy_to_store_vtbl=(mtp_copy_to_store_bash_dummy mtp_copy_to_store_linux_gvfs mtp_copy_to_store_windows_power_shell)

mtp_copy_to_store()
{
	${mtp_copy_to_store_vtbl[$MTP_VARIANT]} "$@"
}

mtp_retrieve_from_store_vtbl=(mtp_retrieve_from_store_bash_dummy mtp_retrieve_from_store_linux_gvfs mtp_retrieve_from_store_windows_power_shell)

mtp_retrieve_from_store()
{
	${mtp_retrieve_from_store_vtbl[$MTP_VARIANT]} "$@"
}

mtp_remove_from_store_vtbl=(mtp_remove_from_store_bash_dummy mtp_remove_from_store_linux_gvfs mtp_remove_from_store_windows_power_shell)

mtp_remove_from_store()
{
	${mtp_remove_from_store_vtbl[$MTP_VARIANT]} "$@"
}

mtp_create_folder_vtbl=(mtp_create_folder_bash_dummy mtp_create_folder_linux_gvfs mtp_create_folder_windows_power_shell)

mtp_create_folder()
{
	${mtp_create_folder_vtbl[$MTP_VARIANT]} "$@"
}

mtp_remove_folder_vtbl=(mtp_remove_folder_bash_dummy mtp_remove_folder_linux_gvfs mtp_remove_folder_windows_power_shell)

mtp_remove_folder()
{
	${mtp_remove_folder_vtbl[$MTP_VARIANT]} "$@"
}

mtp_finish_tests_vtbl=(mtp_finish_tests_bash_dummy mtp_finish_tests_linux_gvfs mtp_finish_tests_windows_power_shell)

mtp_finish_tests()
{
	${mtp_finish_tests_vtbl[$MTP_VARIANT]} "$@"
}
