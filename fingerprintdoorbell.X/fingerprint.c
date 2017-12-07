/* 
 * File: Fingerprint.c  
 * Author: Joe Sluke, some methods adapted form Josh Hawley's example code
 * from Github at https://github.com/sparkfun/Fingerprint_Scanner-TTL.
 * The fingerprint scanner (Model No. GT511C3) documentation was all used as reference.
 * Comments: This source file contains methods for fingerprint functionality.
 * It includes a library of fingerprint functionality and can perform various
 * high level functionality. Enrollment, Fingerprint verification, and Deletion
 * of fingerprints are implemented. This source code communicates with the main
 * through the use of a userInput variable which controls whether the doorbell
 * is performing ring or enrollment.
 */

// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************
#include "tft_master.h"
#include <string.h>

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

//Enroll States
#define StartEnroll 1
#define PerformEnroll1 2
#define PerformEnroll2 3
#define PerformEnroll3 4
#define EndEnroll 5
//Delay used when fingerprint is continuously checking if finger is pressed
#define EnrollDelay 100
//Possible user inputs
#define Ring 0
#define Enroll 1
#define flashLED -1 //Used for debugging
#define DeleteOneFinger 3 //Cannot perform with current system
#define DeleteAllFingers 4
#define VerifyFinger 5 //Used for debugging

//Static start bytes for fingerprint UART command packet
uint8_t COMMAND_START_CODE_1= 0x55; 
uint8_t COMMAND_START_CODE_2=0xAA;
uint8_t COMMAND_DEVICE_ID_1=0x01;
uint8_t COMMAND_DEVICE_ID_2=0x00;

//Buffer for saving strings to be displayed
uint8_t buffer[60];
//Buffer to store UART response from fingerprint scanner
uint8_t UartBuffer[12];
//Buffer to store entire UART command packet to send to fingerprint scanner
uint8_t packetbytes[12];
//Parameter bytes of fingerprint scanner command packet
uint8_t Parameter[4];
//Command bytes of fingerprint command packet
uint8_t command[2];

//Initializes enroll state at beginning of enroll process
int EnrollState=StartEnroll;
//int line=0; used for debugging to LCD display
//Flag to indicate if data expected to be received
int receiving=0;
//Feedback flags
int feedBack=0;
int feedBackData=0;
//Flag indicating if doorbell has been rung
int ring=0;
//Flag indicating if identification of fingerprint was successful.
int identifySuccess=0;
//ID of person who has rung the doorbell. 
int currentRinger=0; //Set 201 if ring is unknown or set to ringerId if know
//true if data packet being received
int dataPacket=0;
//Flag indicating whether enrollment has failed
int enrollFailed=0; 
//Flag indicating if a specific ID is available
int idAvailable = 0;
//ID being checked to see if it is available to store a fingerprint
int currentId = 0;
//ID of ringer as returned by UART signal from fingerprint scanner
int ringerId=-1;

// *****************************************************************************
// *****************************************************************************
// Section: User Functions
// *****************************************************************************
// *****************************************************************************

//State machine for fingerprint functionality
int StateMachine(int userInput, char * feedbackMessage){
    
    //Ring state.
        if(userInput==Ring){        
            feedBack=0;
              doRing();       
        }
             
       //Enroll State
        else if(userInput==Enroll){ //Enroll Fingerprint
            feedBack=0;
                        
            if(EnrollState==StartEnroll){ //Begin Enroll
                beginEnroll(feedbackMessage);                  
            }                       
            else if(EnrollState==PerformEnroll1){ //Enroll 1               
                doEnroll1(feedbackMessage);                
            }      
            else if(EnrollState==PerformEnroll2){// Enroll 2
                doEnroll2(feedbackMessage);
            }          
                else if(EnrollState==PerformEnroll3){ //Enroll 3    
                doEnroll3(feedbackMessage);
            }                               
            else if(EnrollState==EndEnroll){ //End Enroll
                waitUntilFingerIsRemoved();
                userInput=Ring;
                EnrollState=StartEnroll;                
            }                     
        }
        
        //Verify State. Used only in debug
        else if(userInput==VerifyFinger){
            doVerify();  
            userInput=Ring; 
        }
        
        //Deletes finger based on user specified ID.
        //User interface does not give the option to perform this function
        else if(userInput==DeleteOneFinger){              
            DeleteID(10); //would delete ID at index 10
            userInput=Ring; 
        }
        //Deletes all fingers. Performed upon startup of system.
        else if(userInput==DeleteAllFingers){ //Delete all entries
            SetLED(0);
            DeleteAll();   //   debug();  
            SetLED(1);
            userInput=Ring;               
        }
        
    return userInput;
//Code which flashes LED on fingerprint scanner. Was used for debug
//Variable a must be initialized
//        else if(userInput==flashLED){ //DebugState
//     
//        if(a==0){                  
//           SetLED(1);
//           a=1;
//       }
//        
//         else if(a==1){           
//           SetLED(0);
//           a=0;         
//       }     
//    }
              
       // END WHILE(1)
}

