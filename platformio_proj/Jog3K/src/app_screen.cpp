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
#include "jog3k_icons.h"


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

jog_mode_t current_jogmode = {};
enum ScreenMode screenmode = {};
jog_mode_t previous_jogmode = {};
ScreenMode previous_screenmode = {};
int command_error = 0;
bool screenflip = false;
float step_calc = 0;

const uint16_t displayWidth = 128;
const uint16_t displayHeight = 128;
const uint8_t axesAreaHeight = 50; //All axis
const uint8_t axesLabelWidth = 20; //All axis
const uint8_t axesCoordWidth = 50;
const uint8_t axesCoordHeight = 20;
const uint8_t feedRateHeight = 21; //also used for rpm
const uint8_t feedRateWidth = 64; //also used for rpm
//const uint8_t encoderLabelAreaHeight = 6;
//const uint8_t encoderValueAreaHeight[3] = {25, 13, 13}; //Single line, 1st line, 2nd line
//const uint8_t encoderColumnWidth = 50; 
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

DisplayAreas_s areas;

/**
 * For the display of each axis Abs, G5x or DTG position
 */
DisplayNumber axisPosition[4] = { DisplayNumber(gfx), DisplayNumber(gfx), DisplayNumber(gfx), DisplayNumber(gfx) }; 
DisplayNumber feedrate_display = DisplayNumber(gfx);
DisplayNumber rpm_display = DisplayNumber(gfx);
DisplayNumber feed_over_display = DisplayNumber(gfx);
DisplayNumber spindle_over_display = DisplayNumber(gfx);


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

  //set up display areas:
  areas.feedRate = DisplayArea(0,0,feedRateWidth,feedRateHeight);// Feed rate gets half of bar at top
  areas.spindleRPM = DisplayArea(feedRateWidth,0,feedRateWidth,feedRateHeight);// RPM gets other half of width
  areas.axes = DisplayArea(0, feedRateHeight, displayWidth, axesAreaHeight);  //DRO under feed.
  //areas.axesMarkers = DisplayArea(0, 0, 19, axesAreaHeight);
  areas.axesLabels = DisplayArea(0, feedRateHeight, axesLabelWidth, axesAreaHeight); //Axes labels down the side
  areas.axesCoords = DisplayArea(0, axesAreaHeight+feedRateHeight, axesCoordWidth, axesCoordHeight); //Current coordinate system under axes
  areas.infoMessage = DisplayArea(0,120,displayWidth,displayHeight-120); //info message along the bottom
  areas.machineStatus = DisplayArea(axesCoordWidth, axesAreaHeight + feedRateHeight+5, displayWidth - axesCoordWidth, axesCoordHeight-5); //status beside coordinates
  areas.feedOverride = DisplayArea(0,axesAreaHeight+feedRateHeight+axesCoordHeight+3,(128-axesCoordWidth)/2,feedRateHeight);// Feed over gets half of width
  areas.spindleOverride= DisplayArea((128/2),axesAreaHeight+feedRateHeight+axesCoordHeight+3,(128-axesCoordWidth)/2,feedRateHeight);// spindle over gets other half of width

  areas.debugRow = DisplayArea(0, axesAreaHeight, displayWidth, displayHeight - axesAreaHeight);
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
  

  lcdTestPattern();
  delay(250);

  gfx.fillRect(0, 0, 128, 128, BLACK);

  screenmode = none;
  previous_screenmode = DISCONNECTED;

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

void setNumDrawnAxes(uint8_t num) {
  uint8_t axes = min(max(3, num), 4);
  uint8_t topMargin = axes == 4 ? 0 : 0;
  uint8_t incr = (axes == 4 ? 15 : 20);
  for (uint8_t i = 0; i < axes; i++) {
    axisDisplayY[i] = (areas.axes.y() + (i * incr) + topMargin );
    if (axes < 4)
      //axisPosition[i].setFont(&FreeMono12pt7b);
      axisPosition[i].begin(&FreeMono12pt7b);
    else
      axisPosition[i].begin(&FreeMono9pt7b);
    axisPosition[i].setFormat(7, 3); //@TODO 4 for inches
    axisPosition[i].setPosition(gfx.width()-axisPosition[i].w(), axisDisplayY[i]);
  }
}

int axisColour(uint8_t axis) {
  //need to modify this so that the axis color is set by homing status and also possibly the currently selected axis.
  
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

  if (axis == 0)
    return WHITE;

  if (axis == 1)
    return BLUE;

  if (axis == 2)
    return GREEN;        

  return WHITE;
}

