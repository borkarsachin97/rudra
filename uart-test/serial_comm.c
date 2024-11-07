#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include "protocol.h"

#define SERIAL_PORT "/dev/ttyUSB0"
#define BAUD_RATE B921600

int init_serial()
{
	int serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC);
    if (serial_fd < 0) 
    {
        perror("Error opening serial port");
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(serial_fd, &tty) != 0) 
    {
        perror("Error from tcgetattr");
        close(serial_fd);
        return -2;
    }

    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;  // 0.1 second timeout for reads

    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        close(serial_fd);
        return -3;
    }
	return serial_fd;
}

int read_firmware(int serial_fd, packet* pkt, uint32_t addrs, uint32_t flashsize)
{
	pkt->snd->start_signature = 0xAD;
    pkt->snd->cmd = CMD_READ_WORD;
	uint32_t tmpAddr = addrs;
	pkt->snd->sizeOfStream = 0x0007;
	pkt->snd->body.read_word.counter = 0x65;
	uint8_t* stream;
	 
	for( ; tmpAddr < addrs + flashsize; tmpAddr += 4)
	{
		pkt->snd->body.read_word.address = __builtin_bswap32(tmpAddr);
		calCrc(pkt);
		stream = streamGen(pkt);
		
		int write_result = write(serial_fd, stream, pkt->snd->sizeOfPkt);
		if (write_result < 0) 
		{
			perror("Error writing to serial port");
			close(serial_fd);
			return 1;
		}
		printf("Sent: ");
		for (int i = 0; i < pkt->snd->sizeOfPkt; i++) 
		{
			printf("%02X ", stream[i]);
		}
		printf("\n");
		printf("Received: ");
		uint8_t* response = (uint8_t*)malloc(0x30);
		int bytes_read;
		int counter = 0;
		do
		{
			bytes_read = read(serial_fd, response, sizeof(response));
			for (int i = 0; i < bytes_read; i++) 
			{
				printf("%02X ", response[i]);
			}
			counter += bytes_read;
			printf("\n");
		} while (bytes_read > 0);
		printf("Read Bytes: %d\n", counter);
		if (counter < 10)
		{
			break;
		}
		free(stream);
		free(response);
		pkt->snd->body.read_word.counter++;
	}
	return 0;
}

void readWord(int serial_fd, packet* pkt, uint32_t addrs)
{
	//uint32_t* retWord = (uint32_t*)malloc(0x01);
	uint8_t* stream = (uint8_t*)malloc(0x16);
	pkt->snd->start_signature = 0xAD;
	pkt->snd->sizeOfStream = 0x0007;
	pkt->snd->cmd = CMD_READ_WORD;
	pkt->snd->body.read_word.address = __builtin_bswap32(addrs);
	pkt->snd->body.read_word.counter = 0x65;
	calCrc(pkt);
	stream = streamGen(pkt);
	int write_result = write(serial_fd, stream, pkt->snd->sizeOfPkt);
	if (write_result < 0) 
	{
		perror("Error writing to serial port");
		close(serial_fd);
		//return 1;
	}
	printf("Sent: ");
	for (int i = 0; i < pkt->snd->sizeOfPkt; i++) 
	{
		printf("%02X ", stream[i]);
	}
	printf("\n");
	printf("Received: ");
	uint8_t* response = (uint8_t*)malloc(0x30);
	int bytes_read;
	int counter = 0;
	do
	{
		bytes_read = read(serial_fd, response, sizeof(response));
		for (int i = 0; i < bytes_read; i++) 
		{
			printf("%02X ", response[i]);
		}
		counter += bytes_read;
		printf("\n");
	} while (bytes_read > 0);
	printf("Read Bytes: %d\n", counter);
	free(stream);
}

void writeWord(int serial_fd, packet* pkt, uint32_t addrs, uint32_t word)
{
	pkt->snd->start_signature = 0xAD;
	pkt->snd->cmd = CMD_WRITE_WORD;
	pkt->snd->body.write_word.address = __builtin_bswap32(addrs);
	pkt->snd->body.write_word.word = __builtin_bswap32(word);
	pkt->snd->sizeOfStream = 0x000A;
	calCrc(pkt);
	uint8_t* stream = (uint8_t*)malloc(0x12);
	stream = streamGen(pkt);
	int write_result = write(serial_fd, stream, pkt->snd->sizeOfPkt);
	if (write_result < 0) 
	{
		perror("Error writing to serial port");
		close(serial_fd);
		//return 1;
	}
	printf("Sent: ");
	for (int i = 0; i < pkt->snd->sizeOfPkt; i++) 
	{
		printf("%02X ", stream[i]);
	}
	printf("\n");
	free(stream);
}