//Holds system until finger is removed from fingerprint scanner
void waitUntilFingerIsRemoved(){   
    while(FingerIsPressed() == 1){
        delay_ms(EnrollDelay);   
    }
}

//Holds system until finger presses the fingerprint scanner
void waitUnitlFingerPressed(){
     while(FingerIsPressed()==0){
        delay_ms(EnrollDelay);                   
    }        
}

//Finds next available ID to store fingerprint
void findNextAvailableId(){
    currentId=0;
    idAvailable = 0;   
    while(idAvailable == 0){
        CheckEnrolled(currentId);
        currentId++;
    }
}

//Begins enrollment process
beginEnroll(char * feedbackMessage){
    enrollFailed=0;                                              
    findNextAvailableId();           
    EnrollState=PerformEnroll1;  
    SetLED(1); //Turns LED On
}

//Enroll 1 stage. Waits for finger to be pressed, captures image, and stores
//it as the first enrollment image. Returns feedback as to whether fingerprint
//was captured and enrolled successfully.
doEnroll1(char * feedbackMessage){
          EnrollStart(currentId);                  // debug();         
                waitUnitlFingerPressed();       
                CaptureFinger(1);                       //   debug(); 
                Enroll1();                             //   debug();       
                waitUntilFingerIsRemoved();
                strcpy(feedbackMessage, "Success. Please Scan Finger Again");
                EnrollState=PerformEnroll2; //Moves to enroll 2
                if(enrollFailed==1){
                    EnrollState=StartEnroll; //Returns to begin enroll if failure
                    strcpy(feedbackMessage, "Unable to read finger. Please Scan Finger Again");
                }
                feedBack=1;
}

//Enroll 2 stage. Waits for finger to be pressed, captures image, and stores
//it as the first enrollment image. Returns feedback as to whether fingerprint
//was captured and enrolled successfully.
void doEnroll2(char * feedbackMessage){
      waitUnitlFingerPressed();                                  
                CaptureFinger(1);                      //    debug();  
                Enroll2();                              //  debug();           
                waitUntilFingerIsRemoved();
                strcpy(feedbackMessage, "Success. Please Scan Finger Again");
                EnrollState=PerformEnroll3; //Moves to Enroll 3
                if(enrollFailed==1){
                    EnrollState=StartEnroll; //Returns to begin enroll if failure
                    strcpy(feedbackMessage, "Unable to read finger. Please Scan Finger Again");
                }        
    feedBack=1;
    
}

//Enroll 3 stage. Waits for finger to be pressed, captures image, and stores
//it as the first enrollment image. Returns feedback as to whether fingerprint
//was captured and enrolled successfully. After third enroll stage, fingerprint
//should now be saved to specified index
void doEnroll3(char * feedbackMessage){
      waitUnitlFingerPressed();                  
                CaptureFinger(1);                     //    debug(); 
                Enroll3();                             //  debug();             
                waitUntilFingerIsRemoved();
              //  strcpy(feedbackMessage, );
                sprintf(feedbackMessage,"Success. Enrollment Complete. ID: %d", currentId);
               // SetLED(0);
                EnrollState=EndEnroll; //Moves to End Enroll
                if(enrollFailed==1){
                    EnrollState=StartEnroll; //Returns to begin enroll if failure
                    strcpy(feedbackMessage, "Unable to read finger. Please Scan Finger Again");
                    feedBack=1;
                }
                feedBack=2;
}

//Determines if fingerprint matches specified ID. Used only in debug
void doVerify(){
     SetLED(1);
            waitUnitlFingerPressed();               
            CaptureFinger(0);                     //    debug();           
            Verify(0);                            //    debug();
            waitUntilFingerIsRemoved();
            SetLED(0);
}

    //Checks to see if finger is pressed. If finger is pressed,
    //sends current ringerID to main so that the appropriate tone can play.
    //If no finger is pressed, does nothing
