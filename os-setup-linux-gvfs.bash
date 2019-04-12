#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

os_setup_linux_gvfs()
{
	local -n __local_cookie=${1}

	LSUSB=`lsusb -d $MTP_DEVICE_VENDOR:$MTP_DEVICE_PRODUCT`
	BUS=`echo $LSUSB | cut -f2 -d" "`
	DEV=`echo $LSUSB | cut -f4 -d" " | cut -f1 -d:`

	__local_cookie+=($BUS)
	__local_cookie+=($DEV)
	__local_cookie+=(/run/user/$UID/gvfs/mtp:host=%5Busb%3A$BUS%2C$DEV%5D)
	__local_cookie+=(mtp://%5Busb%3A$BUS,$DEV%5D)
}

os_teardown_linux_gvfs()
{
	local -n __local_cookie=${1}
}
