# Loader Files for RDA CPUS and Chips

## How to Use them ?

This File need to be flashed/loaded into RAM
Then they gets executed by CPU and they 
creates "3" Ping-Pong Buffer
1 - 0x81c05954
2 - 0x81c05968
3 - 0x81c0597c		// Arround Here maybe
These buffers can execute commands

These Buffers needs to be tweaked

This Loader can do following things:
- Can erase a SPI Memory / flash
- Can Reprogramm the flash
- Can test hardware
- Might be, give gdb interface

# About Ping-Pong Buffer
In my case RDA/Coolsand 8809 CPU
File:		8809_00400000.fp
I have Loaded the file, file itself contains RAM Location (0x01c0027c)
Before Loading the Loader, some internal register need to be tweaked
for More Info, Read rudra.c, testLoader8809() function,
Now program gets executed, it creates 3 Ping-Pong Buffers
The buffer accept commands and data,
Remember: First data needs to be placed and then command needs to be send

```bash
# Warning: Please don't play with buffer, specially
  with 0x0000001 & 0x00000002 Commands, I am not able 
  to flash/reprogram the flash currently
  Untill and unless we dont find solution to write to flash
  Please do test the hardware, dump the binaries/memory
  tweak GPIO and Internal Regs, dont try to erase and 
  program the flash
  
# Example of buffer commands
1) 0000000000f03f00008000a20000010000000000
 * 0x00000000				cmd
 * 0x00f03f00				flashAddr = 0x003ff000
 * 0x008000a2				ramAddrs  = 0xa2008000
 * 0x00000100				size      = 0x00010000 // 00001000
 * 0x00000000				fcs
 
2) 0400000000000000000000000000000000000000

	FPC_NONE                                    = 0x00000000,
    FPC_PROGRAM                                 = 0x00000001,
    FPC_ERASE_SECTOR                            = 0x00000002,
    FPC_ERASE_CHIP                              = 0x00000003,
    FPC_END                                     = 0x00000004,
    FPC_CHECK_FCS                               = 0x00000005,
    FPC_GET_FINALIZE_INFO                       = 0x00000006,
    FPC_RESTART                                 = 0x00000007,
    FPC_DONE                                    = 0xFFFFFF9C,
    FPC_ERROR                                   = 0xFFFFFF9D,
    FPC_FCS_ERROR                               = 0xFFFFFF9E,
    FPC_FLASH_NOT_AT_FF                         = 0xFFFFFF9F
    Source: flash_prog_map.h
```

# Understanding the loader commands
## Loader also send back the data, via usb
## Here is the list of the codes
```bash
- EVENT_FLASH_PROG_READY                   (0XF0)
- EVENT_FLASH_PROG_ERROR                   (0XE0)
- EVENT_FLASH_PROG_UNKNOWN                 (0XD0)
- EVENT_FLASH_PROG_MEM_RESET               (0XC0)
- EVENT_FLASH_PROG_MEM_ERROR               (0XCE)
- FPC_PROTOCOL_MAJOR                       (0XFA01)
- FPC_PROTOCOL_MINOR                       (0XFB04)
```

# Where are we ? What should we do in future ?
- We are at position, where we can load the loader
- Can Erase Memory
- Can Reprogram it (Currently under Research/ Partially Working)

# We Need
- Custome firmware for these CPUs
- More codes and Contributors/Engineers
- Buy me a cofee so I would able to work on this project ;-)