void doRing(){
    ring=0;
    identifySuccess=0; 
    enrollFailed=0;
    currentRinger;        
            if(FingerIsPressed()==1){
            CaptureFinger(0);                       //  debug(); 
            if(enrollFailed==0){
                Identify();
                ring=1;
                if(identifySuccess==1){
                    currentRinger=ringerId;
                }
                else{
                    currentRinger=201;                    
                }
            }
        }
}

//Converts int to parameter for fingerprint scanner communication
void ParameterFromInt(int i){
	Parameter[0] = (i & 0x000000ff);
	Parameter[1] = (i & 0x0000ff00) >> 8;
	Parameter[2] = (i & 0x00ff0000) >> 16;
	Parameter[3] = (i & 0xff000000) >> 24;
} 

 uint8_t CalculateChecksumLow(){ //Returns low check sum bit
	uint16_t w = 0x0;
	w += COMMAND_START_CODE_1;
	w += COMMAND_START_CODE_2;
	w += COMMAND_DEVICE_ID_1;
	w += COMMAND_DEVICE_ID_2;
	w += Parameter[0];
	w += Parameter[1];
	w += Parameter[2];
	w += Parameter[3];
	w += command[0];
	w += command[1];
    return(uint8_t) w&0x00FF;
} //Credit: John Hawley
 
  uint8_t CalculateChecksumHigh(){ //Returns high check sum bit
	uint16_t w = 0x0;
	w += COMMAND_START_CODE_1;
	w += COMMAND_START_CODE_2;
	w += COMMAND_DEVICE_ID_1;
	w += COMMAND_DEVICE_ID_2;
	w += Parameter[0];
	w += Parameter[1];
	w += Parameter[2];
	w += Parameter[3];
	w += command[0];
	w += command[1];
    w=w>>8;
    return (uint8_t) w &0x00FF;
} //Credit: John Hawley
 
  //Sets all necessary bytes to command packet array which is now ready
  //to be sent to fingerprint scanner
 void SetCommandPacket(){
     
    packetbytes[0] = COMMAND_START_CODE_1;
	packetbytes[1] = COMMAND_START_CODE_2;
	packetbytes[2] = COMMAND_DEVICE_ID_1;
	packetbytes[3] = COMMAND_DEVICE_ID_2;
	packetbytes[4] = Parameter[0];
	packetbytes[5] = Parameter[1];
	packetbytes[6] = Parameter[2];
	packetbytes[7] = Parameter[3];
	packetbytes[8] = command[0];
	packetbytes[9] = command[1];    
    packetbytes[10]=CalculateChecksumLow();
    packetbytes[11]=CalculateChecksumHigh();                
 } 
 
 //Sets Parameter Bytes to 0
 void NoParameters(){
    Parameter[0] = 0x00;
	Parameter[1] = 0x00;
	Parameter[2] = 0x00;
	Parameter[3] = 0x00;    
 }
 
 //Writes 12 byte command packet to fingerprint scanner
 void SendUart(){
  
    receiving = 1;
    int i;
    for (i=0;i<12;i++){ 
        while(!UARTTransmitterIsReady(UART2)){};
        WriteUART2(packetbytes[i]);           
    }       
    ReadUart();
 }

 //Reads 12 incoming UART bytes and stores them in UartBuffer for
 //interpretation based on command that was sent
 void ReadUart(){            
      int i=0; 
      for(i;i<12;i++){ //Waits to Receives and store Data 
      while(!UARTReceivedDataIsAvailable(UART2)){};           
            UartBuffer[i] = UARTGetDataByte(UART2);   
      }
      if(dataPacket==1){
          i=0;
    }
 }
 
 
//Method of debugging
//Prints UART response packet on LCD display
//void debug(){
//    line++;tft_setTextColor(ILI9341_YELLOW); tft_setTextSize(1);tft_setCursor (10, 20+line*10);  int i=0;for (i;i<12;i++){sprintf(buffer, "%X", UartBuffer[i]);tft_writeString(buffer);tft_writeString(" ");}                
//}
 
