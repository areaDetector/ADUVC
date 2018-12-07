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
#include "ADUVC.h"


// libuvc includes
#include <libuvc/libuvc.h>
#include <libuvc/libuvc_config.h>


using namespace std;

static const char* driverName = "ADUVC";

// Constants
static const double ONE_BILLION = 1.E9;



//---------------------------------------------------------
// ADUVC Utility functions
//---------------------------------------------------------

/*
 * External configuration function for ADUVC.
 * Envokes the constructor to create a new ADUVC object
 * This is the function that initializes the driver, and is called in the IOC startup script
 *
 * @params: all passed into constructor
 * @return: status
 */
extern "C" int ADUVCConfig(const char* portName, const char* serial, int productID, int framerate, int xsize, int ysize, int maxBuffers, size_t maxMemory, int priority, int stackSize){
    new ADUVC(portName, serial, productID, framerate, xsize, ysize, maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}


/*
 * Callback function called when IOC is terminated.
 * Deletes created object and frees UVC context
 *
 * @params: pPvt -> pointer to the ADUVC object created in ADUVCConfig
 */
static void exitCallbackC(void* pPvt){
    ADUVC* pUVC = (ADUVC*) pPvt;
    delete(pUVC);
}


/*
 * Function used to display UVC errors
 *
 * @params: status          -> uvc error passed to function
 * @params: functionName    -> name of function in which uvc error occurred
 * return: void
 */
void ADUVC::reportUVCError(uvc_error_t status, const char* functionName){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s UVC Error: %s\n", 
                driverName, functionName, uvc_strerror(status));
    if(status != UVC_ERROR_OTHER){
        char statusMessage[25];
        epicsSnprintf(statusMessage, sizeof(statusMessage), "UVC Error: %s\n", uvc_strerror(status));
        setStringParam(ADStatusMessage, statusMessage);
        callParamCallbacks();
    }
}



//-----------------------------------------------
// ADUVC connection/disconnection functions
//-----------------------------------------------

/*
 * Function responsible for connecting to the UVC device. First, a device context is created,
 * then the device is identified, then opened.
 * NOTE: this driver must have exclusive access to the device as per UVC standards.
 *
 * @params: connectionType  -> UVC can now connect with productID or serial ID. This flag switches
 * @params: serialNumber    -> serial number of device to connect to
 * @params: productID       -> product ID of camera to connect to
 * @return: asynStatus      -> true if connection is successful, false if failed
 */
asynStatus ADUVC::connectToDeviceUVC(int connectionType, const char* serialNumber, int productID){
    static const char* functionName = "connectToDeviceUVC";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_init(&pdeviceContext, NULL);

    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Initialized UVC context\n", driverName, functionName);
    }
    if(connectionType == 0){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Searching for UVC device with serial number: %s\n", driverName, functionName, serialNumber);
        deviceStatus = uvc_find_device(pdeviceContext, &pdevice, 0, 0, serialNumber);
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Searching for UVC device with Product ID: %d\n", driverName, functionName, productID);
        deviceStatus = uvc_find_device(pdeviceContext, &pdevice, 0, productID, NULL);
    }
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Found UVC device\n", driverName, functionName);
    }
    deviceStatus = uvc_open(pdevice, &pdeviceHandle);
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        connected = 1;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Opened UVC device\n", driverName, functionName);
    }
    return status;
}


/*
 * Function that disconnects from a connected UVC device.
 * Closes device handle and context, and unreferences the device pointer from memory
 * 
 * @return: asynStatus -> success if device existed and was disconnected, error if there was no device connected
 * 
 */
asynStatus ADUVC::disconnectFromDeviceUVC(){
    const char* functionName = "disconnectFromDeviceUVC";
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Calling all free functions for ADUVC\n", driverName, functionName);
    if(connected == 1){
        uvc_close(pdeviceHandle);
        uvc_unref_device(pdevice);
        uvc_exit(pdeviceContext);
        return asynSuccess;
    }
    return asynError;
}


/**
 * Function responsible for getting image acquisition information
 * Gets the exposure, gamma, backlight comp, brightness, contrast, gain, hue, power line, saturation, and
 * sharpness from the camera, and sets the values to the appopriate PVs. Then it refreshes the PVs
 * 
 * @return: void
 */
