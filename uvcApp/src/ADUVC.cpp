/*
 * Main source code for ADUVC EPICS driver.
 *
 * This file contains the code for the implementation of the
 * destructor, constructor, and methods for ADUVC
 *
 * Author: Jakub Wlodek
 * Created: July 2018
 * Last Edited: August 2020
 * Copyright (c): 2018-2020 Brookhaven National Laboratory
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


// Local ADUVC include
#include "ADUVC.h"


// libuvc includes
#include <libuvc/libuvc.h>
#include <libuvc/libuvc_config.h>


// standard namespace
using namespace std;


// define driver name for logging
static const char* driverName = "ADUVC";

// Constants
static const double ONE_BILLION = 1.E9;



//---------------------------------------------------------
// ADUVC Utility functions
//---------------------------------------------------------


/**
 * External configuration function for ADUVC.
 * Envokes the constructor to create a new ADUVC object
 * This is the function that initializes the driver, and is called in the IOC startup script
 *
 * @params[in]: all passed into constructor
 * @return: status
 */
extern "C" int ADUVCConfig(const char* portName, const char* serial, 
        int productID, int framerate, int xsize, int ysize, 
        int maxBuffers, size_t maxMemory, int priority, int stackSize){

    new ADUVC(portName, serial, productID, framerate, 
            xsize, ysize, maxBuffers, maxMemory, priority, stackSize);

    return asynSuccess;
}


/**
 * Callback function called when IOC is terminated.
 * Deletes created object and frees UVC context
 *
 * @params[in]: pPvt -> pointer to the ADUVC object created in ADUVCConfig
 */
static void exitCallbackC(void* pPvt){
    ADUVC* pUVC = (ADUVC*) pPvt;
    delete(pUVC);
}


/**
 * Function used to display UVC errors
 *
 * @params[in]: status          -> uvc error passed to function
 * @params[in]: functionName    -> name of function in which uvc error occurred
 * return: void
 */
void ADUVC::reportUVCError(uvc_error_t status, const char* functionName){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s UVC Error: %s\n", 
                driverName, functionName, uvc_strerror(status));

    if(status != UVC_ERROR_OTHER){
        char errorMessage[25];
        epicsSnprintf(errorMessage, sizeof(errorMessage), 
            "UVC Error: %s", uvc_strerror(status));

        updateStatus(errorMessage);
    }
}


/**
 * Function that writes to ADStatus PV
 * 
 * @params[in]: status -> message to write to ADStatus PV
 */
void ADUVC::updateStatus(const char* status){
    if(strlen(status) >= 25) return;

    char statusMessage[25];
    epicsSnprintf(statusMessage, sizeof(statusMessage), "%s", status);
    setStringParam(ADStatusMessage, statusMessage);
    callParamCallbacks();
}


//-----------------------------------------------
// ADUVC Camera Format selector functions
//-----------------------------------------------


/**
 * External C function pulled from libuvc_internal diagnostics.
 * Simply returns a string for a given subtype
 * 
 * @return: string representing certain subtype identifier
 */
extern "C" const char* get_string_for_subtype(uint8_t subtype) {
    switch (subtype) {
        case UVC_VS_FORMAT_UNCOMPRESSED:
            return "UncompressedFormat";
        case UVC_VS_FORMAT_MJPEG:
            return "MJPEGFormat";
        default:
            return "Unknown";
    }
}


/**
 * Function that updates the description for a selected camera format
 */
void ADUVC::updateCameraFormatDesc(){
    const char* functionName = "updateCameraFormatDesc";
    int selectedFormat;
    getIntegerParam(ADUVC_CameraFormat, &selectedFormat);

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Updating Format Description\n", driverName, functionName);
    char description[256];
    epicsSnprintf(description, sizeof(description), "%s", this->supportedFormats[selectedFormat].formatDesc);
    setStringParam(ADUVC_FormatDescription, description);
    updateStatus("Updated format Desc.");

    callParamCallbacks();
}


/**
 * Function that handles applying a selected supported camera format to the approptiate PVs.
 * Sets the data type, color mode, framerate, xsize, ysize, and frame format PVs.
 */
void ADUVC::applyCameraFormat(){
    const char* functionName = "applyCameraFormat";
    int selectedFormat;
    getIntegerParam(ADUVC_CameraFormat, &selectedFormat);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Applying Format\n", driverName, functionName);
    ADUVC_CamFormat_t format = this->supportedFormats[selectedFormat];

    if( (int) format.frameFormat < 0){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s::%s Cannot apply format - is not used\n", driverName, functionName);
    }
    else {
        setIntegerParam(NDDataType, format.dataType);
        setIntegerParam(NDColorMode, format.colorMode);
        setIntegerParam(ADUVC_Framerate, format.framerate);
        setIntegerParam(ADSizeX, format.xSize);
        setIntegerParam(ADSizeY, format.ySize);
        setIntegerParam(ADUVC_ImageFormat, format.frameFormat);
    }

    setIntegerParam(ADUVC_ApplyFormat, 0);
    updateStatus("Applied format");
    callParamCallbacks();
}


/**
 * Function that reads all supported camera formats into a ADUVC_CamFormat_t struct, and then
 * selects the ones that are most likely to be useful for easy selection.
 * 
 * @return: asynSuccess if successful, asynError if error
 */
