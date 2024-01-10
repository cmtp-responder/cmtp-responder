#!/bin/bash
#
# SPDX-License-Identifier: Apache-2.0
#
# (C) 2024 Collabora Ltd.

ARCH="${ARCH:-x86_64}"

rm -f "libusbgx-${ARCH}.tar.gz"
rm -f "gt-${ARCH}.tar.gz"
rm -f "cmtp-responder-${ARCH}.tar.gz"
rm -f "etc-${ARCH}.tar.gz"

pushd . >/dev/null
echo "Packaging libusbgx..."
cd "libusbgx/install-${ARCH}" && tar zcvf "../../libusbgx-${ARCH}.tar.gz" *
popd > /dev/null || exit 1
ls -l "./libusbgx-${ARCH}.tar.gz"

pushd . >/dev/null
echo "Packaging gt..."
cd "gt/install-${ARCH}" && tar zcvf "../../gt-${ARCH}.tar.gz" *
popd > /dev/null || exit 1
ls -l "./gt-${ARCH}.tar.gz"

pushd . >/dev/null
echo "Packaging cmtp-responder..."
cd "cmtp-responder/install-${ARCH}" && tar zcvf "../../cmtp-responder-${ARCH}.tar.gz" *
popd > /dev/null || exit 1
ls -l "./cmtp-responder-${ARCH}.tar.gz"

echo "Packaging systemd units and gadget scheme..."
rm -rf etc
mkdir -p etc/systemd/system
cp cmtp-responder/systemd/* etc/systemd/system
mkdir -p etc/gt/templates
cp cmtp-responder/gt/* etc/gt/templates
tar zcvf "etc-${ARCH}.tar.gz" etc
ls -l "./etc-${ARCH}.tar.gz"
