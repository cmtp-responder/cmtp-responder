#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

device_present_windows()
{
	return 0
}

device_is_mtp_device_windows()
{
	return 0
}

device_disconnect_windows()
{
	powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\device-disconnect.ps1" -WINDOWS_HCD \'$MTP_DEVICE_HCD_WINDOWS\'
	return 0
}

device_connect_windows()
{
	powershell.exe "c:\\Users\\Administrator\\cmtp-responder-tests\\device-connect.ps1" -WINDOWS_HCD \'$MTP_DEVICE_HCD_WINDOWS\'
	return 0
}
