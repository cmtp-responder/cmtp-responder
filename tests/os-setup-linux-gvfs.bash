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
	LSUSB=`lsusb -d $MTP_DEVICE_VENDOR:$MTP_DEVICE_PRODUCT -v`
	MANUFACTURER=`echo "$LSUSB" | grep iManufacturer | sed "s/  */ /g" | cut -f4- -d" " | tr " " "_"`
	PRODUCT=`echo "$LSUSB" | grep iProduct | sed "s/  */ /g" | cut -f4- -d" " | tr " " "_"`
	SERIAL=`echo "$LSUSB" | grep iSerial | sed "s/  */ /g" | cut -f4- -d" " | tr " " "_"`

	__local_cookie+=($BUS)
	__local_cookie+=($DEV)
	__local_cookie+=(/run/user/$UID/gvfs/mtp:host=${MANUFACTURER}_${PRODUCT}_${SERIAL})
	__local_cookie+=(mtp://${MANUFACTURER}_${PRODUCT}_${SERIAL})
}

os_teardown_linux_gvfs()
{
	local -n __local_cookie=${1}
}
