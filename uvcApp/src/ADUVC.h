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
#define ADUVC_REVISION     5
#define ADUVC_MODIFICATION 0


// Unless specified during compilation, use 7 as number of supported formats
#ifndef SUPPORTED_FORMAT_COUNT
#define SUPPORTED_FORMAT_COUNT 7
#endif


#define SUPPORTED_FORMAT_DESC_BUFF  256


// includes
extern "C" {
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"
}

#include "ADDriver.h"


// PV String definitions
#define ADUVC_UVCComplianceLevelString          "UVC_COMPLIANCE"            //asynInt32
#define ADUVC_ReferenceCountString              "UVC_REFCOUNT"              //asynInt32
#define ADUVC_FramerateString                   "UVC_FRAMERATE"             //asynInt32
#define ADUVC_ImageFormatString                 "UVC_FORMAT"                //asynInt32
#define ADUVC_CameraFormatString                "UVC_CAMERA_FORMAT"         //asynInt32
#define ADUVC_FormatDescriptionString           "UVC_FORMAT_DESCRIPTION"    //asynOctet
#define ADUVC_ApplyFormatString                 "UVC_APPLY_FORMAT"          //asynInt32
#define ADUVC_AutoAdjustString                  "UVC_AUTO_ADJUST"           //asynInt32
#define ADUVC_GammaString                       "UVC_GAMMA"                 //asynInt32
#define ADUVC_BacklightCompensationString       "UVC_BACKLIGHT"             //asynInt32
#define ADUVC_BrightnessString                  "UVC_BRIGHTNESS"            //asynInt32
#define ADUVC_ContrastString                    "UVC_CONTRAST"              //asynInt32
#define ADUVC_PowerLineString                   "UVC_POWER"                 //asynInt32
#define ADUVC_HueString                         "UVC_HUE"                   //asynInt32
#define ADUVC_SaturationString                  "UVC_SATURATION"            //asynInt32
#define ADUVC_SharpnessString                   "UVC_SHARPNESS"             //asynInt32
#define ADUVC_PanLeftString                     "UVC_PAN_LEFT"              //asynInt32
#define ADUVC_PanRightString                    "UVC_PAN_RIGHT"             //asynInt32
#define ADUVC_TiltUpString                      "UVC_TILT_UP"               //asynInt32
#define ADUVC_TiltDownString                    "UVC_TILT_DOWN"             //asynInt32
#define ADUVC_ZoomInString                      "UVC_ZOOM_IN"               //asynInt32
#define ADUVC_ZoomOutString                     "UVC_ZOOM_OUT"              //asynInt32
#define ADUVC_PanSpeedString                    "UVC_PAN_SPEED"             //asynInt32
#define ADUVC_TiltSpeedString                   "UVC_TILT_SPEED"            //asynInt32
#define ADUVC_PanTiltStepString                 "UVC_PAN_TILT_STEP"         //asynFloat64

/* enum for getting format from PV */
typedef enum {
    ADUVC_FrameUnsupported      = -1,
    ADUVC_FrameMJPEG            = 0,
    ADUVC_FrameRGB              = 1,
    ADUVC_FrameYUYV             = 2,
    ADUVC_FrameGray8            = 3,
    ADUVC_FrameGray16           = 4,
    ADUVC_FrameUYVY             = 5,
    ADUVC_FrameUncompressed     = 6,
} ADUVC_FrameFormat_t;


/* Struct for individual supported camera format - Used to auto read modes into dropdown for easier operation */
typedef struct ADUVC_CamFormat{
    char formatDesc[SUPPORTED_FORMAT_DESC_BUFF];
    size_t xSize;
    size_t ySize;
    int framerate;
    ADUVC_FrameFormat_t frameFormat;
    NDColorMode_t colorMode;
    NDDataType_t dataType;
} ADUVC_CamFormat_t;



/*
 * Class definition of the ADUVC driver. It inherits from the base ADDriver class
 *
 * Includes constructor/destructor, PV params, function defs and variable defs
 *
 */
class ADUVC : ADDriver{

    public:

        // Constructor
        ADUVC(const char* portName, const char* serial, int productID, 
            int framerate, int xsize, int ysize, int maxBuffers, 
            size_t maxMemory, int priority, int stackSize);


