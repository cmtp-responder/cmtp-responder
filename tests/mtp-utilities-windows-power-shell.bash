#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

mtp_prepare_tests_windows_power_shell()
{
	local -n __local_cookie=${1}
	local STORE="${2}"

	powershell.exe -ExecutionPolicy Bypass "ps1\\rmdir.ps1" -dir \'${__local_cookie[0]}\'
	[ $? -eq 0 ] || { echo "Can't remove ${__local_cookie[0]}"; return 1; }

	powershell.exe -ExecutionPolicy Bypass "ps1\\mkdir.ps1" -dir \'${__local_cookie[0]}\'
	[ $? -eq 0 ] || { echo "Can't create ${__local_cookie[0]}"; return 1; }

	powershell.exe -ExecutionPolicy Bypass "ps1\\rmdir.ps1" -dir \'${__local_cookie[1]}\'
	[ $? -eq 0 ] || { echo "Can't remove ${__local_cookie[1]}"; return 1; }

	powershell.exe -ExecutionPolicy Bypass "ps1\\mkdir.ps1" -dir \'${__local_cookie[1]}\'
	[ $? -eq 0 ] || { echo "Can't create ${__local_cookie[1]}"; return 1; }

	powershell.exe -ExecutionPolicy Bypass "ps1\\open-store-window.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\'
	[ $? -eq 0 ] || { echo "Can't open $STORE window"; return 1; }

	powershell.exe -ExecutionPolicy Bypass "ps1\\empty-store.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -removalsDir \'${__local_cookie[1]}\'
	[ $? -eq 0 ] || { echo "Can't empty the store $STORE"; return 1; }

	return 0
}

mtp_list_files_windows_power_shell()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"

	if [ -z $PARENT ]; then
		powershell.exe -ExecutionPolicy Bypass "ps1\\find-files.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'""\' | tr -d '\015'
		[ $? -eq 0 ] || { echo "Can't list files in $STORE/$PARENT"; return 1; }
	else
		powershell.exe -ExecutionPolicy Bypass "ps1\\find-files.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'$PARENT\' | tr -d '\015'
		[ $? -eq 0 ] || { echo "Can't list files in $STORE/$PARENT"; return 1; }
	fi

	return 0
}

mtp_list_folders_windows_power_shell()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"

	if [ -z $PARENT ]; then
		powershell.exe -ExecutionPolicy Bypass "ps1\\find-folders.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'""\' | tr -d '\015'
		[ $? -eq 0 ] || { echo "Can't list folders in $STORE/$PARENT"; return 1; }
	else
		powershell.exe -ExecutionPolicy Bypass "ps1\\find-folders.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'$PARENT\' | tr -d '\015'
		[ $? -eq 0 ] || { echo "Can't list folders in $STORE/$PARENT"; return 1; }
	fi

	return 0
}

mtp_copy_to_store_windows_power_shell()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local NAME="${4}"

	if [ -z $PARENT ]; then
		powershell.exe -ExecutionPolicy Bypass "ps1\\copy-to-store.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'""\' -fileName \'$NAME\'
		[ $? -eq 0 ] || { echo "Can't create $STORE/$PARENT/$NAME"; return 1; }
	else
		powershell.exe -ExecutionPolicy Bypass "ps1\\copy-to-store.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'$PARENT\' -fileName \'$NAME\'
		[ $? -eq 0 ] || { echo "Can't create $STORE/$PARENT/$NAME"; return 1; }
	fi

	return 0
}

mtp_retrieve_from_store_windows_power_shell()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local REMOTE_NAME="${4}"
	local LOCAL_NAME="${5}"
	local NAME="${4}"

	if [ -z $PARENT ]; then
		powershell.exe -ExecutionPolicy Bypass "ps1\\copy-from-store.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'""\' -remoteFileName \'$REMOTE_NAME\' -localFileName \'$LOCAL_NAME\' -tmpDir \'${__local_cookie[0]}\'
		[ $? -eq 0 ] || { echo "Can't retrieve $STORE/$PARENT/$REMOTE_NAME"; return 1; }
	else
		powershell.exe -ExecutionPolicy Bypass "ps1\\copy-from-store.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'$PARENT\' -remoteFileName \'$REMOTE_NAME\' -localFileName \'$LOCAL_NAME\' -tmpDir \'${__local_cookie[0]}\'
		[ $? -eq 0 ] || { echo "Can't retrieve $STORE/$PARENT/$REMOTE_NAME"; return 1; }
	fi

	return 0
}