// *****************************************************************************
// *****************************************************************************
// Section: Fingerprint Scanner Command Library
// *****************************************************************************
// *****************************************************************************

 
 //Initialized fingerprint scanner. Must be called for any other command
 void Open(){
    NoParameters();
	command[0]=0x01;
	command[1]=0x00;
    SetCommandPacket();
    SendUart();
} 
 
 //Deactivates fingerprint scanner communication until Open() is called
  void Close(){
	NoParameters();
	command[0]=0x02;
	command[1]=0x00;
    SetCommandPacket();
    SendUart();
} 
 
//Turns LED on or off. LED must be on to capture or identify fingers
 void SetLED(int on){
    NoParameters(); 
	if (on==1)	
        Parameter[0] = 0x01;	
	command[0]=0x12;
	command[1]=0x00;   
   SetCommandPacket();
   SendUart();
}; //Turns LED on or off
 
//Returns amounts of fingers enrolled. Not implemented in system
void GetEnrollCount(){
    NoParameters();
	command[0]=0x20;
	command[1]=0x00;
    SetCommandPacket();
    SendUart();
}

//Checks to see if a fingerprint is enrolled in a specified index
void CheckEnrolled(int index){
    ParameterFromInt(index);
	command[0]=0x21;
	command[1]=0x00;
    SetCommandPacket();
    SendUart();
    
    if(UartBuffer[8]==0x31){  //31 means id is not enrolled at specified index
        idAvailable=1;
     currentId--;   
    }
} 

//Starts enrolling a finger at specified index
void EnrollStart(int index){
    ParameterFromInt(index);
	command[0]=0x22;
	command[1]=0x00;
    SetCommandPacket();  
    SendUart();
} 

//Enroll Part 1 at specified index
 void Enroll1(int index){
    ParameterFromInt(index);
	command[0]=0x23;
	command[1]=0x00;
    SetCommandPacket(); 
    SendUart();
    
    if(UartBuffer[8]==0x31)  //31 in byte 9 of response means success
        enrollFailed=1;
} 

 //Enroll Part 2 at specified index
void Enroll2(int index){
    ParameterFromInt(index);
	command[0]=0x24;
	command[1]=0x00;
    SetCommandPacket(); 
    SendUart();
    if(UartBuffer[8]==0x31)  //31 in byte 9 of response means success
        enrollFailed=1;
} 

//Enroll Part 3 at specified index
void Enroll3(int index){
    ParameterFromInt(index);
	command[0]=0x25;
	command[1]=0x00;
    SetCommandPacket(); 
    SendUart();
    if(UartBuffer[8]==0x31)  //31 in byte 9 of response means success
        enrollFailed=1;
} 

//Returns if finger is pressed
int FingerIsPressed(){
    NoParameters();
	command[0]=0x26;
	command[1]=0x00;
    SetCommandPacket(); 
    SendUart();    
      
    if(UartBuffer[4]==0x00)//Response will be 0x00 if returned
        return 1;
    return 0;
}

//Deletes an ID at an indicated index
void DeleteID(int index){
    ParameterFromInt(index);
	command[0]=0x40;
	command[1]=0x00;
    SetCommandPacket();  
    SendUart();
}

//Deletes all fingerprints
void DeleteAll(){
    NoParameters();
	command[0]=0x41;
	command[1]=0x00;
    SetCommandPacket();  
    SendUart();
}

 //Verifies if a fingerprint is stored in a specified position
void Verify(int index){
    ParameterFromInt(index);
	command[0]=0x50;
	command[1]=0x00;
    SetCommandPacket();
    dataPacket=1;
    SendUart();
    dataPacket=0;
}
 
//Returns an index where a fingerprint is stored
void Identify(){    
    NoParameters();
	command[0]=0x51;
	command[1]=0x00;
    SetCommandPacket(); 
    SendUart();
    ringerId=UartBuffer[4];
    if(UartBuffer[8]==0x30){
        identifySuccess=1;  
    }
} 

//Get methods
int getCurrentId(){
    return currentId;
}

int getFeedbackFlag(){   
    return feedBack;
}

int getFeedbackData(){    
    return feedBackData;
}

int getRingerId(){
    return currentRinger;
}

int getRingFlag(){
    return ring;
}

//Captures fingerprint image. Parameter indicates either a high quality
//or low quality capture image
void CaptureFinger(int quality){
    NoParameters();
    if(quality==1)
    Parameter[0] = 0x01; //change to 0x01 for better quality capture
	command[0]=0x60;
	command[1]=0x00;
    SetCommandPacket(); 
    SendUart();
    if(UartBuffer[8]==0x31)  //31 in byte 9 of response means failure
        enrollFailed=1;
}