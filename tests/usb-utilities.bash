#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

. usb-utilities-linux.bash
. usb-utilities-windows.bash

device_present_vtbl=(device_present_linux device_present_windows)

device_present()
{
	${device_present_vtbl[$HOST]} $@
}

device_is_mtp_device_vtbl=(device_is_mtp_device_linux device_is_mtp_device_windows)

device_is_mtp_device()
{
	${device_is_mtp_device_vtbl[$HOST]} $@
}

device_disconnect_vtbl=(device_disconnect_linux device_disconnect_windows)

device_disconnect()
{
	${device_disconnect_vtbl[$HOST]} $@
}

device_connect_vtbl=(device_connect_linux device_connect_windows)

device_connect()
{
	${device_connect_vtbl[$HOST]} $@
}
