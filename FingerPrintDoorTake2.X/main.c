/* Authors: Phil Bedoukian, Joe Sluke, Austin Wiles
 *
 * Target PIC:  PIC32MX250F128B
 *
 * Controls the entire system. Calls method from this class and other classes
 * to integrate all parts of the Musical Fingerprint-Scanning Doorbell.
 * Methods specifically in this class control the interface of the touch
 * display, including drawing the screens and detecting touch overlap.
 * State machines in this class determine what part of the system is active.
 *
 */

// graphics libraries
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>

// threading library
#include <plib.h>
// config.h sets 40 MHz
#define	SYS_FREQ 40000000
#include "pt_cornell_1_2.h"
#include <stdio.h>
#include "math.h"

#include "fat.h"
#include "TouchScreen.h"
#include "adc_intf.h"
#include <string.h>

/*

 *
 ****************************************************/

// PORT B
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;
#define DisablePullDownB(bits) CNPDBCLR=bits;

#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define MAIN_MENU           0x00
#define HISTORY             0x01
#define ENROLL_SONG_SELECT  0x02
#define ENROLL_SONG_SELECT2 0x08
#define ENROLL_SCAN_FINGER  0x03
#define ENROLLMENTS         0x06
#define NO_STATE            0xFF

//First x position of back button
#define HISTORY_BACK_X1 220
//First y position of back button
#define HISTORY_BACK_Y1 190
//Second x position of back button
#define HISTORY_BACK_X2 80
//Second y position of back button
#define HISTORY_BACK_Y2 30



//////////////////////////////////////////Joe's Defines

//User inputs
#define NoInput 0
#define Enroll 1
#define Ring 2
#define flashLED -1
#define DeleteOneFinger 3
#define DeleteAllFingers 4
#define VerifyFinger 5

int userInput = DeleteAllFingers; //flashLED;
char feedbackMessage[60];



//song mapping. prob don't need id if using as a key

struct file allEnrolls[201];
//struct file enrollSong;

/* -----------------------------------------------------------------------------
    Structs
   ---------------------------------------------------------------------------*/

typedef struct Button {
    short x1;
    short y1;
    short x2;
    short y2;
    unsigned short color;
    int next_state;
    char title[11];
} Button;

typedef struct History {
    int id;
    char song_title[8];
} History;

typedef struct Enrollment {
    int id;
    char song_title[8];
} Enrollment;

/* -----------------------------------------------------------------------------
    Initializing variables
   ---------------------------------------------------------------------------*/

//button arrays
Button main_menu_buttons[4];
Button song_select_buttons_1[30];
Button song_select_buttons_2[30];
Button song_nav_buttons_1[5];
Button song_nav_buttons_2[5];

//file struct array containing enrolls
struct file allEnrolls[201];

//History struct array for logging doorbell-rings
History history_entries[100];

//music file arrays
struct file music_files[30];
struct file temp_music_files[30];
struct file selected_song;

uint32_t music_sector;
//total number of songs
uint32_t num_songs = 0;
//number of songs on enroll_song_select
uint32_t num_songs_1 = 0;
//number of songs on enroll_song_select_2
uint32_t num_songs_2 = 0;
// string buffer
uint8_t buffer[60];
//number of entries in history
uint32_t history_count = 0;
//number of enrollees
uint32_t enroll_count = 0;

//default values for touch overlap
unsigned int xPix = 10000;
unsigned int yPix = 10000;

//initial state is the main menu
int state = MAIN_MENU;
int nextState = MAIN_MENU; //next state
int prev_state = NO_STATE;

//prevents multi-touch
int isPressed = 0;


int play_song = 0;
int prev_song = 0;

static struct pt pt_touch, pt_display, pt_song, pt_WriteUART;

// system 1 second interval tick
int sys_time_seconds;

/* -----------------------------------------------------------------------------
    Methods for initializing buttons and drawing buttons
   ---------------------------------------------------------------------------*/

////Draws the big buttons on the main menu
//void draw_button(struct Button *b) {
//    tft_fillRect(b->x1,b->y1,b->x2,b->y2,b->color);
//    tft_setTextColor(ILI9341_BLACK);
//    tft_setCursor(b->x1 + 3, b->y1+30);
//    tft_writeString(b->title);
//}
//
////Draws the smaller song select buttons and song navigation buttons
//void draw_button_2(struct Button *b) {
//    tft_fillRect(b->x1,b->y1,b->x2,b->y2,b->color);
//    tft_setTextColor(ILI9341_BLACK);
//    tft_setCursor(b->x1 + 3, b->y1+5);
//    tft_writeString(b->title);
//}

