/* Code roughly based on Musical Fingerprint-Scanning Doorbell by
 * Phil Bedoukian, Joe Sluke, Austin Wiles
 * Authors: Connor Nace and Geoff Watson
 * 
 * Target PIC:  PIC32MX250F128B
 * 
 * Sound production. Reads data from the SD card and sends data to the DAC via
 * a DMA channel. CS for the DAC is created using output capture 
 * in dual compare mode. All of these operations happen concurrently.
 * 
 * Three buffers are used in between the DAC SPIBUF and the SD card.
 * One buffer is dedicated to getting a sector (512 bytes) from the SD card.
 * The other two buffers are used interchangeably and swap function every other
 * cycle. One of these two buffers get formatted data from the most recent 
 * read operation. At the same time, the other buffer's contents is transfered
 * to the DAC SPIBUF using a DMA channel.
 * 
 * 
 */

#include <plib.h>
#include "structs.h"

//DAC chip select
#define DAC_cs         LATAbits.LATA0
#define DAC_TRIS_cs     TRISAbits.TRISA0
#define DAC_cs_high()  {LATASET = 1;}
#define DAC_cs_low()   {LATACLR = 1;}

//DMA channel
#define DMA_SPI_chn 0

//DAC control bits
#define DAC_CONTROL 0x3000

//raw data from SD card placed here
uint8_t readBuf[512];
//buffers for DMA transfers. One buffer's contents is sent to the DAC while
//the other buffer is loaded with formated read data
uint16_t transferBufA_1[128];
uint16_t transferBufA_2[128];
//used to determine what buffer is used in DMA
uint8_t bufferSelect = 0;

//flag to start new DMA transfer and read 
volatile uint8_t isDMAcomplete = 1;


//initialize spi communication between pic32 and DAC and set slave select pin
void initDAC(void){
    DAC_TRIS_cs = 0;
    DAC_cs = 1; 
    
    //already open from tft library
    //16 bit mode and no MISO pin
    //SPI at max clk rate. Does not have to be related to sample rate
    //DAC will ignore all bits past 16th bit
    SpiChnOpen(1, SPI_OPEN_MSTEN | SPI_OPEN_MODE16 | SPI_OPEN_ON |
            SPI_OPEN_DISSDI | SPI_OPEN_CKE_REV , 2);
    
     PPSOutput(2, RPB5, SDO1);   
         
}

//sets up DMA channel used to send audio data to the DAC
void initDMA(void){
    //basic dma setup
    DmaChnOpen(DMA_SPI_chn, 0, DMA_OPEN_DEFAULT);
    
    //temporary transfer setup
    DmaChnSetTxfer(DMA_SPI_chn, transferBufA_1, (void*)&SPI1BUF, 256,
            2, 2);
    
    
    //cell transfer triggered by timer
    DmaChnSetEventControl(DMA_SPI_chn, DMA_EV_START_IRQ(_TIMER_2_IRQ));
    
    //interrupt to flag when DMA transfer complete
    DmaChnSetEvEnableFlags(DMA_SPI_chn, DMA_EV_BLOCK_DONE);
    INTSetVectorPriority(INT_VECTOR_DMA(DMA_SPI_chn), INT_PRIORITY_LEVEL_2);
    INTEnable(INT_SOURCE_DMA(DMA_SPI_chn), INT_ENABLED);
    
}
//loads in music files from the SD card
void initNotes(struct file * noteFiles){
  //DMA to DAC
  initDMA();
  //DAC spi channel 2
  initDAC();
  //sets SD card in SPI and takes out of idle mode
  SD_init();
  //scans fat32 boot sector. gets file system parameters,
  //fat table and root locations
  fat32_init();
  
  //finds files in the music directory
  uint32_t musicSector = directory_find("NOTES  ");
  file_finds(musicSector, noteFiles);

}

//formats read data to be sent to the DAC
void DMA_format(uint8_t * RxBuffer, uint16_t RxSize){
    //formating procedure:
    //remove second byte/4 bytes (LSB in littleEndian)   
    //flip to bigEndian
    //compute with midpoint value of DAC
    //add control bits to the front
    //no need to scale. range will be between 0 and 2^12 (4095)
        
    //loads data into the buffer not being used in a DMA transfer
    if (bufferSelect == 0){
        uint16_t i;
        for (i = 0; i < RxSize; i += 4){   
            //format data bits flip and drop lowest byte
            //16 bit little endian -> 12 bit big endian
            //stored in int because data in twos complement form
            int16_t res = (RxBuffer[i + 1] << 4) | (RxBuffer[i] >> 4);
            
            //add or subtract to the middle value of the 12 bit DAC
            res = 2047 + res;
            //add DAC control bits. channel A. normal gain
            transferBufA_1[i >> 2] = DAC_CONTROL | (uint16_t)res;  

        }

        bufferSelect = 1;
    }
    else{
         uint16_t i;
        for (i = 0; i < RxSize; i += 4){   
            //format data bits flip and drop lowest byte
            //16 bit little endian -> 12 bit big endian
            //stored in int because data in twos complement form
            int16_t res = (RxBuffer[i + 1] << 4) | (RxBuffer[i] >> 4);
            //add or subtract to the middle value of the 12 bit DAC
            res = 2047 + res;
            //add DAC control bits. channel A. normal gain
            transferBufA_2[i >> 2] = DAC_CONTROL | (uint16_t)res;  

        }

        bufferSelect = 0;
    }
}

