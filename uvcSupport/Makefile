TOP=..
include $(TOP)/configure/CONFIG


SRC_DIRS += ../libuvc

INC += libuvc/libuvc.h
INC += libuvc/libuvc_config.h
INC += libuvc/libuvc_internal.h
INC += utlist.h

#DIRS := $(DIRS) uvcSupport/libuvc
#uvcSupport_DEPEND_DIRS += uvcSupport/libuvc


# should be able to compile libuvc on arm
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../libuvc/O.$(EPICS_HOST_ARCH)/libuvc.a
LIB_INSTALLS_Linux += ../libuvc/O.$(EPICS_HOST_ARCH)/libuvc.so
endif

# Should be able to compile libuvc on arm
ifeq (linux-arm, $(findstring linux-arm, $(T_A)))
LIB_INSTALLS_Linux += ../libuvc/O.$(EPICS_HOST_ARCH)/libuvc.a
LIB_INSTALLS_Linux += ../libuvc/O.$(EPICS_HOST_ARCH)/libuvc.so
endif

# TODO: Try building libuvc on windows
#ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
#endif

include $(TOP)/configure/RULES
