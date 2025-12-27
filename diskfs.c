// diskfs.c - Disk-based filesystem

extern int ata_read_sector(unsigned int lba, unsigned char* buffer);
extern int ata_write_sector(unsigned int lba, unsigned char* buffer);
extern void memcpy(void* dest, const void* src, int n);
extern int strcmp(const char* s1, const char* s2);
extern void strcpy(char* dest, const char* src);
extern int strlen(const char* str);

#define FILE_TABLE_SECTOR 50
#define FILE_DATA_START_SECTOR 51
#define MAX_FILES 16
#define FILENAME_LEN 16
#define MAX_FILE_SIZE 1024
#define SECTORS_PER_FILE 2  // 1KB = 2 sectors

// File table entry structure (32 bytes each, 16 fit in 512 bytes)
struct disk_file_entry {
    char filename[FILENAME_LEN];    // 16 bytes
    unsigned short start_sector;    // 2 bytes
    unsigned short size_bytes;      // 2 bytes
    unsigned char used;             // 1 byte
    char reserved[11];              // 11 bytes padding
};

// Cache for file table in memory
static struct disk_file_entry file_table[MAX_FILES];
static int file_table_loaded = 0;

// Load file table from disk
static int load_file_table(void) {
    unsigned char buffer[512];
    
    if (ata_read_sector(FILE_TABLE_SECTOR, buffer) != 0) {
        return -1;  // Read failed
    }
    
    // Copy to file table
    memcpy(file_table, buffer, sizeof(file_table));
    file_table_loaded = 1;
    
    return 0;
}

// Save file table to disk
static int save_file_table(void) {
    unsigned char buffer[512];
    
    // Clear buffer
    for (int i = 0; i < 512; i++) {
        buffer[i] = 0;
    }
    
    // Copy file table to buffer
    memcpy(buffer, file_table, sizeof(file_table));
    
    if (ata_write_sector(FILE_TABLE_SECTOR, buffer) != 0) {
        return -1;  // Write failed
    }
    
    return 0;
}

// Initialize filesystem (load file table)
int diskfs_init(void) {
    return load_file_table();
}

// Find file in table
static int diskfs_find_file(const char* filename) {
    if (!file_table_loaded) {
        load_file_table();
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].filename, filename) == 0) {
            return i;
        }
    }
    
    return -1;
}

// Save file to disk
int diskfs_save_file(const char* filename, const char* data, int size) {
    if (size > MAX_FILE_SIZE) return -1;
    if (!file_table_loaded) load_file_table();
    
    // Find existing file or empty slot
    int idx = diskfs_find_file(filename);
    
    if (idx == -1) {
        // Find empty slot
        for (int i = 0; i < MAX_FILES; i++) {
            if (!file_table[i].used) {
                idx = i;
                break;
            }
        }
    }
    
    if (idx == -1) return -2;  // No space
    
    // Allocate disk sectors for this file
    unsigned short start_sector = FILE_DATA_START_SECTOR + (idx * SECTORS_PER_FILE);
    
    // Update file table entry
    strcpy(file_table[idx].filename, filename);
    file_table[idx].start_sector = start_sector;
    file_table[idx].size_bytes = size;
    file_table[idx].used = 1;
    
    // Write file data to disk (up to 2 sectors)
    unsigned char buffer[512];
    int remaining = size;
    int data_offset = 0;
    
    for (int sector = 0; sector < SECTORS_PER_FILE && remaining > 0; sector++) {
        // Clear buffer
        for (int i = 0; i < 512; i++) {
            buffer[i] = 0;
        }
        
        // Copy data to buffer
        int copy_size = (remaining > 512) ? 512 : remaining;
        memcpy(buffer, data + data_offset, copy_size);
        
        // Write sector
        if (ata_write_sector(start_sector + sector, buffer) != 0) {
            return -3;  // Write failed
        }
        
        data_offset += copy_size;
        remaining -= copy_size;
    }
    
    // Save file table
    if (save_file_table() != 0) {
        return -4;  // Table save failed
    }
    
    return 0;  // Success
}

// Load file from disk
int diskfs_load_file(const char* filename, char* buffer, int max_size) {
    if (!file_table_loaded) load_file_table();
    
    int idx = diskfs_find_file(filename);
    if (idx == -1) return -1;  // Not found
    
    int file_size = file_table[idx].size_bytes;
    int copy_size = (file_size > max_size) ? max_size : file_size;
    
    // Read file data from disk
    unsigned char sector_buffer[512];
    int remaining = copy_size;
    int buffer_offset = 0;
    
    for (int sector = 0; sector < SECTORS_PER_FILE && remaining > 0; sector++) {
        if (ata_read_sector(file_table[idx].start_sector + sector, sector_buffer) != 0) {
            return -2;  // Read failed
        }
        
        int read_size = (remaining > 512) ? 512 : remaining;
        memcpy(buffer + buffer_offset, sector_buffer, read_size);
        
        buffer_offset += read_size;
        remaining -= read_size;
    }
    
    return copy_size;
}

// Delete file
int diskfs_delete_file(const char* filename) {
    if (!file_table_loaded) load_file_table();
    
    int idx = diskfs_find_file(filename);
    if (idx == -1) return -1;  // Not found
    
    file_table[idx].used = 0;
    
    if (save_file_table() != 0) {
        return -2;  // Save failed
    }
    
    return 0;
}

// List all files
int diskfs_count_files(void) {
    if (!file_table_loaded) load_file_table();
    
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used) count++;
    }
    return count;
}

// Get file info by index
int diskfs_get_file_info(int index, char* name_out, int* size_out) {
    if (!file_table_loaded) load_file_table();
    
    int current = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used) {
            if (current == index) {
                strcpy(name_out, file_table[i].filename);
                *size_out = file_table[i].size_bytes;
                return 0;
            }
            current++;
        }
    }
    
    return -1;  // Index out of range
}
