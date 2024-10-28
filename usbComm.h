/* 
 * File: usbComm.h
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

#ifndef USBCOMM_H
#define USBCOMM_H

#include <stdbool.h>
#include <libusb-1.0/libusb.h>
#include <stdio.h>

typedef struct 
{
	uint16_t vid;
	uint16_t pid;
	uint8_t endPointOUT;
	uint8_t endPointOUT1;
	uint8_t endPointOUT2;
	uint8_t endPointIN;
	uint8_t endPointIN1;
	uint8_t endPointIN2;
	uint8_t bRequest;
	uint8_t bmRequestType;
    uint16_t wValue;
    uint16_t wIndex;
	uint8_t *data;
	int sizeOfData;
	int transferred;
	int read;
	int transTime;
	int readTime;
	bool isDev;
} USB;

typedef struct
{
	libusb_device_handle* handle;
	libusb_context* ctx;
} libusbAPI;


//bool isDeviceConnected(libusb_context* ctx, libusb_device_handle** handle, USB *dev);
int stConfig(USB *dev, libusbAPI *api);
bool isDeviceConnected(libusbAPI *api, USB *dev);
int ctlTransfer(USB *dev, libusbAPI *api);
int readData(USB *dev, libusbAPI *api);
int sendData(USB *dev, libusbAPI *api);

#endif // USBCOMM_H