void intRegWrite(int serial_fd, packet* pkt, uint32_t reg, uint8_t value)
{
	pkt->snd->start_signature = 0xAD;
	pkt->snd->cmd = CMD_WRITE_INTERNAL_REG;
	pkt->snd->body.write_internal_reg.unkn2 = __builtin_bswap32(reg);
	pkt->snd->body.write_internal_reg.unkn1 = value;
	pkt->snd->sizeOfStream = 0x0007;
	calCrc(pkt);
	uint8_t* stream = (uint8_t*)malloc(0x12);
	stream = streamGen(pkt);
	int write_result = write(serial_fd, stream, pkt->snd->sizeOfPkt);
	if (write_result < 0) 
	{
		perror("Error writing to serial port");
		close(serial_fd);
		//return 1;
	}
	printf("Sent: ");
	for (int i = 0; i < pkt->snd->sizeOfPkt; i++) 
	{
		printf("%02X ", stream[i]);
	}
	printf("\n");
	free(stream);
}

int getPropVia_Loader(int serial_fd, packet* pkt)
{
	writeWord(serial_fd, pkt, 0x81c000a0, 0x000000FF);
	intRegWrite(serial_fd, pkt, 0x00000005, 0xFF);
	//readUnk(dev, api);
	readWord(serial_fd, pkt, 0x81c0027c);
	readWord(serial_fd, pkt, 0x81c05b78);
	readWord(serial_fd, pkt, 0x81c05950);
	readWord(serial_fd, pkt, 0x81c0597c);
	readWord(serial_fd, pkt, 0x81c05980);
	readWord(serial_fd, pkt, 0x81c05984);
	readWord(serial_fd, pkt, 0x81c05988);
	//readUnk(dev, api);
	return 0;
}

int preLoaderFunc(int serial_fd, packet* pkt)
{
	readWord(serial_fd, pkt, 0xa1a04410);
	writeWord(serial_fd, pkt, 0xa1a25000, 0x00000066);
	writeWord(serial_fd, pkt, 0xa1a25000, 0x00000099);
	intRegWrite(serial_fd, pkt, 0x00000005, 0xEE);
	readWord(serial_fd, pkt, 0x01a24000);
	readWord(serial_fd, pkt, 0x01a000a0);
	writeWord(serial_fd, pkt, 0xa1a25000, 0x00000066);
	writeWord(serial_fd, pkt, 0xa1a25000, 0x00000099);
	intRegWrite(serial_fd, pkt, 0x00000005, 0xFD);
	writeWord(serial_fd, pkt, 0x01a000a0, 0x00200000);
	readWord(serial_fd, pkt, 0x01a000a0);
	readWord(serial_fd, pkt, 0x81c000cc);
	writeWord(serial_fd, pkt, 0x81c000cc, 0x00000006);
	//readUnk(dev, api);
	return 0;
}

void sendLoader(int serial_fd, packet* pkt, const char *loader) 
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
        
        uint32_t* ptr;
        ptr = data;
        
        for(uint32_t i = 0; i < size; i+=4)
        {
			//printf("%08X ", *ptr);
			writeWord(serial_fd, pkt, address, *ptr);
			address += 4;
			ptr++;
		}
        
        //sendBinData(serial_fd, pkt, address, data, size);
        
        free(data);
        // For subsequent blocks, read 1 unknown byte before reading the address
        uint8_t unknown_byte;
        if (fread(&unknown_byte, 1, 1, file) != 1) break;
        printf("Unknown byte: %02x\n", unknown_byte);
    }

    fclose(file);
}

void test_8809Loader(int serial_fd, packet* pkt, const char *loader)
{
	preLoaderFunc(serial_fd, pkt);
	sendLoader(serial_fd, pkt, loader);
	getPropVia_Loader(serial_fd, pkt);
	
	//for(uint32_t addr = 0x81e00000; addr < 0x81e01000; addr+=4)
	//{
	//	readWord(serial_fd, pkt, addr);
	//} // 02 F4 58 C0 81
	writeWord(serial_fd, pkt, 0x88000000, 0x00000000);
	readWord(serial_fd, pkt, 0x88000000);
}

int main() 
{
    
    int serial_fd = init_serial();
    if(serial_fd < 0)
    {
		return 1;
	}
    
    // Flush any residual data in the input buffer before sending new data
    tcflush(serial_fd, TCIFLUSH);
    
    packet* pkt = initSnR();
    //test_8809Loader(serial_fd, pkt, "8809_00400000.fp");
	//read_firmware(serial_fd, pkt, 0x08000000, 0x00400000);
	//sendLoader(1, pkt, "8809_00400000.fp");
	
	
    close(serial_fd);
    return 0;
}
