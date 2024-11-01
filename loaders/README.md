# Loader Files for RDA CPUs and Chips

## How to Use

1. **Load the Loader File into RAM**:  
   This file is loaded into RAM and executed by the CPU, creating two "Ping-Pong" buffers (in this case, located at `0x81c05954` and `0x81c05968`). These buffers allow the execution of commands.

2. **Ping-Pong Buffers for Command Execution**:  
   The buffers need to be configured as follows to execute specific commands effectively.

### Loader Capabilities
The loader supports:
- Erasing SPI memory/flash
- Flash reprogramming
- Hardware testing
- Possible debugging (gdb interface)

---

# About Ping-Pong Buffer
For RDA/Coolsand 8809 CPUs:
- **File**: `8809_00400000.fp`
- **RAM Location**: `0x01c0027c` (contained within the file)

To execute the loader properly:
1. **Pre-Load Tweaks**: Certain internal registers need adjustments (see `rudra.c`, `preLoaderFunc()`).
2. **Buffer Command Structure**: Each buffer is 20 bytes (5 words) representing:
   - **Command**: The operation to perform
   - **Flash Sector Address**: Sector of flash memory
   - **RAM Address**: RAM address for flash programming
   - **Size**: Size of the sector
   - **fcs**: Checksum (not typically needed)

**Note**: Data is in little-endian format (source: `progBuffer.h`).

### Flash Programming Steps
To program the flash:
1. **Load data** into the first buffer (sector and size).
2. **Send the erase command** (e.g., `0x00000002`) to the first buffer.
3. **Load the second buffer** with relevant data (sector, size, RAM address where data is stored).
4. **Send data to RAM** for each 64KB part.
5. **Send the program command** (`0x00000001`) to the second buffer.
6. **Repeat steps 1-5** until the firmware is completely flashed.

**Status Notifications**: After command execution, the loader sends USB events:
- `F0`: First buffer completed, ready for a new command.
- `F1`: Second buffer completed, ready for the next task.

---

# Buffer Data Example
Below is sample data sent by Rudra during flash/erase:

```plaintext
0000000000000000000000a00000010000000000
----------
0000000000000100000000a00000010000000000
----------
...and so on.
```
Explanation `e.g., 0000000000000100000000a00000010000000000`:

**Command: Empty**; will be filled later
**Sector Address**: `0x00000001`
**RAM Location**: `0xa0000000` (where firmware is stored, 64KB per sector)
**Size**: `0x00010000` (sector/data size in RAM)
**Checksum**: `0x00000000` (leave empty)

# Commands
```plaintext
    NONE                   = 0x00000000
    PROG                   = 0x00000001
    ERASE_SECT             = 0x00000002
    ERASE_CHIP             = 0x00000003
    END                    = 0x00000004
    CHECK_CS               = 0x00000005
    GET_FINALIZE_INF       = 0x00000006
    RESTART                = 0x00000007
    DONE                   = 0xFFFFFF9C
    ERROR                  = 0xFFFFFF9D
    FCS_ERROR              = 0xFFFFFF9E
    FLASH_NOT_AT_FF        = 0xFFFFFF9F
```
# Event Codes from the Loader
## The loader also returns status codes over USB:

```plaintext
- EVENT_FLASH_PROG_READY           = 0xF0
- EVENT_FLASH_PROG_ERROR           = 0xE0
- EVENT_FLASH_PROG_UNKNOWN         = 0xD0
- EVENT_FLASH_PROG_MEM_RESET       = 0xC0
- EVENT_FLASH_PROG_MEM_ERROR       = 0xCE
- FPC_PROTOCOL_MAJOR               = 0xFA01
- FPC_PROTOCOL_MINOR               = 0xFB04
```
# Current Progress and Next Steps
- We have successfully loaded the loader, erased memory, and reprogrammed it.
# Future needs:
- Custom firmware development for these CPUs.
- More contributors and engineers for further development.
- Support for continued development (consider supporting with a coffee 😉).
