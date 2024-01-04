#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param ($directory)

New-Item -Confirm:$false -ItemType directory -Force $directory | Out-Null
