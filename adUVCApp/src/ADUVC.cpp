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

//#ifdef LINUX
#include <unistd.h>
//#endif
//WINDOWS include here

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
static int moving = 0;

static const double ONE_BILLION = 1.E9;

/*
 * External configuration function for ADUVC.
 * Envokes the constructor to create a new ADUVC object
 *
 * @params: all passed into constructor
 * @return: status
 */
extern "C" int ADUVCConfig(const char* portName, const char* serial, int framerate, int maxBuffers, size_t maxMemory, int priority, int stackSize){
    new ADUVC(portName, serial, framerate, maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/*
 * Callback function called when IOC is terminated.
 * Deletes created object
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
 * @params: status -> uvc error passed to function
 * @params: functionName -> name of function in which uvc error occurred
 * return: void
 */
void ADUVC::reportUVCError(uvc_error_t status, const char* functionName){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s UVC Error: %s\n", driverName, functionName, uvc_strerror(status));
}

/*
 * Function responsible for connecting to the UVC device. First, a device context is created,
 * then the device is identified, then opened.
 * NOTE: this driver must have exclusive access to the device as per UVC standards.
 *
 * @params: serial number of device to connect to
 * @return: asynStatus -> true if connection is successful, false if failed
 */
asynStatus ADUVC::connectToDeviceUVC(const char* serialNumber){
    static const char* functionName = "connectToDeviceUVC";
    asynStatus status = asynSuccess;
    deviceStatus = uvc_init(&pdeviceContext, NULL);

    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Initialized UVC context", driverName, functionName);
    }
    deviceStatus = uvc_find_device(pdeviceContext, &pdevice, 0, 0, serialNumber);
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Found UVC device", driverName, functionName);
    }
    deviceStatus = uvc_open(pdevice, &pdeviceHandle);
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Opened UVC device", driverName, functionName);
    }
    return status;
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
    setStringParam(ADManufacturer, pdeviceInfo->manufacturer);
    //setStringParam(ADUVC_SerialNumber, pdeviceInfo->serialNumber);
    sprintf(modelName, "UVC Vendor: %d, UVC Product: %d", pdeviceInfo->idVendor, pdeviceInfo->idProduct);
    setStringParam(ADModel, modelName);
    callParamCallbacks();
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Finished Getting device information\n", driverName, functionName);
}

/*
 * Function that starts the acquisition of the camera.
 * In the case of UVC devices, a function is called to first negotiate a stream with the camera at
 * a particular resolution and frame rate. Then the uvc_start_streaming function is called, with a 
 * function name being passed as a parameter as the callback function.
 *
 * @return: uvc_error_t -> return 0 if successful, otherwise return error code
 */
uvc_error_t ADUVC::acquireStart(){
    static const char* functionName = "acquireStart";
    void* pData;
    deviceStatus = uvc_get_stream_ctrl_format_size(pdeviceHandle, &deviceStreamCtrl, UVC_FRAME_FORMAT_MJPEG, 640, 480, 30);
    if(deviceStatus<0){
        setIntegerParam(ADAcquire, 0);
        callParamCallbacks();
        return deviceStatus;
    }
    else{
        setIntegerParam(ADNumImagesCounter, 0);
        callParamCallbacks();
        deviceStatus = uvc_start_streaming(pdeviceHandle, &deviceStreamCtrl, newFrameCallback, pData, 0);
        if(deviceStatus<0){
            setIntegerParam(ADAcquire, 0);
            callParamCallbacks();
            return deviceStatus;
        }
        else{
            moving = 1;
            setIntegerParam(ADStatus, ADStatusAcquire);
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Image aquisition start\n", driverName, functionName);
            callParamCallbacks();
            imageHandlerThread();
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
    moving = 0;

    //stop_streaming will block until last callback is processed.
    uvc_stop_streaming(pdeviceHandle);
    setIntegerParam(ADStatus, ADStatusIdle);
    setIntegerParam(ADAcquire, 0);
    callParamCallbacks();
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Stopping aquisition\n",driverName, functionName);
}

/*
 * Function responsible for converting between a uvc_frame_t type image to
 * the EPICS area detector standard NDArray type. First, we convert any given uvc_frame to
 * the rgb format. This is because all of the supported uvc formats can be converted into this type,
 * simplyfying the conversion process. Then, the NDDataType is taken into account, and a scaling factor is used
 *
 * @params: frame -> frame collected from the uvc camera
 * @params: pArray -> output of function. NDArray conversion of uvc frame
 * @params: dataType -> data type of NDArray output image
 * @return: void, but output into pArray
 */
asynStatus ADUVC::uvc2NDArray(uvc_frame_t* frame, NDArray* pArray, NDArrayInfo* arrayInfo, NDDataType_t &dataType){
    static const char* functionName = "uvc2NDArray";
    uvc_frame_t* rgb;
    rgb = uvc_allocate_frame(frame->width * frame->height *3);
    if(!rgb){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR: Unable to allocate frame\n", driverName, functionName);
        return asynError;
    }
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
            return asynError;
    }
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return asynError;
    }
    else{
        int dataSpan = rgb->width * rgb->height * 3;
        unsigned char* dataInit = (unsigned char*) rgb->data;
        unsigned char* dataEnd = dataInit+dataSpan;
        int ndims;
        size_t* dims;
        //eventually add if else here for color v mono
        ndims = 3;
        dims[0] = 3;
        dims[1] = rgb->width;
        dims[2] = rgb->height;
        if(dataType!=NDUInt8){
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Unsupported data format\n", driverName, functionName);
            return asynError;
        }
        this->pArrays[0] = pNDArrayPool->alloc(ndims, dims, dataType, 0, NULL);
        if(this->pArrays[0]!=NULL){
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Copying from frame to NDArray\n", driverName, functionName);
            pArray = this->pArrays[0];
            unsigned char* pArrayData = (unsigned char*) pArray->pData;
            copy(dataInit, dataEnd, pArrayData);
            uvc_free_frame(rgb);
            return asynSuccess;
        }
    }
    uvc_free_frame(rgb);
    return asynError;
}

