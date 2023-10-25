#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($store)

$windows_to_close = @('deepest child', 'folder 6', 'sub dir', 'folder 5', 'sub folder', 'folder 4', 'folder 3', 'folder 2', 'folder', $store)

$windows=(New-Object -comObject Shell.Application).Windows()

foreach ($title in $windows_to_close) {
	$windows | where-object {$_.LocationName -eq $title} | foreach-object {$_.quit()}
}
