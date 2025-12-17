// ata.h - ATA PIO driver header

#ifndef ATA_H
#define ATA_H

#define ATA_PRIMARY_DATA       0x1F0
#define ATA_PRIMARY_ERROR      0x1F1
#define ATA_PRIMARY_SECCOUNT   0x1F2
#define ATA_PRIMARY_LBA_LO     0x1F3
#define ATA_PRIMARY_LBA_MID    0x1F4
#define ATA_PRIMARY_LBA_HI     0x1F5
#define ATA_PRIMARY_DRIVE_HEAD 0x1F6
#define ATA_PRIMARY_STATUS     0x1F7
#define ATA_PRIMARY_COMMAND    0x1F7

#define ATA_CMD_READ_PIO       0x20
#define ATA_CMD_WRITE_PIO      0x30

#define ATA_STATUS_BSY         0x80
#define ATA_STATUS_DRDY        0x40
#define ATA_STATUS_DRQ         0x08
#define ATA_STATUS_ERR         0x01

// Read a sector from disk
int ata_read_sector(unsigned int lba, unsigned char* buffer);

// Write a sector to disk (implement later)
int ata_write_sector(unsigned int lba, unsigned char* buffer);

#endif
