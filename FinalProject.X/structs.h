/* Code roughly based on Musical Fingerprint-Scanning Doorbell by
 * Phil Bedoukian, Joe Sluke, Austin Wiles
 * Authors: Connor Nace and Geoff Watson
 * 
 * Target PIC:  PIC32MX250F128B
 * Target SD: 8GB SanDisk SDHC
 * 
 * Header containing mainly structs used for storing SD card information
 * 
 *
 */
#include <stdint.h>
#ifndef __STRUCTS_H
#define __STRUCTS_H

//used for storing boot sector parameters and other info useful for traversal
//of the file system
struct FAT32param{
    uint16_t bytesPerSector; //0x0B
    uint8_t sectorsPerCluster;//0x0D
    uint32_t reservedSectors;//0x0E
    uint32_t sectorsPerFat;//0x24
    uint16_t rootStartCluster;//0x2C
    uint32_t fatStart; //calculated
    uint32_t rootStart; //calculated
};
//file information used for accessing a song in the SD card
struct file{
    char fileName[8];
    uint32_t sector;
    uint32_t cluster;
    uint16_t sectorInCluster;
    uint16_t clusterNum;
    uint32_t fileLeft;
};

// parameters of the data chunk from the header chunk of a WAVE file.
struct WAVEformat{
  uint16_t channels;
  uint32_t sampleRate;
  uint16_t bitsPerSample; 
  uint16_t dataStartByte;
};

//location of the boot sector
#define BOOT_START 8192

//gets storage parameters of the FAT32 SD card
uint8_t fat32_init();



#endif