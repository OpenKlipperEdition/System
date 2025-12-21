################################################################################
#
# canutils
#
################################################################################

CANUTILS_VERSION = 4.0.6
CANUTILS_SITE = https://public.pengutronix.de/software/socket-can/canutils/v4.0
CANUTILS_SOURCE = canutils-$(CANUTILS_VERSION).tar.bz2
CANUTILS_DEPENDENCIES = libsocketcan
CANUTILS_AUTORECONF = YES
$(eval $(autotools-package))
