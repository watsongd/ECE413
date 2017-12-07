
/* Code roughly based on Code and Life by Joonas Pihlajamaa
 * Authors: Phil Bedoukian, Joe Sluke, Austin Wiles
 * 
 * Target PIC:  PIC32MX250F128B
 * Target SD: 8GB SanDisk SDHC
 * 
 * Accesses data in the FAT32 file system within the SD card.
 * Note that FAT32 uses little endian.
 * 
 * Changes: Reads from FAT32 file system instead of original FAT16 file system.
 * Added useful functions for music production (FAT TABLE preloading)
 * 
 */

#include "fat.h"

struct FAT32param FAT32;


uint8_t data[512];
uint8_t readBuffer[512];

#define FILE_NAME_LENGTH 8

//reads the boot sector and bundles data into a FAT32 struct.
//relation between byte offset and data field is based on documentation.
uint8_t fat32_init() {
      
    //get boot sector data
    fat_read(BOOT_START, 0, readBuffer, 512);

    //switch entry to big endian and store in appropriate variable
    endianessSwitch(0xB, 2, readBuffer, 1);
    FAT32.bytesPerSector = (data[0] << 8) + data[1];
    
    endianessSwitch(0xD, 1, readBuffer, 1);
    FAT32.sectorPerCluster = data[0];
    
    endianessSwitch(0xE, 2, readBuffer, 1);
    FAT32.reservedSectors = (uint32_t)((data[0] << 8) + data[1]);
    
    endianessSwitch(0x24, 2, readBuffer, 1);
    FAT32.sectorsPerFat = (uint32_t)((data[0] << 8) + data[1]);
    
    endianessSwitch(0x2C, 2, readBuffer, 1);
    FAT32.rootStartCluster = (uint32_t)((data[0] << 8) + data[1]);
    
    //calculate fat table starting sector and root directory starting sector
    //based on boot sector parameters
    FAT32.fatStart = (uint32_t)BOOT_START + FAT32.reservedSectors;
    //Two fats and reserved sectors between root and boot
    FAT32.rootStart = (uint32_t)BOOT_START + FAT32.reservedSectors +
            (uint32_t)(FAT32.sectorsPerFat * 2);
    
    
    delay_ms(10);

    return 0;
}
//finds starting sector of a directory from the root directory
//music files should all be in a "music" directory
uint32_t directory_find(char *folderName){
    //need to reinitialize card if inactive for ~10ms
    SD_init();
    
    //read the full root directory
    fat_read(FAT32.rootStart, 0, &readBuffer, 512);
    
    //find music folder
    //8 bytes are used in the full name (only 8 character filenames allowed)
    uint16_t offset;
    uint16_t i;
    uint8_t name[8];
    

    //find file or directory name at offset
    //names are in big endian already
    for (offset = 0; offset < 512; offset+=0x20){
        endianessSwitch(offset, FILE_NAME_LENGTH, readBuffer, 0);
        for (i = 0; i < FILE_NAME_LENGTH; i++){
            name[i] = data[i];
        }
        if (strcmp(name, folderName) == 0){
            break;
        }
    }
    
    //further explore directory to get location of starting cluster
    offset += 0x1A;
    endianessSwitch(offset, 2, readBuffer, 1);
    uint32_t startCluster = (data[0] << 8) + data[1];
    //relative cluster -> relative sector -> absolute sector
    return (((startCluster - 2) * FAT32.sectorPerCluster) + FAT32.rootStart);
      
}

//find names and locations of all files in directory
//contains multiple hard coded values. should be more dynamic
void file_finds (uint32_t directorySector, struct file *allFiles){

    uint16_t entry = 0;
    uint8_t j = 0;
    //read 2 sectors of the root directory (temporary)
    for (j = 0; j < 2; j++){
        //read sector
        fat_read(directorySector + j, 0, readBuffer, 512);

        //read 12 entries
        for (entry = 0; entry < 12; entry++){
            struct file newFile;
            uint16_t i;
            //skip header
            uint16_t offset;
            //go to the next file entry
            if (j == 0){
                offset = 0x60 + (0x40 * entry);
                //due to weird formatting in beginning of directory
                if (entry >= 1){
                    offset += 0x20;
                }    
            }
            else {
                offset = 0x40 * entry;
                //due to weird formatting
                if (j == 1 && entry >= 5){
                    offset -= 0x20 * (entry - 4);
                }
            }
            //get file name
            endianessSwitch(offset, FILE_NAME_LENGTH, readBuffer, 0);
            for (i = 0; i < FILE_NAME_LENGTH; i++){
                    newFile.fileName[i] = data[i];
            }
            //get starting sector and cluster
            offset += 0x1A;
            endianessSwitch(offset, 2, readBuffer, 1);
            newFile.cluster = (data[0] << 8) + data[1];
            newFile.sector = ((newFile.cluster - 2) * FAT32.sectorPerCluster) 
                    + FAT32.rootStart;
            //get file size
            offset += 0x02;
            endianessSwitch(offset, 4, readBuffer, 1);
            //file size. not used
            newFile.fileLeft = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) +
                    (data[3]);

            //used during file reading
            newFile.sectorInCluster = 0;
            newFile.clusterNum = 0;

            //add to array of known files
            allFiles[entry + (j*12)] = newFile;

        }
        
    }
    
}

