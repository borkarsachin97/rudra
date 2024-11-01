/* 
 * Project Rudra
 * 
 * File: protocol.h
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
 
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdint.h>
#include <stdlib.h>


#define DEBUG 					0xFF

// Command definitions
#define CMD_CPU_EVENT           0x00
#define CMD_READ_WORD           0x02
#define CMD_READ_BULK_DATA      0x03
#define CMD_READ_INTERNAL_REG    0x04
#define CMD_WRITE_WORD          0x82
#define CMD_WRITE_BULK_DATA     0x83
#define CMD_WRITE_INTERNAL_REG    0x84

#define HEADER_SKIP_BYTES 5
#define TRAILER_SKIP_BYTES 1


#define PACKET_START 				0xAD
#define MAX_STREAM_SIZE 			0x1030 // MAX USB Capacity
#define MAX_DATA_SIZE 				0x1000

// Structure for the USB packet
typedef struct 
{
    uint8_t start_signature;          // 0xAD
    uint16_t sizeOfStream;            // 0xXXXX data size, flow_id to end crc expelled
    uint8_t flow_id;                  // [4] Flow ID, CRC (initially)
    
    uint8_t cmd;                      // [5] Command byte
    union 
    //struct
    {
		
        struct 
        {
            uint32_t address;        // Address for the word read
            uint8_t frame_id;		 // Frame id for frame answer verification method
            uint16_t ask_size;
        } read_word;                 // For CMD 0x02 (Read WORD)

        struct 
        {
            uint32_t address;        // Address for the bulk data read
            uint8_t frame_id;         // for Frame Matching
            uint16_t askdSize;		 // Data per request
        } read_bulk_data;            // For CMD 0x03 (Read BULK DATA)

        struct 
        {
            uint32_t value;        // Address for internal register read
        } read_internal_reg;         // For CMD 0x04 (Read Internal Register)

        struct 
        {
            uint32_t address;        // Address for the word write
            uint32_t word;           // Data to write
        } write_word;                // For CMD 0x82 (Write WORD)
	
        struct 
        {
            uint32_t address;        // Address for the bulk data write
            uint8_t* data; 			// Pointer to data, stream of binary
            int sizeOfData;		// size of data, so we can append it in future
        } write_bulk_data;           // For CMD 0x83 (Write BULK DATA)

        struct 
        {
            uint8_t unkn1;    // Register number to write
            uint32_t unkn2;            // Data to write to the register
        } write_internal_reg;        // For CMD 0x84 (Write Internal Register)
    } body;
    
    uint8_t crc;                      // CRC byte
    int sizeOfPkt;					  // Size of Packet
} rdaUsbPkt;

rdaUsbPkt* initPkt()
{
	rdaUsbPkt* pkt = (rdaUsbPkt*)malloc(sizeof(rdaUsbPkt));
	pkt->start_signature = PACKET_START;
	pkt->flow_id = DEBUG;
	
	return pkt;
}

void free_pkt(rdaUsbPkt* pkt)
{
    if (pkt) 
    {
        if (pkt->cmd == CMD_WRITE_BULK_DATA && pkt->body.write_bulk_data.data) 
        {
            //free(pkt->body.write_bulk_data.data);
        }
        free(pkt);
    }
}

typedef struct
{
	rdaUsbPkt* snd;
	rdaUsbPkt* recv;	
}packet;

packet* initSnR()
{
	packet* pkt = (packet*)malloc(sizeof(packet));
	rdaUsbPkt* snd = initPkt();
	rdaUsbPkt* recv  = initPkt();
	pkt->snd = snd;
	pkt->recv = recv;
	return pkt;
}

void freeSnR(packet* pkt)
{
	free_pkt(pkt->snd);
	free_pkt(pkt->recv);
	free(pkt);
}

uint8_t xorCrc32(uint32_t addr, uint8_t crc)
{
	crc ^= (addr >> 24) & 0xFF;
	crc ^= (addr >> 16) & 0xFF;
	crc ^= (addr >> 8) & 0xFF;
	crc ^= addr & 0xFF;
	return crc;
}

uint8_t xorCrc16(uint16_t askdSize, uint8_t crc)
{
	crc ^= (askdSize >> 8);
	crc ^= askdSize & 0xFF;
	return crc;
}

uint8_t crcData(uint8_t *data, size_t size, uint8_t oldCrc) 
{
    uint8_t newCrc = oldCrc;
    for (size_t i = 0; i < size; i++) 
    {
        newCrc ^= data[i];
    }
    return newCrc;
}

uint8_t calCrc(packet* pkt)
{
	uint8_t crc = pkt->snd->flow_id;
	
	switch(pkt->snd->cmd)
	{
		case CMD_READ_WORD:
		{
			crc ^= pkt->snd->cmd;
			crc = xorCrc32(pkt->snd->body.read_word.address, crc);
			crc ^= pkt->snd->body.read_word.frame_id;
			crc = xorCrc16(pkt->snd->body.read_word.ask_size, crc);
			break;
		}
		
		case CMD_READ_BULK_DATA:
		{
			crc ^= pkt->snd->cmd;
			crc = xorCrc32(pkt->snd->body.read_bulk_data.address, crc);
			crc ^= pkt->snd->body.read_bulk_data.frame_id;
			crc = xorCrc16(pkt->snd->body.read_bulk_data.askdSize, crc);
			break;
		}
		
		case CMD_WRITE_WORD:
		{
			crc ^= pkt->snd->cmd;
			crc = xorCrc32(pkt->snd->body.write_word.address, crc);
			crc = xorCrc32(pkt->snd->body.write_word.word, crc);
			break;
		}
		
		case CMD_WRITE_BULK_DATA:
		{
			crc ^= pkt->snd->cmd;
			crc = xorCrc32(pkt->snd->body.write_bulk_data.address, crc);
			crc = crcData(pkt->snd->body.write_bulk_data.data, pkt->snd->body.write_bulk_data.sizeOfData, crc);
			break;
		}
		
		case CMD_WRITE_INTERNAL_REG:
		{
			crc ^= pkt->snd->cmd;
			crc = xorCrc32(pkt->snd->body.write_internal_reg.unkn2, crc);
			crc ^= pkt->snd->body.write_internal_reg.unkn1;
			break;
		}
		
		default:
		//printf("Unknown crc");
	}
	pkt->snd->crc = crc;
	return crc;
}

// Helper function to add a uint8_t to the buffer
void add_uint8(uint8_t** buffer, uint8_t value) 
{
    **buffer = value;
    (*buffer)++;
}

// Helper function to add a uint16_t to the buffer (in big-endian)
void add_uint16(uint8_t** buffer, uint16_t value) 
{
    **buffer = (value >> 8) & 0xFF;
    (*buffer)++;
    **buffer = value & 0xFF;
    (*buffer)++;
}

// Helper function to add a uint32_t to the buffer (in big-endian)
void add_uint32(uint8_t** buffer, uint32_t value) 
{
    **buffer = (value >> 24) & 0xFF;
    (*buffer)++;
    **buffer = (value >> 16) & 0xFF;
    (*buffer)++;
    **buffer = (value >> 8) & 0xFF;
    (*buffer)++;
    **buffer = value & 0xFF;
    (*buffer)++;
}

void addDatainStream(uint8_t** buffer, uint8_t *data, size_t size)
{
	for(int i = 0; i < size; i++)
	{
		**buffer = data[i];
		(*buffer)++;
	}
}

// Function to create the raw stream
uint8_t* streamGen(packet* pkt) 
{
    uint8_t* buffer = (uint8_t*)malloc(MAX_STREAM_SIZE);
    uint8_t* ptr = buffer;

	switch(pkt->snd->cmd)
		{
			
			case CMD_READ_WORD:
			{
				add_uint8(&ptr, pkt->snd->start_signature);
				add_uint16(&ptr, pkt->snd->sizeOfStream);
				add_uint8(&ptr, pkt->snd->flow_id);
				add_uint8(&ptr, pkt->snd->cmd);
				add_uint32(&ptr, pkt->snd->body.read_word.address);
				add_uint8(&ptr, pkt->snd->body.read_word.frame_id);
				add_uint16(&ptr, pkt->snd->body.read_word.ask_size);
				add_uint8(&ptr, pkt->snd->crc);
				pkt->snd->sizeOfPkt = ptr - buffer;
				break;
			}
			
			case CMD_READ_BULK_DATA:
			{
				add_uint8(&ptr, pkt->snd->start_signature);
				add_uint16(&ptr, pkt->snd->sizeOfStream);
				add_uint8(&ptr, pkt->snd->flow_id);
				add_uint8(&ptr, pkt->snd->cmd);
				add_uint32(&ptr, pkt->snd->body.read_bulk_data.address);
				add_uint8(&ptr, pkt->snd->body.read_bulk_data.frame_id);
				add_uint16(&ptr, pkt->snd->body.read_bulk_data.askdSize);
				add_uint8(&ptr, pkt->snd->crc);
				pkt->snd->sizeOfPkt = ptr - buffer;
				break;
			}
			
			case CMD_WRITE_WORD:
			{
				add_uint8(&ptr, pkt->snd->start_signature);
				add_uint16(&ptr, pkt->snd->sizeOfStream);
				add_uint8(&ptr, pkt->snd->flow_id);
				add_uint8(&ptr, pkt->snd->cmd);
				add_uint32(&ptr, pkt->snd->body.write_word.address);
				add_uint32(&ptr, pkt->snd->body.write_word.word);
				add_uint8(&ptr, pkt->snd->crc);
				pkt->snd->sizeOfPkt = ptr - buffer;
				break;
			}
			
			case CMD_WRITE_BULK_DATA:
			{
				add_uint8(&ptr, pkt->snd->start_signature);
				add_uint16(&ptr, pkt->snd->sizeOfStream);
				add_uint8(&ptr, pkt->snd->flow_id);
				add_uint8(&ptr, pkt->snd->cmd);
				add_uint32(&ptr, pkt->snd->body.write_bulk_data.address);
				addDatainStream(&ptr, pkt->snd->body.write_bulk_data.data, pkt->snd->body.write_bulk_data.sizeOfData);
				add_uint8(&ptr, pkt->snd->crc);
				pkt->snd->sizeOfPkt = ptr - buffer;
				break;
			}
			
			case CMD_WRITE_INTERNAL_REG:
			{
				add_uint8(&ptr, pkt->snd->start_signature);
				add_uint16(&ptr, pkt->snd->sizeOfStream);
				add_uint8(&ptr, pkt->snd->flow_id);
				add_uint8(&ptr, pkt->snd->cmd);
				add_uint32(&ptr, pkt->snd->body.write_internal_reg.unkn2);
				add_uint8(&ptr, pkt->snd->body.write_internal_reg.unkn1);
				add_uint8(&ptr, pkt->snd->crc);
				pkt->snd->sizeOfPkt = ptr - buffer;
				break;
			}
			
			default:
				//printf("Unknown cmd inside streamGen");
		}
    return buffer;
}

#endif // _PROTOCOL_H_
