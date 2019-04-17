#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($store)

$windows_to_close = @('child', 'folder6', 'subdir', 'folder5', 'subfolder', 'folder4', 'folder3', 'folder2', 'folder', $store)

$windows=(New-Object -comObject Shell.Application).Windows()

foreach ($title in $windows_to_close) {
	$windows | where-object {$_.LocationName -eq $title} | foreach-object {$_.quit()}
}