void ADUVC::getDeviceImageInformation(){
    const char* functionName = "getDeviceImageInformation";
    uint32_t exposure;
    uint16_t gamma, backlightCompensation, contrast, gain,  saturation, sharpness;
    int16_t brightness, hue;
    uint8_t powerLineFrequency;

    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Populating camera function PVs.\n", driverName, functionName);

    //get values from UVC camera
    uvc_get_exposure_abs(pdeviceHandle, &exposure, UVC_GET_CUR);
    uvc_get_gamma(pdeviceHandle, &gamma, UVC_GET_CUR);
    uvc_get_backlight_compensation(pdeviceHandle, &backlightCompensation, UVC_GET_CUR);
    uvc_get_brightness(pdeviceHandle, &brightness, UVC_GET_CUR);
    uvc_get_contrast(pdeviceHandle, &contrast, UVC_GET_CUR);
    uvc_get_gain(pdeviceHandle, &gain, UVC_GET_CUR);
    uvc_get_power_line_frequency(pdeviceHandle, &powerLineFrequency, UVC_GET_CUR);
    uvc_get_hue(pdeviceHandle, &hue, UVC_GET_CUR);
    uvc_get_saturation(pdeviceHandle, &saturation, UVC_GET_CUR);
    uvc_get_sharpness(pdeviceHandle, &sharpness, UVC_GET_CUR);

    //put values into appropriate PVs
    setDoubleParam(ADAcquireTime, (double) exposure);
    setIntegerParam(ADUVC_Gamma, (int) gamma);
    setIntegerParam(ADUVC_BacklightCompensation, (int) backlightCompensation);
    setIntegerParam(ADUVC_Brightness, (int) brightness);
    setIntegerParam(ADUVC_Contrast, (int) contrast);
    setDoubleParam(ADGain, (double) gain);
    setIntegerParam(ADUVC_PowerLine, (int) powerLineFrequency);
    setIntegerParam(ADUVC_Hue, (int) hue);
    setIntegerParam(ADUVC_Saturation, (int) saturation);
    setIntegerParam(ADUVC_Sharpness, (int) sharpness);
    
    //refresh PV values
    callParamCallbacks();
}


/*
 * Function responsible for getting device information once device has been connected.
 * Calls the uvc_get_device_descriptor function from libuvc, and gets the manufacturer,
 * serial, model.
 *
 * @return: void
 */
void ADUVC::getDeviceInformation(){
    static const char* functionName = "getDeviceInformation";
    char modelName[50];
    uvc_get_device_descriptor(pdevice, &pdeviceInfo);
    if(pdeviceInfo->manufacturer!=NULL) setStringParam(ADManufacturer, pdeviceInfo->manufacturer);
    if(pdeviceInfo->serialNumber!=NULL) setStringParam(ADSerialNumber, pdeviceInfo->serialNumber);
    sprintf(modelName, "Vendor: %d, Product: %d", pdeviceInfo->idVendor, pdeviceInfo->idProduct);
    setIntegerParam(ADUVC_UVCComplianceLevel, pdeviceInfo->bcdUVC);
    setStringParam(ADModel, modelName);
    callParamCallbacks();
    uvc_free_device_descriptor(pdeviceInfo);
    getDeviceImageInformation();
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Finished Getting device information\n", driverName, functionName);
}


/**
 * Function responsible for converting value from PV into UVC frame format type
 * 
 * @return: uvc_frame_format    -> conversion if valid, unknown if invalid
 */
uvc_frame_format ADUVC::getFormatFromPV(){
    const char* functionName = "getFormatFromPV";
    int format;
    getIntegerParam(ADUVC_ImageFormat, &format);
    ADUVC_FrameFormat_t frameFormat = (ADUVC_FrameFormat_t) format;
    switch(frameFormat){
        case ADUVC_FrameMJPEG:
            return UVC_FRAME_FORMAT_MJPEG;
        case ADUVC_FrameRGB:
            return UVC_FRAME_FORMAT_RGB;
        case ADUVC_FrameYUYV:
            return UVC_FRAME_FORMAT_YUYV;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error: invalid frame format\n", driverName, functionName);
            return UVC_FRAME_FORMAT_UNKNOWN;
    }
}



//----------------------------------------------------------------------
// UVC acquisition start and stop functions
//----------------------------------------------------------------------

/*
 * Function that starts the acquisition of the camera.
 * In the case of UVC devices, a function is called to first negotiate a stream with the camera at
 * a particular resolution and frame rate. Then the uvc_start_streaming function is called, with a 
 * function name being passed as a parameter as the callback function.
 *
 * @params: imageFormat -> type of image format to use
 * @return: uvc_error_t -> return 0 if successful, otherwise return error code
 */
