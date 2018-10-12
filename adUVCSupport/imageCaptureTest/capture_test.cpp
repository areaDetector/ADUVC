/*
 * This is a testing file that coverts a uvc frame into an openCV Mat.
 * This is for checking if you are able to aquire images outside of the AD Driver
 *
 * Author: Jakub Wlodek
 * Created: October 12, 2018
 * Copyright (c): Brookhaven National Laboratory
 */



// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <cstddef>
#include <string.h>



// uvc include
#include <libuvc/libuvc.h>



//opencv include
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>



// namespaces
using namespace std;
using namespace cv;



//frame counter
static int frameNum = 1;



/**
 * Function that prints program usage
 */
void print_help(){
    printf("USAGE\n");
    printf("-------------------------------------\n");
    printf("-s $SERIAL_NUMBER       ->      finds device using serial number.\n");
    printf("-p $PROD_ID             ->      finds device using productID.\n");
    printf("-h                      ->      prints this message.\n");
}



/**
 * Callback function. Takes uvc_frame, converts it from mjpeg to rgb, then to an OpenCV Mat, then converts
 * to BGR
 * 
 * @params: frame -> uvc frame to convert
 * @params: ptr -> unused but required
 * @return: void
 */
void newFrameCallback(uvc_frame_t* frame, void* ptr){
    uvc_frame_t* rgb;
    uvc_error_t deviceStatus;
    printf("Entering callback function on frame number %d\n", frameNum);

    rgb = uvc_allocate_frame(frame->width * frame->height *3);
    if(!rgb) {
        printf("Abort: unable to allocate frame\n");
        //uvc_free_frame(frame);
        return;
    }

    deviceStatus = uvc_mjpeg2rgb(frame, rgb);
    if(deviceStatus) {
        uvc_perror(deviceStatus, "uvc_mjpeg2rgb");
        uvc_free_frame(rgb);
        return;
    }

    Mat cvImg(rgb->height, rgb->width, CV_8UC3, (uchar*)rgb->data);
    //uvc_free_frame(frame);
    cvtColor(cvImg, cvImg, COLOR_RGB2BGR);

    imshow("UVC Image", cvImg);
    waitKey(1);
    frameNum = frameNum+1;
    uvc_free_frame(rgb);
}



/**
 * Main function. First parses args, then connects to uvc device and streams.
 */
int main(int argc, char** argv){

    char* serialNumber = "";
    int productID = 0;
    if(argv[1]!=NULL){
        if(strcmp(argv[1], "-h")==0){
            print_help();
            return 0;
        }
    }
    if(argc<2){
        printf("Invalid arguments!");
        print_help();
        return -1;
    }
    if(strcmp(argv[1], "-s")==0) serialNumber = argv[2];
    else if(strcmp(argv[1], "-p")==0) productID = atoi(argv[2]);
    else{
        printf("Invalid arguments!");
        print_help();
        return -1;
    }
    uvc_context_t* context;
    uvc_device_t* device;
    uvc_device_handle_t* deviceHandle;
    uvc_stream_ctrl_t ctrl;
    uvc_error_t status;

    status = uvc_init(&context, NULL);
    if(status < 0){
        uvc_perror(status, "uvc_init");
        return status;
    }

    if(strcmp(serialNumber,"")!=0){
        printf("Trying to find device with serial number %s\n", serialNumber);
        status = uvc_find_device(context, &device, 0, 0, serialNumber);

    }
    else{
        printf("Trying to find device with pID %d\n", productID);
        status = uvc_find_device(context, &device, 0, productID, NULL);
    }

    if(status < 0){
        uvc_perror(status, "uvc_find_device");
        return status;
    }
    puts("Device initalized and found");
    status = uvc_open(device, &deviceHandle);
    if(status < 0){
        uvc_perror(status, "uvc_open");
        return status;
    }

    status = uvc_get_stream_ctrl_format_size(deviceHandle, &ctrl, UVC_FRAME_FORMAT_MJPEG, 640, 480, 30);
    void* frame_data;
    if(status<0) uvc_perror(status, "get_mode");
    else {
        status = uvc_start_streaming(deviceHandle, &ctrl, newFrameCallback, frame_data, 0);
        if(status<0)uvc_perror(status, "start_streaming");
        else{
            printf("Streaming...\n");
            uvc_set_ae_mode(deviceHandle, 1);
            while(frameNum<200){}

            uvc_stop_streaming(deviceHandle);
            printf("Done streaming.\n");
        }
        uvc_unref_device(device);
    }

    uvc_exit(context);

    return 0;

}