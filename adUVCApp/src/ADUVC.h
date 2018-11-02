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
#define ADUVC_VERSION      0
#define ADUVC_REVISION     1
#define ADUVC_MODIFICATION 0

// includes
#include <libuvc/libuvc.h>
#include "ADDriver.h"

//#include <Magick++.h>

//using namespace Magick;

// PV definitions
#define ADUVC_OperatingModeString           "UVC_OPERATINGMODE" //asynInt32
#define ADUVC_UVCComplianceLevelString      "UVC_COMPLIANCE"    //asynInt32
#define ADUVC_ReferenceCountString          "UVC_REFCOUNT"      //asynInt32
#define ADUVC_FramerateString               "UVC_FRAMERATE"     //asynInt32
#define ADUVC_SerialNumberString            "UVC_SERIAL"        //asynOctet
#define ADUVC_VendorIDString                "UVC_VENDOR"        //asynInt32
#define ADUVC_ProductIDString               "UVC_PRODUCT"       //asynInt32

/*
 * Class definition of the ADUVC driver. It inherits from the base ADDriver class
 *
 * Includes constructor/destructor, PV params, function defs and variable defs
 *
 */
class ADUVC : ADDriver{

    public:

        ADUVC(const char* portName, const char* serial, int vendorID, int productID, int framerate, int maxBuffers, size_t maxMemory, int priority, int stackSize);

        //TODO: add overrides of ADDriver functions
        virtual asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);

        void newFrameCallback(uvc_frame_t* frame, void* ptr);


        ~ADUVC();

    protected:
        int ADUVC_OperatingMode;
        #define ADUVC_FIRST_PARAM ADUVC_OperatingMode
        int ADUVC_UVCComplianceLevel;
        int ADUVC_ReferenceCount;
        int ADUVC_Framerate;
        int ADUVC_SerialNumber;
        int ADUVC_VendorID;
        int ADUVC_ProductID;
	#define ADUVC_LAST_PARAM ADUVC_ProductID

    private:

        // Some data variables
        //Image image;
        epicsEventId startEventId;
        epicsEventId endEventId;
        

        // variables
	// checks uvc device operations status
        uvc_error_t deviceStatus;
	//pointer to device
        uvc_device_t* pdevice;
	//pointer to device context. generated when connecting
        uvc_context_t* pdeviceContext;
	//pointer to device handle. used for controlling device. Each UVC device can allow for one handle at a time
        uvc_device_handle_t* pdeviceHandle;
	//pointer to device stream controller. used to controll streaming from device
        uvc_stream_ctrl_t deviceStreamCtrl;
	//pointer containing device info, such as vendor, product id
        uvc_device_descriptor_t* pdeviceInfo;

        // functions
	//function used to report errors in uvc operations
        void reportUVCError(uvc_error_t status, const char* functionName);
	//function used for connecting to a UVC device
        asynStatus connectToDeviceUVC(int connectionType, const char* serialNumber, int productID);
	//function that begins image aquisition
        uvc_error_t acquireStart();
	//function that stops aquisition
        void acquireStop();
	//function that converts a UVC frame into an NDArray
        asynStatus uvc2NDArray(uvc_frame_t* frame, NDArray* pArray, NDDataType_t dataType, NDColorMode_t colorMode, int imBytes);
	//function that gets information from a UVC device
        void getDeviceInformation();
	//function used to process a uvc frame
        static void newFrameCallbackWrapper(uvc_frame_t* frame, void* ptr);
	//function that decides how long to aquire images (to support the various modes)
        void imageHandlerThread();

        asynStatus createImageHandlerThread();
        void killImageHandlerThread();
};

#define NUM_UVC_PARAMS ((int)(&ADUVC_LAST_PARAM - &ADUVC_FIRST_PARAM + 1))

#endif
