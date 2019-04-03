#!/usr/bin/env bats
#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

setup()
{
	load mtp-device-id
	load config
	load usb-utilities
}

@test "Verify mtp device presence" {
	device_present
}

@test "Verify mtp device as mtp device" {
	device_present
	sleep 1
	run device_is_mtp_device
	[ $status -eq 0 ] || echo "Device not present"
	[ $status -eq 0 ]
}

@test "Disconnect and connect mtp device a number of times" {
	local count=0

	while [ $count -lt $DISCONNECT_CONNECT_COUNT ]; do
		device_disconnect
		sleep 1
		device_connect
		sleep 6
		run device_present
		[ $status -eq 0 ] || echo "Device not present"
		[ $status -eq 0 ]
		run device_is_mtp_device
		[ $status -eq 0 ] || echo "Not an mtp device"
		[ $status -eq 0 ]
		let count=count+1
	done
}