////initializes buttons for the main menu
//void init_buttons() {
//    Button history = {.x1=4, .y1=30, .x2=130, .y2=70, .color=WHITE, .next_state=HISTORY, .title="History"};
//    Button enroll = {.x1=93, .y1=130, .x2=130, .y2=70, .color=WHITE, .next_state=ENROLL_SONG_SELECT, .title="Enroll"};
//    Button enrollments = {.x1=174, .y1=30, .x2=132, .y2=70, .color=WHITE, .next_state=ENROLLMENTS, .title="Enrollments"};
//    main_menu_buttons[0]=history;
//    main_menu_buttons[1]=enroll;
//    main_menu_buttons[2]=enrollments;
//}

//Reads data from SD card, parses the data searching for songs, and
//creates buttons for each song found on the SD card.

void init_songs() {
    initSongs(&temp_music_files);
    int i;
    //Finds songs on SD card and puts them in an array (some blocks aren't songs)
    for (i = 0; i < 30; i++) {
        //if cluster == 0, then ignore (not a song)
        if (temp_music_files[i].cluster != 0) {
            music_files[num_songs] = temp_music_files[i];
            num_songs++;
        }
    }
}


/* -----------------------------------------------------------------------------
      Protothreads
   ---------------------------------------------------------------------------*/

////This protothread determines the display state of the interface
//static PT_THREAD (protothread_display(struct pt *pt))
//{
//
//    PT_BEGIN(pt);
//    while(1){
//        // yield time
//        PT_YIELD_TIME_msec(16);
//
//        if(userInput==0){ //Locks display if no user input
//
//        if (state != nextState) {
//            state = nextState;
//        }
//        switch(state) {
//            case MAIN_MENU         :  main_menu(); break;
//            case HISTORY           :  history(0); break;
//            case ENROLL_SONG_SELECT:  enroll_song_select(); break;
//            case ENROLL_SONG_SELECT2: enroll_song_select_2(); break;
//            case ENROLL_SCAN_FINGER:  enroll_scan_finger(); break;
//            case ENROLLMENTS       :  enrollments(); break;
//        }
//        prev_state = state;
//    }
//    }
//    PT_END(pt);
//}


////This protothread gets the touch coordinates of the screen and converts to
////the corresponding pixel
//static PT_THREAD (protothread_touch(struct pt *pt))
//{
//    PT_BEGIN(pt);
//      while(1) {
//        PT_YIELD_TIME_msec(16);
//        struct TSPoint p;
//        p.x = 0;
//        p.y = 0;
//        p.z = 0;
//        getPoint(&p);
//
//
//
//        //register touch and converts to corresponding pixel
//        if (p.z > 0 && p.z < 10000 && isPressed == 0){
//            xPix = (unsigned int)(3174337 * p.x - 336479705) >> 23;
//            yPix = (unsigned int)(2712960 * p.y - 398805095) >> 23;
//
//            nextState = is_touch_overlap(xPix, yPix);
//            //prevent multi touch
//            isPressed = 1;
//        }
//        //wait for no press before allowing touch to update. enforces "whack"
//        else if (isPressed == 1 && p.z == 0){
//            isPressed = 0;
//        }
//        //not pressed
//        else {
//            xPix = 10000;
//            yPix = 10000;
//        }
//    }//END WHILE(1)
//    PT_END(pt);
//}

//This protothread checks for a play_song flag and plays the selected song when high

static PT_THREAD(protothread_song(struct pt *pt)) {
    PT_BEGIN(pt);
    while (1) {
        //check every millisecond to play song
        PT_YIELD_TIME_msec(1);
        //if play_song flag is high, play song and then reset the flag
        if (play_song == 1) {
            playSong(&selected_song);
            //play_song = 0;
        }
    }//END WHILE(1)
    PT_END(pt);
}

