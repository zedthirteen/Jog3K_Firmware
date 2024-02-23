/**
 * Example program for basic use of pico as an I2C peripheral (previously known as I2C slave)
 * 
 * This example allows the pico to act as a 256byte RAM
 * 
 * Author: Graham Smith (graham@smithg.co.uk)
 */


// Usage:
//
// When writing data to the pico the first data byte updates the current address to be used when writing or reading from the RAM
// Subsequent data bytes contain data that is written to the ram at the current address and following locations (current address auto increments)
//
// When reading data from the pico the first data byte returned will be from the ram storage located at current address
// Subsequent bytes will be returned from the following ram locations (again current address auto increments)
//
// N.B. if the current address reaches 255, it will autoincrement to 0 after next read / write

#include <stdio.h>
#include "pico/stdlib.h"
#include <SPI.h>
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"

#include "pico/stdio.h"
#include "pico/time.h"

#include "i2c_jogger.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>


#include <ManualmaticFonts.h>
#include <ManualmaticUtils.h>
#include <DisplayUtils.h>

#include "app_screen.h"

#define SPI_TX_PIN 19
#define SPI_CLK_PIN 18
#define SPI_CSn_PIN 17
#define SCR_DC_PIN 16
#define SCR_RESET_PIN 20

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

// Used for software SPI
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128 // Change this to 96 for 1.27" OLED.

#define SCLK_PIN SPI_CLK_PIN
#define MOSI_PIN SPI_TX_PIN
#define DC_PIN   SCR_DC_PIN
#define CS_PIN   SPI_CSn_PIN
#define RST_PIN  SCR_RESET_PIN

#define LOOP_PERIOD 35 // Display updates every 35 ms

//#define SHOWJOG 1
//#define SHOWOVER 1
#define SHOWRAM 1
#define TWOWAY 0

#define SCREEN_ENABLED 0

#define OLED_SCREEN_FLIP 1

enum Jogmode current_jogmode = {};
enum Jogmodify current_jogmodify = {};
enum ScreenMode screenmode = {};
Machine_status_packet prev_packet = {};
Jogmode previous_jogmode = {};
Jogmodify previous_jogmodify = {};
ScreenMode previous_screenmode = {};
int command_error = 0;
bool screenflip = false;
float step_calc = 0;

const uint16_t displayWidth = 128;
const uint16_t displayHeight = 128;
const uint8_t axesAreaHeight = 54; //All axis
const uint8_t encoderLabelAreaHeight = 6;
const uint8_t encoderValueAreaHeight[3] = {25, 13, 13}; //Single line, 1st line, 2nd line
const uint8_t encoderColumnWidth = 50; //Centre colum is reduced to 105
const uint8_t buttonColumnWidth = 30;
//The top Y of each axis row.
uint8_t axisDisplayY[4] = {0, 18, 36, 52};

// RPI Pico

char buf[8];

const uint8_t * flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

// Option 1: use any pins but a little slower
//Adafruit_SSD1351 gfx = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);  
  // Option 2: must use the hardware SPI pins 
  // (for UNO thats sclk = 13 and sid = 11) and pin 10 must be 
  // an output. This is much faster - also required if you want
  // to use the microSD card (see the image drawing example)

Adafruit_SSD1351 gfx = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

/**
 * For the display of each axis Abs, G5x or DTG position
 */
DisplayNumber axisPosition[4] = { DisplayNumber(gfx), DisplayNumber(gfx), DisplayNumber(gfx), DisplayNumber(gfx) }; 

/**************************************************************************/
/*! 
    @brief  Renders a simple test pattern on the screen
*/
/**************************************************************************/
void lcdTestPattern(void)
{
  static const uint16_t PROGMEM colors[] =
    { RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA, BLACK, WHITE };

  for(uint8_t c=0; c<8; c++) {
    gfx.fillRect(0, gfx.height() * c / 8, gfx.width(), gfx.height() / 8,
      pgm_read_word(&colors[c]));
  }
}