uvc_error_t ADUVC::acquireStart(uvc_frame_format imageFormat){
    static const char* functionName = "acquireStart";
    // get values for image format from PVs set in IOC shell
    int framerate;
    int xsize;
    int ysize;
    getIntegerParam(ADUVC_Framerate, &framerate);
    getIntegerParam(ADSizeX, &xsize);
    getIntegerParam(ADSizeY, &ysize);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Starting acquisition: x-size: %d, y-size %d, framerate %d\n", driverName, functionName, xsize, ysize, framerate);
    switch(imageFormat){
        case UVC_FRAME_FORMAT_MJPEG:
            //MJPEG format
            deviceStatus = uvc_get_stream_ctrl_format_size(pdeviceHandle, &deviceStreamCtrl, UVC_FRAME_FORMAT_MJPEG, xsize, ysize, framerate);
            break;
        case UVC_FRAME_FORMAT_RGB:
            //RGB format
            deviceStatus = uvc_get_stream_ctrl_format_size(pdeviceHandle, &deviceStreamCtrl, UVC_FRAME_FORMAT_RGB, xsize, ysize, framerate);
            break;
        case UVC_FRAME_FORMAT_YUYV:
            //YUYV format
            deviceStatus = uvc_get_stream_ctrl_format_size(pdeviceHandle, &deviceStreamCtrl, UVC_FRAME_FORMAT_YUYV, xsize, ysize, framerate);
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Cannot start acquisition invalid frame format\n", driverName, functionName);
            deviceStatus = UVC_ERROR_NOT_SUPPORTED;
            reportUVCError(deviceStatus, functionName);
            setIntegerParam(ADAcquire, 0);
            setIntegerParam(ADStatus, ADStatusIdle);
            callParamCallbacks();
            return deviceStatus;
    }
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        setIntegerParam(ADAcquire, 0);
        setIntegerParam(ADStatus, ADStatusIdle);
        callParamCallbacks();
        return deviceStatus;
    }
    else{
        setIntegerParam(ADNumImagesCounter, 0);
        callParamCallbacks();
        //Here is where we initialize the stream and set the callback function to the static wrapper and pass 'this' as the void pointer.
        //This function is in the form uvc_start_streaming(device_handle, device_stream_ctrl*, Callback Function* , void*, int)
        deviceStatus = uvc_start_streaming(pdeviceHandle, &deviceStreamCtrl, ADUVC::newFrameCallbackWrapper, this, 0);
        if(deviceStatus<0){
            reportUVCError(deviceStatus, functionName);
            setIntegerParam(ADAcquire, 0);
            setIntegerParam(ADStatus, ADStatusIdle);
            callParamCallbacks();
            return deviceStatus;
        }
        else{
            setIntegerParam(ADStatus, ADStatusAcquire);
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Image aquisition start\n", driverName, functionName);
            callParamCallbacks();
        }
    }
    return deviceStatus;
}


/*
 * Function responsible for stopping aquisition of images from UVC camera
 * Calls uvc_stop_streaming function
 *
 * @return: void
 */
void ADUVC::acquireStop(){

    static const char* functionName = "acquireStop";

    //stop_streaming will block until last callback is processed.
    uvc_stop_streaming(pdeviceHandle);

    //update PV values
    setIntegerParam(ADStatus, ADStatusIdle);
    setIntegerParam(ADAcquire, 0);
    this->firstFrame = 0;
    callParamCallbacks();
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Stopping aquisition\n",driverName, functionName);
}



//-------------------------------------------------------
// UVC Image Processing and callback functions
//-------------------------------------------------------

/*
 * Function used as a wrapper function for the callback.
 * This is necessary beacuse the callback function must be static for libuvc, meaning that function calls
 * inside the callback function can only be run by using the driver name. Because libuvc allows for a void*
 * to be passed through the callback function, however, the solution is to pass 'this' as the void pointer,
 * cast it as an ADUVC pointer, and simply run the non-static callback function with that object.
 * 
 * @params: frame   -> pointer to uvc_frame received
 * @params: ptr     -> 'this' cast as a void pointer. It is cast back to ADUVC object and then new frame callback is called
 * @return: void
 */
void ADUVC::newFrameCallbackWrapper(uvc_frame_t* frame, void* ptr){
    ADUVC* pPvt = ((ADUVC*) ptr);
    if(pPvt->firstFrame == 0) pPvt->firstFrame = 1;
    else pPvt->newFrameCallback(frame, pPvt);
}


