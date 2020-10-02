
// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <cstddef>
#include <string.h>
#include <inttypes.h>
#include "unistd.h"


// uvc include
#include <libuvc/libuvc.h>


using namespace std;


/**
 * Function that prints program usage
 */
void print_help(){
    printf("USAGE:\n\n");
    printf("./ptz_test DIRECTION [-s/-p] CONNECTION\n\n");
    printf("DIRECTION can be ['left', 'right', 'up', 'down']\n");
    printf("-------------------------------------\n");
    printf("-s $SERIAL_NUMBER       ->      finds device using serial number.\n");
    printf("-p $PROD_ID             ->      finds device using productID.\n");
    printf("-h                      ->      prints this message.\n");
    printf("Example call using: ./ptz_test left -p 23456 \n");
}



/**
 * Main function. First parses args, then connects to uvc device and streams.
 */
int main(int argc, char** argv){

    //store our connection parameter and operation
    char* serialNumber = NULL;
    int productID = 0;
    char* operation;

    // Check if the help option is listed
    if(argv[1]!=NULL){
        if(strcmp(argv[1], "-h")==0){
            print_help();
            return 0;
        }
    }

    // Otherwise, we know we need 4 arguments
    if(argc!=4){
        printf("Invalid arguments!\n");
        print_help();
        return -1;
    }

    // Find serial/product number and operation
    if(strcmp(argv[1], "-s")==0){
        serialNumber = argv[2];
        operation = argv[3];
    }
    else if(strcmp(argv[1], "-p")==0){
        productID = atoi(argv[2]);
        operation = argv[3];
    }
    else{
        operation = argv[1];
        if(strcmp(argv[2], "-s") == 0){
            serialNumber = argv[3];
        }
        else if(strcmp(argv[2], "-p") == 0){
            productID = atoi(argv[3]);
        }
        else{
            printf("Invalid arguments!\n");
            print_help();
            return -1;
        }
    }


    //connecting to uvc device
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

    //use serial or pID
    if(serialNumber != NULL){
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

    uint16_t aMin, aMax;
    int8_t zMax, zMin;
    uint8_t dZMax, dZMin, sMax, sMin;

    uvc_get_zoom_abs(deviceHandle, &aMin, UVC_GET_MIN);
    uvc_get_zoom_abs(deviceHandle, &aMax, UVC_GET_MAX);
    

    uvc_get_zoom_rel(deviceHandle, &zMax, &dZMax, &sMax, UVC_GET_MAX);
    uvc_get_zoom_rel(deviceHandle, &zMin, &dZMin, &sMin, UVC_GET_MIN);

    printf("Abs: %ld - %ld\n", aMin, aMax);
    printf("Zoom: %d - %d\n", zMin, zMax);
    printf("Digital Zoom: %d - %d\n", dZMin, dZMax);
    printf("Speed: %d - %d\n", sMin, sMax);

    int8_t zoomDir = 0;

    printf("Moving device: %s...\n", operation);

    // based on input operation, select pan/tilt direction
    if(strcmp(operation, "in") == 0) zoomDir = 1;
    else if(strcmp(operation, "out") == 0) zoomDir = -1;
    else{
        printf("Invalid operation: %s!\n", operation);
        print_help();
        return -1;
    }

    //disconnect from the device
    printf("Disconnecting from device...\n");
    uvc_unref_device(device);
    uvc_exit(context);

    printf("Done.\n");
}