mtp_remove_from_store_windows_power_shell()
{
	local -n __local_cookie=${1}
	local STORE="${2}"
	local PARENT="${3}"
	local NAME="${4}"

	if [ -z $PARENT ]; then
		powershell.exe -ExecutionPolicy Bypass "ps1\\remove-from-store.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'""\' -remoteFileName \'$NAME\' -removalsDir \'${__local_cookie[1]}\'
		[ $? -eq 0 ] || { echo "Can't remove $STORE/$PARENT/$NAME"; return 1; }
	else
		powershell.exe -ExecutionPolicy Bypass "ps1\\remove-from-store.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'$PARENT\' -remoteFileName \'$NAME\' -removalsDir \'${__local_cookie[1]}\'
		[ $? -eq 0 ] || { echo "Can't remove $STORE/$PARENT/$NAME"; return 1; }
	fi

	return 0
}

mtp_create_folder_windows_power_shell()
{
	local -n __local_cookie=${1}
	local STORE="${2#s:}"
	local PARENT="${3#p:}"
	local NAME="${4#n:}"

	if [ -z $PARENT ]; then
		powershell.exe -ExecutionPolicy Bypass "ps1\\create-folder.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'""\' -folderName \'$NAME\'
		[ $? -eq 0 ] || { echo "Can't create folder $STORE/$PARENT/$NAME"; return 1; }
	else
		powershell.exe -ExecutionPolicy Bypass "ps1\\create-folder.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'$PARENT\' -folderName \'$NAME\'
		[ $? -eq 0 ] || { echo "Can't create folder $STORE/$PARENT/$NAME"; return 1; }
	fi

	return 0
}

mtp_remove_folder_windows_power_shell()
{
	local -n __local_cookie=${1}
	local STORE="${2#s:}"
	local PARENT="${3#p:}"
	local NAME="${4#n:}"

	if [ -z $PARENT ]; then
		powershell.exe -ExecutionPolicy Bypass "ps1\\remove-folder.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'""\' -folderName \'$NAME\' -removalsDir \'${__local_cookie[1]}\'
		[ $? -eq 0 ] || { echo "Can't remove folder $STORE/$PARENT/$NAME"; return 1; }
	else
		powershell.exe -ExecutionPolicy Bypass "ps1\\remove-folder.ps1" -mtpDeviceName \'${__local_cookie[2]}\' -storeName \'$STORE\' -parentName \'$PARENT\' -folderName \'$NAME\' -removalsDir \'${__local_cookie[1]}\'
		[ $? -eq 0 ] || { echo "Can't remove folder $STORE/$PARENT/$NAME"; return 1; }
	fi

	return 0
}

mtp_finish_tests_windows_power_shell()
{
	local -n __local_cookie=${1}

	powershell.exe -ExecutionPolicy Bypass "ps1\\rmdir.ps1" -dir \'${__local_cookie[0]}\'
	[ $? -eq 0 ] || { echo "Can't remove ${__local_cookie[0]}"; return 1; }

	powershell.exe -ExecutionPolicy Bypass "ps1\\rmdir.ps1" -dir \'${__local_cookie[1]}\'
	[ $? -eq 0 ] || { echo "Can't remove ${__local_cookie[1]}"; return 1; }

	powershell.exe -ExecutionPolicy Bypass "ps1\\close-windows.ps1" -store \'${__local_cookie[3]}\'
	[ $? -eq 0 ] || { echo "Can't close device folder windows"; return 1; }

	return 0
}