void print_info_string(char *infostring){
  static uint8_t numaxes = 0;
  gfx.setCursor(0, 120);
  gfx.setTextColor(WHITE);  
  gfx.setTextSize(1);
  gfx.println(infostring);
}

void drawAxisCoord(uint8_t numaxes, coord_system_id_t current_wcs) { //[3]) {
    uint8_t da = numaxes;
    gfx.fillRect(areas.axesCoords.x(), areas.axesCoords.y(), areas.axesCoords.w(), areas.axesCoords.h(), BLACK );
    gfx.setFont(&Arimo_Regular_12);
    gfx.setCursor(areas.axesCoords.x(), areas.axesCoords.y()+areas.axesCoords.h());
    //gfx.setCursor(areas.axesCoords.x(), areas.axesCoords.y());

    gfx.print("G");
    gfx.print(map_coord_system(current_wcs));

}

void setup_dro_readout(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){
  static uint8_t numaxes = 0;
  print_info_string("Draw DRO Info");
  //change this to read the a coord and update to 3 or 4 axes
  numaxes = 3;
  setNumDrawnAxes(numaxes);
  drawAxisCoord(numaxes, packet->current_wcs);
  gfx.fillRect(areas.axesLabels.x(), areas.axesLabels.y(), areas.axesLabels.w(), areas.axesLabels.h(), BLACK );
  uint8_t axes = min(max(3, numaxes), 4);
  uint8_t topMargin = 15;
  uint8_t incr = (axes == 4 ? 15 : 20);
  if (numaxes < 4)
    gfx.setFont(&FreeMonoBold12pt7b);
  else
    gfx.setFont(&FreeMono9pt7b);
  for (int i = 0; i<numaxes; i++){
    gfx.setTextColor(axisColour(i));
    gfx.setCursor(areas.axesLabels.x(), (areas.axes.y() + (i * incr) + topMargin ));
    gfx.print(AXIS_NAME[i]);
  }
           
}

void draw_dro_readout(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){

  static uint8_t numaxes = 0;

  if(packet->coordinate.x != previous_packet->coordinate.x || 
      packet->coordinate.y != previous_packet->coordinate.y || 
      packet->coordinate.z != previous_packet->coordinate.z || 
      packet->coordinate.a != previous_packet->coordinate.a ||
      screenmode != previous_screenmode){
    //if(true){      
          if(screenmode != previous_screenmode){
            setup_dro_readout(packet, previous_packet);
          }

          for( int axis = 0; axis < 3; axis++){
              axisPosition[axis].draw(packet->coordinate.values[axis], axisColour(axis), false);
          }
  }


}

void draw_feedrate(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){

      //update the section on state changes
      if(previous_packet->machine_state!=packet->machine_state){      
        //clear the feedrate section and write text and set up the number
        gfx.fillRect(areas.feedRate.x(), areas.feedRate.y(), areas.feedRate.w(), areas.feedRate.h(), BLACK );
        feedrate_display.begin(&FreeMono9pt7b);
        feedrate_display.setFormat(3,0);
        //feedrate_display.setPosition(areas.feedRate.x()+20,areas.feedRate.y());
        feedrate_display.setPosition((feedRateWidth-(feedrate_display.w())) -3, areas.feedRate.y()+1);
        gfx.setFont(&Arimo_Regular_12);
      }
  
  if (packet->machine_state == STATE_HOLD){
    if(previous_packet->machine_state!=STATE_HOLD){
        gfx.drawRGBBitmap(areas.feedRate.x(), areas.feedRate.y(), pausebutton, 20, 20);
    }
    feedrate_display.draw(packet->feed_rate,0);
    return;
  }
  
  if(packet->machine_state == STATE_CYCLE){
    //if entering cycle mode, clear and redraw the text
    if(previous_packet->machine_state!=STATE_CYCLE){
      gfx.drawRGBBitmap(areas.feedRate.x(), areas.feedRate.y(), playbutton, 20, 20);
    }
    feedrate_display.draw(packet->feed_rate,0);
    return;
  }

  if((packet->machine_state == STATE_IDLE)){
    //if entering cycle mode, clear and redraw the icon
    if(previous_packet->machine_state!=STATE_IDLE){
      //select the jog icon based on the jog mode.
      switch (current_jogmode.value) {
        case FAST :
            gfx.drawRGBBitmap(areas.feedRate.x(), areas.feedRate.y(), hare, 20, 20);        
          break;
        case SLOW : 
            gfx.drawRGBBitmap(areas.feedRate.x(), areas.feedRate.y(), turtle, 20, 20);        
          break;
        case STEP : 
            gfx.drawRGBBitmap(areas.feedRate.x(), areas.feedRate.y(), onestep, 20, 20);        
          break;
        default :
          gfx.drawRGBBitmap(areas.feedRate.x(), areas.feedRate.y(), error_icon, 20, 20);
        break; 
          }//close jog states      
    }
    feedrate_display.draw(packet->jog_stepsize,0);

    return;
  }  

  if(packet->machine_state == STATE_DISCONNECTED){
    //if entering cycle mode, clear and redraw the text
    if(previous_packet->machine_state!=STATE_DISCONNECTED){
      gfx.drawRGBBitmap(areas.feedRate.x()-3, areas.feedRate.y(), disconnected, 20, 20);
    }
    feedrate_display.draw(packet->feed_rate,0);
    return;
  }

}

