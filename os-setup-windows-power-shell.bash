#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

os_setup_windows_power_shell()
{
	local -n __local_cookie=${1}

	TMP_DIR=`powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\get-windows-tmp.ps1" | tr -d '\015'`

	__local_cookie+=("$TMP_DIR""\\cmtp-responder-tests\\")
	__local_cookie+=("$TMP_DIR""\\cmtp-responder-tests-removals\\")
	__local_cookie+=("$PRODUCT_WINDOWS")
	powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\rmdir.ps1" -dir \'${__local_cookie[0]}\'
	[ $? -eq 0 ] || return 1
	powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\mkdir.ps1" -dir \'${__local_cookie[0]}\'
	[ $? -eq 0 ] || return 1
	powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\rmdir.ps1" -dir \'${__local_cookie[1]}\'
	[ $? -eq 0 ] || return 1
	powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\mkdir.ps1" -dir \'${__local_cookie[1]}\'
	[ $? -eq 0 ] || return 1
	return 0
}

os_teardown_windows_power_shell()
{
	local -n __local_cookie=${1}
	powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\rmdir.ps1" -dir \'${__local_cookie[0]}\'
	[ $? -eq 0 ] || return 1
	powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\rmdir.ps1" -dir \'${__local_cookie[1]}\'
	[ $? -eq 0 ] || return 1
	return 0
}
