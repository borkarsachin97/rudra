/* 
 * Project Rudra
 * 
 * File: rudra.c
 *
 * Copyright (C) 2024 vixxxkigoli
 * Author: vixxxkigoli
 *
 * This file is part of Project Rudra, an open-source project.
 * 
 * Project Rudra is free software: you can redistribute it and/or modify
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
 * along with Project Rudra. If not, see <https://www.gnu.org/licenses/>.
 */
 
 
#include "protocol.h"
#include "usbComm.h"
#include <unistd.h> // for sleep()
#include <string.h> // for memcpy


void loadDevProp(USB *dev)
{	
	dev->vid = 0x1e04;
	dev->pid = 0x0904;
	dev->endPointIN = 0x01;
	dev->endPointOUT = 0x81;
	dev->transTime = 5000;
	dev->readTime = 5000;
	dev->isDev = false;
}

void setupLibUsbApi(libusbAPI* api, USB* dev)
{
		api->handle = NULL;
		api->ctx = NULL;
		if (libusb_init(&api->ctx) != 0)
		{
			printf("Failed to initialize libusb\n");
			return;
		}

		printf("\n\t << Waiting for the device to be connected >>\n");
		// Check device connection
		while (!isDeviceConnected(api, dev)) 
		{
			printf("...\n");
			sleep(1); // Sleep for 1 second
		}
}

int read_firmware(USB* dev, libusbAPI api, packet* pkt, uint32_t addrs, uint16_t rdprcyl, uint32_t flashsize, char* filename)
{
	pkt->snd->cmd = CMD_READ_BULK_DATA;
	uint32_t tmpAddr = addrs;
	pkt->snd->body.read_bulk_data.askdSize = rdprcyl;
	
	pkt->snd->sizeOfStream = 0x0009;
	uint8_t* stream;
	uint8_t* recvStream = (uint8_t*)malloc(MAX_STREAM_SIZE);
	
	FILE *file = fopen(filename, "wb");
    if (!file) 
    {
        perror("Failed to open file");
        //free(recvStream);
        return -1;
    }
    

	
	for( pkt->snd->body.read_bulk_data.frame_id = 0x03; tmpAddr < addrs + flashsize; tmpAddr += rdprcyl)
	{
		pkt->snd->body.read_bulk_data.address = __builtin_bswap32(tmpAddr);
		calCrc(pkt);
		stream = streamGen(pkt);
		

		for(int i = 0; i < pkt->snd->sizeOfPkt; i++)
		{
			printf(" %02X |", stream[i]);
		}
		printf("\n");

		dev->sizeOfData = pkt->snd->sizeOfPkt;
		dev->data = stream;
		
		sendData(dev, &api);
		
		dev->data = recvStream;
		dev->sizeOfData = MAX_STREAM_SIZE;
		readData(dev, &api);		
		
        uint8_t* binParts = (uint8_t*)malloc(dev->read);
        if (!binParts) 
        {
            printf("Memory allocation failed\n");
            fclose(file);
            free(recvStream);
            return -1;
        }
		size_t binSize = dev->read - HEADER_SKIP_BYTES - TRAILER_SKIP_BYTES;
        // Copy data starting from the 6th byte to the second-last byte
        memcpy(binParts, recvStream + HEADER_SKIP_BYTES, binSize);

        // Write processed data to file
        fwrite(binParts, 1, binSize, file);

		
		free(binParts);
		free(stream);
		//pkt->snd->body.read_bulk_data.frame_id++;
	}
	free(recvStream);
	fclose(file);

	
	return 0; // 0 on Success
}

void sendBinData(USB* dev, libusbAPI api, packet* pkt, uint32_t address, uint8_t *data, size_t size) 
{
	pkt->snd->cmd = CMD_WRITE_BULK_DATA;
    
    size_t crOffset = 0;
    uint8_t* stream;

    // Send data in chunks of max upto 4096 bytes
    while (size > 0)
    {
        // Calculate the size of the current part (either MAX_DATA_SIZE or the remaining data size)
        size_t prtSize = (size > MAX_DATA_SIZE) ? MAX_DATA_SIZE : size;

        pkt->snd->body.write_bulk_data.address = __builtin_bswap32(address + crOffset);
        pkt->snd->body.write_bulk_data.data = data + crOffset;
        pkt->snd->body.write_bulk_data.sizeOfData = prtSize;
        pkt->snd->sizeOfStream = prtSize + 6;
        calCrc(pkt);
        
		stream = streamGen(pkt);

        dev->sizeOfData = pkt->snd->sizeOfPkt;
		dev->data = stream;
		
		sendData(dev, &api);
        crOffset += prtSize;
        size -= prtSize;
        free(stream);
    }
}

