TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
USR_CPPFLAGS += -D__X64
endif

USR_CPPFLAGS += -std=c++11

UVC_HOME = /epics/support/areaDetector-3-2/ADUVC
USR_INCLUDES += -I$(UVC_HOME)/include/libuvc

LIBRARY_IOC = adUVC
LIB_SRCS += ADUVC.cpp
LIB_LIBS += uvc
#SYS_PROD_LIBS += boost_system

DBD += adUVCSupport.dbd

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
