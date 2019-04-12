#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

Param ( $Manufacturer = $(Throw "Manufacturer string required"),
	$Product = $(Throw "Product string required"),
	$Serial = $(Throw "Serial string required"),
	$DeviceClass = $(Throw "Class string required, 2 digits decimal with leading zero"),
	$DeviceSubclass = $(Throw "Subclass string required, decimal with leading zero"),
	$Protocol = $(Throw "Protocol string required, 2 digits decimal with leading zero")
)

(
	Get-PnpDevice |
		Where-Object { $_.Class -eq 'WPD' -and
				$_.Status -eq 'OK' -and
				$_.Manufacturer -eq $Manufacturer -and
				$_.Name -eq $Product -and
				$_.DeviceID -match $Serial -and
				$_.CompatibleID -match 'Class_' + $DeviceClass -and
				$_.CompatibleID -match 'SubClass_' + $DeviceSubclass -and
				$_.CompatibleID -match 'Prot_' + $Protocol
		} |
		Measure-Object -line
).Lines