//sets up DMA transfer by selecting the correct transfer buffer
void DMA_begin(){

    //check what buffer was last updated and use that one for the next 
    //DMA transfer
    if (bufferSelect == 0){
       DmaChnSetTxfer(DMA_SPI_chn, transferBufA_2, (void*)&SPI1BUF, 256,
           2, 2);


    }
    else if (bufferSelect == 1){
        DmaChnSetTxfer(DMA_SPI_chn, transferBufA_1, (void*)&SPI1BUF, 256,
            2, 2);
    }
    
    //start transfer
    isDMAcomplete = 0;
    WriteTimer2(0x0000);
    DmaChnEnable(DMA_SPI_chn);
    
}
//OC in dual compare mode to generate pulse for CS signal. 
//Operation can take place outside of the CPU using this generation method.
void initCS(void){

  PPSOutput(1,RPB4,OC1);//pin 11
  
  //pulse parameters
  //timing recommendations from
  //http://tahmidmc.blogspot.com/2014/10/pic32-dmaspidac-analog-output-synthesis
  //.html
  //goes low. PR2 - 4
  OC1RS = 903;
  //goes high. .8 * PR2
  OC1R = 726;
  //open OC in dual compare mode
  OC1CON = 0x8005;
  
}
//plays a song using the information the file sent file struct
//the file is modified and should not be used after playSong is done
void playSong (struct file * currentFile){
    //wake up SD card
    SD_init();
        
    //gets header information for chosen .wav file
    file_read(currentFile, readBuf); 

    //wav file data format
    struct  WAVEformat wavForm;  
    wav_setup(&wavForm, readBuf);

    //reinitializes with faster baud rate for faster read time to keep up with 
    //music demand. Read speed should keep up with desired sample rate for 
    //samples to be output concurrently with reading
    SpiChnSetBrg(2, 8);   

    //preloads fat table sectors used in the chosen song. When it is time to move
    //to the next cluster check the array with cluster information rather than
    //having to request info from the SD card. Timing will mess up if fat table
    //read during sound production
    
    //number of fat sectors loaded. determines how long song will play
    uint8_t fatLoadAmt = 3;
    uint32_t preloadBuf[128 * fatLoadAmt];
    uint16_t entryOffset = fat_preload(currentFile, fatLoadAmt, preloadBuf);
    //location of starting cluster entry for the song in the preload array

    //conversion between fat table location to array location
    entryOffset = entryOffset >> 2;

    //start CS signal
    initCS();

    //DMA cell transfer rate. SPEED DETERIMED BY THIS. 
    OpenTimer2(T2_ON | T2_PS_1_1, 907);
    
    
    //flag to exit music playing
    uint8_t ret = 0;
    
    uint16_t sectorsRead = 0;
    //Audio playing scheme (first two cycles. second cycle repeats)
    //<-Read next sector-><-Format data-> <-Read next sector-><-Format data-> -->
    //                                    <-DMA transfer to DAC based on last read->
    //
    //Formated data is output to the DAC at the same time as new data is being 
    //loaded in for the next transfer to the DAC. A DMA based transfer to the DAC
    //SPIBUF allows data input and data output to occur simultaneously.
    while (ret == 0){
        //Wait until a full DMA block has finished. 
        if (isDMAcomplete == 1){
               //begins new DMA transfer to DAC SPIBUF
               DMA_begin(); 
               //Request and receive the next sector of a music file. 
               ret = file_read_preloaded(currentFile, readBuf, preloadBuf, entryOffset, fatLoadAmt);
               //Format received data for the MCP4822 DAC.
               DMA_format(readBuf, sizeof readBuf); 
               
               sectorsRead++;
               if (sectorsRead == 10350){ //30s
                   ret = 5;
               }
        }
    }
    
    //turns off timer and output compare
     OC1CON = 0x0005;
     T2CON = 0x0000;

}
//triggered upon a DMA block transfer completion
//sets flag that DMA transfer is done and disables DMA
void __ISR(_DMA_0_VECTOR, ipl2soft) blockComplete(void){
    
    DmaChnDisable(DMA_SPI_chn);
    isDMAcomplete = 1;
    
    //clear all flags
    DmaChnClrEvFlags(DMA_SPI_chn, DMA_EV_BLOCK_DONE);
    INTClearFlag(INT_SOURCE_DMA(DMA_CHANNEL0));
    
}


