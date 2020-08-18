/*
 * This file contains code that is used to generate information about all UVC devices connected to
 * a particular machine. This is useful for finding the serial number of the desired device, as well
 * as for identifying camera information such as supported framerates, resolutions, and formats.
 *
 * Author: Jakub Wlodek
 * Created: July 27, 2018
 * Copyright (c): Brookhaven National Laboratory
 */

#include <stdlib.h>
#include "libuvc/libuvc.h"
#include <stdio.h>
#include <cstddef>
#include <string.h>

using namespace std;



/*
 * Simple helper function that prints program usage
 *
 * @return: void
 */
void print_help(){
    printf("USAGE:\n");
    printf("This tool is used to identify UVC devices connected to the machine and their specifications\n");
    printf("-------------------------------------------------------------------------------------------\n");
    printf("NO_ARGS                             -> Gets a list of all devices, and some basic information, such as serial numbers.\n");
    printf("-h or --help                        -> View this help messag.\n");
    printf("-s or --serial + SERIAL_NUMBER      -> To see more detailed information about a specific camera.\n");
    printf("Check the README.md file in this directory for examples of all use cases.\n");
}



/*
 * Function that lists all connected UVC cameras and some basic information, such
 * as serial number, vendorID etc.
 *
 * @return: status -> 0 if success, -1 if failure
 */
int list_all(){
    uvc_context_t* ctx;
    uvc_device_t** device_list;
    uvc_device_descriptor_t* desc;
    uvc_error_t status;

    /*
     * Initialize a UVC service context.
     * Uses libusb to set up context.
     */

    status = uvc_init(&ctx, NULL);

    if(status < 0){
        uvc_perror(status, "uvc_init");
        return status;
    }

    puts("UVC initialized successfully");

    //generates list of available devices
    status = uvc_get_device_list(ctx, &device_list);

    if(status < 0){
        uvc_perror(status, "uvc_get_device_list");
        return status;
    }
    else{
        int counter = 0;
        //iterate over list, get device descriptor, and print information
        while(*(device_list+counter)!=NULL){
            status = uvc_get_device_descriptor(*(device_list+counter), &desc);
            if(status < 0){
                uvc_perror(status, "uvc_get_device_descriptor");
                return status;
            }
            printf("-------------------------------------------------------------\n");
            printf("Serial Number:      %s\n", desc->serialNumber);
            printf("Vendor ID:          %d\n", desc->idVendor);
            printf("ProductID:          %d\n", desc->idProduct);
            printf("Manufacturer:       %s\n", desc->manufacturer);
            printf("Product:            %s\n", desc->product);
            printf("UVC Compliance:     %d\n", desc->bcdUVC);
            uvc_free_device_descriptor(desc);
            counter++;
        }
    }
    // free remaining uvc structs
    uvc_free_device_list(device_list, 0);
    uvc_exit(ctx);
    return status;
}



/*
 * Function that connects to device with specific serial number, and lists information
 * about said device.
 *
 * @params: serialNumber -> serial number of desired device
 * @return: status -> 0 for success, -1 for error
 */
int list_detailed_information(const char* serialNumber, int productID){
    uvc_context_t* context;
    uvc_device_t* device;
    uvc_device_handle_t* deviceHandle;
    uvc_error_t status;

    status = uvc_init(&context, NULL);
    if(status < 0){
        uvc_perror(status, "uvc_init");
        return status;
    }
    if(serialNumber != NULL){
        status = uvc_find_device(context, &device, 0,0, serialNumber);
    }
    else{
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
    puts("Device Diagnostic Information:");
    puts("------------------------------------------------------------");
    uvc_print_diag(deviceHandle, stderr);
    uvc_close(deviceHandle);
    uvc_unref_device(device);
    uvc_exit(context);
    puts("Disconnected from device");
    return 0;
}


/*
 * Main function. Iterates over the command line arguments, and based on input,
 * calls one of the three helper functions.
 *
 * @params: argc -> number of CLI args
 * @params: argv -> actual CLI args
 * @return: status -> 0 if success, -1 if failure
 */
int main(int argc, char** argv){
    //if no args, list all devices
    if(argc == 1){
        int status = list_all();
        return status;
    }
    else{
        int arg_num = 1;
        while(arg_num<=argc){
            char* arg = *(argv+arg_num);
            // printf("The arg is %s\n", arg);
            // if first arg is -h or --help print help
            if(strcmp(arg,"-h")==0 || strcmp(arg,"--help")==0){
                printf("here\n");
                print_help();
                return 0;
            }
            // otherwise if -s or --serial print detailed info
            else if(strcmp(arg,"-s")==0 || strcmp(arg,"--serial")==0){
                const char* serialNum = *(argv+arg_num+1);
                if(serialNum == NULL){
                    printf("ERROR: serial number not passed\n");
                    print_help();
                    return -1;
                }
                else{
                    printf("Searching for UVC device with serial number: %s\n", serialNum);
                    int status = list_detailed_information(serialNum, 0);
                    return status;
                }
            }
            else if(strcmp(arg, "-p")==0 || strcmp(arg,"--product")==0){
                const char* pIDString = *(argv+arg_num+1);
                if(pIDString==NULL){
                    printf("ERROR: ProductID not passed\n");
                    print_help();
                    return -1;
                }
                else{
                    int pID = atoi(*(argv+arg_num+1));
                    printf("Searching for UVC device with product ID: %d\n", pID);
                    int status = list_detailed_information(NULL, pID);
                    return status;
                }
            }
            // otherwise, invalid arg
            else{
                printf("ERROR: Invalid argument\n");
                print_help();
                return -1;
            }
        }
    }
    return 0;
}

