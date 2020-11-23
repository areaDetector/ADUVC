TOP=../..
include $(TOP)/configure/CONFIG



LIBRARY = uvc
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