////This protothread controls communication with the fingerprint scanner
//static PT_THREAD (protothread_WriteUART(struct pt *pt))
//{
//
//    PT_BEGIN(pt);
//      while(1) {
//
//        PT_YIELD_TIME_msec(25);
//
//        userInput=StateMachine(userInput, feedbackMessage);
//
//
//        if(getFeedbackFlag()==1){
//
//         tft_fillRoundRect(0, 100, 320, 50, 10, BLACK);
//
//
//         tft_setCursor (30, 100);
//         tft_setTextColor(YELLOW); tft_setTextSize(1);
//         tft_writeString(feedbackMessage);
//        }
//
//        if(getFeedbackFlag()==2){
//         tft_fillRoundRect(0, 100, 320, 50, 10, BLACK);
//         tft_setCursor (30, 100);
//         tft_setTextColor(YELLOW); tft_setTextSize(1);
//         tft_writeString(feedbackMessage);
//
//
//          allEnrolls[getCurrentId()] = music_files[prev_song];//selected_song;
//          enroll_count++;
//        }
//
//        if(getRingFlag()==1){
//            int ringer=getRingerId();
//            //sound goes here
//            play_song=1;
//            selected_song = allEnrolls[ringer];
//            History entry = {.id=ringer,.song_title='temp'};
//            strcpy(entry.song_title, allEnrolls[ringer].fileName);
//            shift_entries();
//            history_entries[0]=entry;
//            history_count++;
//            if(state==HISTORY) {
//                history(1);
//            }
//        }
//  PT_END(pt);
//  } // keypad thread
//}

/* -----------------------------------------------------------------------------
    Main
   ---------------------------------------------------------------------------*/
void main(void) {
//    SYSTEMConfigPerformance(PBCLK);
//
//    ANSELA = 0;
//    ANSELB = 0;
//    CM1CON = 0;
//    CM2CON = 0;
//
//    // === config threads ==========
//    PT_setup();
//    
//    //INTEnableSystemMultiVectoredInt();
//    
//    // init the threads
//    PT_INIT(&pt_song);
//
//    // init the display
//    
//    //init_songs();
//    
//    tft_init_hw();
//    tft_begin();
//    tft_fillScreen(ILI9341_BLACK);
//    //240x320 vertical display
//    tft_setRotation(1); // Use tft_setRotation(1) for 320x240
//
//    //Debug interface
//    tft_fillRoundRect(0, 0, 200, 50, 1, ILI9341_BLACK); // x,y,w,h,radius,color
//    tft_setCursor(0, 0);
//    tft_setTextColor(ILI9341_WHITE);
//    tft_setTextSize(2);
////    sprintf(buffer, "%i", music_files[0].cluster);
////    tft_writeString(buffer);
//    tft_writeString("Hello World 4");
//
//
//
//
//
//    //
//    //delay_ms(10);
//    
////    selected_song = music_files[0];
////    play_song = 1;
//
//    while (1) {
//        //PT_SCHEDULE(protothread_song(&pt_song));
//
//    }
    
    
 SYSTEMConfigPerformance(PBCLK);
  ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;
  TRISB = 0x0780; TRISA = 0x0008; 
  // CNPUB = 0x01C0;
  CNPDB = 0x0780;

  
  // === config threads ==========
  // turns OFF UART support and debugger pin
  PT_setup(); 
  
  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  //PT_INIT(&pt_pushbutton);
  //PT_INIT(&pt_switch);
  //PT_INIT(&pt_led);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9341_BLACK);
  //240x320 vertical display
  tft_setRotation(3); // Use tft_setRotation(1) for 320x240
  tft_setCursor(0, 0);
  tft_setTextColor(ILI9341_WHITE);  tft_setTextSize(1);
  tft_writeString("Hello world 5" "\n");
  // seed random color
  srand(1);

  // round-robin scheduler for threads
  while (1){
      //PT_SCHEDULE(protothread_pushbutton(&pt_pushbutton));
      //PT_SCHEDULE(protothread_switch(&pt_switch));
      //PT_SCHEDULE(protothread_led(&pt_led));
      }    
    
    
}


/* -----------------------------------------------------------------------------
    Methods for drawing screens
   ---------------------------------------------------------------------------*/

////draws the main menu
//void main_menu() {
//    if (state == prev_state) return;
//    tft_fillScreen(ILI9341_BLACK);
//    tft_setCursor (100, 10);
//    tft_setTextColor(WHITE); tft_setTextSize(2);
//    sprintf(buffer,"Welcome!");
//    tft_writeString(buffer);
//    int i;
//    //Draws the buttons on the main menu
//    for (i=0; i<3; i++) {
//        draw_button(&main_menu_buttons[i]);
//    }
//}

