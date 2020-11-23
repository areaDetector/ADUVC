TOP=..
include $(TOP)/configure/CONFIG


SRC_DIRS += ../libuvc

INC += libuvc/libuvc.h
INC += libuvc/libuvc_config.h
INC += libuvc/libuvc_internal.h
INC += utlist.h


# should be able to compile libuvc on arm
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIBRARY = uvc
endif

# Should be able to compile libuvc on arm
ifeq (linux-arm, $(findstring linux-arm, $(T_A)))
LIBRARY = uvc
endif

# TODO: Try building libuvc on windows
#ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
#endif

uvc_SRCS += ctrl.c
uvc_SRCS += ctrl-gen.c
uvc_SRCS += device.c
uvc_SRCS += diag.c
uvc_SRCS += frame.c
uvc_SRCS += frame-mjpeg.c
uvc_SRCS += init.c
uvc_SRCS += misc.c
uvc_SRCS += stream.c

uvc_LIBS += jpeg
uvc_SYS_LIBS += usb-1.0

include $(TOP)/configure/RULES
