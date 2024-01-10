#!/bin/bash
#
# SPDX-License-Identifier: Apache-2.0
#
# (C) 2024 Collabora Ltd.

for i in *.tar.gz; do
	tar zxv -C / --keep-directory-symlink -f "$i"
done

systemctl enable --now usb-gadget.service
systemctl enable --now run-ffs_mtp.mount
systemctl enable --now ffs.socket
