TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
USR_CPPFLAGS += -D__X64
endif

USR_CPPFLAGS += -std=c++11

# Define our IOC library as libADUVC
LIBRARY_IOC = ADUVC

# Define our source code file
LIB_SRCS += ADUVC.cpp

# Link against libuvc
LIB_LIBS += uvc

# Define DBD file for ioc shell command registration
DBD += uvcSupport.dbd

# Include top level Library Makefile
include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
