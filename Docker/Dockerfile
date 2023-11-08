# base tools + libusbgx sources
FROM debian:trixie AS cmtp-responder-build-base
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV NSS_UNKNOWN_HOME=/home/user
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get -y upgrade && apt-get -y dist-upgrade && \
	apt-get -y install \
		cmake \
		git \
		autoconf \
		libtool \
		pkg-config \
		asciidoc-base

WORKDIR /opt/src
RUN git clone https://github.com/libusbgx/libusbgx.git && cd libusbgx && autoreconf -i
WORKDIR /
CMD ["/bin/bash" ]

# The following Dockerfile targets are for generating build systems
# targetting their respective architectures.
# In any case the build system runs inside a Docker container, but
# generates binaries for the respective architecture.

# x86_64
FROM cmtp-responder-build-base AS cmtp-responder-build-x86_64
RUN apt-get update && \
	apt-get -y upgrade && apt-get -y dist-upgrade && \
	apt-get -y install gcc \
		g++ \
		libconfig-dev \
		libglib2.0-dev \
		libsystemd-dev
WORKDIR /opt/src/libusbgx
RUN ./configure --prefix=/usr && \
	make && make install
WORKDIR /
RUN useradd -M x86_64
USER x86_64

# armhf
FROM cmtp-responder-build-base AS cmtp-responder-build-armhf
RUN dpkg --add-architecture armhf && apt-get update && \
	apt-get -y upgrade && apt-get -y dist-upgrade && \
	apt-get -y install gcc-arm-linux-gnueabihf \
		g++-arm-linux-gnueabihf \
		libconfig-dev:armhf \
		libglib2.0-dev:armhf \
		libsystemd-dev:armhf
WORKDIR /opt/src/libusbgx
RUN PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig ./configure --host=arm-linux-gnueabihf --prefix=/usr/arm-linux-gnueabihf && \
	make && make install
WORKDIR /opt/src
COPY armhf-toolchain.txt .
WORKDIR /
RUN useradd -M armhf
USER armhf

# aarch64
FROM cmtp-responder-build-base AS cmtp-responder-build-aarch64
RUN dpkg --add-architecture arm64 && apt-get update && \
	apt-get -y upgrade && apt-get -y dist-upgrade && \
	apt-get -y install gcc-aarch64-linux-gnu \
		g++-aarch64-linux-gnu \
		libconfig-dev:arm64 \
		libglib2.0-dev:arm64 \
		libsystemd-dev:arm64
WORKDIR /opt/src/libusbgx
RUN PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig ./configure --host=aarch64-linux-gnu --prefix=/usr/aarch64-linux-gnu && \
	make && make install
WORKDIR /opt/src
COPY aarch64-toolchain.txt .
WORKDIR /
RUN useradd -M aarch64
USER aarch64