asynStatus ADUVC::readSupportedCameraFormats(){
    const char* functionName = "readSupportedCameraFormats";
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
        "%s::%s Reading in supported camera formats\n", driverName, functionName);
    asynStatus status = asynSuccess;
    ADUVC_CamFormat_t* formatBuffer = (ADUVC_CamFormat_t*) calloc(1, 64 * sizeof(ADUVC_CamFormat_t));
    int bufferIndex = 0;

    if(this->pdeviceHandle != NULL){
        uvc_streaming_interface_t* interfaces = this->pdeviceHandle->info->stream_ifs;
        while(interfaces != NULL){
            uvc_format_desc_t* interfaceFormats = interfaces->format_descs;
            while(interfaceFormats != NULL) {
                uvc_frame_desc_t* frameFormats = interfaceFormats->frame_descs;
                while(frameFormats != NULL) {
                    populateCameraFormat(&formatBuffer[bufferIndex], interfaceFormats, frameFormats);
                    bufferIndex++;
                    frameFormats = frameFormats->next;
                }
                interfaceFormats = interfaceFormats->next;
            }
            interfaces = interfaces->next;
        }
    }
    else{
        status = asynError;
    }

    int formatIndex = selectBestCameraFormats(formatBuffer, bufferIndex);
    free(formatBuffer);

    int i;
    for(i = formatIndex; i < SUPPORTED_FORMAT_COUNT; i++){
        initEmptyCamFormat(i);
    }

    return status;
}


/**
 * Function that takes a format descriptor and a frame descriptor, and populates a camera format
 * structure with the appropriate values.
 * 
 * @params[out]: camFormat      -> Output camFormat struct
 * @params[in]:  format_desc    -> Format descriptor struct
 * @params[in]:  frame_desc     -> Frame descriptor struct
 * @return: void
 */
void ADUVC:: populateCameraFormat(ADUVC_CamFormat_t* camFormat, 
        uvc_format_desc_t* format_desc, uvc_frame_desc_t* frame_desc){

    const char* functionName = "populateCameraFormat";

    switch(format_desc->bDescriptorSubtype){
        case UVC_VS_FORMAT_MJPEG:
            camFormat->frameFormat  = ADUVC_FrameMJPEG;
            camFormat->dataType     = NDUInt8;
            camFormat->colorMode    = NDColorModeRGB1;
            break;
        case UVC_VS_FORMAT_UNCOMPRESSED:
            camFormat->frameFormat  = ADUVC_FrameUncompressed;
            camFormat->dataType     = NDUInt16;
            camFormat->colorMode    = NDColorModeMono;
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s Unsupported format desc.\n", driverName, functionName);
            break;
    }

    camFormat->xSize = frame_desc->wWidth;
    camFormat->ySize = frame_desc->wHeight;
    camFormat->framerate = 10000000 / frame_desc->dwDefaultFrameInterval;

    epicsSnprintf(camFormat->formatDesc, SUPPORTED_FORMAT_DESC_BUFF, "%s, X: %d, Y: %d, Rate: %d/s", 
        get_string_for_subtype(format_desc->bDescriptorSubtype), (int) camFormat->xSize, 
        (int) camFormat->ySize, camFormat->framerate);
}


/**
 * Function that initializes a Camera Format struct with empty placeholder
 * 
 * @params[in]: arrayIndex -> index in array of supported formats 
 * @return: void
 */
void ADUVC::initEmptyCamFormat(int arrayIndex){
    epicsSnprintf(this->supportedFormats[arrayIndex].formatDesc, SUPPORTED_FORMAT_DESC_BUFF, "Unused Camera Format");
    this->supportedFormats[arrayIndex].frameFormat = ADUVC_FrameUnsupported;
}


/**
 * Function that compares two camera format structs against one another
 * 
 * @params[in]: cameraFormat1   -> first camera format struct
 * @params[in]: cameraFormat2   -> second camera format struct
 * @return: 0 if the two structs are identical, -1 if they are not
 */
int ADUVC::compareFormats(ADUVC_CamFormat_t camFormat1, ADUVC_CamFormat_t camFormat2){
    if (camFormat1.xSize        != camFormat2.xSize)        return -1;
    if (camFormat1.ySize        != camFormat2.ySize)        return -1;
    if (camFormat1.colorMode    != camFormat2.colorMode)    return -1;
    if (camFormat1.dataType     != camFormat2.dataType)     return -1;
    if (camFormat1.framerate    != camFormat2.framerate)    return -1;
    if (camFormat1.frameFormat  != camFormat2.frameFormat)  return -1;
    return 0;
}


/**
 * Function that checks if a Cam format struct is already in the camera's format array
 * 
 * @params[in]: camFormat -> format to test
 * @return: true if it is in the supportedFormats array, false otherwise
 */
bool ADUVC::formatAlreadySaved(ADUVC_CamFormat_t camFormat){
    int i;
    for(i = 0; i< SUPPORTED_FORMAT_COUNT; i++){
        if(compareFormats(camFormat, this->supportedFormats[i]) == 0){
            return true;
        }
    }
    return false;
}



/**
 * Function that selects the best discovered formats from the set of all discovered formats.
 * The current order of importance is MJPEG > Uncompressed (Higher end devices only have uncompressed,
 * and lower end devices exhibit poor performance when using Uncompressed), then image resolution,
 * then framerate.
 * 
 * @params[in]: formatBuffer    -> Buffer containing all detected camera formats
 * @params[in]: numFormats      -> counter for how many formats were detected
 * @return: readFormats         -> The number of formats read from the formatBuffer
 */