void init_screen (void){

  if (*flash_target_contents != 0xff)
    screenflip = *flash_target_contents;

  SPI.setCS(SPI_CSn_PIN);
  SPI.setTX(SPI_TX_PIN);
  SPI.setSCK(SPI_CLK_PIN);
  // Init Display  
  gfx.begin();
  //gfx.setRotation(1);
  //Set up areas

  areas.axes = DisplayArea(0, 0, displayWidth, axesAreaHeight);
  areas.axesMarkers = DisplayArea(0, 0, 19, axesAreaHeight);
  areas.axesLabels = DisplayArea(20, 0, 50, axesAreaHeight);
  areas.axesCoords = DisplayArea(50, 0, 52, axesAreaHeight);
  areas.encoderLabel[0] = DisplayArea(0, axesAreaHeight, encoderColumnWidth, encoderLabelAreaHeight);
  areas.encoderLabel[1] = DisplayArea(encoderColumnWidth + 1, axesAreaHeight, encoderColumnWidth, encoderLabelAreaHeight);
  areas.encoderLabel[2] = DisplayArea((encoderColumnWidth * 2) + 2, axesAreaHeight, encoderColumnWidth, encoderLabelAreaHeight);
  areas.encoderValue[0] = DisplayArea(0, axesAreaHeight + encoderLabelAreaHeight, encoderColumnWidth, encoderValueAreaHeight[0] );
  areas.encoderValue[1] = DisplayArea(encoderColumnWidth + 1, axesAreaHeight + encoderLabelAreaHeight, encoderColumnWidth, encoderValueAreaHeight[0] );
  areas.encoderValue[2] = DisplayArea((encoderColumnWidth * 2) + 2, axesAreaHeight + encoderLabelAreaHeight, encoderColumnWidth, encoderValueAreaHeight[0] );

  areas.buttonLabels[0] = DisplayArea( 0, areas.encoderValue[0].b() + 1, buttonColumnWidth, displayHeight - areas.encoderValue[0].b() - 1);
  areas.buttonLabels[1] = DisplayArea( buttonColumnWidth + 1, areas.encoderValue[0].b() + 1, buttonColumnWidth, displayHeight - areas.encoderValue[0].b() - 1);
  areas.buttonLabels[2] = DisplayArea( (buttonColumnWidth * 2) + 2, areas.encoderValue[0].b() + 1, buttonColumnWidth, displayHeight - areas.encoderValue[0].b() - 1);
  areas.buttonLabels[3] = DisplayArea( (buttonColumnWidth * 3) + 3, areas.encoderValue[0].b() + 1, buttonColumnWidth, displayHeight - areas.encoderValue[0].b() - 1);
  areas.buttonLabels[4] = DisplayArea( (buttonColumnWidth * 4) + 4, areas.encoderValue[0].b() + 1, buttonColumnWidth + 1, displayHeight - areas.encoderValue[0].b() - 1);

  areas.axisMarkers = DisplayArea(0, 0, 16, axesAreaHeight);

  areas.debugRow = DisplayArea(0, areas.buttonLabels[0].y(), displayWidth, areas.buttonLabels[0].h());

  lcdTestPattern();
  //delay(100);

  gfx.fillRect(0, 0, 128, 128, BLACK);

}

// Converts an uint32 variable to string.
char *uitoa (uint32_t n)
{
    char *bptr = buf + sizeof(buf);

    *--bptr = '\0';

    if (n == 0)
        *--bptr = '0';
    else while (n) {
        *--bptr = '0' + (n % 10);
        n /= 10;
    }

    return bptr;
}

static char *map_coord_system (coord_system_id_t id)
{  
    uint8_t g5x = id + 54;

    strcpy(buf, uitoa((uint32_t)(g5x > 59 ? 59 : g5x)));
    if(g5x > 59) {
        strcat(buf, ".");
        strcat(buf, uitoa((uint32_t)(g5x - 59)));
    }

    return buf;
}

