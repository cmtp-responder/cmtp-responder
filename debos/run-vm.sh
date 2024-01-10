#!/bin/bash
#
# SPDX-License-Identifier: Apache-2.0
#
# (C) 2024 Collabora Ltd.

MEMORY_MB="${MEMORY_MB:-2048}"
BIOS=/usr/share/qemu/OVMF.fd

kvm -m "${MEMORY_MB}" -hda x86_64-uefi-disk.qcow2 -bios "${BIOS}" &