int ADUVC::selectBestCameraFormats(ADUVC_CamFormat_t* formatBuffer, int numFormats){
    const char* functionName = "selectBestCameraFormats";
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
        "%s::%s Selecting best camera formats\n", driverName, functionName);

    int readFormats = 0;
    while(readFormats < SUPPORTED_FORMAT_COUNT && readFormats != numFormats){
        int bestFormatIndex = 0;
        int i;
        for(i = 0; i< numFormats; i++){
            if(!formatAlreadySaved(formatBuffer[i])){
                if(formatBuffer[i].frameFormat == ADUVC_FrameMJPEG && formatBuffer[bestFormatIndex].frameFormat != ADUVC_FrameMJPEG){
                    bestFormatIndex = i;
                }
                else if(formatBuffer[i].xSize > formatBuffer[bestFormatIndex].xSize){
                    bestFormatIndex = i;
                }
                else if(formatBuffer[i].framerate > formatBuffer[bestFormatIndex].framerate){
                    bestFormatIndex = i;
                }
            }
        }
        this->supportedFormats[readFormats].colorMode       = formatBuffer[bestFormatIndex].colorMode;
        this->supportedFormats[readFormats].dataType        = formatBuffer[bestFormatIndex].dataType;
        this->supportedFormats[readFormats].frameFormat     = formatBuffer[bestFormatIndex].frameFormat;
        this->supportedFormats[readFormats].framerate       = formatBuffer[bestFormatIndex].framerate;
        this->supportedFormats[readFormats].xSize           = formatBuffer[bestFormatIndex].xSize;
        this->supportedFormats[readFormats].ySize           = formatBuffer[bestFormatIndex].ySize;

        memcpy(this->supportedFormats[readFormats].formatDesc, formatBuffer[bestFormatIndex].formatDesc, SUPPORTED_FORMAT_DESC_BUFF);
        if(readFormats == 0){
            setIntegerParam(ADMaxSizeX, formatBuffer[bestFormatIndex].xSize);
            setIntegerParam(ADMaxSizeY, formatBuffer[bestFormatIndex].ySize);
        }
        
        readFormats++;
    }
    return readFormats;
}


//-----------------------------------------------
// ADUVC connection/disconnection functions
//-----------------------------------------------


/**
 * Ovderride of NDDriver base function, calls connect to camera
 * 
 * @params[in]: pasynUser -> asyn User instance for the driver
 */
asynStatus ADUVC::connect(asynUser* pasynUser) {
    return connectToDeviceUVC();
}


/**
 * Function responsible for connecting to the UVC device. First, a device context is created,
 * then the device is identified, then opened.
 * NOTE: this driver must have exclusive access to the device as per UVC standards.
 *
 * @return: asynStatus      -> true if connection is successful, false if failed
 */
asynStatus ADUVC::connectToDeviceUVC(){
    static const char* functionName = "connectToDeviceUVC";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_init(&pdeviceContext, NULL);

    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s Initialized UVC context\n", driverName, functionName);
    }
    if(this->connectionType == 0){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s Searching for UVC device with serial number: %s\n", driverName, functionName, serialNumber);
        deviceStatus = uvc_find_device(pdeviceContext, &pdevice, 0, 0, this->serialNumber);
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s Searching for UVC device with Product ID: %d\n", driverName, functionName, this->productID);
        deviceStatus = uvc_find_device(pdeviceContext, &pdevice, 0, this->productID, NULL);
    }
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s Found UVC device\n", driverName, functionName);
    }
    deviceStatus = uvc_open(pdevice, &pdeviceHandle);
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        connected = 1;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s Opened UVC device\n", driverName, functionName);
    }
    return status;
}


asynStatus ADUVC::disconnect(asynUser* pasynUser) {
    return disconnectFromDeviceUVC();
}


/**
 * Function that disconnects from a connected UVC device.
 * Closes device handle and context, and unreferences the device pointer from memory
 * 
 * @return: asynStatus -> success if device existed and was disconnected, error if there was no device connected
 */