/*
 * Function that performs the callbacks onto new frames generated by the camera.
 * Here we convert the uvc_frame_t into an NDArray and pass it into the area
 * detector program using processCallbacksGenericPointer
 *
 * @params: frame -> uvc_frame recieved from the camera
 * @params: ptr -> void pointer with data from the frame
 * @return: void
 */
void ADUVC::newFrameCallback(uvc_frame_t* frame, void* ptr){
    NDArray* pArray;
    NDArrayInfo arrayInfo;
    int dataType;
    NDDataType_t ndDataType;
    int operatingMode;
    epicsTimeStamp currentTime;
    static const char* functionName = "newFrameCallback";
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Entering callback function\n", driverName, functionName);
    this->lock();
    getIntegerParam(NDDataType, &dataType);
    ndDataType = (NDDataType_t) dataType;
    getIntegerParam(ADImageMode, &operatingMode);
    //single shot mode
    if(operatingMode == ADImageSingle){
        int numImages;
        getIntegerParam(ADNumImagesCounter, &numImages);
        numImages++;
        setIntegerParam(ADNumImagesCounter, numImages);
        //This function needs to be finalized
        uvc2NDArray(frame, pArray, &arrayInfo, ndDataType);
        acquireStop();
    }
    // block shot mode
    else if(operatingMode == ADImageMultiple){
        int numImages;
        int desiredImages;
        getIntegerParam(ADNumImagesCounter, &numImages);
        numImages++;
        setIntegerParam(ADNumImagesCounter, numImages);
        //This function needs to be finalized
        uvc2NDArray(frame, pArray, &arrayInfo, ndDataType);
        getIntegerParam(ADNumImages, &desiredImages);
	if(numImages>=desiredImages) acquireStop();
    }
    //continuous mode
    else if(operatingMode == ADImageContinuous){
        int numImages;
        getIntegerParam(ADNumImagesCounter, &numImages);
        numImages++;
        setIntegerParam(ADNumImagesCounter, numImages);
        //This function needs to be finalized
        uvc2NDArray(frame, pArray, &arrayInfo, ndDataType);
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR: Unsupported operating mode\n", driverName, functionName);
        acquireStop();
        return;
    }
    pArray->getInfo(&arrayInfo);
    int arrayCounter;
    getIntegerParam(NDArrayCounter, &arrayCounter);
    arrayCounter++;
    setIntegerParam(NDArrayCounter, arrayCounter);
    setIntegerParam(NDArraySize, arrayInfo.totalBytes);
    epicsTimeGetCurrent(&currentTime);
    pArray->timeStamp = currentTime.secPastEpoch + currentTime.nsec/ONE_BILLION;
    updateTimeStamp(&pArray->epicsTS);
    callParamCallbacks();
    getAttributes(pArray->pAttributeList);
    doCallbacksGenericPointer(pArray, NDArrayData, 0);

}

/*
 * Function that is used to set the length of the stream based on the operating mode.
 * 1) in Single shot mode, only the thread sleeps for one second and only accepts the first image
 * 2) in Snap shot mode, the thread will sleep for numFrames/framerate seconds, to get the correct number of frames
 * 3) in continuous mode, the thread will sleep for one second at a time until it acquireStop() is called.
 * 
 * @return: void
 */
void ADUVC::imageHandlerThread(){
    static const char* functionName = "imageHandlerThread";
    int operatingMode;
    int framerate;
    int numFrames;
    getIntegerParam(ADUVC_OperatingMode, &operatingMode);
    getIntegerParam(ADUVC_Framerate, &framerate);
    getIntegerParam(ADNumImages, &numFrames);
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Starting image collection thread\n", driverName, functionName);
    //single shot
    if(operatingMode == ADImageSingle){
        usleep(1000);
    }
    // snap shot
    else if(operatingMode == ADImageMultiple){
        int seconds = numFrames/framerate;
        seconds = seconds + 1;
        int second_counter = 0;
        while(moving == 1 && second_counter!=seconds){
            usleep(1000);
            second_counter = second_counter+1;
        }
    }
    // continuous mode
    else if(operatingMode == ADImageContinuous){
        while(moving == 1){
            usleep(1000);
        }
    }
    acquireStop();
    moving = 0;
}

