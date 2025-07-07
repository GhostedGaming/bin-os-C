#ifndef FS_H
#define FS_H

#include <stdint.h>

typedef struct structure {
    // FAT16 Parameters
    uint16_t bytes_per_sector;        // Bytes per sector (usually 512)
    uint8_t  sectors_per_cluster;     // Sectors per cluster (1,2,4,8,16,32,64)
    uint16_t reserved_sectors;        // Reserved sectors (usually 1)
    uint8_t  number_of_fats;          // Number of FATs (usually 2)
    uint16_t root_directory_entries;  // Root directory entries (usually 512)
    uint16_t sectors_per_fat;         // Sectors per FAT
    uint32_t total_sectors;           // Total sectors in volume
    
    // Volume Information
    uint32_t volume_serial;           // Volume serial number
    uint8_t  volume_label[11];        // Volume label
    
    // Calculated Layout Information
    uint32_t fat_start_sector;        // Starting sector of first FAT
    uint32_t root_dir_start_sector;   // Starting sector of root directory
    uint32_t data_start_sector;       // Starting sector of data area
    uint32_t total_clusters;          // Total number of data clusters
    
    // Runtime Pointers/Buffers
    uint16_t *fat_table;              // Pointer to loaded FAT
    uint8_t  *root_directory;         // Pointer to loaded root directory
    
} structure;

#endif // FS_H