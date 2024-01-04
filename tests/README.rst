SPDX-License-Identifier: CC-BY-SA-4.0

Copyright Collabora Ltd

Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>

Date: 25th October 2023

cmtp-responder tests
====================

The purpose of the tests is to detect regressions.

The tests are written in `bats <https://github.com/bats-core>`_

They are built according to a layered architecture: the ["driver"] test cases are
architecture-agnostic and they run using os-setup, usb-utilities and mtp-utilities.
The os-setup, usb-utilities and mtp-utilities are wrappers around specific variant,
selected in config.bash.

As of this writing there exist 3 such backends:

- bash dummy (for debugging purposes, no actual interaction with any MTP device)
- Linux gvfs (for running on top of gvfs on Linux)
- Windows Power Shell (for running in WSL2 on Windows)

os_setup can create an OS-specific "cookie", which is then passed unchanged to the
toplevel "driver" bats tests, and then to the actually running backend. As of this
writing the Linux variant stores USB bus id and device number, the expected mountpoint
of a gvfs directory corresponding to the MTP device contents and a gio handle of the
MTP device under test. The Windows Power Shell variant stores locations of helper
directories, "product" string as seen by the Windows OS and MTP store name.

The tests are run like this:

bats 01_enum.bats

bats 02_file_operations.bats
