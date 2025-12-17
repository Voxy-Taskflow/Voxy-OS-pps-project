// ata.c - ATA PIO driver implementation

#include "ata.h"

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);

// Read 16-bit word from port
static unsigned short inw(unsigned short port) {
    unsigned short result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Wait for drive to be ready
static int ata_wait_ready(void) {
    unsigned char status;
    int timeout = 100000;
    
    while (timeout--) {
        status = inb(ATA_PRIMARY_STATUS);
        
        // Check if busy bit is clear and ready bit is set
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRDY)) {
            return 0;  // Success
        }
    }
    
    return -1;  // Timeout
}

// Wait for data ready
static int ata_wait_data(void) {
    unsigned char status;
    int timeout = 100000;
    
    while (timeout--) {
        status = inb(ATA_PRIMARY_STATUS);
        
        // Check if data request bit is set
        if (status & ATA_STATUS_DRQ) {
            return 0;  // Success
        }
        
        // Check for error
        if (status & ATA_STATUS_ERR) {
            return -1;  // Error
        }
    }
    
    return -1;  // Timeout
}

// Read a sector from disk
int ata_read_sector(unsigned int lba, unsigned char* buffer) {
    // Wait for drive to be ready
    if (ata_wait_ready() != 0) {
        return -1;  // Drive not ready
    }
    
    // Select drive and set LBA mode (drive 0, LBA mode, bits 24-27 of LBA)
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    // Set sector count to 1
    outb(ATA_PRIMARY_SECCOUNT, 1);
    
    // Set LBA address
    outb(ATA_PRIMARY_LBA_LO, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);
    
    // Send READ command
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);
    
    // Wait for data to be ready
    if (ata_wait_data() != 0) {
        return -1;  // Data not ready or error
    }
    
    // Read 256 words (512 bytes)
    unsigned short* buf = (unsigned short*)buffer;
    for (int i = 0; i < 256; i++) {
        buf[i] = inw(ATA_PRIMARY_DATA);
    }
    
    return 0;  // Success
}

// Write sector (stub for now)
int ata_write_sector(unsigned int lba, unsigned char* buffer) {
    // TODO: Implement tomorrow
    return -1;
}
