TOP=..
include $(TOP)/configure/CONFIG

SRC_DIRS += ../include

INC += libuvc/libuvc.h
INC += libuvc/libuvc_config.h
INC += libuvc/libuvc_internal.h
INC += utlist.h

# should be able to compile libuvc on arm
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/$(EPICS_HOST_ARCH)/libuvc.a
LIB_INSTALLS_Linux += ../os/$(EPICS_HOST_ARCH)/libuvc.so
LIB_INSTALLS_Linux += ../os/$(EPICS_HOST_ARCH)/libuvc.so.0
LIB_INSTALLS_Linux += ../os/$(EPICS_HOST_ARCH)/libuvc.so.0.0.6
endif

# Should be able to compile libuvc on arm
ifeq (linux-arm, $(findstring linux-arm, $(T_A)))
LIB_INSTALLS_Linux += ../os/$(EPICS_HOST_ARCH)/libuvc.a
LIB_INSTALLS_Linux += ../os/$(EPICS_HOST_ARCH)/libuvc.so
LIB_INSTALLS_Linux += ../os/$(EPICS_HOST_ARCH)/libuvc.so.0
LIB_INSTALLS_Linux += ../os/$(EPICS_HOST_ARCH)/libuvc.so.0.0.6
endif

include $(TOP)/configure/RULES
