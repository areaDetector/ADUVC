/*
 * Header file for the ADUVC EPICS driver
 * 
 * This file contains the definitions of PV params, and the definition of the ADUVC class and functions.
 * 
 * Author: Jakub Wlodek
 * Created: July 2018
 * 
 * Copyright (c) : 2018 Brookhaven National Laboratory
 * 
 */

// header guard
#ifndef ADUVC_H
#define ADUVC_H

// version numbers
#define DRIVER_VERSION      0
#define DRIVER_REVISION     0
#define DRIVER_MODIFICATION 1

// includes
#include <libuvc.h>
#include "ADDriver.h"

// PV definitions
#define ADUVC_UVCComplianceLevelString      "UVC_COMPLIANCE"    //asynInt32
#define ADUVC_ReferenceCountString          "UVC_REFCOUNT"      //asynInt32
#define ADUVC_SerialNumberString            "UVC_SERIAL"        //asynInt32

/*
 * Class definition of the ADUVC driver. It inherits from the base ADDriver class
 * 
 * Includes constructor/destructor, PV params, function defs and variable defs
 *
 */
class ADUVC : ADDriver{

    public:

        ADUVC(const char* portName, int devIndex, int maxBuffers, size_t maxMemory, int priority, int stackSize);

        //TODO: add overrides of ADDriver functions

        ~ADUVC();

    protected:
        int ADUVC_UVCComplianceLevel;
        #define ADUVC_FIRST_PARAM ADUVC_UVCComplianceLevel
        int ADUVC_ReferenceCount;
        int ADUVC_SerialNumber;

    private:

        // variables
        uvc_error_t deviceStatus;
        uvc_device_t* pdevice;
        uvc_context_t* pdeviceContext;
        uvc_device_handle_t* pdeviceHandle;
        uvc_stream_ctrl_t deviceStreamCtrl;
        uvc_device_descriptor_t* pdeviceInfo;

        // functions
        void ADUVC::reportUVCError(uvc_error_t status, const char* functionName);
        bool ADUVC::connectToDeviceUVC();
        void ADUVC::getDeviceInformation();
};
#endif