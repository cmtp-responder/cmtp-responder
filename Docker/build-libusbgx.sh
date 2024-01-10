#!/bin/bash
#
# SPDX-License-Identifier: Apache-2.0
#
# (C) 2024 Collabora Ltd.
#
# Dedicated libusbgx build script for use inside a Docker container
#
# By default INSTALL_DIR is "install-${ARCH}/usr" inside libusbgx directory.
# It can be customized by passing INSTALL_DIR to this script, e.g.
#
# INSTALL_DIR=${PWD}/libusbgx-install ./build-libusbgx.sh

ARCH=$(whoami)

if [[ "${ARCH}" != "x86_64"  && "${ARCH}" != "armhf" && "${ARCH}" != "aarch64" ]]; then
	echo "This script only supports: x86_64, armhf, aarch64"
	echo "It is designed to be run from inside an appropriate Docker container."
	echo "The arch is chosen by the username, which is: ${ARCH}"
	exit 1
fi
echo "$0 invoked for ${ARCH}"

GIT_REPO=https://github.com/libusbgx/libusbgx.git
ARCH_OPTION=
PKG_CONFIG_PATH=

if [[ "${ARCH}" == "armhf" ]]; then
	ARCH_OPTION="--host=arm-linux-gnueabihf"
	PKG_CONFIG_PATH="/usr/lib/arm-linux-gnueabihf/pkgconfig"
elif [[ "${ARCH}" == "aarch64" ]]; then
	ARCH_OPTION="--host=aarch64-linux-gnu"
	PKG_CONFIG_PATH="/usr/lib/aarch64-linux-gnu/pkgconfig"
fi

if [[ ! -d libusbgx ]]; then
	git clone "${GIT_REPO}"
	cd libusbgx || exit 1
	autoreconf -i
else
	cd libusbgx || exit 1
fi

INSTALL_DIR=${INSTALL_DIR:-"${PWD}/install-${ARCH}/usr"}

if [[ -n "$PKG_CONFIG_PATH" ]]; then
	PKG_CONFIG_PATH="${PKG_CONFIG_PATH}" ./configure "${ARCH_OPTION}" --prefix="${INSTALL_DIR}"
else
	./configure "${ARCH_OPTION}" --prefix="${INSTALL_DIR}"
fi
make
make install
