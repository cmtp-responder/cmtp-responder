#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($store)

$windows=(New-Object -comObject Shell.Application).Windows()

$windows | where-object {$_.LocationName -eq 'subfolder'} | foreach-object {$_.quit()}
$windows | where-object {$_.LocationName -eq 'folder'} | foreach-object {$_.quit()}
$windows | where-object {$_.LocationName -eq $store} | foreach-object {$_.quit()}
