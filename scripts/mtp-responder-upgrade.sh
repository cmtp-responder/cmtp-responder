#!/bin/sh

#----------------------------------------------#
# mtp-responder patch for upgrade (2.4 -> 3.0) #
#----------------------------------------------#

# Macro
MTP_RESPONDER_CONF=/opt/var/lib/misc/mtp-responder.conf

chown owner:users $MTP_RESPONDER_CONF
chsmack -a "_" $MTP_RESPONDER_CONF