static void drawEncoderValue(uint8_t pos, uint8_t lineNum, const char *val, const char *uom, int bg /*= BLACK*/, int fg /*= WHITE*/ ) {
  int16_t  x, y;
  uint16_t w, h;
  char buf[10];
  strcpy(buf, val);
  if ( strlen(uom) > 0 ) {
    strcat(buf, uom);
  }
  trimTrailing(buf);
  char b[10];
  dtostrf(strlen(buf), 3, 0, b);
  //If line num is 0 then only one line, if 1 then first (of 2) lines, if 2 then second (of 2) lines
  uint8_t div = lineNum > 0 ? 2 : 1;
  uint8_t row = lineNum > 1 ? 1 : 0;
  gfx.fillRect(areas.encoderValue[pos].x(), areas.encoderValue[pos].yDiv(div, row), areas.encoderValue[pos].w(), areas.encoderValue[pos].hDiv(div), bg);
  gfx.setFont(&FreeSansBold18pt7b);
  gfx.getTextBounds(buf, areas.encoderValue[pos].x(), areas.encoderValue[pos].y(), &x, &y, &w, &h);
  if ( lineNum > 0 || w > areas.encoderValue[pos].w() ) {
    gfx.setFont(&FreeSansBold12pt7b);
    gfx.getTextBounds(buf, areas.encoderValue[pos].x(), areas.encoderValue[pos].y(), &x, &y, &w, &h);
  }
  gfx.setTextColor(fg);
  gfx.setCursor(areas.encoderValue[pos].xCl()-(w/2), areas.encoderValue[pos].yCl(div, row) + (h/2));
  gfx.print(buf);
  gfx.setTextColor(WHITE);
}

static void setNumDrawnAxes(uint8_t num) {
  uint8_t axes = min(max(3, num), 4);
  uint8_t topMargin = axes == 4 ? 0 : 5;
  uint8_t incr = (axes == 4 ? 34 : 44);
  for (uint8_t i = 0; i < axes; i++) {
    axisDisplayY[i] = ( (i * incr) + topMargin );
    axisPosition[i].setFont(&FreeMonoBold24pt7b);
    axisPosition[i].setFormat(7, 3); //@TODO 4 for inches
    axisPosition[i].setPosition(gfx.width()-axisPosition[i].w(), axisDisplayY[i]);    
  }
}

static int axisColour(uint8_t axis) {
  /*if ( !state.homed[axis] ) {
    return RED;
  }
  if ( state.displayedCoordSystem == DISPLAY_COORDS_ABS ) { //Abs
    return YELLOW;
  } else if ( state.displayedCoordSystem == DISPLAY_COORDS_DTG ) { //DTG
    return BLUE;
  } else if ( state.displayedCoordSystem == DISPLAY_COORDS_G5X ) { //G5x
    return GREEN;
  }*/
  return WHITE;
}

static void drawAxisLabel(uint8_t axis, bool forceRefresh /*= false*/) {
  if ( forceRefresh ) {
    uint8_t da = 4;
    gfx.fillRect(areas.axesLabels.x(), areas.axes.yDiv(da, axis), areas.axesLabels.w(), areas.axes.hDiv(da), BLACK );
    gfx.setFont(&FreeMonoBold24pt7b);
    gfx.setCursor(areas.axesLabels.x(), areas.axes.yCl(da, axis) + 12);
    gfx.print(AXIS_NAME[axis]);
  }
}


static void drawAxisCoord(Machine_status_packet *packet, uint8_t axis, bool forceRefresh /*= false*/) { //[3]) {
  if ( forceRefresh ) {
    uint8_t da = 4;
    gfx.fillRect(areas.axesCoords.x(), areas.axes.yDiv(da, axis), areas.axesCoords.w(), areas.axes.hDiv(da), BLACK );
    gfx.setFont(&FreeMono9pt7b);
    gfx.setCursor(areas.axesCoords.x(), areas.axes.yCl(da, axis) + 12);
    gfx.print(map_coord_system(packet->current_wcs));
    
  }
}

