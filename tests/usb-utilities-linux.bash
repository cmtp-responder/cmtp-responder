#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

device_present_linux()
{
	run lsusb -d $MTP_DEVICE_VENDOR:$MTP_DEVICE_PRODUCT
	[ "$status" -eq 0 ] && return 0

	echo "Cannot find MTP device!"
	return 1
}

lsusb_field_4()
{
	local PATTERN=${1}

	lsusb -d $MTP_DEVICE_VENDOR:$MTP_DEVICE_PRODUCT -v | grep "${PATTERN}" | tr -s " " | cut -f4- -d' '
}

device_is_mtp_device_linux()
{
	local LSUSB_MANUFACTURER=`lsusb_field_4 iManufacturer`
	[ "$LSUSB_MANUFACTURER" == "$MANUFACTURER" ] || { echo "Unexpected manufacturer: $LSUSB_MANUFACTURER"; return 1; }

	local LSUSB_PRODUCT=`lsusb_field_4 iProduct`
	[ "$LSUSB_PRODUCT" == "$PRODUCT" ] || { echo "Unexpected product: $LSUSB_PRODUCT"; return 1; }

	local LSUSB_SERIAL=`lsusb_field_4 iSerial`
	[ "$LSUSB_SERIAL" == "$SERIAL" ] || { echo "Unexpected serial number: $LSUSB_SERIAL"; return 1; }

	local LSUSB_NUM_INTERFACES=`lsusb -d $MTP_DEVICE_VENDOR:$MTP_DEVICE_PRODUCT -v | grep iInterface | wc -l`
	[ $LSUSB_NUM_INTERFACES -eq 1 ] || { echo "Unexpected number of interfaces: $LSUSB_NUM_INTERFACES"; return 1; }

	local LSUSB_INTERFACE=`lsusb_field_4 iInterface`
	[ "$LSUSB_INTERFACE" == "$INTERFACE" ] || { echo "Unexpected interface: $LSUSB_INTERFACE"; return 1; }

	local LSUSB_INTERFACE_CLASS=`lsusb_field_4 bInterfaceClass`
	[ "$LSUSB_INTERFACE_CLASS" == "$INTERFACE_CLASS" ] || { echo "Unexpected interface class: $LSUSB_INTERFACE_CLASS"; return 1; }

	local LSUSB_INTERFACE_SUBCLASS=`lsusb_field_4 bInterfaceSubClass`
	[ "$LSUSB_INTERFACE_SUBCLASS" == "$INTERFACE_SUBCLASS" ] || { echo "Unexpected interface subclass: $LSUSB_INTERFACE_SUBCLASS"; return 1; }

	local LSUSB_INTERFACE_PROTOCOL=`lsusb_field_4 bInterfaceProtocol`
	[ "$LSUSB_INTERFACE_PROTOCOL" == "$INTERFACE_PROTOCOL" ] || { echo "Unexpected protocol: $LSUSB_INTERFACE_PROTOCOL"; return 1; }

	lsusb -d $MTP_DEVICE_VENDOR:$MTP_DEVICE_PRODUCT -v | grep -A2 bEndpointAddress | awk \
		'BEGIN{bulk_in=0;bulk_out=0;int_in=0;int_out=0;current_type=-1} \
		/IN/{current_type=8} \
		/OUT/{current_type=0} \
		/Bulk/{if (current_type == 0) ++bulk_out; if (current_type == 8) ++bulk_in; current_type=-1}\
		/Interrupt/{if (current_type == 0) ++int_out; if (current_type == 8) ++int_in; current_type=-1}\
		END{ \
			print "Bulk IN:" bulk_in "[expected 1] Int IN:" int_in "[expected 1] Bulk OUT:" bulk_out "[expected 1] Int OUT:" int_out "[expected 0]"; \
			if (bulk_in!=1 || int_in!=1 || bulk_out !=1 || int_out != 0) \
				exit 1\
		}'
}

device_disconnect_linux()
{
	echo -n $MTP_DEVICE_HCD | sudo tee $MTP_DEVICE_HCD_DRIVER/unbind > /dev/null
	return 0
}

device_connect_linux()
{
	echo -n $MTP_DEVICE_HCD | sudo tee $MTP_DEVICE_HCD_DRIVER/bind > /dev/null
	return 0
}
