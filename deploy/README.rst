SPDX-License-Identifier: CC-BY-SA-4.0

Copyright 2024 Collabora Ltd

These are example packaging and deployment scripts. Distributions are encouraged to do their own packaging and deployment, including dependencies management.

Creating .tar.gz deployable packages:

`$ ARCH=aarch64 cmtp-responder/deploy/make-dist.sh`

`ARCH` is `x86_64` by default.

`make-dist.sh assumes that there exist following sudirectories of the current directory:

```
libusubgx/install-${ARCH}
gt/install-${ARCH}
cmtp-responder/install-${ARCH}
cmtp-responder/systemd
cmtp-responder/gt
```

and simply packages their contents into respective archives, placed in the current directory.

On the target system, to deploy, use:

`$ deploy-tar-gz-to-target.sh`

and appropriate privileges must be ensured by the caller.

Similarly, on the target systen to revert the installation use:

`$ remove-from-target.sh`