static void drawAxis(Machine_status_packet *packet, uint8_t axis, bool forceRefresh /*= false*/) {
 /*if ( drawn.homed[axis] != state.homed[axis] ) {
    forceRefresh = true;
    drawn.homed[axis] = state.homed[axis];
  }*/
  gfx.setTextColor(axisColour(axis));
  drawAxisLabel(axis, forceRefresh);
  drawAxisCoord(packet, axis, forceRefresh);
  bool updated = true;
  if ( forceRefresh || updated ) {
    switch (axis){
      case 0:
        axisPosition[axis].draw(packet->x_coordinate, axisColour(axis), forceRefresh );
      break;
      case 1:
        axisPosition[axis].draw(packet->y_coordinate, axisColour(axis), forceRefresh );
      break;
      case 2:
        axisPosition[axis].draw(packet->z_coordinate, axisColour(axis), forceRefresh );
      break;
      case 3:
        axisPosition[axis].draw(packet->a_coordinate, axisColour(axis), forceRefresh );
      break;
    }
  }
}

static void drawAxes (Machine_status_packet *packet, bool forceRefresh) {

  uint8_t numaxes;

  if(packet->a_coordinate < 65535)
    numaxes = 4;
  else
    numaxes =3;

  if ( forceRefresh) {
    forceRefresh = true;
    gfx.fillRect(areas.axes.x(), areas.axes.y(), areas.axes.w(), areas.axes.h(), BLACK);
    setNumDrawnAxes(numaxes);
  }

  for ( int axis = 0; axis < numaxes; axis++ ) {
    drawAxis(packet, axis, forceRefresh);
  }
}

void draw_dro_readout(Machine_status_packet *previous_packet, Machine_status_packet *packet){

  #if 1
  char charbuf[32];

  if (packet->current_wcs != previous_packet->current_wcs || screenmode != previous_screenmode){

    //need to find a place to put the G5xx word.

    //oledWriteString(&oled, 0,0,2,(char *)"                G", FONT_6x8, 0, 1);
    //oledWriteString(&oled, 0,-1,-1,map_coord_system(packet->current_wcs), FONT_6x8, 0, 1);
    //oledWriteString(&oled, 0,-1,-1,(char *)"  ", FONT_6x8, 0, 1);
  }

  if(packet->x_coordinate != previous_packet->x_coordinate || 
      packet->y_coordinate != previous_packet->y_coordinate || 
      packet->z_coordinate != previous_packet->z_coordinate || 
      packet->a_coordinate != previous_packet->a_coordinate ||
      screenmode != previous_screenmode){
        drawAxes(packet, 0);
  }

  #endif
}

void draw_feedrate(Machine_status_packet *previous_packet, Machine_status_packet *packet){

#if SCREEN_ENABLED
  char charbuf[32];

  if (packet->machine_state == STATE_HOLD){
    oledWriteString(&oled, 0,0,INFOLINE,(char *)"    HOLDING     ", JOGFONT, 0, 1);
    return;
  }
  
  if(packet->machine_state == STATE_CYCLE){
    sprintf(charbuf, "        : %3.3f ", packet->feed_rate);
    oledWriteString(&oled, 0,0,INFOLINE,charbuf, INFOFONT, 0, 1);

    oledWriteString(&oled, 0,0,INFOLINE,(char *)"RUN FEED", INFOFONT, 0, 1);
    return;
  }

  if (packet->jog_mode!=previous_packet->jog_mode || packet->jog_stepsize!=previous_packet->jog_stepsize ||
      screenmode != previous_screenmode){
    sprintf(charbuf, "        : %3.3f ", packet->jog_stepsize);
    oledWriteString(&oled, 0,0,INFOLINE,charbuf, INFOFONT, 0, 1);
    sprintf(charbuf, "        : %3.3f ", step_calc);
    oledWriteString(&oled, 0,0,1,charbuf, INFOFONT, 0, 1);          
    switch (current_jogmode) {
      case FAST :
      case SLOW : 
        oledWriteString(&oled, 0,0,INFOLINE,(char *)"JOG FEED", INFOFONT, 0, 1); 
        break;
      case STEP : 
        oledWriteString(&oled, 0,0,INFOLINE,(char *)"JOG STEP", INFOFONT, 0, 1);
        break;
      default :
        oledWriteString(&oled, 0,0,INFOLINE,(char *)"ERROR ", INFOFONT, 0, 1);
      break; 
        }//close jog states
  }
#endif
}

