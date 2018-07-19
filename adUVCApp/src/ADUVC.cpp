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
#include <libuvc.h>
#include <libuvc_config.h>

static const char* driverName = "ADUVC";


/*
 * External configuration function for ADUVC.
 * Envokes the constructor to create a new ADUVC object
 * 
 * @params: all passed into constructor
 * @return: status
 */
extern "C" int ADUVCConfig(const char* portName, int devIndex, int maxBuffers, size_t maxMemory, int priority, int stackSize){
    new ADUVC(portName, devIndex, maxBuffers, maxMemory, priority, stackSize);
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
 * @return: bool -> true if connection is successful, false if failed
 */
bool ADUVC::connectToDeviceUVC(int serialNumber){
    static const char* functionName = "connectToDeviceUVC";
    deviceStatus = uvc_init(&pdeviceContext, NULL);

    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return false;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Initialized UVC context", driverName, functionName);
    }
    deviceStatus = uvc_find_device(pdeviceContext, &pdevice, 0, 0, serialNumber);
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return false;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Found UVC device", driverName, functionName);
    }
    deviceStatus = uvc_open(pdevice, &pdeviceHandle);
    if(deviceStatus<0){
        reportUVCError(deviceStatus, functionName);
        return false;
    }
    else{
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Opened UVC device", driverName, functionName);
    }
    return true;
}

/*
 * Function responsible for getting device information once device has been connected.
 * Calls the uvc_get_device_descriptor function from libuvc, and gets the manufacturer,
 * serial, model.
 * 
 * @return: void
 */
void ADUVC::getDeviceInformation(){
    uvc_get_device_descriptor(pdeviceHandle->dev, pdeviceInfo);
    setStringParam(ADManufacturer, pdeviceInfo->manufacturer);
    setIntegerParam(ADUVC_SerialNumber, pdeviceInfo->serialNumber);
    sprintf(modelName, "UVC Vendor: %d, UVC Product: ", pdeviceInfo->idVendor, pdeviceInfo->idProduct);
    setStringParam(ADModel, modelName);
}

uvc_error_t ADUVC::acquireStart(){
    //TODO
}

uvc_error_t ADUVC::acquireStop(){
    //TODO
}

void ADUVC::newFrameCallback(uvc_frame_t* fram, void* ptr){
    void* pData;
    int aquiring;
    NDArray* pArray;
    NDDataType_t dataType = NDUInt16;
    epicsTimeStamp currentTime;
    static const char* functionName = "newFrameCallback";
    asynPrint(this->pasynUserSelf, ASYN_TRACE_IO_DRIVER, "%s::%s Entering callback function\n", driverName, functionName);
    this->lock();

}

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
            if(deviceStatus < 0) reportUVCError(deviceStatus, functionName);
        }
        if(!value && acquiring){
            deviceStatus = acquireStop();
            if(deviceStatus < 0) reportUVCError(deviceStatus, functionName);
        }
    }
    else{
        if (function < DEX_FIRST_PARAM) {
            status = ADDriver::writeInt32(pasynUser, value);
        }
    }
    callParamCallbacks();

    if(status){
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s ERROR status=%d, function=%d, value=%d\n", driverName, functionName, status, function, value);
    }
    else asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s function=%d value=%d\n", driverName, functionName, function, value);
}

/*
 * Constructor for ADUVC driver. Most params are passed to the parent ADDriver constructor. 
 * Connects to the camera, then gets device information, and is ready to aquire images.
 * 
 * @params: portName -> port for NDArray recieved from camera
 * @params: devIndex -> this parameter selects which device to connect to. Currently connects to first device detected
 * @params: maxBuffers -> max buffer size for NDArrays
 * @params: maxMemory -> maximum memory allocated for driver
 * @params: priority -> what thread priority this driver will execute with
 * @params: stackSize -> size of the driver on the stack
 */
ADUVC::ADUVC(const char* portName, int serialNumber, int maxBuffers, size_t maxMemory, int priority, int stackSize)
    : ADDriver(portName, 1, (int)NUM_UVC_PARAMS, maxBuffers, maxMemory, asynEnumMask, asynEnumMask, ASYN_CANBLOCK, 1, priority, stackSize){
        
    int status = asynSuccess;
    static char* functionName = "ADUVC";
        
    createParam(ADUVC_UVCComplianceLevelString,     asynParamInt32,     ADUVC_UVCComplianceLevel);
    createParam(ADUVC_ReferenceCountString,         asynParamInt32,     ADUVC_ReferenceCount);
    createParam(ADUVC_SerialNumberString,           asynParamInt32,     ADUVC_SerialNumber);

    bool connected = connectToDeviceUVC(serialNumber);
    if(!connected){
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
    delete(pdevice);
}