//This function Decode and sends Loader

void sendLoader(USB* dev, libusbAPI api, packet* pkt, const char *loader) 
{
    FILE *file = fopen(loader, "rb");
    if (file == NULL) 
    {
        perror("Error opening file");
        return;
    }

    // First block - has the "LOD" signature
    char signature[4];
    fread(signature, 1, 3, file);
    signature[3] = '\0'; // Null-terminate for safety

    if (strcmp(signature, "LOD") != 0) 
    {
        printf("Invalid signature. Expected 'LOD', found '%s'\n", signature);
        fclose(file);
        return;
    }

    printf("Signature: %s\n", signature);

    // Read the 3 unknown bytes after the signature
    uint8_t unknown_bytes[3];
    fread(unknown_bytes, 1, 3, file);
    printf("Unknown bytes: %02x %02x %02x\n", unknown_bytes[0], unknown_bytes[1], unknown_bytes[2]);

    while (!feof(file)) 
    {
        // Read 4-byte little-endian address
        uint32_t address;
        if (fread(&address, 1, 4, file) != 4) break;
        printf("Address: 0x%08x\n", address);

        // Read 2-byte little-endian size
        uint16_t size;
        if (fread(&size, 1, 2, file) != 2) break;
        printf("Data size: 0x%04x (%d bytes)\n", size, size);

        // Read the data of size "size"
        uint8_t *data = malloc(size);
        if (data == NULL) 
        {
            perror("Memory allocation failed");
            fclose(file);
            return;
        }

        if (fread(data, 1, size, file) != size) 
        {
            printf("Error reading data block.\n");
            free(data);
            break;
        }

        // Send data
        printf("Sending block\n");
        sendBinData(dev, api, pkt, address, data, size);
        free(data);
        // For subsequent blocks, read 1 unknown byte before reading the address
        uint8_t unknown_byte;
        if (fread(&unknown_byte, 1, 1, file) != 1) break;
        printf("Unknown byte: %02x\n", unknown_byte);
    }

    fclose(file);
}


int writeLoc(USB* dev, libusbAPI api, packet* pkt, uint32_t addrs, const char *dta)
{
    size_t length = strlen(dta);
    uint8_t *data = malloc(length/2);
    
    if((length % 2) == 1)
    {
        printf("Invalid Command!!!!!");
        return -3;
    }
    
    for(int i = 0; i < length; i += 2)
    {
		sscanf(&dta[i], "%2hhx", &data[i / 2]);
	}
	
	printf("Sending RAW Data of size: %lu\n", length/2);

	printf("\n");
	
	sendBinData(dev, api, pkt, addrs, data, length/2);
	
    free(data);
	return 0;
}

// This function is to send raw Binary File on address
int sendBinaryFile(USB* dev, libusbAPI api, packet* pkt, uint32_t address, const char *binFile)
{
	FILE *file = fopen(binFile, "rb");
    if (file == NULL) 
    {
        printf("Error opening file");
        return 0;
    }
    
    // Move file pointer to the end of the file
    fseek(file, 0, SEEK_END);

    // Get the current position in the file, which is the file size
    size_t size = ftell(file);
	
	// Set the pointer again at start
	fseek(file, 0, SEEK_SET);
	
	uint8_t *data = malloc(size);
        if (data == NULL) 
        {
            perror("Memory allocation failed");
            fclose(file);
            return -1;
        }

        if (fread(data, 1, size, file) != size) 
        {
            printf("Error reading data block.\n");
            free(data);
        }
    printf("sendBinaryFile: sending binary, size: %08zX", size);
	sendBinData(dev, api, pkt, address, data, size);
    free(data);
    
    fclose(file);
	return 0;
}

