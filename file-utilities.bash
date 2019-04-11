#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

random_file()
{
	local NAME="${1}"

	cat /dev/urandom | head -c$((($RANDOM % 128 + 1) * $((RANDOM + 1)))) > "$NAME"
}
