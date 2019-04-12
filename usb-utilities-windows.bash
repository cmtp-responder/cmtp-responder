#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

device_present_windows()
{
	DEV_VID=`echo $MTP_DEVICE_VENDOR | sed 's/0x//g'`
	DEV_PID=`echo $MTP_DEVICE_PRODUCT | sed 's/0x//g'`
	RESULT=`powershell.exe "ps1\\device-present.ps1" -VendorId $DEV_VID -ProductId $DEV_PID | head -c 1`
	[ "$RESULT" == "1" ] || { echo "Cannot find MTP device"; return 1; }
	return 0
}

device_is_mtp_device_windows()
{
	RESULT=`powershell.exe "ps1\\device-is-mtp.ps1" -Manufacturer \'$MANUFACTURER\' -Product \'$PRODUCT_WINDOWS\' -Serial \'$SERIAL\' -DeviceClass \'$INTERFACE_CLASS_WINDOWS\' -DeviceSubclass \'$INTERFACE_SUBCLASS_WINDOWS\' -Protocol \'$INTERFACE_PROTOCOL_WINDOWS\' | head -c 1`
	[ "$RESULT" == "1" ] || { echo "Unexpected device"; return 1; }
	return 0
}

device_disconnect_windows()
{
	powershell.exe "ps1\\device-disconnect.ps1" -WINDOWS_HCD \'$MTP_DEVICE_HCD_WINDOWS\'
	return 0
}

device_connect_windows()
{
	powershell.exe "ps1\\device-connect.ps1" -WINDOWS_HCD \'$MTP_DEVICE_HCD_WINDOWS\'
	return 0
}
