#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

Param ( $VendorId = $(Throw "Vendor ID required, hex no 0x prefix"),
	$ProductId = $(Throw "Product ID required, hex no 0x prefix")
)

(
	Get-PnpDevice |
		Where-Object { $_.Class -eq 'WPD' -and
				$_.Status -eq 'OK' -and
				$_.DeviceID -match $VendorId -and
				$_.DeviceID -match $ProductId
		} |
		Measure-Object -line
).Lines
