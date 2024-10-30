/* 
 * File: usbComm.c
 *
 * Copyright (C) 2024 vixxxkigoli
 * Author: vixxxkigoli
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdbool.h>
#include "usbComm.h"
#include <unistd.h>
#include <string.h> // for strlen
#include <stdlib.h> // for malloc

int cstCmd(char* cmd)
{
    size_t cmdStrLen = strlen(cmd);
    
    if((cmdStrLen % 2) == 1)
    {
        printf("Invalid Command!!!!!");
        return -3;
    }
    
    uint8_t *hxCmd = malloc(cmdStrLen / 2);
        if (hxCmd == NULL) {
            perror("Memory allocation failed");
            return -2; // TO-DO: Need to structure ret values
        }
    for (size_t i = 0; i < cmdStrLen; i += 2) 
    {
        sscanf(&cmd[i], "%2hhx", &hxCmd[i / 2]);
    }
    //sendOverUSB();
    return 0;
}


bool isDeviceConnected(libusbAPI *api, USB *dev) 
{
    libusb_device** device_list = NULL;
    ssize_t device_count = 0;

    device_count = libusb_get_device_list(api->ctx, &device_list);

    if (device_count < 0) 
    {
        printf("Failed to get device list\n");
        return false;
    }

    for (ssize_t i = 0; i < device_count; ++i) 
    {
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(device_list[i], &desc) == 0) 
        {
            if (desc.idVendor == dev->vid && desc.idProduct == dev->pid)
            {
                int open_result = libusb_open(device_list[i], &api->handle);
                if (open_result == LIBUSB_SUCCESS) 
                {
                    libusb_free_device_list(device_list, 1); // Freeing the device list with 1
                    dev->isDev = true;
                    return true;
                }
                else 
                {
                    printf("Failed to open device: %s\n", libusb_error_name(open_result));
                }
            }
        }
    }

    libusb_free_device_list(device_list, 0); // Freeing device list

    return false;
}

/*
 * 
#define GET_DESCRIPTOR 0x06
#define CONFIGURATION_DESCRIPTOR_TYPE 0x02

int getConfigurationDescriptor(libusb_device_handle* handle) {
    unsigned char buffer[256]; // Buffer to hold the descriptor
    int transferred; // To store the number of bytes transferred

    // Send GET_DESCRIPTOR request
    int result = libusb_control_transfer(
        handle,
        LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_ENDPOINT_IN, // bmRequestType: Device-to-host, Standard, Device
        GET_DESCRIPTOR,          // bRequest: GET_DESCRIPTOR
        (CONFIGURATION_DESCRIPTOR_TYPE << 8), // wValue: Configuration descriptor
        0,                        // wIndex: No specific index (usually 0)
        buffer,                  // Buffer to receive the descriptor
        sizeof(buffer),          // wLength: Size of the buffer
        1000                     // Timeout: 1 second
    );

    if (result < 0) {
        fprintf(stderr, "Failed to get configuration descriptor: %s\n", libusb_error_name(result));
        return -1; // Error
    }

    printf("Received configuration descriptor: %d bytes\n", result);
    // Here you can process the buffer to read the descriptor information
    return 0; // Success
}

 * 
 */

int stConfig(USB *dev, libusbAPI *api)
{
    // Send SET_CONFIGURATION request
    int result = libusb_control_transfer(
        api->handle,
        dev->bmRequestType, // bmRequestType: Host-to-device, Standard, Device
        dev->bRequest,          // bRequest: SET_CONFIGURATION
        dev->wValue, 	             // wValue: Configuration index (1-based)
        dev->wIndex,                          // wIndex: No specific index (usually 0)
        NULL,                       // No data to send
        0,                          // wLength: No data
        dev->readTime                        // Timeout: 1 second
    );

    if (result < 0) 
    {
        fprintf(stderr, "Failed to set configuration: %s\n", libusb_error_name(result));
        return -1; // Error
    }

    printf("Configuration set successfully.\n");
    return 0; // Success
}


int ctlTransfer(USB *dev, libusbAPI *api)
{
	int rdRes = libusb_control_transfer(api->handle,
                                dev->bmRequestType,
                                dev->bRequest, // bRequest
                                dev->wValue, // wValue
                                dev->wIndex, // wIndex
                                dev->data, // Buffer to receive data
                                sizeof(dev->data), // Size of the buffer
                                dev->readTime); // Timeout in milliseconds

    if (rdRes < 0) 
    {
        fprintf(stderr, "Control transfer failed: %s\n", libusb_error_name(rdRes));
    } else 
    {
        printf("Received %d bytes:\n", rdRes);
        for (int i = 0; i < rdRes; i++) 
        {
            printf(" %02X |", dev->data[i]);
        }
        printf("\n");
    }

	
	return 0;
}

int readData(USB *dev, libusbAPI *api)
{
	
	int rdResult = libusb_bulk_transfer(api->handle, dev->endPointOUT, dev->data, dev->sizeOfData, &dev->read, dev->readTime);
	
	if (rdResult == LIBUSB_SUCCESS)
	{
#if DEBUGMODE == 1
        printf("readData(): rdResult: %d, dev->read:%d \n", rdResult, dev->read);
		
		for(int i = 0; i < dev->read; i++)
		{
			printf(" %02X |", dev->data[i]);
		}
		printf("\n-----------------------------\n");
#endif    
    }
    else 
    {
        printf("Failed to read data: %d and dev->read:%d", rdResult, dev->read);
        return 1;
    }
	
	return 0;
}

int sendData(USB *dev, libusbAPI *api)
{
	
    int transResult = libusb_bulk_transfer(api->handle, dev->endPointIN, dev->data, dev->sizeOfData, &dev->transferred, dev->transTime);
    if (transResult == LIBUSB_SUCCESS) 
    {
		
        printf("Data sent successfully: %d\n", dev->transferred);
    }
    else
    {
        printf("Failed to send data: %s\n", libusb_error_name(transResult));
        libusb_close(api->handle);
        libusb_exit(api->ctx);
        return 1;
    }

    // Close and exit
#if DEBUGMODE == 1
    printf("\nsendData(): Send: %d \n", dev->transferred);
    for(int i = 0; i < dev->transferred; ++i)
    {
		printf(" %02X |", dev->data[i]);
	}
    printf("\n-----------------------------\n");
#endif
    //printf("libusb exit");
    return 0;
}