asynStatus ADUVC::disconnectFromDeviceUVC(){
    const char* functionName = "disconnectFromDeviceUVC";
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, 
        "%s::%s Calling all free functions for ADUVC\n", driverName, functionName);

    if(connected == 1){
        uvc_close(pdeviceHandle);
        uvc_unref_device(pdevice);
        uvc_exit(pdeviceContext);
        printf("Disconnected from device.\n");
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
    uint8_t powerLineFrequency, panSpeed, tiltSpeed;
    int8_t pan, tilt;

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
    uvc_get_pantilt_rel(pdeviceHandle, &pan, &panSpeed, &tilt, &tiltSpeed, UVC_GET_CUR);
    uvc_get_zoom_abs(pdeviceHandle, &(this->zoomMin), UVC_GET_MIN);
    uvc_get_zoom_abs(pdeviceHandle, &(this->zoomMax), UVC_GET_MAX);
    this->zoomStepSize = (int) ((this->zoomMax - this->zoomMin) / this->zoomSteps);
    this->currentZoom = this->zoomMin;


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
    setIntegerParam(ADUVC_PanSpeed, (int) panSpeed);
    setIntegerParam(ADUVC_TiltSpeed, (int) tiltSpeed);
    
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
    sprintf(modelName, "%s", pdeviceInfo->product);
    setIntegerParam(ADUVC_UVCComplianceLevel, (int) pdeviceInfo->bcdUVC);
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
        case ADUVC_FrameGray8:
            return UVC_FRAME_FORMAT_GRAY8;
        case ADUVC_FrameGray16:
            return UVC_FRAME_FORMAT_GRAY16;
        case ADUVC_FrameUYVY:
            return UVC_FRAME_FORMAT_UYVY;
        case ADUVC_FrameUncompressed:
            return UVC_FRAME_FORMAT_UNCOMPRESSED;
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
 * @params[in]: imageFormat -> type of image format to use
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

    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, 
            "%s::%s Starting acquisition: x-size: %d, y-size %d, framerate %d\n", 
            driverName, functionName, xsize, ysize, framerate);
        
    deviceStatus = uvc_get_stream_ctrl_format_size(pdeviceHandle, &deviceStreamCtrl, 
            imageFormat, xsize, ysize, framerate);

    if(imageFormat == UVC_FRAME_FORMAT_UNCOMPRESSED) printf("opening stream for uncompressed\n");

    if(deviceStatus<0){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Cannot start acquisition invalid frame format\n", driverName, functionName);
        deviceStatus = UVC_ERROR_NOT_SUPPORTED;
        reportUVCError(deviceStatus, functionName);
        setIntegerParam(ADAcquire, 0);
        setIntegerParam(ADStatus, ADStatusIdle);
        callParamCallbacks();
        return deviceStatus;
    }
    else{
        setIntegerParam(ADNumImagesCounter, 0);
        callParamCallbacks();
    
        // Here is where we initialize the stream and set the callback function 
        // to the static wrapper and pass 'this' as the void pointer.
        // This function is in the form:
        // uvc_start_streaming(device_handle, device_stream_ctrl*, Callback Function* , void*, int)
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

        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, 
                "%s::%s Image aquisition start\n", driverName, functionName);

        callParamCallbacks();
        }
    }

    updateStatus("Started acquisition");
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

    // stop_streaming will block until last callback is processed.
    uvc_stop_streaming(pdeviceHandle);

    // reset the validatedFrameSize flag
    this->validatedFrameSize = false;

    //update PV values
    setIntegerParam(ADStatus, ADStatusIdle);
    setIntegerParam(ADAcquire, 0);
    callParamCallbacks();
    updateStatus("Stopped acquisition");

    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s::%s Stopping aquisition\n", driverName, functionName);
}



//-------------------------------------------------------
// UVC Image Processing and callback functions
//-------------------------------------------------------


/**
 * Function that is meant to adjust the NDDataType and NDColorMode. First check if current settings
 * are already valid. If not then attempt to adjust them to fit the frame recieved from the camera.
 * If able to adjust properly to fit the frame size, then set a validated tag to true - only compute
 * on the first frame on acquisition start.
 * 
 * @params[in]: frame   -> pointer to frame recieved from the camera
 */
void ADUVC::checkValidFrameSize(uvc_frame_t* frame){
    // if user has auto adjust toggled off, skip this.
    int adjust;
    getIntegerParam(ADUVC_AutoAdjust, &adjust);

    if(adjust == 0){
        this->validatedFrameSize = true;
        return;
    }

    const char* functionName = "checkValidFrameSize";
    int reg_sizex, reg_sizey, colorMode, dataType;
    getIntegerParam(NDColorMode, &colorMode);
    getIntegerParam(NDDataType, &dataType);
    getIntegerParam(ADSizeX, &reg_sizex);
    getIntegerParam(ADSizeY, &reg_sizey);

    int computedBytes = reg_sizex * reg_sizey;
    if((NDDataType_t) dataType == NDUInt16 || (NDDataType_t) dataType == NDInt16)
        computedBytes = computedBytes * 2;
    if((NDColorMode_t) colorMode == NDColorModeRGB1)
        computedBytes = computedBytes * 3;

    int num_bytes = frame->data_bytes;
    if(computedBytes == num_bytes){
        this->validatedFrameSize = true;
        return;
    }

    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s::%s Selected dtype and color mode incompatible, attempting to auto-adjust.\n", 
        driverName, functionName);

    int xsize = frame->width;
    int ysize = frame->height;
    int res   = num_bytes / xsize;
    res       = res / ysize;
    switch(res) {
        case 2:
            // num bytes / (xsize * ysize) = 2 means a 16 bit mono image. 2 bytes per pixel
            setIntegerParam(NDColorMode,    NDColorModeMono);
            setIntegerParam(NDDataType,     NDUInt16);
            break;
        case 3:
            // num bytes / (xsize * ysize) = 3 means 8 bit rgb image. 1 byte per pixel per 3 colors.
            setIntegerParam(NDColorMode,    NDColorModeRGB1);
            setIntegerParam(NDDataType,     NDUInt8);
            break;
        case 6:
            // num bytes / (xsize * ysize) = 6 means 16 bit rgb image. 2 bytes per pixel per 3 colors
            setIntegerParam(NDColorMode,    NDColorModeRGB1);
            setIntegerParam(NDDataType,     NDUInt16);
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Couldn't validate frame size.\n", driverName, functionName);
            return;
    }

    this->validatedFrameSize = true;
}


/*
 * Function used as a wrapper function for the callback.
 * This is necessary beacuse the callback function must be static for libuvc, meaning that function calls
 * inside the callback function can only be run by using the driver name. Because libuvc allows for a void*
 * to be passed through the callback function, however, the solution is to pass 'this' as the void pointer,
 * cast it as an ADUVC pointer, and simply run the non-static callback function with that object.
 * 
 * @params[in]:  frame   -> pointer to uvc_frame received
 * @params[out]: ptr     -> 'this' cast as a void pointer. It is cast back to ADUVC object and then new frame callback is called
 * @return: void
 */
