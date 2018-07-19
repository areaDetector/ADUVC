/*
 * Main source code for ADUVC EPICS driver.
 * 
 * This file contains the code for all functions, destrucot/constructor for ADUVC
 * 
 * Author: Jakub Wlodek
 * Created: July 2018
 * Copyright (c): 2018 Brookhaven National Laboratory
 * 
 */

// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// EPICS includes
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <iocsh.h>
#include <epicsExport.h>

// Area Detector include
#include "ADDriver.h"

// libuvc includes
#include <libuvc.h>
#include <libuvc_config.h>

static const char* driverName = "ADUVC";

static void exitCallbackC(void *drvPvt);


extern "C" int UVCConfig(const char* portName, int detIndex, int maxBuffers, size_t maxMemory, int priority, int stackSize){
    new ADUVC(portName, detIndex, maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}


ADUVC::ADUVC(const char* portName, int detIndex, int maxBuffers, size_t maxMemory, int priority, int stackSize){
    : ADDriver(portName, 1, (int)NUM_UVC_PARAMS, maxBuffers, maxMemory, asynEnumMask, asynEnumMask, ASYN_CANBLOCK, 1, priority, stackSize){
        int status = asynSuccess;
        static char* functionName = "ADUVC";
        
        createParam(ADUVC_UVCComplianceLevelString,     asynParamInt32,     ADUVC_UVCComplianceLevel);
        createParam(ADUVC_ReferenceCountString,         asynParamInt32,     ADUVC_ReferenceCount);
    }
}



