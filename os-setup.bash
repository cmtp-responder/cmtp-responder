#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

. os-setup-bash-dummy.bash
. os-setup-linux-gvfs.bash

os_setup_vtbl=(os_setup_bash_dummy os_setup_linux_gvfs)

os_setup()
{
	${os_setup_vtbl[$MTP_VARIANT]} $@
}

os_teardown_vtbl=(os_teardown_bash_dummy os_teardown_linux_gvfs)

os_teardown()
{
	${os_teardown_vtbl[$MTP_VARIANT]} $@
}