void ADUVC::newFrameCallbackWrapper(uvc_frame_t* frame, void* ptr){
    ADUVC* pPvt = ((ADUVC*) ptr);
    pPvt->newFrameCallback(frame, pPvt);
}


/*
 * Function responsible for converting between a uvc_frame_t type image to
 * the EPICS area detector standard NDArray type. First, we convert any given uvc_frame to
 * the rgb format. This is because all of the supported uvc formats can be converted into this type,
 * simplyfying the conversion process. Then, the NDDataType is taken into account, and a scaling factor is used
 *
 * @params[in]:  frame       -> frame collected from the uvc camera
 * @params[out]: pArray      -> output of function. NDArray conversion of uvc frame
 * @params[in]:  dataType    -> data type of NDArray output image
 * @params[in]:  colorMode   -> image color mode. So far only RGB1 is supported
 * @params[in]:  imBytes     -> number of bytes in the image
 * @return: void, but output into pArray
 */
asynStatus ADUVC::uvc2NDArray(uvc_frame_t* frame, NDArray* pArray, 
            NDDataType_t dataType, NDColorMode_t colorMode, size_t imBytes){

    static const char* functionName = "uvc2NDArray";
    asynStatus status = asynSuccess;
    // if data is grayscale, we do not need to convert it, we just copy over the data.
    
    if(colorMode == NDColorModeMono){
        if(frame->data_bytes != imBytes){
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                    "%s::%s Error invalid frame size. Frame has %d bytes and array has %d bytes\n", 
                    driverName, functionName, (int) frame->data_bytes, (int) imBytes);

            status = asynError;
        }
        else{
            if(dataType == NDUInt8 || dataType == NDInt8){
                memcpy((unsigned char*) pArray->pData,(unsigned char*) frame->data, imBytes);
            }
            else memcpy((uint16_t*) pArray->pData, (uint16_t*) frame->data, imBytes);
        }
    }
    //otherwise we need to convert to a common type (rgb)
    else{
        // non grayscale images have 3 channels, so frame size * 3
        uvc_frame_t* rgb = uvc_allocate_frame(frame->width * frame->height *3);
        
        if(!rgb){
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                    "%s::%s ERROR: Unable to allocate frame\n", driverName, functionName);

            status = asynError;
        }
        else{
            // convert any uvc frame format to rgb
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
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                            "%s::%s ERROR: Unsupported UVC format\n", driverName, functionName);
                    
                    uvc_free_frame(rgb);
                    status = asynError;
            }
        
            if(status != asynError){
                if(deviceStatus<0){
                    reportUVCError(deviceStatus, functionName);
                    status = asynError;
                }
                else{
                    if(rgb->data_bytes != imBytes){
                        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                        "%s::%s Error invalid frame sizeFrame has %d bytes and array has %d bytes\n", 
                        driverName, functionName, (int) rgb->data_bytes, (int) imBytes);

                        status = asynError;
                    }
                    else{
                        memcpy(pArray->pData, rgb->data, imBytes);
                    }
                }
                uvc_free_frame(rgb);
            }
        }
    }

    // only push image if the data transfer was successful
    if(status == asynSuccess){
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
    }
    
    // Always free array whether successful or not
    pArray->release();
    return status;
}


/*
 * Function that performs the callbacks onto new frames generated by the camera.
 * First, a new NDArray pointer is allocated, then the given uvc_frame pointer is converted
 * into this allocated NDArray. Then, based on the operating mode, the acquisition moves to the 
 * next frame, or stops. The NDArray is passed on to the doCallbacksGenericPointer function.
 *
 * @params[in]: frame   -> uvc_frame recieved from the camera
 * @params[in]: ptr     -> void pointer with data from the frame
 * @return: void
 */
void ADUVC::newFrameCallback(uvc_frame_t* frame, void* ptr){
    NDArray* pArray;
    NDArrayInfo arrayInfo;
    int dataType;
    int operatingMode;
    int colorMode;
    int ndims;

    //TODO: Timestamps for images
    //epicsTimeStamp currentTime;
    static const char* functionName = "newFrameCallback";

    // Check to see if frame size matches.
    // If not, adjust color mode and data type to try and fit frame.
    // **ONLY FOR UNCOMPRESSED FRAMES - otherwise byte sizes will not match **
    if(!this->validatedFrameSize && getFormatFromPV() == UVC_FRAME_FORMAT_UNCOMPRESSED)
        checkValidFrameSize(frame);

    getIntegerParam(NDColorMode, &colorMode);
    getIntegerParam(NDDataType, &dataType);

    if((NDColorMode_t) colorMode == NDColorModeMono) ndims = 2;
    else ndims = 3;
    
    size_t dims[ndims];
    if(ndims == 2){
        dims[0] = frame->width;
        dims[1] = frame->height;
    }
    else{
        dims[0] = 3;
        dims[1] = frame->width;
        dims[2] = frame->height;
    }

    getIntegerParam(ADImageMode, &operatingMode);

    // allocate memory for a new NDArray, and set pArray to a pointer for this memory
    this->pArrays[0] = pNDArrayPool->alloc(ndims, dims, (NDDataType_t) dataType, 0, NULL);
    
    if(this->pArrays[0]!=NULL){ 
        pArray = this->pArrays[0];   
    }
    else{
        this->pArrays[0]->release();
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s Unable to allocate array\n", driverName, functionName);
        return;
    }

    updateTimeStamp(&pArray->epicsTS);

    // Update camera image parameters
    pArray->getInfo(&arrayInfo);
    setIntegerParam(NDArraySize, (int)arrayInfo.totalBytes);
    setIntegerParam(NDArraySizeX, arrayInfo.xSize);
    setIntegerParam(NDArraySizeY, arrayInfo.ySize);

    int numImages;
    getIntegerParam(ADNumImagesCounter, &numImages);
    numImages++;
    setIntegerParam(ADNumImagesCounter, numImages);
    pArray->uniqueId = numImages;
    
    // Copy data from our uvc frame into our NDArray
    uvc2NDArray(frame, pArray, (NDDataType_t) dataType, (NDColorMode_t) colorMode, arrayInfo.totalBytes);

    //single shot mode stops after one images
    if(operatingMode == ADImageSingle){
        acquireStop();
    }

    // block shot mode stops once numImages reaches the number of desired images
    else if(operatingMode == ADImageMultiple){
        int desiredImages;
        getIntegerParam(ADNumImages, &desiredImages);
        
        if(numImages>=desiredImages){
            acquireStop();
        }
    }

    // Otherwise, if continuous mode, just keep looping, if not then we have an unsupported mode
    else if(operatingMode != ADImageContinuous){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s ERROR: Unsupported operating mode\n", driverName, functionName);

        acquireStop();
    }
    
}