static void draw_machine_status(Machine_status_packet *previous_packet, Machine_status_packet *packet){
#if SCREEN_ENABLED
  char charbuf[32];

  oledWriteString(&oled, 0,94,4,(char *)" ", FONT_6x8, 0, 1);
  switch (packet->machine_state){
    case STATE_IDLE :
    oledWriteString(&oled, 0,-1,-1,(char *)"IDLE", FONT_6x8, 0, 1); 
    break;
    case STATE_JOG :
    oledWriteString(&oled, 0,-1,-1,(char *)"JOG ", FONT_6x8, 0, 1);
    break;
    case STATE_TOOL_CHANGE :
    oledWriteString(&oled, 0,-1,-1,(char *)"TOOL", FONT_6x8, 0, 1); 
    break;                   
  }  
#endif
}

static void draw_alt_screen(Machine_status_packet *previous_packet, Machine_status_packet *packet){
#if SCREEN_ENABLED 
 char charbuf[32];

  sprintf(charbuf, "MAC_1", packet->y_coordinate);
  oledWriteString(&oled, 0,0,2,charbuf, FONT_8x8, 0, 1);

  sprintf(charbuf, "MAC_4", packet->y_coordinate);
  oledWriteString(&oled, 0,0,3,charbuf, FONT_8x8, 0, 1);

  sprintf(charbuf, "MAC_2", packet->y_coordinate);
  oledWriteString(&oled, 0,12,3,charbuf, FONT_8x8, 0, 1);

  sprintf(charbuf, "MAC_3", packet->y_coordinate);
  oledWriteString(&oled, 0,0,4,charbuf, FONT_8x8, 0, 1);

  sprintf(charbuf, "MAC_6", packet->y_coordinate);
  oledWriteString(&oled, 0,24,2,charbuf, FONT_8x8, 0, 1);

  sprintf(charbuf, "MAC_6", packet->y_coordinate);
  oledWriteString(&oled, 0,24,4,charbuf, FONT_8x8, 0, 1);
#endif
}

static void draw_homing_screen(Machine_status_packet *previous_packet, Machine_status_packet *packet){
#if SCREEN_ENABLED 
 char charbuf[32];

#endif
}

static void draw_alarm_screen(Machine_status_packet *previous_packet, Machine_status_packet *packet){
#if SCREEN_ENABLED 
 char charbuf[32];

#endif
}

static void draw_disconnected_screen(Machine_status_packet *previous_packet, Machine_status_packet *packet){
#if SCREEN_ENABLED 
 char charbuf[32];

#endif
}