        // ADDriver overrides
        virtual asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);
        virtual asynStatus writeFloat64(asynUser* pasynUser, epicsFloat64 value);
        virtual asynStatus connect(asynUser* pasynUser);
        virtual asynStatus disconnect(asynUser* pasynUser);

        // Callback function envoked by the driver object through the wrapper
        void newFrameCallback(uvc_frame_t* frame, void* ptr);

        // destructor. Disconnects from camera, deletes the object
        ~ADUVC();

    protected:

        
        int ADUVC_UVCComplianceLevel;
        #define ADUVC_FIRST_PARAM ADUVC_UVCComplianceLevel
        int ADUVC_ReferenceCount;
        int ADUVC_Framerate;
        int ADUVC_ImageFormat;
        int ADUVC_CameraFormat;
        int ADUVC_FormatDescription;
        int ADUVC_ApplyFormat;
        int ADUVC_AutoAdjust;
        int ADUVC_Gamma;
        int ADUVC_BacklightCompensation;
        int ADUVC_Brightness;
        int ADUVC_Contrast;
        int ADUVC_PowerLine;
        int ADUVC_Hue;
        int ADUVC_Saturation;
        int ADUVC_Sharpness;
        int ADUVC_PanLeft;
        int ADUVC_PanRight;
        int ADUVC_TiltUp;
        int ADUVC_TiltDown;
        int ADUVC_ZoomIn;
        int ADUVC_ZoomOut;
        int ADUVC_PanSpeed;
        int ADUVC_TiltSpeed;
        int ADUVC_PanTiltStep;
        #define ADUVC_LAST_PARAM ADUVC_PanTiltStep

    private:

        // ----------------------------------------
        // UVC Variables
        //-----------------------------------------

        // Connection information
        int connectionType;
        int productID;
        const char* serialNumber;

        // Checks uvc device operations status
        uvc_error_t deviceStatus;

        // Pointer to uvc device struct
        uvc_device_t* pdevice;

        // Pointer to device context. generated when connecting
        uvc_context_t* pdeviceContext;

        // Pointer to device handle. 
        // Used for controlling device. 
        // Each UVC device can allow for one handle at a time
        uvc_device_handle_t* pdeviceHandle;

        // Device stream controller. used to control streaming from device
        uvc_stream_ctrl_t deviceStreamCtrl;

        // Pointer to struct containing device info, such as vendor, product id
        uvc_device_descriptor_t* pdeviceInfo;

        // Array of supported formats that will allow for easy switching of operating modes.
        ADUVC_CamFormat_t supportedFormats[SUPPORTED_FORMAT_COUNT];

        // Flag that stores if driver is connected to device
        int connected = 0;

        //flag that sees if shutter is on or off
        int withShutter = 0;

        // Flag for checking if frame size was validated with selected dtype and color mode
        bool validatedFrameSize = false;

        // ----------------------------------------
        // UVC Functions - Logging/Reporting
        //-----------------------------------------

        // Function used to report errors in uvc operations
        void reportUVCError(uvc_error_t status, const char* functionName);

        // Reports device and driver info into a log file
        void report(FILE* fp, int details);

        // Writes to ADStatus PV
        void updateStatus(const char* status);

        // ----------------------------------------
        // UVC Functions - Connecting to camera
        //-----------------------------------------

        // Function used for connecting to a UVC device and reading supported camera modes.
        asynStatus connectToDeviceUVC();

        // Function used to disconnect from UVC device
        asynStatus disconnectFromDeviceUVC();

        // ----------------------------------------
        // UVC Functions - Camera Format Detection
        //-----------------------------------------

        // Functions for reading camera formats, and linking them to the appropriate PVs
        asynStatus readSupportedCameraFormats();
        void populateCameraFormat(ADUVC_CamFormat_t* camFormat, uvc_format_desc_t* format_desc, uvc_frame_desc_t* frame_desc);
        int selectBestCameraFormats(ADUVC_CamFormat_t* formatBuffer, int numberInterfaces);
        
        // Helper functions
        void initEmptyCamFormat(int arrayIndex);
        bool formatAlreadySaved(ADUVC_CamFormat_t camFormat);
        int compareFormats(ADUVC_CamFormat_t camFormat1, ADUVC_CamFormat_t camFormat2);

        // Functions for applying saved camera mode to PVs
        void updateCameraFormatDesc();
        void applyCameraFormat();

        // ----------------------------------------
        // UVC Functions - Camera functions
        //-----------------------------------------

        // Functions that set image processing and acquisiton controls
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

        // Functions that allow for PTZ (Pan/Tilt/Zoom) control for supported devices
        asynStatus processPanTilt(int panDirection, int tiltDirection);
        asynStatus processZoom(int zoomDirection);

        uint16_t zoomMin, zoomMax, currentZoom, zoomStepSize;
        int zoomSteps = 10;

        // Functions that start/stop image aquisition
        uvc_error_t acquireStart(uvc_frame_format format);
        void acquireStop();

        // Function that converts a UVC frame into an NDArray
        asynStatus uvc2NDArray(uvc_frame_t* frame, NDArray* pArray, NDDataType_t dataType, NDColorMode_t colorMode, size_t imBytes);

        // Function that attempts to fit data type + color mode to frame if size doesn't match
        void checkValidFrameSize(uvc_frame_t* frame);

        // Functions that get information about device and image
        void getDeviceImageInformation();
        void getDeviceInformation();

        // Function that converts ADUVC_Format PV value into uvc_frame_format
        uvc_frame_format getFormatFromPV();

        // Static wrapper function for callback. 
        // Necessary becuase callback in UVC must be static but we want the driver running the callback
        static void newFrameCallbackWrapper(uvc_frame_t* frame, void* ptr);
};

// Stores number of additional PV parameters are added by the driver
#define NUM_UVC_PARAMS ((int)(&ADUVC_LAST_PARAM - &ADUVC_FIRST_PARAM + 1))

#endif
