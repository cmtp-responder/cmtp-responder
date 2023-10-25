#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param ($directory)

if (Test-Path $directory) {
	Remove-Item -Confirm:$false -Recurse $directory -Force | Out-Null
}
