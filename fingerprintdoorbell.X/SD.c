/*   
 * Code modified from Code and Life by Joonas Pihlajamaa
 * Authors: Phil Bedoukian, Joe Sluke, Austin Wiles
 * 
 * Target PIC:  PIC32MX250F128B
 * Target SD: 8GB SanDisk SDHC
 * 
 * Low level management of communication with an 8GB SanDisk SDHC card 
 * (sector addressable).
 * 
 * Changes made: SPI functionality is much different on the pic than the 
 * original chip (Arduino?). SDHC requires different startup protocol. 
 * SDHC requires sector addressing rather than original byte addressing.
 *
 */
#include "fat.h"
#include <plib.h>
#include <stdint.h>

//SD card chip select
#define SD_cs         LATBbits.LATB9
#define SD_TRIS_cs    TRISBbits.TRISB9
#define SD_cs_high()  {LATBSET = 512;}
#define SD_cs_low()   {LATBCLR = 512;}



//writes 8 bits to SPI2BUF. CS for the SD card must be enabled before the
//operation
void SD_write(uint8_t data){
    while (TxBufFullSPI2());
    WriteSPI2(data);
 
}
//receives 8 bits on the SD card channel
uint8_t SD_read(){
 
    //assert logic high while reading
    SD_write(0xFF);
    
    //clear the overflow. otherwise no new data will be put into the RxBuffer
     SPI2STATbits.SPIROV = 0;
    
    //wait for bits to be sent. bits should also be received after this time
    while( SPI2STATbits.SPIRBF==0){};

    //returns 8 bits from the RxBuffer
    return ReadSPI2();
}


//48 bits: 01, 6 bit cmd, 32 bit argument, 7 bit crc, 1 (stop)
uint8_t SD_command(uint8_t cmd, uint32_t arg, 
                uint8_t crc, uint8_t read) {
     
    uint8_t i, buffer[32], ret = 0xFF;
    
    
    SD_cs_low();
    //send info to SD card in 8 bit increments
    SD_write(cmd); //and two start bits
    SD_write(arg>>24);
    SD_write(arg>>16);
    SD_write(arg>>8);
    SD_write(arg);
    SD_write(crc); //and ending stop bit

    //monitor the SD card output
    for(i = 0; i < read; i++){
        buffer[i] = SD_read();
    }
    
    SD_cs_high(); 
    
    //look for response (not 0xFF)
    //need delay in between cmds
    for(i = 0; i < read; i++) {
        delay_ms(5);
        if(buffer[i] != 0xFF){
            ret = buffer[i];
        }
    }
	
	return ret;
}

//similar to SD command, but does not need to verify response
uint8_t SD_request(uint8_t cmd, uint32_t arg, 
                uint8_t crc) {
     
    //send info to SD card in 8 bit increments
    SD_write(cmd); //and two start bits
    SD_write(arg>>24);
    SD_write(arg>>16);
    SD_write(arg>>8);
    SD_write(arg);
    SD_write(crc);

}
//Sends read request, waits, and then reads a full data packet from the SD card
//For SDHC, only works with full sector reads. Only use a length of 512 bytes.
void SD_getData(uint32_t sector, uint16_t sectorOffset, uint8_t * buffer,
uint16_t length) {
    int i;
    SD_cs_low();
 
    //prepare to read physical(winhex) sector of data
    //CMD17
    SD_request(0x51, sector, 0xFF);
    //can't have printf statements. misses data then
   
    //wait for cmd response
    while(SD_read()!=0x00){};
    //skips 0xFE (data start byte)
    while(SD_read()==0xFF){};
 
    //read a portion of data
    for (i = 0; i < length + sectorOffset; i++){
        buffer[i] = SD_read();
    }
    
    //skip crc
    SD_read();
    SD_read();
    
    SD_cs_high();
}
//start up the SD card based on SD SPI protocol
uint8_t SD_init() {
    
    //set to output pin
    SD_TRIS_cs = 0; 
    //protocol requires cs high during startup
    SD_cs_high(); 

    //clear the overflow of the RxBuffer
    SPI2STATbits.SPIROV = 0; 
    //configure SPI channel. during command sequence the clock speed should
    //be fairly slow (clkdiv on the order of 128). The speed can be increased
    //once the sd card is initialized
    SpiChnOpen(2, SPICON_MSTEN|SPICON_FRMEN|SPICON_SMP|SPICON_ON, 128);
    //PPS 
    PPSOutput(2, RPB11, SDO2);//pin 22
    PPSInput(3 ,SDI2, RPB13);//pin 24
       
    //protocol requires idle for 80 clocks to wake up SD card
    uint8_t i;
    for(i = 0; i < 10; i++) { 
        SD_write(0xFF);
    }

    //http://www.microchip.com/forums/m572718.aspx useful link for confirming
    //command responses 
    
    //CMD0 - SPI MODE
    while ((SD_command(0x40, 0x00000000, 0x95, 5)) != 1){}
    
    //CMD8
    SD_command(0x48, 0x000001AA, 0x87, 10);
    
    //CMD58
    SD_command(0x7A, 0x00000000, 0xFD, 10);
    
    //ACMD41. cmd55 then cmd41
    //take chip out of idle
    //need to repeat a few times for activation
    uint8_t result = 0xFF;
    i = 0;
    while (result != 0x00){
        SD_command(0x77, 0x00000000, 0x65, 4);
        //hcs in arg logic high
        result = SD_command(0x69, 0x40000000, 0xE5, 4);
        delay_ms(5);
        i++;
        if (i > 20){
            return 10;
        }
    }
    
    //CMD58
    SD_command(0x7A, 0x00000000, 0xFD, 10);
    
    //set block length
    //CMD16
    SD_command(0x50, 0x00000200, 0xFF, 8);
    

    return 0;
}