uint32_t* readWord(USB* dev, libusbAPI api, packet* pkt, uint32_t addrs)
{
	uint32_t* retWord = (uint32_t*)malloc(0x01);
	uint8_t* stream = (uint8_t*)malloc(0x16);
	pkt->snd->cmd = CMD_READ_WORD;
	pkt->snd->body.read_word.address = __builtin_bswap32(addrs);
	
	pkt->snd->sizeOfStream = 0x0009;
	pkt->snd->body.read_word.ask_size = 0x0001;
	pkt->snd->body.read_word.frame_id = 0x03;
	
	calCrc(pkt);
	stream = streamGen(pkt);
	
	dev->sizeOfData = pkt->snd->sizeOfPkt;
	dev->data = stream;
	
	sendData(dev, &api);
	
	dev->data = stream;
	dev->sizeOfData = 0x16;
	readData(dev, &api);
	memcpy(retWord, dev->data + HEADER_SKIP_BYTES, 0x04);
	
	printf("\nRead WORD: %08X\n------------------", *retWord);
	free(stream);
	return retWord;
}

void writeWord(USB* dev, libusbAPI api, packet* pkt, uint32_t addrs, uint32_t word)
{
#if DEBUGMODE == 1
	printf("\nwriteWord(): \n");
#endif

	pkt->snd->cmd = CMD_WRITE_WORD;
	pkt->snd->body.write_word.address = __builtin_bswap32(addrs);
	pkt->snd->body.write_word.word = __builtin_bswap32(word);
	pkt->snd->sizeOfStream = 0x000A;
	calCrc(pkt);
	uint8_t* stream = (uint8_t*)malloc(0x12);
	stream = streamGen(pkt);
	dev->sizeOfData = pkt->snd->sizeOfPkt;
	dev->data = stream;
	sendData(dev, &api);
	free(stream);
}

void intRegWrite(USB* dev, libusbAPI api, packet* pkt, uint32_t reg, uint8_t value)
{
#if DEBUGMODE == 1
	printf("\nintRegWrite: \n");
#endif

	pkt->snd->cmd = CMD_WRITE_INTERNAL_REG;
	pkt->snd->body.write_internal_reg.unkn2 = __builtin_bswap32(reg);
	pkt->snd->body.write_internal_reg.unkn1 = value;
	pkt->snd->sizeOfStream = 0x0007;
	calCrc(pkt);
	uint8_t* stream = (uint8_t*)malloc(0x12);
	stream = streamGen(pkt);
	dev->sizeOfData = pkt->snd->sizeOfPkt;
	dev->data = stream;
	sendData(dev, &api);
	free(stream);
}

void readUnk(USB* dev, libusbAPI api)
{
	uint8_t* stream = (uint8_t*)malloc(MAX_STREAM_SIZE);
	dev->data = stream;
	dev->sizeOfData = 0x16;
	while((readData(dev, &api) != 1 ));
	free(stream);
}

/*
 * This function is to test 8809 Loader
 * You can change properties if you want but remember
 * We do have tw ping-pong buffers here 1) 0x81c05954
 * 2) 0x81c05968 3) 0x81c05980
 * 
 * 0x00000000				cmd
 * 0x00f03f00				flashAddr = 0x003ff000
 * 0x008000a2				ramAddrs  = 0xa2008000
 * 0x00000100				size      = 0x00010000 // 00001000
 * 0x00000000				fcs
 * 
 * 
 * 
 */