/*
 * Function responsible for converting between a uvc_frame_t type image to
 * the EPICS area detector standard NDArray type. First, we convert any given uvc_frame to
 * the rgb format. This is because all of the supported uvc formats can be converted into this type,
 * simplyfying the conversion process. Then, the NDDataType is taken into account, and a scaling factor is used
 *
 * @params: frame       -> frame collected from the uvc camera
 * @params: pArray      -> output of function. NDArray conversion of uvc frame
 * @params: dataType    -> data type of NDArray output image
 * @params: colorMode   -> image color mode. So far only RGB1 is supported
 * @params: imBytes     -> number of bytes in the image
 * @return: void, but output into pArray
 */
asynStatus ADUVC::uvc2NDArray(uvc_frame_t* frame, NDArray* pArray, NDDataType_t dataType, NDColorMode_t colorMode, int imBytes){
    static const char* functionName = "uvc2NDArray";
    uvc_frame_t* rgb;
    rgb = uvc_allocate_frame(frame->width * frame->height *3);
    if(!rgb){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR: Unable to allocate frame\n", driverName, functionName);
        return asynError;
    }
	// convert any uvc frame format to rgb (to simplify the converson to NDArray, since area detector supports the rgb format)
    uvc_frame_format frameFormat = frame->frame_format;
    switch(frameFormat){
        case UVC_FRAME_FORMAT_YUYV:
            deviceStatus = uvc_any2rgb(frame, rgb);
            break;
        case UVC_FRAME_FORMAT_UYVY:
            deviceStatus = uvc_any2rgb(frame, rgb);
            break;
        case UVC_FRAME_FORMAT_MJPEG:
            deviceStatus = uvc_mjpeg2rgb(frame, rgb);
            break;
        case UVC_FRAME_FORMAT_RGB:
            deviceStatus = uvc_any2rgb(frame, rgb);
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR: Unsupported UVC format\n", driverName, functionName);
			uvc_free_frame(rgb);
            return asynError;
    }
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
		uvc_free_frame(rgb);
        return asynError;
    }
    else{

        // cast frame data to char (8-bit/1-byte) Comment this out, just setting pointer to avoid memcpy
        // unsigned char* dataInit = (unsigned char*) rgb->data;

		//currently only support NDUInt8 and RGB1(most UVC cameras only have this anyway)
        if(dataType!=NDUInt8 && colorMode!=NDColorModeRGB1){
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Unsupported data format\n", driverName, functionName);
            return asynError;
        }

        //Copy data from UVC frame to NDArray, and label it as the correct color mode
        //memcpy(pArray->pData, dataInit, imBytes);
        //try to do this without memcpy for performance reasons
        pArray->pData = rgb->data;
        pArray->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &colorMode);

        //increment the array counter
        int arrayCounter;
        getIntegerParam(NDArrayCounter, &arrayCounter);
        arrayCounter++;
        setIntegerParam(NDArrayCounter, arrayCounter);

        //refresh PVs
        callParamCallbacks();

        //Sends image to the ArrayDataPV
        getAttributes(pArray->pAttributeList);
        doCallbacksGenericPointer(pArray, NDArrayData, 0);

        //frees up memory
        pArray->release();
        uvc_free_frame(rgb);
        return asynSuccess;
    }
}


/*
 * Function that performs the callbacks onto new frames generated by the camera.
 * First, a new NDArray pointer is allocated, then the given uvc_frame pointer is converted
 * into this allocated NDArray. Then, based on the operating mode, the acquisition moves to the 
 * next frame, or stops. The NDArray is passed on to the doCallbacksGenericPointer function.
 *
 * @params: frame   -> uvc_frame recieved from the camera
 * @params: ptr     -> void pointer with data from the frame
 * @return: void
 */
