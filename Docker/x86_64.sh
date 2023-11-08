#!/bin/bash

WORKDIR="${WORKDIR:-${HOME}}"

case "${1}" in
	run)
		docker run -it \
			--network=host \
			-v ${WORKDIR}:/home/build \
			-w /home/build \
			-u $(id -u):$(id -g) \
			--hostname cmtp-responder-build \
			cmtp-responder-build:x86_64
		;;
	build)
		docker buildx --target cmtp-responder-build-x86_64 \
			-t cmtp-responder-build:x86_64 \
			.
		;;
	*)
		echo "Usage: $0 {run|build}"
		exit 1
esac
