/* 
 * Project Rudra
 * 
 * File: progBuffer.h
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

#ifndef _FLASH_PROG_MAP_H_
#define _FLASH_PROG_MAP_H_
#include <stdint.h>
#include <stdlib.h> // mmloc
#include "protocol.h"

#define MAX_CACHE 0x00010000

typedef enum
{
    NONE                                    = 0x00000000,
    PROG              	                    = 0x00000001,
    ERASE_SECT                              = 0x00000002,
    ERASE_CHIP                              = 0x00000003,
    END                                     = 0x00000004,
    CHECK_CS                                = 0x00000005,
    GET_FINALIZE_INF                        = 0x00000006,
    RESTART                                 = 0x00000007,
    DONE                                    = 0xFFFFFF9C,
    ERROR                                   = 0xFFFFFF9D,
    FCS_ERROR                               = 0xFFFFFF9E,
    FLASH_NOT_AT_FF                         = 0xFFFFFF9F
} cmd_type;

#define FLASH_PROG_READY                   (0XF0)
#define FLASH_PROG_ERROR                   (0XE0)
#define FLASH_PROG_UNKNOWN                 (0XD0)
#define FLASH_PROG_MEM_RESET               (0XC0)
#define FLASH_PROG_MEM_ERROR               (0XCE)
#define BUFFER_SIZE                          (4*1024)
#define PROTOCOL_MAJOR                       (0XFA01)
#define PROTOCOL_MINOR                       (0XFB04)


typedef struct
{
    uint32_t                         flashAddr;                    //0x00000004
    uint32_t                         ramAddr;                      //0x00000008
    uint32_t                         size;                         //0x0000000C
    uint32_t                         fcs;                          //0x00000010
    cmd_type             			 cmd;                          //0x00000010
    uint32_t						 bufferStrmSize;				// size for future use
} command; //Size : 0x14

typedef struct
{
    uint16_t                         major;                        //0x00000000
    uint16_t                         minor;                        //0x00000002
} prot_ver; //Size : 0x4

typedef struct
{
    prot_ver           				 protocolVersion;              //0x00000000
    command             			 commandDescr[2];              //0x00000004
    uint8_t*                         dataBufferA;                  //0x0000002C
    uint8_t*                         dataBufferB;                  //0x00000030
    uint32_t                         fpcSize;                      //0x00000038
} progMap; //Size : 0x3C

uint8_t* blStreamGen(command* cmd)
{
	uint8_t *data = malloc(0x14);
	uint8_t* ptr = data;
	add_uint32(&ptr, __builtin_bswap32(cmd->cmd));
	add_uint32(&ptr, __builtin_bswap32(cmd->flashAddr));
	add_uint32(&ptr, __builtin_bswap32(cmd->ramAddr));
	add_uint32(&ptr, __builtin_bswap32(cmd->size));
	add_uint32(&ptr, __builtin_bswap32(cmd->fcs));
	cmd->bufferStrmSize = ptr - data;
	return data;
}

#endif