void ADUVC::newFrameCallback(uvc_frame_t* frame, void* ptr){
    NDArray* pArray;
    NDArrayInfo arrayInfo;
    NDDataType_t ndDataType;
    int operatingMode;
    NDColorMode_t colorMode;

    //TODO: Timestamps for images
    //epicsTimeStamp currentTime;
    static const char* functionName = "newFrameCallback";
	
	// initialize the NDArray here. Otherwise causes segfault. Currently onlu 24bit rgb supported
	int ndims = 3;
   	size_t dims[ndims];
    dims[0] = 3;
    dims[1] = frame->width;
	dims[2] = frame->height;
    
    //only color mode  and data type currently supported
    colorMode = NDColorModeRGB1;
    ndDataType = NDUInt8;

    getIntegerParam(ADImageMode, &operatingMode);

    // allocate memory for a new NDArray, and set pArray to a pointer for this memory
	this->pArrays[0] = pNDArrayPool->alloc(ndims, dims, ndDataType, 0, NULL);
	if(this->pArrays[0]!=NULL){ 
        pArray = this->pArrays[0];   
    }
	else{
        this->pArrays[0]->release();
	    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Unable to allocate array\n", driverName, functionName);
	    return;
	}

    // Update camera image parameters
    setIntegerParam(ADSizeX, dims[1]);
    setIntegerParam(NDArraySizeX, dims[1]);
    setIntegerParam(ADSizeY, dims[2]);
    setIntegerParam(NDArraySizeY, dims[2]);
    pArray->getInfo(&arrayInfo);
    setIntegerParam(NDArraySize, (int)arrayInfo.totalBytes);
    setIntegerParam(NDDataType, ndDataType);
    setIntegerParam(NDColorMode, colorMode);

    //single shot mode stops after one images
    if(operatingMode == ADImageSingle){
        int numImages;
        getIntegerParam(ADNumImagesCounter, &numImages);
        numImages++;
        setIntegerParam(ADNumImagesCounter, numImages);
        uvc2NDArray(frame, pArray, ndDataType, colorMode, arrayInfo.totalBytes);
        acquireStop();
        
    }
    // block shot mode stops once numImages reaches the number of desired images
    else if(operatingMode == ADImageMultiple){
        int numImages;
        int desiredImages;
        getIntegerParam(ADNumImagesCounter, &numImages);
        numImages++;
        setIntegerParam(ADNumImagesCounter, numImages);
        uvc2NDArray(frame, pArray, ndDataType, colorMode, arrayInfo.totalBytes);
        getIntegerParam(ADNumImages, &desiredImages);
	    if(numImages>=desiredImages){
            acquireStop();
            return;
        }
    }
    //continuous mode runs until user stops acquisition
    else if(operatingMode == ADImageContinuous){
        int numImages;
        getIntegerParam(ADNumImagesCounter, &numImages);
        numImages++;
        setIntegerParam(ADNumImagesCounter, numImages);
        uvc2NDArray(frame, pArray, ndDataType, colorMode, arrayInfo.totalBytes);
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR: Unsupported operating mode\n", driverName, functionName);
        acquireStop();
        return;
    }
    
}



//---------------------------------------------------------
// Base UVC Camera functionality
//---------------------------------------------------------

/**
 * Function that sets exposure time in seconds
 * 
 * @params: exposureTime -> the exposure time in seconds
 * @return: status
 */