static void draw_machine_status(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){

  //update the section on state changes
  if(previous_packet->machine_state != packet->machine_state){
    gfx.fillRect(areas.machineStatus.x(), areas.machineStatus.y(), areas.machineStatus.w(), areas.machineStatus.h(), BLACK );
    gfx.setTextColor(WHITE);  
    gfx.setTextSize(1); 
    gfx.setCursor(areas.machineStatus.x(), areas.machineStatus.y()+5);

    gfx.setTextColor(WHITE);      
    switch (packet->machine_state){
      case STATE_IDLE :
      gfx.println("IDLE");
      break;
      case STATE_JOG :
      gfx.println("JOGGING");
      break;
      case STATE_TOOL_CHANGE :
      gfx.println("TOOL CHANGE");
      break;
      case STATE_CYCLE :
      gfx.println("RUN CYCLE");
      break;      
      default:
      gfx.println("Disconnected");
      break;               
    }
  }
}

static void draw_jog_mode(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){

  //This presents some kind of indication when the jog mode is between linear and rotational mode.  Future feature

}

static void draw_alt_screen(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){
 gfx.setCursor(0, 0);
 gfx.setTextColor(WHITE);  
 gfx.setTextSize(1);
 gfx.println("Alt Screen");

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

static void draw_homing_screen(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){
 gfx.setCursor(0, 0);
 gfx.setTextColor(WHITE);  
 gfx.setTextSize(1);
 gfx.println("Homing");
}

static void draw_alarm_screen(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){
 gfx.setCursor(0, 0);
 gfx.setTextColor(WHITE);  
 gfx.setTextSize(1);
 gfx.println("Alarm Screen");
}

static void draw_rpm(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){
      //update the section on state changes
  
  if(previous_packet->machine_state!=packet->machine_state){      
    //clear the feedrate section and write text and set up the number
    gfx.fillRect(areas.spindleRPM.x(), areas.spindleRPM.y(), areas.spindleRPM.w(), areas.spindleRPM.h(), BLACK );
    rpm_display.begin(&FreeMono9pt7b);
    rpm_display.setFormat(3,0);
    gfx.setFont(&Arimo_Regular_12);
    //if(packet->spindle_rpm <= 9999)
    //  rpm_display.setPosition(128-(rpm_display.w()+10), areas.spindleRPM.y()+1);
    //else
      rpm_display.setPosition(128-(rpm_display.w()), areas.spindleRPM.y()+1);
    switch (packet->machine_modes.mode){
      case 0:
        gfx.drawRGBBitmap(areas.spindleRPM.x(), areas.spindleRPM.y()-3, rpm_icon, 20, 20);
      break;
      case 1:
        gfx.drawRGBBitmap(areas.spindleRPM.x(), areas.spindleRPM.y(), power_icon, 20, 20);
      break;
      default:
        gfx.drawRGBBitmap(areas.spindleRPM.x(), areas.spindleRPM.y(), error_icon, 20, 20);
      break;
    }
  }
  rpm_display.draw(packet->spindle_rpm,1);
}

static void draw_overrides(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){

  //update the section on state changes
  if(previous_packet->machine_state!=packet->machine_state){      
    //clear the override section and write text and set up the numbers
    gfx.fillRect(areas.feedOverride.x(), areas.feedOverride.y(), areas.feedOverride.w(), areas.feedOverride.h(), BLACK );
    feed_over_display.begin(&FreeMono9pt7b);
    feed_over_display.setFormat(1,0);
    //feedrate_display.setPosition(areas.feedRate.x()+20,areas.feedRate.y());
    feed_over_display.setPosition(areas.feedOverride.x()+27, areas.feedOverride.y()+3);
    gfx.setFont(&Arimo_Regular_12);
    gfx.drawRGBBitmap(areas.feedOverride.x(), areas.feedOverride.y(), runperson, 20, 20);

      

    //clear the override section and write text and set up the numbers
    gfx.fillRect(areas.spindleOverride.x(), areas.spindleOverride.y(), areas.spindleOverride.w(), areas.spindleOverride.h(), BLACK );
    spindle_over_display.begin(&FreeMono9pt7b);
    spindle_over_display.setFormat(1,0);
    //feedrate_display.setPosition(areas.feedRate.x()+20,areas.feedRate.y());
    spindle_over_display.setPosition(areas.spindleOverride.x()+25, areas.spindleOverride.y()+3);
    gfx.setFont(&Arimo_Regular_12);    
    switch (packet->machine_modes.mode){
      case 0:
        gfx.drawRGBBitmap(areas.spindleOverride.x(), areas.spindleOverride.y(), driller, 20, 20);
      break;
      case 1:
        gfx.drawRGBBitmap(areas.spindleOverride.x(), areas.spindleOverride.y(), laser, 20, 20);
      break;
      default:
        gfx.drawRGBBitmap(areas.spindleOverride.x(), areas.spindleOverride.y(), error_icon, 20, 20);
      break;
    }
  }

  feed_over_display.draw(packet->feed_override,0);
  spindle_over_display.draw(packet->spindle_override,0);

}

void draw_main_screen(machine_status_packet_t *previous_packet, machine_status_packet_t *packet, bool force){ 
#if 1

  unsigned long now = to_ms_since_boot(get_absolute_time());

  int i = 0;
  int j = 0;
  char charbuf[32];

  int x, y;
 
  switch (screenmode){
  case JOG_MODIFY:
  //put hints about alternate button functions here. 
  if(screenmode != JOG_MODIFY){
    gfx.fillScreen(0);
    draw_alt_screen(previous_packet, packet);
  }
  break;

  //just small edits for most screens

  case ALARM:
  if(screenmode != previous_screenmode){
    gfx.fillScreen(0);
    draw_alarm_screen(previous_packet, packet);
  }
  break;

  case HOMING:
  if(screenmode != previous_screenmode){
    gfx.fillScreen(0);
    draw_homing_screen(previous_packet, packet);
  }
  break;

  //most of the time use the DRO screen
  case DISCONNECTED:
  case JOGGING:
  case RUN:
  case HOLD:
  case TOOL_CHANGE:
  default:  
    if(previous_screenmode != screenmode){
      gfx.fillScreen(0);
    }

    //if(packet->machine_state != previous_packet->machine_state)
    //  oledFill(&oled, 0,1);//clear screen on state change

    switch (packet->machine_state){

      case STATE_CYCLE :
        //can still adjust overrides during hold
        //no jog during hold, show feed rate.
        draw_feedrate(previous_packet, packet);
        draw_machine_status(previous_packet, packet);
        draw_dro_readout(previous_packet, packet);
        draw_rpm(previous_packet, packet);   
      break; //close cycle case        

      case STATE_HOLD :
        draw_feedrate(previous_packet, packet);
        draw_machine_status(previous_packet, packet);
        draw_dro_readout(previous_packet, packet);
        draw_rpm(previous_packet, packet);                 
      break; //close hold case

      case STATE_TOOL_CHANGE :
        //dream feature is to put tool info/comment/number on screen during tool change.
        //cannot adjust overrides during tool change
        //jogging allowed during tool change
        draw_feedrate(previous_packet, packet);
        draw_machine_status(previous_packet, packet);
        draw_dro_readout(previous_packet, packet);
        draw_rpm(previous_packet, packet);      
      break; //close tool case

      case STATE_JOG : //jogging is allowed       
      case STATE_IDLE : //jogging is allowed
      default :
        draw_feedrate(previous_packet, packet);
        draw_machine_status(previous_packet, packet);
        draw_jog_mode(previous_packet, packet);
        draw_dro_readout(previous_packet, packet);
        draw_rpm(previous_packet, packet);
        draw_overrides(previous_packet, packet);                    
      break; //close default case
    }//close machine_state switch statement
  }//close screen mode switch statement
#endif  
  //prev_packet = *packet;
  previous_jogmode = current_jogmode;
  previous_screenmode = screenmode;
}//close draw main screen