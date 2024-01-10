#!/bin/bash
#
# SPDX-License-Identifier: Apache-2.0
#
# (C) 2024 Collabora Ltd.

docker run --rm --interactive --tty --device /dev/kvm --workdir /recipes --mount "type=bind,source=$(pwd),destination=/recipes" --privileged  godebos/debos x86_64-uefi-disk.yaml
qemu-img convert -f raw -O qcow2 x86_64-uefi-disk.img  x86_64-uefi-disk.qcow2
rm -f x86_64-uefi-disk.img