asynStatus ADUVC::setExposure(int exposureTime){
    const char* functionName = "setExposure";
    asynStatus status = asynSuccess;

    deviceStatus = uvc_set_exposure_abs(pdeviceHandle, exposureTime);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets Gamma value
 * 
 * @params: gamma -> value for gamma
 * @return: status
 */
asynStatus ADUVC::setGamma(int gamma){
    const char* functionName = "setGamma";
    asynStatus status = asynSuccess;

    deviceStatus = uvc_set_gamma(pdeviceHandle, gamma);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets backlight compensation. Used when camera has a light behind it that oversaturates image
 * 
 * @params: backlightCompensation -> degree of backlight comp.
 * @return: status
 */
asynStatus ADUVC::setBacklightCompensation(int backlightCompensation){
    const char* functionName = "setBacklightCompensation";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_set_backlight_compensation(pdeviceHandle, backlightCompensation);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets image brightness (similar to setGamma)
 * 
 * @params: brighness -> degree of brightness: for example, a 1.5 value would change a pixel value 30 to 45
 * @return: status
 */
asynStatus ADUVC::setBrightness(int brightness){
    const char* functionName = "setBrightness";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_set_brightness(pdeviceHandle, brightness);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets value for contrast. Higher value increases difference between whites and blacks
 * 
 * @params: contrast -> level of contrast
 * @return: status
 */
asynStatus ADUVC::setContrast(int contrast){
    const char* functionName = "setContrast";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_set_contrast(pdeviceHandle, contrast);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets camera gain value
 * 
 * @params: gain -> value for gain
 * @return: status
 */
asynStatus ADUVC::setGain(int gain){
    const char* functionName = "setGain";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_set_gain(pdeviceHandle, gain);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets Power line frequency. Used to avoid camera flicker when signal is not correct
 * 
 * @params: powerLineFrequency -> frequency value (50Hz, 60Hz etc.)
 * @return status
 */
asynStatus ADUVC::setPowerLineFrequency(int powerLineFrequency){
    const char* functionName = "setPowerLineFrequency";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_set_power_line_frequency(pdeviceHandle, powerLineFrequency);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets camera Hue. For example, a hue of 240 will result in a blue shifted image,
 * while 0 will result in red-shifted
 * 
 * @params: hue -> value for image hue or tint
 * @return: status
 */
asynStatus ADUVC::setHue(int hue){
    const char* functionName = "setHue";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_set_hue(pdeviceHandle, hue);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets saturation. Higher value will result in more vivid colors
 * 
 * @params: saturation -> degree of saturation
 * @return status
 */
asynStatus ADUVC::setSaturation(int saturation){
    const char* functionName = "setSaturation";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_set_saturation(pdeviceHandle, saturation);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}


/**
 * Function that sets the degree of sharpening. Too high will cause oversharpening
 * 
 * @params: sharpness -> degree of sharpening
 * @return: status
 */
asynStatus ADUVC::setSharpness(int sharpness){
    const char* functionName = "setSharpness";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_set_sharpness(pdeviceHandle, sharpness);
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    return status;
}



//-------------------------------------------------------------------------
// ADDriver function overwrites
//-------------------------------------------------------------------------

/*
 * Function overwriting ADDriver base function.
 * Takes in a function (PV) changes, and a value it is changing to, and processes the input
 *
 * @params: pasynUser       -> asyn client who requests a write
 * @params: value           -> int32 value to write
 * @return: asynStatus      -> success if write was successful, else failure
 */
asynStatus ADUVC::writeInt32(asynUser* pasynUser, epicsInt32 value){
    int function = pasynUser->reason;
    int acquiring;
    int status = asynSuccess;
    static const char* functionName = "writeInt32";
    getIntegerParam(ADAcquire, &acquiring);

    status = setIntegerParam(function, value);
    // start/stop acquisition
    if(function == ADAcquire){
        if(value && !acquiring){
            //asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Entering aquire\n", driverName, functionName);
            deviceStatus = acquireStart(getFormatFromPV());
            if(deviceStatus < 0){
                reportUVCError(deviceStatus, functionName);
                return asynError;
            }
        }
        if(!value && acquiring){
            acquireStop();
        }
    }
    //switch image mode
    else if(function == ADImageMode){
        if(acquiring == 1) acquireStop();
        if(value == ADImageSingle) setIntegerParam(ADNumImages, 1);
        else if(value == ADImageMultiple) setIntegerParam(ADNumImages, 300);
        else if(value == ADImageContinuous) setIntegerParam(ADNumImages, 3000);
        else{
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR: Unsupported camera operating mode\n", driverName, functionName);
            return asynError;
        }
    }
    //switch image format
    else if(function == ADUVC_ImageFormat || function == ADUVC_Framerate){
        if(acquiring == 1) acquireStop();
    }
    //setting different camera functions
    else if(function == ADUVC_Gamma) setGamma(value);
    else if(function == ADUVC_BacklightCompensation) setBacklightCompensation(value);
    else if(function == ADUVC_Brightness) setBrightness(value);
    else if(function == ADUVC_Contrast) setContrast(value);
    else if(function == ADUVC_Hue) setHue(value);
    else if(function == ADUVC_PowerLine) setPowerLineFrequency(value);
    else if(function == ADUVC_Saturation) setSaturation(value);
    else if(function == ADUVC_Sharpness) setSharpness(value);
    else{
        if (function < ADUVC_FIRST_PARAM) {
            status = ADDriver::writeInt32(pasynUser, value);
        }
    }
    callParamCallbacks();

    if(status){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR status=%d, function=%d, value=%d\n", driverName, functionName, status, function, value);
        return asynError;
    }
    else asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s function=%d value=%d\n", driverName, functionName, function, value);
    return asynSuccess;
}


/*
 * Function overwriting ADDriver base function.
 * Takes in a function (PV) changes, and a value it is changing to, and processes the input
 * This is the same functionality as writeInt32, but for processing doubles.
 *
 * @params: pasynUser       -> asyn client who requests a write
 * @params: value           -> int32 value to write
 * @return: asynStatus      -> success if write was successful, else failure
 */
asynStatus ADUVC::writeFloat64(asynUser* pasynUser, epicsFloat64 value){
    int function = pasynUser->reason;
    int acquiring;
    int status = asynSuccess;
    static const char* functionName = "writeFloat64";
    getIntegerParam(ADAcquire, &acquiring);

    status = setDoubleParam(function, value);

    if(function == ADAcquireTime){
        if(acquiring) acquireStop();
        setExposure((int) value);
    }
    else if(function == ADGain) setGain((int) value);
    else{
        if(function < ADUVC_FIRST_PARAM){
            status = ADDriver::writeFloat64(pasynUser, value);
        }
    }
    callParamCallbacks();

    if(status){
        asynPrint(this-> pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR status = %d, function =%d, value = %f\n", driverName, functionName, status, function, value);
        return asynError;
    }
    else asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s function=%d value=%f\n", driverName, functionName, function, value);
    return asynSuccess;
}


/*
 * Function used for reporting ADUVC device and library information to a external
 * log file. The function first prints all libuvc specific information to the file,
 * then continues on to the base ADDriver 'report' function
 * 
 * @params: fp      -> pointer to log file
 * @params: details -> number of details to write to the file
 * @return: void
 */
void ADUVC::report(FILE* fp, int details){
    const char* functionName = "report";
    int framerate;
    int height;
    int width;
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s reporting to external log file\n",driverName, functionName);
    if(details > 0){
        fprintf(fp, " LIBUVC Version        ->      %d.%d.%d\n", LIBUVC_VERSION_MAJOR, LIBUVC_VERSION_MINOR, LIBUVC_VERSION_PATCH);
        fprintf(fp, " -------------------------------------------------------------------\n");
        if(!connected){
            fprintf(fp, " No connected devices\n");
            ADDriver::report(fp, details);
            return;
        }
        fprintf(fp, " Connected Device Information\n");
        fprintf(fp, " Serial number         ->      %s\n", pdeviceInfo->serialNumber);
        fprintf(fp, " VendorID              ->      %d\n", pdeviceInfo->idVendor);
        fprintf(fp, " ProductID             ->      %d\n", pdeviceInfo->idProduct);
        fprintf(fp, " UVC Compliance Level  ->      %d\n", pdeviceInfo->bcdUVC);
        getIntegerParam(ADUVC_Framerate, &framerate);
        getIntegerParam(ADSizeX, &width);
        getIntegerParam(ADSizeY, &height);
        fprintf(fp, " Camera Framerate      ->      %d\n", framerate);
        fprintf(fp, " Image Width           ->      %d\n", width);
        fprintf(fp, " Image Height          ->      %d\n", height);
        fprintf(fp, " -------------------------------------------------------------------\n");
        fprintf(fp, "\n");
        
        ADDriver::report(fp, details);
    }
}



//----------------------------------------------------------------------------
// ADUVC Constructor/Destructor
//----------------------------------------------------------------------------

/*
 * Constructor for ADUVC driver. Most params are passed to the parent ADDriver constructor.
 * Connects to the camera, then gets device information, and is ready to aquire images.
 *
 * @params: portName    -> port for NDArray recieved from camera
 * @params: serial      -> serial number of device to connect to
 * @params: productID   -> id of device used to connect if serial is unavailable
 * @params: framerate   -> framerate at which camera should operate
 * @params: maxBuffers  -> max buffer size for NDArrays
 * @params: maxMemory   -> maximum memory allocated for driver
 * @params: priority    -> what thread priority this driver will execute with
 * @params: stackSize   -> size of the driver on the stack
 */
ADUVC::ADUVC(const char* portName, const char* serial, int productID, int framerate, int xsize, int ysize, int maxBuffers, size_t maxMemory, int priority, int stackSize)
    : ADDriver(portName, 1, (int)NUM_UVC_PARAMS, maxBuffers, maxMemory, asynEnumMask, asynEnumMask, ASYN_CANBLOCK, 1, priority, stackSize){
    static const char* functionName = "ADUVC";

    // create PV Params
    createParam(ADUVC_UVCComplianceLevelString,     asynParamInt32,     &ADUVC_UVCComplianceLevel);
    createParam(ADUVC_ReferenceCountString,         asynParamInt32,     &ADUVC_ReferenceCount);
    createParam(ADUVC_FramerateString,              asynParamInt32,     &ADUVC_Framerate);
    createParam(ADUVC_VendorIDString,               asynParamInt32,     &ADUVC_VendorID);
    createParam(ADUVC_ProductIDString,              asynParamInt32,     &ADUVC_ProductID);
    createParam(ADUVC_ImageFormatString,            asynParamInt32,     &ADUVC_ImageFormat);
    createParam(ADUVC_BrightnessString,             asynParamInt32,     &ADUVC_Brightness);
    createParam(ADUVC_ContrastString,               asynParamInt32,     &ADUVC_Contrast);
    createParam(ADUVC_PowerLineString,              asynParamInt32,     &ADUVC_PowerLine);
    createParam(ADUVC_HueString,                    asynParamInt32,     &ADUVC_Hue);
    createParam(ADUVC_SaturationString,             asynParamInt32,     &ADUVC_Saturation);
    createParam(ADUVC_GammaString,                  asynParamInt32,     &ADUVC_Gamma);
    createParam(ADUVC_BacklightCompensationString,  asynParamInt32,     &ADUVC_BacklightCompensation);
    createParam(ADUVC_SharpnessString,              asynParamInt32,     &ADUVC_Sharpness);

    // set initial size and framerate params
    setIntegerParam(ADUVC_Framerate, framerate);
    setIntegerParam(ADSizeX, xsize);
    setIntegerParam(ADSizeY, ysize);

    //sets serial number and productID
    if(strcmp(serial, "") == 0) setStringParam(ADSerialNumber, "No Serial Detected");
    else setStringParam(ADSerialNumber, serial);

    setIntegerParam(ADUVC_ProductID, productID);

    //sets libuvc version
    char uvcVersionString[25];
    epicsSnprintf(uvcVersionString, sizeof(uvcVersionString), "%d.%d.%d", LIBUVC_VERSION_MAJOR, LIBUVC_VERSION_MINOR, LIBUVC_VERSION_PATCH);
    setStringParam(ADSDKVersion, uvcVersionString);

    //sets driver version
    char versionString[25];
    epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", ADUVC_VERSION, ADUVC_REVISION, ADUVC_MODIFICATION);
    setStringParam(NDDriverVersion, versionString);
    
    asynStatus connected;

    // decide if connecting with serial number or productID
    if(strlen(serial)!=0){
        connected = connectToDeviceUVC(0, serial, productID);
    }
    else{
        connected = connectToDeviceUVC(1, NULL, productID);
    }

    // check if connected successfully, and if so, get the device information
    if(connected == asynError){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Connection failed, abort\n", driverName, functionName);
    }
    else{
	    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Acquiring device information\n", driverName, functionName);
		getDeviceInformation();
    }

    // when epics is exited, delete the instance of this class
    epicsAtExit(exitCallbackC, this);

}


/*
 * ADUVC destructor. Called by the exitCallbackC function when IOC is shut down
 */
ADUVC::~ADUVC(){
    static const char* functionName = "~ADUVC";
    disconnectFromDeviceUVC();
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,"%s::%s ADUVC driver exiting\n", driverName, functionName);
    disconnect(this->pasynUserSelf);
}



//-------------------------------------------------------------
// ADUVC ioc shell registration
//-------------------------------------------------------------

/* UVCConfig -> These are the args passed to the constructor in the epics config function */
static const iocshArg UVCConfigArg0 = { "Port name",        iocshArgString };
static const iocshArg UVCConfigArg1 = { "Serial number",    iocshArgString };
static const iocshArg UVCConfigArg2 = { "Product ID",       iocshArgInt };
static const iocshArg UVCConfigArg3 = { "Framerate",        iocshArgInt };
static const iocshArg UVCConfigArg4 = { "XSize",            iocshArgInt };
static const iocshArg UVCConfigArg5 = { "YSize",            iocshArgInt };
static const iocshArg UVCConfigArg6 = { "maxBuffers",       iocshArgInt };
static const iocshArg UVCConfigArg7 = { "maxMemory",        iocshArgInt };
static const iocshArg UVCConfigArg8 = { "priority",         iocshArgInt };
static const iocshArg UVCConfigArg9 = { "stackSize",        iocshArgInt };


/* Array of config args */
static const iocshArg * const UVCConfigArgs[] =
        { &UVCConfigArg0, &UVCConfigArg1, &UVCConfigArg2,
        &UVCConfigArg3, &UVCConfigArg4, &UVCConfigArg5,
        &UVCConfigArg6, &UVCConfigArg7, &UVCConfigArg8, &UVCConfigArg9 };


/* what function to call at config */
static void configUVCCallFunc(const iocshArgBuf *args) {
    ADUVCConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival,
            args[4].ival, args[5].ival, args[6].ival, args[7].ival, args[8].ival, args[9].ival);
}


/* information about the configuration function */
static const iocshFuncDef configUVC = { "ADUVCConfig", 9, UVCConfigArgs };


/* IOC register function */
static void UVCRegister(void) {
    iocshRegister(&configUVC, configUVCCallFunc);
}


/* external function for IOC register */
extern "C" {
    epicsExportRegistrar(UVCRegister);
}
