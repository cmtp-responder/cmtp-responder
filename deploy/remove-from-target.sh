#!/bin/bash
#
# SPDX-License-Identifier: Apache-2.0
#
# (C) 2024 Collabora Ltd.

systemctl disable --now usb-gadget.service
systemctl disable --now run-ffs_mtp.mount
systemctl disable --now ffs.socket

rm -f \
/usr/bin/cmtp-responder \
/etc/cmtp-responder/cmtp-responder.conf \
/etc/cmtp-responder/descs \
/etc/cmtp-responder/strs \
/usr/bin/gt \
/etc/gt/gt.conf \
/usr/share/man/gt.1.gz \
/usr/share/bash-completion/completions/gt \
/usr/bin/show-udcs \
/usr/bin/gadget-export \
/usr/bin/gadget-hid \
/usr/bin/gadget-ffs \
/usr/bin/show-gadgets \
/usr/bin/gadget-printer \
/usr/bin/gadget-rndis-os-desc \
/usr/bin/gadget-import \
/usr/bin/gadget-ms \
/usr/bin/gadget-acm-ecm \
/usr/bin/gadget-uac2 \
/usr/bin/gadget-uvc \
/usr/bin/gadget-vid-pid-remove \
/usr/bin/gadget-midi \
/usr/lib/pkgconfig/libusbgx.pc \
/usr/lib/libusbgx.so.2.0.0 \
/usr/lib/cmake/LibUsbgx/LibUsbgxConfig.cmake \
/usr/lib/libusbgx.a \
/usr/lib/libusbgx.la \
/usr/include/usbg/function/net.h \
/usr/include/usbg/function/uac2.h \
/usr/include/usbg/function/printer.h \
/usr/include/usbg/function/ffs.h \
/usr/include/usbg/function/serial.h \
/usr/include/usbg/function/midi.h \
/usr/include/usbg/function/phonet.h \
/usr/include/usbg/function/hid.h \
/usr/include/usbg/function/loopback.h \
/usr/include/usbg/function/uvc.h \
/usr/include/usbg/function/ms.h \
/usr/include/usbg/usbg_version.h \
/usr/include/usbg/usbg.h

rm -rf /etc/cmtp-responder /etc/gt /lib/cmake/LibUsbgx /usr/include/usbg
