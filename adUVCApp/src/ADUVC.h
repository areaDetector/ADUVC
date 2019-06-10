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
#define ADUVC_VERSION      1
#define ADUVC_REVISION     2
#define ADUVC_MODIFICATION 0


#define SUPPORTED_FORMAT_COUNT 10


// includes
#include <libuvc/libuvc.h>
#include "ADDriver.h"


// PV definitions

#define ADUVC_UVCComplianceLevelString          "UVC_COMPLIANCE"        //asynInt32
#define ADUVC_ReferenceCountString              "UVC_REFCOUNT"          //asynInt32
#define ADUVC_FramerateString                   "UVC_FRAMERATE"         //asynInt32
#define ADUVC_VendorIDString                    "UVC_VENDOR"            //asynInt32
#define ADUVC_ProductIDString                   "UVC_PRODUCT"           //asynInt32
#define ADUVC_ImageFormatString                 "UVC_FORMAT"            //asynInt32
#define ADUVC_GammaString                       "UVC_GAMMA"             //asynInt32
#define ADUVC_BacklightCompensationString       "UVC_BACKLIGHT"         //asynInt32
#define ADUVC_BrightnessString                  "UVC_BRIGHTNESS"        //asynInt32
#define ADUVC_ContrastString                    "UVC_CONTRAST"          //asynInt32
#define ADUVC_PowerLineString                   "UVC_POWER"             //asynInt32
#define ADUVC_HueString                         "UVC_HUE"               //asynInt32
#define ADUVC_SaturationString                  "UVC_SATURATION"        //asynInt32
#define ADUVC_SharpnessString                   "UVC_SHARPNESS"         //asynInt32    


/* enum for getting format from PV */
typedef enum {
    ADUVC_FrameMJPEG            = 0,
    ADUVC_FrameRGB              = 1,
    ADUVC_FrameYUYV             = 2,
    ADUVC_FrameGray8            = 3,
    ADUVC_FrameGray16           = 4,
    ADUVC_FrameUYVY             = 5,
    ADUVC_FrameUncompressed     = 6,
} ADUVC_FrameFormat_t;


/* Struct for individual supported camera format */
typedef struct {
    char* formatName;
    size_t xSize;
    size_t ySize;
    int framerate;
    ADUVC_FrameFormat_t frameFormat;
    NDColorMode_t colorMode;
    NDDataType_t dataType;
} ADUVC_CameraFormat_t;



/*
 * Class definition of the ADUVC driver. It inherits from the base ADDriver class
 *
 * Includes constructor/destructor, PV params, function defs and variable defs
 *
 */
class ADUVC : ADDriver{

    public:

        // Constructor
        ADUVC(const char* portName, const char* serial, int productID, int framerate, int xsize, int ysize, int maxBuffers, size_t maxMemory, int priority, int stackSize);

        //TODO: add overrides of ADDriver functions

        // ADDriver overrides
        virtual asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);
        virtual asynStatus writeFloat64(asynUser* pasynUser, epicsFloat64 value);

        // Callback function envoked by the driver object through the wrapper
        void newFrameCallback(uvc_frame_t* frame, void* ptr);

        // destructor. Disconnects from camera, deletes the object
        ~ADUVC();

    protected:

        
        int ADUVC_UVCComplianceLevel;
        #define ADUVC_FIRST_PARAM ADUVC_UVCComplianceLevel
        int ADUVC_ReferenceCount;
        int ADUVC_Framerate;
        int ADUVC_VendorID;
        int ADUVC_ProductID;
        int ADUVC_ImageFormat;
        int ADUVC_Gamma;
        int ADUVC_BacklightCompensation;
        int ADUVC_Brightness;
        int ADUVC_Contrast;
        int ADUVC_PowerLine;
        int ADUVC_Hue;
        int ADUVC_Saturation;
        int ADUVC_Sharpness;
	#define ADUVC_LAST_PARAM ADUVC_Sharpness

    private:

        // Some data variables
        epicsEventId startEventId;
        epicsEventId endEventId;
        

        // ----------------------------------------
        // UVC Variables
        //-----------------------------------------

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

        //array of supported formats that will allow for easy switching of operating modes.
        ADUVC_CameraFormat_t supportedFormats[SUPPORTED_FORMAT_COUNT];

        //flag that stores if driver is connected to device
        int connected = 0;

        //flag that sees if shutter is on or off
        int withShutter = 0;

        int firstFrame = 0;

        // ----------------------------------------
        // UVC Functions - Logging/Reporting
        //-----------------------------------------

        //function used to report errors in uvc operations
        void reportUVCError(uvc_error_t status, const char* functionName);

        // reports device and driver info into a log file
        void report(FILE* fp, int details);

        // ----------------------------------------
        // UVC Functions - Connecting to camera
        //-----------------------------------------

        //function used for connecting to a UVC device and reading supported camera modes.
        asynStatus connectToDeviceUVC(int connectionType, const char* serialNumber, int productID);
        asynStatus readSupportedCameraFormats();

        //function used to disconnect from UVC device
        asynStatus disconnectFromDeviceUVC();

        // ----------------------------------------
        // UVC Functions - Camera functions
        //-----------------------------------------

        //function that sets exposure time
        asynStatus setExposure(int exposureTime);
        asynStatus setGamma(int gamma);
        asynStatus setBacklightCompensation(int backlightCompensation);
        asynStatus setBrightness(int brightness);
        asynStatus setContrast(int contrast);
        asynStatus setGain(int gain);
        asynStatus setPowerLineFrequency(int powerLineFrequency);
        asynStatus setHue(int hue);
        asynStatus setSaturation(int saturation);
        asynStatus setSharpness(int sharpness);

        //function that begins image aquisition
        uvc_error_t acquireStart(uvc_frame_format format);

        //function that stops aquisition
        void acquireStop();

        //function that converts a UVC frame into an NDArray
        asynStatus uvc2NDArray(uvc_frame_t* frame, NDArray* pArray, NDDataType_t dataType, NDColorMode_t colorMode, size_t imBytes);

        //function that gets information from a UVC device
        void getDeviceImageInformation();
        void getDeviceInformation();

        uvc_frame_format getFormatFromPV();

        //static wrapper function for callback. Necessary becuase callback in UVC must be static but we want the driver running the callback
        static void newFrameCallbackWrapper(uvc_frame_t* frame, void* ptr);
};

// Stores number of additional PV parameters are added by the driver
#define NUM_UVC_PARAMS ((int)(&ADUVC_LAST_PARAM - &ADUVC_FIRST_PARAM + 1))

#endif