void testLoader8809(USB* dev, libusbAPI api, packet* pkt, const char *loader)
{
	readWord(dev, api, pkt, 0xa1a04410);
	writeWord(dev, api, pkt, 0xa1a25000, 0x00000066);
	writeWord(dev, api, pkt, 0xa1a25000, 0x00000099);
	intRegWrite(dev, api, pkt, 0x00000005, 0xEE);
	readWord(dev, api, pkt, 0x01a24000);
	readWord(dev, api, pkt, 0x01a000a0);
	writeWord(dev, api, pkt, 0xa1a25000, 0x00000066);
	writeWord(dev, api, pkt, 0xa1a25000, 0x00000099);
	intRegWrite(dev, api, pkt, 0x00000005, 0xFD);
	writeWord(dev, api, pkt, 0x01a000a0, 0x00200000);
	readWord(dev, api, pkt, 0x01a000a0);
	readWord(dev, api, pkt, 0x81c000cc);
	writeWord(dev, api, pkt, 0x81c000cc, 0x00000006);
	readUnk(dev, api);
	sendLoader(dev, api, pkt, loader);
	writeWord(dev, api, pkt, 0x81c000a0, 0x000000FF);
	intRegWrite(dev, api, pkt, 0x00000005, 0xFF);
	readUnk(dev, api);
	readWord(dev, api, pkt, 0x81c0027c);
	readWord(dev, api, pkt, 0x81c05b78);
	readWord(dev, api, pkt, 0x81c05950);
	readWord(dev, api, pkt, 0x81c0597c);
	readWord(dev, api, pkt, 0x81c05980);
	readWord(dev, api, pkt, 0x81c05984);
	readWord(dev, api, pkt, 0x81c05988);
	
	readUnk(dev, api);
	
	writeLoc(dev, api, pkt, 0x81c05954, "0400000000000000000000000000000000000000");
	
	
	readUnk(dev, api);
	printf("\nreadfirm\n");
	readUnk(dev, api);
}


int main(int argc, char *argv[])
{
	
	
	USB dev;
	libusbAPI api;
	loadDevProp(&dev);
	
	packet* pkt = initSnR();
	
	
	if (argc < 2) {
        printf("\n\nRUDRA - RDA Firmware Tool v0.2 - Open Source Software\n");
		printf("Author: @vixxkigoli\n");
		printf("This software is open source and intended for educational purposes.\n\n");
		printf("Usage:\n");
		printf("  -l <loader_file.fp> : Load firmware loader file\n");
		printf("  -r <start_addr> <read_bytes_per_cycle> <total_capacity> <output_file> : Read firmware\n");
		printf("  -b <location> <binary_file> : Send binary file to specified memory location\n");
		printf("  -w <location> <data> : Write data to specified memory location\n");
		printf("  -t <loader_file.fp> : Test 8809 CPU with Loader\n");
		printf("\nExample Commands:\n");
		printf("  sudo ./rudra -l ./8809_00400000_usb.fp\n");
		printf("  sudo ./rudra -r 0x08000000 0x0400 0x00400000 firmware.bin\n");
		printf("  sudo ./rudra -b 0x01c0027c binary.bin\n");
		printf("  sudo ./rudra -w 0x01c000a0 \"000000008002c08100000000000000000000000000000000\"\n");
		printf("  sudo ./rudra -t ./8809_00400000_usb.fp \n");
        return 1;
    }

	
	setupLibUsbApi(&api, &dev);
	
	
	if (strcmp(argv[1], "-l") == 0 && argc == 3) {
        sendLoader(&dev, api, pkt, argv[2]);
    }
    else if (strcmp(argv[1], "-r") == 0 && argc == 6) {
        unsigned long start_addr = strtoul(argv[2], NULL, 16);
        unsigned long read_bytes_per_cycle = strtoul(argv[3], NULL, 16);
        unsigned long total_capacity = strtoul(argv[4], NULL, 16);
        read_firmware(&dev, api, pkt, start_addr, read_bytes_per_cycle, total_capacity, argv[5]);
    } 
    else if (strcmp(argv[1], "-b") == 0 && argc == 4) {
        unsigned long location = strtoul(argv[2], NULL, 16);
        sendBinaryFile(&dev, api, pkt, location, argv[3]);
    } 
    else if (strcmp(argv[1], "-w") == 0 && argc == 4) {
        unsigned long location = strtoul(argv[2], NULL, 16);
        writeLoc(&dev, api, pkt, location, argv[3]);
    } 
    else if (strcmp(argv[1], "-t") == 0 && argc == 3) {
        testLoader8809(&dev, api, pkt, argv[2]);
    } 
    else 
    {
        printf("Invalid usage. Please follow the correct syntax.\n");
        return 1;
    }
	
	libusb_close(api.handle);
    libusb_exit(api.ctx);
    
	freeSnR(pkt);
	
	
	return 0;
}