/*
 * Function overwriting ADDriver base function.
 * Takes in a function (PV) changes, and a value it is changing to, and processes the input
 * 
 * @params: pasynUser -> asyn client who requests a write
 * @params: value -> int32 value to write
 * @return: asynStatus -> success if write was successful, else failure
 */
asynStatus ADUVC::writeInt32(asynUser* pasynUser, epicsInt32 value){
    
    int function = pasynUser->reason;
    int acquiring;
    int status = asynSuccess;
    static const char* functionName = "writeInt32";
    getIntegerParam(ADAcquire, &acquiring);

    
    status = setIntegerParam(function, value);

    if(function == ADAcquire){
        if(value && !acquiring){
            deviceStatus = acquireStart();
            if(deviceStatus < 0){
                reportUVCError(deviceStatus, functionName);
                return asynError;
            }
        }
        if(!value && acquiring){
            acquireStop();
        }
    }
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
    else asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s function=%d value=%d\n", driverName, functionName, function, value);
    return asynSuccess;
}

/*
 * Constructor for ADUVC driver. Most params are passed to the parent ADDriver constructor. 
 * Connects to the camera, then gets device information, and is ready to aquire images.
 * 
 * @params: portName -> port for NDArray recieved from camera
 * @params: serial -> serial number of device to connect to
 * @params: framerate -> framerate at which camera should operate
 * @params: maxBuffers -> max buffer size for NDArrays
 * @params: maxMemory -> maximum memory allocated for driver
 * @params: priority -> what thread priority this driver will execute with
 * @params: stackSize -> size of the driver on the stack
 */
ADUVC::ADUVC(const char* portName, const char* serial, int framerate, int maxBuffers, size_t maxMemory, int priority, int stackSize)
    : ADDriver(portName, 1, (int)NUM_UVC_PARAMS, maxBuffers, maxMemory, asynEnumMask, asynEnumMask, ASYN_CANBLOCK, 1, priority, stackSize){
    static const char* functionName = "ADUVC";

    // create PV Params
    createParam(ADUVC_OperatingModeString,          asynParamInt32,     &ADUVC_OperatingMode);
    createParam(ADUVC_UVCComplianceLevelString,     asynParamInt32,     &ADUVC_UVCComplianceLevel);
    createParam(ADUVC_ReferenceCountString,         asynParamInt32,     &ADUVC_ReferenceCount);
    createParam(ADUVC_FramerateString,              asynParamInt32,     &ADUVC_Framerate);
    createParam(ADUVC_SerialNumberString,           asynParamOctet,     &ADUVC_SerialNumber);

    setIntegerParam(ADUVC_Framerate, framerate);
    char versionString[25];
    epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", ADUVC_VERSION, ADUVC_REVISION, ADUVC_MODIFICATION);
    setStringParam(NDDriverVersion, versionString);


    asynStatus connected = connectToDeviceUVC(serial);

    if(connected == asynError){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Connection failed, abort\n", driverName, functionName);
    }
    else{
        getDeviceInformation();
    }

}

/*
 * ADUVC destructor. Called by the exitCallbackC function when IOC is shut down
 */
ADUVC::~ADUVC(){
    static const char* functionName = "~ADUVC";
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s Calling all free functions for ADUVC\n", driverName, functionName);
    uvc_free_device_descriptor(pdeviceInfo);
    uvc_close(pdeviceHandle);
    uvc_unref_device(pdevice);
    uvc_exit(pdeviceContext);
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,"ADUVC driver exiting\n");
    disconnect(this->pasynUserSelf);
}



/* Code for iocsh registration */

/* UVCConfig -> These are the args passed to the constructor in the epics config function */
static const iocshArg UVCConfigArg0 = { "Port name", iocshArgString };
static const iocshArg UVCConfigArg1 = { "Serial number", iocshArgString };
static const iocshArg UVCConfigArg2 = { "Framerate", iocshArgInt };
static const iocshArg UVCConfigArg3 = { "maxBuffers", iocshArgInt };
static const iocshArg UVCConfigArg4 = { "maxMemory", iocshArgInt };
static const iocshArg UVCConfigArg5 = { "priority", iocshArgInt };
static const iocshArg UVCConfigArg6 = { "stackSize", iocshArgInt };

static const iocshArg * const UVCConfigArgs[] =
        { &UVCConfigArg0, &UVCConfigArg1, &UVCConfigArg2,
        &UVCConfigArg3, &UVCConfigArg4, &UVCConfigArg5,
        &UVCConfigArg6 };

static void configUVCCallFunc(const iocshArgBuf *args) {
    ADUVCConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival,
            args[4].ival, args[5].ival, args[6].ival);
}

static const iocshFuncDef configUVC = { "ADUVCConfig", 6, UVCConfigArgs };

static void UVCRegister(void) {
    iocshRegister(&configUVC, configUVCCallFunc);
}

extern "C" {
    epicsExportRegistrar(UVCRegister);
}
