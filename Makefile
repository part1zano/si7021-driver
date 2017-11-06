# change the following .PATH
.PATH:		/home/che/git/si7021-driver/
KMOD		= si7021
SRCS		= si7021.c si7021.h device_if.h bus_if.h iicbus_if.h

.include <bsd.kmod.mk>
