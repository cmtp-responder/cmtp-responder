#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param ($WINDOWS_HCD = $(Throw "HCD VID/PID required in 'VIDabcd""&""PIDdef0' format"))

Get-PnpDevice |
	Where-Object { $_.Class -eq 'USB'} |
		Where-Object { $_.DeviceID -match  'ROOT_HUB' } |
			Where-Object { $_.HardwareID -match $WINDOWS_HCD} |
				ForEach-Object {
					$_ | Enable-PnpDevice -Confirm:$false
				}
