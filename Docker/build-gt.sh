#!/bin/bash
#
# SPDX-License-Identifier: Apache-2.0
#
# (C) 2024 Collabora Ltd.
#
# Dedicated gt build script for use inside a Docker container
#
# By default INSTALL_DIR is "install-${ARCH}" inside gt directory.
# It can be customized by passing INSTALL_DIR to this script, e.g.
#
# INSTALL_DIR=${PWD}/gt-install ./build-gt.sh
#
# By default BUILD_DIR is "build-${ARCH}" inside gt directory.
# It can be customized by passing BUILD_DIR to this cript, e.g.
#
# BUILD_DIR=${PWD}/gt-build ./build-gt.sh
#
# The two options can be used simultaneously.

ARCH=$(whoami)

if [[ "${ARCH}" != "x86_64"  && "${ARCH}" != "armhf" && "${ARCH}" != "aarch64" ]]; then
	echo "This script only supports: x86_64, armhf, aarch64"
	echo "It is designed to be run from inside an appropriate Docker container."
	echo "The arch is chosen by the username, which is: ${ARCH}"
	exit 1
fi
echo "$0 invoked for ${ARCH}"

GIT_REPO=https://github.com/kopasiak/gt.git
TOOLCHAIN_FILE=
if [[ "${ARCH}" != "x86_64" ]]; then
	TOOLCHAIN_FILE="/opt/src/${ARCH}-toolchain.txt"
fi

if [[ ! -d gt ]]; then
	git clone "${GIT_REPO}"
fi
cd gt || exit 1

GT_DIR="${PWD}"

BUILD_DIR=${BUILD_DIR:-"${GT_DIR}/build-${ARCH}"}
INSTALL_DIR=${INSTALL_DIR:-"${GT_DIR}/install-${ARCH}"}

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}" || exit 1

if [[ -n "${TOOLCHAIN_FILE}" ]]; then
	cmake -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" -DCMAKE_INSTALL_PREFIX=/usr "${GT_DIR}/source"
else
	cmake -DCMAKE_INSTALL_PREFIX=/usr "${GT_DIR}/source"
fi
make
make DESTDIR="${INSTALL_DIR}" install
