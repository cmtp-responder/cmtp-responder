SPDX-License-Identifier: CC-BY-SA-4.0

Copyright 2019 Collabora Ltd

Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>

Date: 4th March 2019

cmtp-responder
==============

What cmtp-responder is?
-----------------------

cmtp-responder forks from the Tizen's mtp-responder and belongs to a general
class of software known as "mtp responders". Any mtp responder's task is to
provide device side implementation of the MTP protocol and cmtp-responder is
no different.

The rationale for a fork
------------------------

While we appreciate the work Tizen has put into creating mtp-responder, we
must acknowledge that the original Tizen's code is hard to reuse, because
it depends on a lot of Tizen-specific libraries. We forked it to make
cmtp-responder depend only on generic libraries. This of course implies
removing all the Tizen-specific features.

The purpose of cmtp-responder
-----------------------------

The purpose of cmtp-responder is to provide a reusable minimal MTP
implementation for everyone to reuse. The long-term goal is to make this
project the implementation of choice for a variety of projects, including
commercial products, and make this project as feature-complete as possible
while maintaining generality and minimal dependencies.

commit guidelines
=================

These are experimental, and might turn out not-matching real life!

Commit messages shall start with a capital letter, be short and to the point,
and shall not end with a fullstop.

Commit messages shall be prefixed with a component name and a colon, e.g.:

transport: The remainder of the commit message ....

A component consists of both an src directory and its counterpart include
subdirectory, e.g.:

include/transport/*

and

src/transport/*

together constitute the "transport" component.

If a change spans across multiple components, think again whether it has to
and restructure your commits if possible.

If a change must span across multiple components, then start your message
with "major rework:".

If a change refers to the project build files (e.g. CMakeLists.txt), then
use the name "build:".

Any new functionality needs its accompanying test(s) which must be added
together with it and must pass.