////draws the history menu
////param override: if a new entry is added to history_entries, redraw the screen
//void history(int override) {
//    if (state == prev_state && !override) return;
//    tft_fillScreen(ILI9341_BLACK);
//    tft_setCursor (80, 10);
//    tft_setTextColor(WHITE); tft_setTextSize(2);
//    sprintf(buffer,"History");
//    tft_writeString(buffer);
//    //draws back button
//    tft_fillRect(HISTORY_BACK_X1,HISTORY_BACK_Y1, HISTORY_BACK_X2,HISTORY_BACK_Y2,WHITE);
//    tft_setCursor(HISTORY_BACK_X1+3,HISTORY_BACK_Y1+5);
//    tft_setTextColor(ILI9341_BLACK);
//    tft_writeString("Home");
//    int i;
//    int cursor=30;
//    //draws each entry in history_entries
//    //cursor increments after each
//    for (i=0; i < history_count; i++){
//        tft_setCursor(10,cursor);
//        tft_setTextColor(WHITE); tft_setTextSize(1);
//        if(history_entries[i].id==201)
//            sprintf(buffer,"UNKNOWN");
//        else
//            sprintf(buffer,"ID: %d\tSong:%s",history_entries[i].id,history_entries[i].song_title);
//        tft_writeString(buffer);
//        cursor = cursor + 15;
//    }
//}
//
////draws the first song select screen
//void enroll_song_select() {
//    if (state == prev_state) return;
//    tft_fillScreen(ILI9341_BLACK);
//    tft_setCursor (80, 10);
//    tft_setTextColor(WHITE); tft_setTextSize(2);
//    sprintf(buffer,"Select song");
//    tft_writeString(buffer);
//    //draws back button
//    tft_fillRect(HISTORY_BACK_X1,HISTORY_BACK_Y1, HISTORY_BACK_X2,HISTORY_BACK_Y2,WHITE);
//    tft_setCursor(HISTORY_BACK_X1+3,HISTORY_BACK_Y1+5);
//    tft_setTextColor(ILI9341_BLACK);
//    tft_writeString("Home");
//    int i;
//    //draws each song's button
//    for (i=0; i<10; i++) {
//        song_select_buttons_1[i].color = WHITE;
//        draw_button_2(&song_select_buttons_1[i]);
//    }
//    //draws navigation buttons
//    for (i=0; i < 5; i++){
//        draw_button_2(&song_nav_buttons_1[i]);
//    }
//}
//
////draws the second song select screen
//void enroll_song_select_2() {
//    if (state == prev_state) return;
//    tft_fillScreen(ILI9341_BLACK);
//    tft_setCursor (80, 10);
//    tft_setTextColor(WHITE); tft_setTextSize(2);
//    sprintf(buffer,"Select song");
//    tft_writeString(buffer);
//    //draws back button
//    tft_fillRect(HISTORY_BACK_X1,HISTORY_BACK_Y1, HISTORY_BACK_X2,HISTORY_BACK_Y2,WHITE);
//    tft_setCursor(HISTORY_BACK_X1+3,HISTORY_BACK_Y1+5);
//    tft_setTextColor(ILI9341_BLACK);
//    tft_writeString("Home");
//    int i;
//    //draws each song's button
//    for (i=0; i<10; i++) {
//        song_select_buttons_2[i].color = WHITE;
//        draw_button_2(&song_select_buttons_2[i]);
//    }
//    //draws navigation buttons
//    for (i=0; i < 5; i++){
//        draw_button_2(&song_nav_buttons_2[i]);
//    }
//}
//
////draws the scan finger screen that occurs after you select a song
//void enroll_scan_finger() {
//    if (state == prev_state) return;
//    tft_fillScreen(ILI9341_BLACK);
//    tft_setCursor (80, 10);
//    tft_setTextColor(WHITE); tft_setTextSize(2);
//    sprintf(buffer,"Scan finger");
//    tft_writeString(buffer);
//    //draws back button
//    tft_fillRect(HISTORY_BACK_X1,HISTORY_BACK_Y1, HISTORY_BACK_X2,HISTORY_BACK_Y2,WHITE);
//    tft_setCursor(HISTORY_BACK_X1+3,HISTORY_BACK_Y1+5);
//    tft_setTextColor(ILI9341_BLACK);
//    tft_writeString("Home");
//    //sets Fingerprint scanner to Enroll
//    userInput=Enroll;
//    tft_setCursor (30, 100);
//    tft_setTextColor(YELLOW); tft_setTextSize(1);
//    tft_writeString("Please Scan Finger. Remove after 3 Seconds.");
//}
//
////draws the enrollments screen
//void enrollments() {
//    if (state == prev_state) return;
//    tft_fillScreen(ILI9341_BLACK);
//    tft_setCursor (80, 10);
//    tft_setTextColor(WHITE); tft_setTextSize(2);
//    sprintf(buffer,"Enrollments");
//    tft_writeString(buffer);
//    //draws the back button
//    tft_fillRect(HISTORY_BACK_X1,HISTORY_BACK_Y1, HISTORY_BACK_X2,HISTORY_BACK_Y2,WHITE);
//    tft_setCursor(HISTORY_BACK_X1+3,HISTORY_BACK_Y1+5);
//    tft_setTextColor(ILI9341_BLACK);
//    tft_writeString("Home");
//     int i;
//     int cursor = 30;
//     //similar to history screen, draws each individual enrollment then increments cursor
//     for (i = 0; i < enroll_count; i++){
//        tft_setCursor(10, cursor);
//        tft_setTextColor(WHITE); tft_setTextSize(1);
//        sprintf(buffer, "ID: %d - Song: %s", i, allEnrolls[i].fileName);
//        tft_writeString(buffer);
//        cursor = cursor+15;
//    }
//
//}
//
///* -----------------------------------------------------------------------------
//    Methods for getting touch overlap of screens
//   ---------------------------------------------------------------------------*/
//
////Method that determines the next state of the interface based on touch overlaps
////param x: x position of touch
////param y: y position of touch
////return next state
//int is_touch_overlap(int x, int y) {
//    switch(state) {
//        case MAIN_MENU:           return get_main_menu_overlap(x, y);
//        case HISTORY:             return get_history_overlap(x, y);
//        case ENROLL_SONG_SELECT:  return get_enroll_song_select_overlap(x, y);//return get_enroll_song_select_overlap();
//        case ENROLL_SONG_SELECT2: return get_enroll_song_select_overlap_2(x, y);
//        case ENROLL_SCAN_FINGER:  return get_enroll_scan_finger_overlap(x, y);//return get_enroll_scan_finger_overlap();
//        case ENROLLMENTS:         return get_enrollments_overlap(x, y);
//    }
//    //Nothing interactive was touched, stay at this state
//    return state;
//}
//
////detects any overlap on the main menu
////param x: x position of touch
////param y: y position of touch
////return next state
//int get_main_menu_overlap(int x, int y) {
//    int i;
//    for (i=0; i<3; i++){
//        if( x > main_menu_buttons[i].x1 &&
//                            x < (main_menu_buttons[i].x1 + main_menu_buttons[i].x2) &&
//                            y > main_menu_buttons[i].y1 &&
//                            y < (main_menu_buttons[i].y1 + main_menu_buttons[i].y2)) {
//            return main_menu_buttons[i].next_state;
//        }
//    }
//    return MAIN_MENU;
//}
//
////detects any overlap on the history menu
////param x: x position of touch
////param y: y position of touch
////return next state
//int get_history_overlap(int x, int y) {
//    if (x > HISTORY_BACK_X1 && x < (HISTORY_BACK_X1 + HISTORY_BACK_X2) && y > HISTORY_BACK_Y1 && y < (HISTORY_BACK_Y1 + HISTORY_BACK_Y2)) {
//        return MAIN_MENU;
//    }
//    else {
//        return HISTORY;
//    }
//}
//
////detects any overlap on the first song select menu
////param x: x position of touch
////param y: y position of touch
////return next state
//int get_enroll_song_select_overlap(int x, int y) {
//    //If we press the back button, go back to Main Menu
//    if (x > HISTORY_BACK_X1 && x < (HISTORY_BACK_X1 + HISTORY_BACK_X2) && y > HISTORY_BACK_Y1 && y < (HISTORY_BACK_Y1 + HISTORY_BACK_Y2)) {
//        return MAIN_MENU;
//    }
//    int i;
//    //detects any presses on navigation buttons
//    for (i=0; i<5; i++) {
//      if( x > song_nav_buttons_1[i].x1 &&
//                          x < (song_nav_buttons_1[i].x1 + song_nav_buttons_1[i].x2) &&
//                          y > song_nav_buttons_1[i].y1 &&
//                          y < (song_nav_buttons_1[i].y1 + song_nav_buttons_1[i].y2)) {
//            if (i == 1) { //if pressed demo song button, play song
//              play_song=1;
//            }
//            return song_nav_buttons_1[i].next_state;
//        }
//    }
//    //Detect any presses on song buttons. If a song is pressed, highlight that
//    //song and set a flag to play that song.
//    for (i=0; i<num_songs_1; i++) {
//
//        if( x > song_select_buttons_1[i].x1 &&
//                            x < (song_select_buttons_1[i].x1 + song_select_buttons_1[i].x2) &&
//                            y > song_select_buttons_1[i].y1 &&
//                            y < (song_select_buttons_1[i].y1 + song_select_buttons_1[i].y2)) {
//            //sets the selected song to play
//            selected_song = music_files[i];
//            //sets flag to play selected song
//            //removes highlighting from previously played song, if any
//            song_select_buttons_1[prev_song].color = WHITE;
//            draw_button_2(&song_select_buttons_1[prev_song]);
//            //highlights selected song
//            song_select_buttons_1[i].color = GREEN;
//            draw_button_2(&song_select_buttons_1[i]);
//            prev_song = i;
//            //Next state is current state
//            return song_select_buttons_1[i].next_state;
//        }
//    }
//    return ENROLL_SONG_SELECT;
//}
//
////detects any presses on second song select menu
////param x: x position of touch
////param y: y position of touch
////return next state
//int get_enroll_song_select_overlap_2(int x, int y) {
//    //If we press the back button, go back to Main Menu
//    if (x > HISTORY_BACK_X1 && x < (HISTORY_BACK_X1 + HISTORY_BACK_X2) && y > HISTORY_BACK_Y1 && y < (HISTORY_BACK_Y1 + HISTORY_BACK_Y2)) {
//        return MAIN_MENU;
//    }
//    int i;
//    //detect any presses on navigation buttons
//    for (i=0; i<5; i++) {
//      if( x > song_nav_buttons_2[i].x1 &&
//                          x < (song_nav_buttons_2[i].x1 + song_nav_buttons_2[i].x2) &&
//                          y > song_nav_buttons_2[i].y1 &&
//                          y < (song_nav_buttons_2[i].y1 + song_nav_buttons_2[i].y2)) {
//            if (i == 1) { //if pressed demo song button, play song
//              play_song=1;
//            }
//            return song_nav_buttons_2[i].next_state;
//        }
//    }
//    //Detect any presses. If a song is pressed, highlight that song and set
//    //a flag to play that song.
//    for (i=0; i<num_songs_2; i++) {
//
//        if( x > song_select_buttons_2[i].x1 &&
//                            x < (song_select_buttons_2[i].x1 + song_select_buttons_2[i].x2) &&
//                            y > song_select_buttons_2[i].y1 &&
//                            y < (song_select_buttons_2[i].y1 + song_select_buttons_2[i].y2)) {
//            //sets the selected song to play
//            selected_song = music_files[i+10];
//            //sets flag to play selected song
//            //removes highlighting from previously played song, if any
//            song_select_buttons_2[prev_song-10].color = WHITE;
//            draw_button_2(&song_select_buttons_2[prev_song-10]);
//            //highlights selected song
//            song_select_buttons_2[i].color = GREEN;
//            draw_button_2(&song_select_buttons_2[i]);
//            prev_song = i+10;
//            //Next state is current state
//            return song_select_buttons_2[i].next_state;
//        }
//    }
//    return ENROLL_SONG_SELECT2;
//}
//
////detects any overlap on the enrollments menu
////param x: x position of touch
////param y: y position of touch
////return next state
//int get_enrollments_overlap(int x, int y) {
//    if (x > HISTORY_BACK_X1 && x < (HISTORY_BACK_X1 + HISTORY_BACK_X2) && y > HISTORY_BACK_Y1 && y < (HISTORY_BACK_Y1 + HISTORY_BACK_Y2)) {
//        return MAIN_MENU;
//    }
//    else {
//        return ENROLLMENTS;
//    }
//}
//
////detects any overlap on the scan finger menu
////param x: x position of touch
////param y: y position of touch
////return next state
//int get_enroll_scan_finger_overlap(int x, int y) {
//    if (x > HISTORY_BACK_X1 && x < (HISTORY_BACK_X1 + HISTORY_BACK_X2) && y > HISTORY_BACK_Y1 && y < (HISTORY_BACK_Y1 + HISTORY_BACK_Y2)) {
//        return MAIN_MENU;
//    }
//    return ENROLL_SCAN_FINGER;
//}
//
///* -----------------------------------------------------------------------------
//    Misc methods
//   ---------------------------------------------------------------------------*/
//
////Shifts the entries in the history page so most recent entry is on top
//void shift_entries(){
//    int i;
//    for(i=history_count-1;i>=0;i--){
//        history_entries[i+1]=history_entries[i];
//    }
//}