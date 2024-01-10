SPDX-License-Identifier: CC-BY-SA-4.0

Copyright 2024 Collabora Ltd

Example recipe and scripts to build a qemu image with debos and run the image. The image contains minimum software required to run cmtp-responder (on an x86\_64 platform).

login: cmtp-responder
password: cmtp-responder

In order to successfuly deploy cmtp-responder, modprobe libcomposite and modprobe dummy_hcd. This way your emulated machine acts both as a USB device running cmtp-responder and a USB host. Useful if you have no access to appropriate hardware.