static void draw_overrides_rpm(Machine_status_packet *previous_packet, Machine_status_packet *packet){
#if SCREEN_ENABLED
  char charbuf[32];

  if (packet->machine_state == STATE_TOOL_CHANGE){
    oledWriteString(&oled, 0,0,6,(char *)"                    ", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0,0,BOTTOMLINE,(char *)" TOOL CHANGE", INFOFONT, 0, 1);
    return;
  }

  oledWriteString(&oled, 0,0,6,(char *)"                 RPM", FONT_6x8, 0, 1);           
  sprintf(charbuf, "S:%3d  F:%3d    ", packet->spindle_override, packet->feed_override);
  oledWriteString(&oled, 0,0,BOTTOMLINE,charbuf, FONT_6x8, 0, 1);
  //this is the RPM number
  sprintf(charbuf, "%5d", packet->spindle_rpm);
  oledWriteString(&oled, 0,-1,-1,charbuf, FONT_6x8, 0, 1);      
#endif
}

void draw_main_screen(Machine_status_packet *previous_packet, Machine_status_packet *packet, bool force){ 
#if 1
  int i = 0;
  int j = 0;
  char charbuf[32];

  int x, y;
  
  switch (screenmode){
  case JOG_MODIFY:
  //put hints about alternate button functions here. 
  if(screenmode != previous_screenmode){
    gfx.fillScreen(0);
    draw_alt_screen(previous_packet, packet);
  }
  break;
  default:  
    if(previous_screenmode == JOG_MODIFY){
      gfx.fillScreen(0);
    }

    if((previous_screenmode == ALARM || previous_screenmode == HOMING) &&
        (screenmode == DEFAULT ||
        screenmode == JOGGING ||
        screenmode == TOOL_CHANGE ||
        screenmode == RUN)){
      gfx.fillScreen(0);
    } 

      //if(packet->machine_state != previous_packet->machine_state)
      //  oledFill(&oled, 0,1);//clear screen on state change

      switch (packet->machine_state){
        case STATE_JOG : //jogging is allowed       
        case STATE_IDLE : //jogging is allowed
          draw_feedrate(previous_packet, packet);
          draw_machine_status(previous_packet, packet);
          draw_dro_readout(previous_packet, packet);
          draw_overrides_rpm(previous_packet, packet);        
        break;//close idle state

        case STATE_CYCLE :
          //can still adjust overrides during hold
          //no jog during hold, show feed rate.
          draw_feedrate(previous_packet, packet);
          draw_machine_status(previous_packet, packet);
          draw_dro_readout(previous_packet, packet);
          draw_overrides_rpm(previous_packet, packet);   
        break; //close cycle case        

        case STATE_HOLD :
          draw_feedrate(previous_packet, packet);
          draw_machine_status(previous_packet, packet);
          draw_dro_readout(previous_packet, packet);
          draw_overrides_rpm(previous_packet, packet);                 
        break; //close hold case

        case STATE_TOOL_CHANGE :
          //dream feature is to put tool info/comment/number on screen during tool change.
          //cannot adjust overrides during tool change
          //jogging allowed during tool change
          draw_feedrate(previous_packet, packet);
          draw_machine_status(previous_packet, packet);
          draw_dro_readout(previous_packet, packet);
          draw_overrides_rpm(previous_packet, packet);      
        break; //close tool case

        case STATE_HOMING : //no overrides during homing
          //oledFill(&oled, 0,1);
          draw_homing_screen(previous_packet, packet);
        break; //close home case

        case STATE_ALARM : //no overrides during homing
          draw_alarm_screen(previous_packet, packet);      
        break; //close home case                               
        default :
          draw_disconnected_screen(previous_packet, packet);          
        break; //close default case
      }//close machine_state switch statement
  }//close screen mode switch statement
#endif  
  prev_packet = *packet;
  previous_jogmode = current_jogmode;
  previous_jogmodify = current_jogmodify;
  previous_screenmode = screenmode;
}//close draw main screen

// Main loop - initilises system and then loops while interrupts get on with processing the data

/*
red = (255, 0, 0)
orange = (255, 165, 0)
yellow = (255, 150, 0)
green = (0, 255, 0)
blue = (0, 0, 255)
indigo = (75, 0, 130)
violet = (138, 43, 226)
off = (0, 0, 0)
*/