//reads the fat table sectors
//each fat32 entry is 4 bytes (32 bits)
uint32_t FAT_TABLE (uint32_t finishedCluster){

    uint8_t fatBuf[512];
    //each fat sector is 128 entries. add 1 sector for every 128 clusters
    //to search correct fat sector
    
    //get entry and sector offset
    //divide integer by 128
    uint16_t sectorOffset = finishedCluster >> 7;
    //-> entry in correct fat sector -> byte in fat sector
    //this line is questionable think it works through rounding
    uint16_t entryOffset = (finishedCluster - (sectorOffset * 128)) * 4;    
    
    //read one entry of the fat table
    fat_read(FAT32.fatStart + sectorOffset, 0, fatBuf, 512);

    endianessSwitch (entryOffset, 4, fatBuf, 1);    
    return (data[0] << 24) + (data[1] << 16) + (data[2] << 8) +
            (data[3]);
      
}

//reads data file one sector at a time. file struct sent keeps track of location
//when the end of a cluster has been reached (64 sectors), check the fat table
//for the location of the next cluster
//end of file is seen in fat table return 1 instead of 0
//might need to reset current file at the end or use temp (hard copied) one in operation
uint8_t file_read(struct file * currentFile, uint8_t * buffer){
    
    //read data from next sector
    fat_read(currentFile->sector, 0, buffer, 512);
    
    //increment sector
    currentFile->sector++;
    currentFile->sectorInCluster++;
    
    //at the end of a cluster. check the fat table for the next cluster 
    //or end of file
    if (currentFile->sectorInCluster == 64){
        
        //new cluster from fat table
        currentFile->cluster = FAT_TABLE(currentFile->cluster);
        if (currentFile->cluster == 0x0FFFFFFF){
            return 1;
        }
        //update sector based on new cluster
        currentFile->sector = ((currentFile->cluster - 2) * FAT32.sectorPerCluster) 
            + FAT32.rootStart;
        currentFile->sectorInCluster = 0;

        
    }
    return 0;

}
//get starting parameters about wave file from header
void wav_setup(struct WAVEformat *wavForm, uint8_t *readBuf){
  endianessSwitch(0x16, 2, readBuf, 1);
  wavForm->channels = (data[0] << 8) + data[1];

  endianessSwitch(0x18, 4, readBuf, 1);
  wavForm->sampleRate = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) +
            (data[3]);
  
  endianessSwitch(0x22, 2, readBuf, 1);
  wavForm->bitsPerSample = (data[0] << 8) + data[1];
  
  //data chunk start. some wav files take a while to start
  wavForm->dataStartByte = 0x50;
}
//similar to file_read, but optimized for system. Get fat table from a preloaded 
//array instead of requesting fat sector from the SD card during music 
//production. In a non preloaded read, a double SD read would be required to 
//get the data sector and the appropriate fat sector. 
//There is not enough time to perform this action while playing music
uint8_t file_read_preloaded(struct file * currentFile, uint8_t * buffer,
        uint32_t * preloadBuf, uint16_t entryOffset, uint8_t preloadedSectors){

    
    //read data from next sector
    fat_read(currentFile->sector, 0, buffer, 512);
    
    //increment sector
    currentFile->sector++;
    currentFile->sectorInCluster++;
    
    //at the end of a cluster. check the fat table for the next cluster 
    //or end of file
    if (currentFile->sectorInCluster == 64){
        
        //if ((currentFile-> clusterNum + startingEntryOffset) >= (preloadedSectors * 128) - 1){
        if ((currentFile->clusterNum + entryOffset) >= (preloadedSectors << 7)){    
            return 1;
        }
        //new cluster from preloaded fat table. offset in terms of fat entries not array entires
        currentFile->cluster = preloadBuf[currentFile->clusterNum + entryOffset];
        if (currentFile->cluster == 0x0FFFFFFF){
            return 1;
        }
        //update sector based on new cluster
        currentFile->sector = ((currentFile->cluster - 2) * FAT32.sectorPerCluster) 
            + FAT32.rootStart;
        currentFile->sectorInCluster = 0;
        currentFile->clusterNum++;
        
    }
    return 0;

}

//load 128 entries per fat sector (4 bytes each) into an array
//returns starting entry offset
uint16_t fat_preload (struct file * currentFile, uint8_t fatSectorAmt, 
        uint32_t * preloadBuffer){
    
    uint16_t i;
    uint8_t fatBuf[512];
    
    //load a variable amount of factor sectors into the preload buffer
    for (i = 0; i < fatSectorAmt; i++){
        //want to read whole fat sector cluster exists in
        //divide cluster by 128 to get fat sector cluster is handled in
        uint16_t fatSector = FAT32.fatStart + (currentFile->cluster >> 7) + i;
        
        //read one sector in the fat table
        fat_read(fatSector, 0, fatBuf, 512);

        uint16_t j;
        for (j = 0; j < 512; j+=4){
            endianessSwitch (j, 4, fatBuf, 1);
            preloadBuffer[(i * 128) + (j >> 2)] = 
                  (data[0] << 24) + (data[1] << 16) + (data[2] << 8) +(data[3]);
        }
    }
    //weird line but i think works through rounding. alt modulus operator
    return (currentFile->cluster - (currentFile->cluster >> 7) * 128) * 4; 

}
//renamed SD_getData
void fat_read(uint32_t sector, uint16_t sectorOffset, uint8_t * buffer,
   uint16_t length){
   SD_getData(sector, sectorOffset, buffer, length);

}
//takes an entry at a specified offset and converts it from little endian
//to big endian. Data is placed into the beginning of the 'data' buffer
void endianessSwitch(uint16_t startIndex, uint16_t length, uint8_t * buffer,
        uint8_t swap){
    uint16_t i;
    uint16_t j = 0;
    uint16_t endIndex = startIndex + length - 1;
        
    //converts endianess
    if (swap == 1){
        //need wrap detection
        for (i = endIndex; i >= startIndex && i < 512; i--){
            data[j] = buffer[i];
            j++;
        }  
    }
    else {
        for (i = startIndex; i < endIndex; i++){
            data[j] = buffer[i];
            j++;
        }  
    }
}