//---------------------------------------------------------
// Base UVC Camera functionality
//---------------------------------------------------------


/**
 * Function that sets exposure time in seconds
 * 
 * @params[in]: exposureTime -> the exposure time in seconds
 * @return: status
 */
asynStatus ADUVC::setExposure(int exposureTime){

    if(this->pdevice == NULL) return asynError;

    const char* functionName = "setExposure";
    asynStatus status = asynSuccess;
    updateStatus("Set Exposure");
    
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
 * @params[in]: gamma -> value for gamma
 * @return: status
 */
asynStatus ADUVC::setGamma(int gamma){

    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setGamma";
    asynStatus status = asynSuccess;
    updateStatus("Set Gamma");
    
    deviceStatus = uvc_set_gamma(pdeviceHandle, gamma);
    
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    
    }
    return status;
}


/**
 * Function that sets backlight compensation. 
 * Used when camera has a light behind it that oversaturates image
 * 
 * @params[in]: backlightCompensation -> degree of backlight comp.
 * @return: status
 */
asynStatus ADUVC::setBacklightCompensation(int backlightCompensation){
    
    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setBacklightCompensation";
    asynStatus status = asynSuccess;
    updateStatus("Set Backlight Comp.");
    
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
 * @params[in]: brighness -> degree of brightness: for example, a 1.5 value would change a pixel value 30 to 45
 * @return: status
 */
asynStatus ADUVC::setBrightness(int brightness){
    
    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setBrightness";
    asynStatus status = asynSuccess;
    updateStatus("Set Brightness");
    
    deviceStatus = uvc_set_brightness(pdeviceHandle, brightness);
    
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    
    return status;
}


/**
 * Function that sets value for contrast.
 * Higher value increases difference between whites and blacks
 * 
 * @params[in]: contrast -> level of contrast
 * @return: status
 */
asynStatus ADUVC::setContrast(int contrast){
    
    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setContrast";
    asynStatus status = asynSuccess;
    updateStatus("Set Contrast");
    
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
 * @params[in]: gain -> value for gain
 * @return: status
 */
asynStatus ADUVC::setGain(int gain){
    
    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setGain";
    asynStatus status = asynSuccess;
    updateStatus("Set Gain");
    
    deviceStatus = uvc_set_gain(pdeviceHandle, gain);
    
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    
    return status;
}


/**
 * Function that sets Power line frequency. 
 * Used to avoid camera flicker when signal is not correct
 * 
 * @params[in]: powerLineFrequency -> frequency value (50Hz, 60Hz etc.)
 * @return status
 */
asynStatus ADUVC::setPowerLineFrequency(int powerLineFrequency){
    
    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setPowerLineFrequency";
    asynStatus status = asynSuccess;
    updateStatus("Set Power Line Freq.");
    
    deviceStatus = uvc_set_power_line_frequency(pdeviceHandle, powerLineFrequency);
    
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    
    return status;
}


/**
 * Function that sets camera Hue. 
 * For example, a hue of 240 will result in a blue shifted image,
 * while 0 will result in red-shifted
 * 
 * @params[in]: hue -> value for image hue or tint
 * @return: status
 */
asynStatus ADUVC::setHue(int hue){
    
    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setHue";
    asynStatus status = asynSuccess;
    
    updateStatus("Set Hue");
    
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
 * @params[in]: saturation -> degree of saturation
 * @return status
 */
asynStatus ADUVC::setSaturation(int saturation){
    
    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setSaturation";
    asynStatus status = asynSuccess;
    updateStatus("Set Saturation");
    
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
 * @params[in]: sharpness -> degree of sharpening
 * @return: status
 */
asynStatus ADUVC::setSharpness(int sharpness){
    
    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "setSharpness";
    asynStatus status = asynSuccess;
    updateStatus("Set Sharpness");
    
    deviceStatus = uvc_set_sharpness(pdeviceHandle, sharpness);
    
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    
    return status;
}


/**
 * Function that adjust camera pan/tilt options if supported
 * 
 * @params[in]: panDirection -> -1, 0, 1 - left, stay, right
 * @params[in]: tiltDirection -> -1, 0, 1 - up stay down
 * @return: status
 */
asynStatus ADUVC::processPanTilt(int panDirection, int tiltDirection){
    
    int panSpeed, tiltSpeed;
    double panTiltStepTime;
    getIntegerParam(ADUVC_PanSpeed, &panSpeed);
    getIntegerParam(ADUVC_TiltSpeed, &tiltSpeed);
    getDoubleParam(ADUVC_PanTiltStep, &panTiltStepTime);

    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "processPanTilt";
    asynStatus status = asynSuccess;
    
    deviceStatus = uvc_set_pantilt_rel(pdeviceHandle, (int8_t) panDirection, (uint8_t) panSpeed,
                                                        (int8_t) tiltDirection, (uint8_t) tiltSpeed);
    
    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }

    else{
        epicsThreadSleep(panTiltStepTime);
        deviceStatus = uvc_set_pantilt_rel(pdeviceHandle, 0, (uint8_t) panSpeed,
                                                    0, (uint8_t) tiltSpeed);
    

        if(deviceStatus == UVC_SUCCESS) updateStatus("Processed Pan/Tilt");
        else{
            reportUVCError(deviceStatus, functionName);
            status = asynError;
        }
    }

    return status;
}


/**
 * Function that sets the degree of sharpening. Too high will cause oversharpening
 * 
 * @params[in]: sharpness -> degree of sharpening
 * @return: status
 */
asynStatus ADUVC::processZoom(int zoomDirection){

    if(this->pdevice == NULL) return asynError;
    
    const char* functionName = "processZoom";
    
    asynStatus status = asynSuccess;

    if(zoomDirection == 1){
        currentZoom += zoomStepSize;
        if(currentZoom > zoomMax) currentZoom = zoomMax;
    }
    else{
        currentZoom -= zoomStepSize;
        if(currentZoom < zoomMin) currentZoom = zoomMin;
    }
    
    //deviceStatus = uvc_set_zoom_rel(pdeviceHandle, 1, , 10);
    deviceStatus = uvc_set_zoom_abs(pdeviceHandle, currentZoom);


    if(deviceStatus < 0){
        reportUVCError(deviceStatus, functionName);
        status = asynError;
    }
    else{
        updateStatus("Processed Zoom");
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
 * @params[in]: pasynUser       -> asyn client who requests a write
 * @params[in]: value           -> int32 value to write
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
    else if(function == ADUVC_ApplyFormat && value == 1){
        if(acquiring)
            acquireStop();
        
        applyCameraFormat();
    }
    else if(function == ADUVC_CameraFormat) updateCameraFormatDesc();
    //switch image mode
    else if(function == ADImageMode){
        if(acquiring == 1) acquireStop();
        if(value == ADImageSingle) setIntegerParam(ADNumImages, 1);
        else if(value == ADImageMultiple) setIntegerParam(ADNumImages, 300);
        else{
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                    "%s::%s ERROR: Unsupported camera operating mode\n", 
                    driverName, functionName);

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
    else if(function == ADUVC_PanLeft) processPanTilt(-1, 0);
    else if(function == ADUVC_PanRight) processPanTilt(1, 0);
    else if(function == ADUVC_TiltUp) processPanTilt(0, 1);
    else if(function == ADUVC_TiltDown) processPanTilt(0, -1);
    else if(function == ADUVC_ZoomIn) processZoom(1);
    else if(function == ADUVC_ZoomOut) processZoom(-1);
    else{
        if (function < ADUVC_FIRST_PARAM) {
            status = ADDriver::writeInt32(pasynUser, value);
        }
    }

    callParamCallbacks();

    if(status){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s ERROR status=%d, function=%d, value=%d\n", 
                driverName, functionName, status, function, value);

        return asynError;
    }
    else asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                "%s::%s function=%d value=%d\n", 
                driverName, functionName, function, value);
    
    return asynSuccess;
}


/*
 * Function overwriting ADDriver base function.
 * Takes in a function (PV) changes, and a value it is changing to, and processes the input
 * This is the same functionality as writeInt32, but for processing doubles.
 *
 * @params[in]: pasynUser       -> asyn client who requests a write
 * @params[in]: value           -> int32 value to write
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
        asynPrint(this-> pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s ERROR status = %d, function =%d, value = %f\n", 
                driverName, functionName, status, function, value);

        return asynError;
    }
    else asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                "%s::%s function=%d value=%f\n", 
                driverName, functionName, function, value);

    return asynSuccess;
}


/*
 * Function used for reporting ADUVC device and library information to a external
 * log file. The function first prints all libuvc specific information to the file,
 * then continues on to the base ADDriver 'report' function
 * 
 * @params[in]: fp      -> pointer to log file
 * @params[in]: details -> number of details to write to the file
 * @return: void
 */
void ADUVC::report(FILE* fp, int details){
    const char* functionName = "report";
    int framerate;
    int height;
    int width;

    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, 
                "%s::%s reporting to external log file\n", 
                driverName, functionName);

    if(details > 0){
        fprintf(fp, " LIBUVC Version        ->      %d.%d.%d\n", 
                LIBUVC_VERSION_MAJOR, LIBUVC_VERSION_MINOR, LIBUVC_VERSION_PATCH);
        fprintf(fp, " -----------------------------------------------------\n");
        
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

        fprintf(fp, " --------------------------------------------\n\n");
        
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
 * @params[in]: portName    -> port for NDArray recieved from camera
 * @params[in]: serial      -> serial number of device to connect to
 * @params[in]: productID   -> id of device used to connect if serial is unavailable
 * @params[in]: framerate   -> framerate at which camera should operate
 * @params[in]: maxBuffers  -> max buffer size for NDArrays
 * @params[in]: maxMemory   -> maximum memory allocated for driver
 * @params[in]: priority    -> what thread priority this driver will execute with
 * @params[in]: stackSize   -> size of the driver on the stack
 */
ADUVC::ADUVC(const char* portName, const char* serial, int productID, 
            int framerate, int xsize, int ysize, int maxBuffers, 
            size_t maxMemory, int priority, int stackSize)
    : ADDriver(portName, 1, (int)NUM_UVC_PARAMS, maxBuffers, 
                maxMemory, asynEnumMask, asynEnumMask, 
                ASYN_CANBLOCK, 1, priority, stackSize){

    static const char* functionName = "ADUVC";

    // Create PV Params
    createParam(ADUVC_UVCComplianceLevelString,     asynParamInt32,     &ADUVC_UVCComplianceLevel);
    createParam(ADUVC_ReferenceCountString,         asynParamInt32,     &ADUVC_ReferenceCount);
    createParam(ADUVC_FramerateString,              asynParamInt32,     &ADUVC_Framerate);
    createParam(ADUVC_ImageFormatString,            asynParamInt32,     &ADUVC_ImageFormat);
    createParam(ADUVC_CameraFormatString,           asynParamInt32,     &ADUVC_CameraFormat);
    createParam(ADUVC_FormatDescriptionString,      asynParamOctet,     &ADUVC_FormatDescription);
    createParam(ADUVC_ApplyFormatString,            asynParamInt32,     &ADUVC_ApplyFormat);
    createParam(ADUVC_AutoAdjustString,             asynParamInt32,     &ADUVC_AutoAdjust);
    createParam(ADUVC_BrightnessString,             asynParamInt32,     &ADUVC_Brightness);
    createParam(ADUVC_ContrastString,               asynParamInt32,     &ADUVC_Contrast);
    createParam(ADUVC_PowerLineString,              asynParamInt32,     &ADUVC_PowerLine);
    createParam(ADUVC_HueString,                    asynParamInt32,     &ADUVC_Hue);
    createParam(ADUVC_SaturationString,             asynParamInt32,     &ADUVC_Saturation);
    createParam(ADUVC_GammaString,                  asynParamInt32,     &ADUVC_Gamma);
    createParam(ADUVC_BacklightCompensationString,  asynParamInt32,     &ADUVC_BacklightCompensation);
    createParam(ADUVC_SharpnessString,              asynParamInt32,     &ADUVC_Sharpness);
    createParam(ADUVC_PanLeftString,                    asynParamInt32,     &ADUVC_PanLeft);
    createParam(ADUVC_PanRightString,                    asynParamInt32,     &ADUVC_PanRight);
    createParam(ADUVC_TiltUpString,                   asynParamInt32,     &ADUVC_TiltUp);
    createParam(ADUVC_TiltDownString,                   asynParamInt32,     &ADUVC_TiltDown);
    createParam(ADUVC_ZoomInString,                   asynParamInt32,     &ADUVC_ZoomIn);
    createParam(ADUVC_ZoomOutString,                   asynParamInt32,     &ADUVC_ZoomOut);
    createParam(ADUVC_PanSpeedString,               asynParamInt32,     &ADUVC_PanSpeed);
    createParam(ADUVC_TiltSpeedString,              asynParamInt32,     &ADUVC_TiltSpeed);
    createParam(ADUVC_PanTiltStepString,            asynParamFloat64,   &ADUVC_PanTiltStep);


    // Set initial size and framerate params
    setIntegerParam(ADUVC_Framerate, framerate);
    setIntegerParam(ADSizeX, xsize);
    setIntegerParam(ADSizeY, ysize);

    // Sets serial number and productID
    if(strcmp(serial, "") == 0) setStringParam(ADSerialNumber, "No Serial Detected");
    else setStringParam(ADSerialNumber, serial);

    char pIDBuff[32];
    epicsSnprintf(pIDBuff, 32, "%d", productID);
    setStringParam(ADSerialNumber, pIDBuff);

    //sets libuvc version
    char uvcVersionString[25];
    epicsSnprintf(uvcVersionString, sizeof(uvcVersionString), 
                "%d.%d.%d", LIBUVC_VERSION_MAJOR, 
                LIBUVC_VERSION_MINOR, LIBUVC_VERSION_PATCH);

    setStringParam(ADSDKVersion, uvcVersionString);

    //sets driver version
    char versionString[25];
    epicsSnprintf(versionString, sizeof(versionString), 
                "%d.%d.%d", ADUVC_VERSION, ADUVC_REVISION, ADUVC_MODIFICATION);

    setStringParam(NDDriverVersion, versionString);
    
    // Begin to establish connection
    asynStatus connected;

    this->serialNumber = serial;
    this->productID = productID;

    // decide if connecting with serial number or productID
    if(strlen(serial) != 0){
        this->connectionType = 0;
    }
    else{
        this->connectionType = 1;
    }

    connected = connect(this->pasynUserSelf);

    // Check if connected successfully, and if so, get the device information
    if(connected == asynError){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s Connection failed, abort\n", 
                driverName, functionName);
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, 
                "%s::%s Acquiring device information\n", 
                driverName, functionName);

        readSupportedCameraFormats();
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
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s::%s ADUVC driver exiting\n", 
                driverName, functionName);

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
            args[4].ival, args[5].ival, args[6].ival, 
            args[7].ival, args[8].ival, args[9].ival);
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

