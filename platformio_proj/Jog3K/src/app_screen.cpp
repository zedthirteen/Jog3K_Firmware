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
const uint8_t axesAreaHeight = 64; //All axis
const uint8_t axesLabelWidth = 20; //All axis
const uint8_t axesCoordWidth = 20;
const uint8_t axesCoordHeight = 10;
const uint8_t feedRateHeight = 20;
const uint8_t feedRateWidth = 64;
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
DisplayNumber spindlee_over_display = DisplayNumber(gfx);


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
  areas.spindleRPM = DisplayArea(0,feedRateWidth,feedRateWidth,feedRateHeight);// RPM gets other half of width
  areas.axes = DisplayArea(0, feedRateHeight, displayWidth, axesAreaHeight);  //DRO under feed.
  //areas.axesMarkers = DisplayArea(0, 0, 19, axesAreaHeight);
  areas.axesLabels = DisplayArea(0, feedRateHeight, axesLabelWidth, axesAreaHeight); //Axes labels down the side
  areas.axesCoords = DisplayArea(0, axesAreaHeight+feedRateHeight, axesCoordWidth, axesCoordHeight); //Current coordinate system under axes
  areas.infoMessage = DisplayArea(0,120,displayWidth,displayHeight-120); //info message along the bottom
  areas.machineStatus = DisplayArea(axesCoordWidth, axesAreaHeight + feedRateHeight, displayWidth - axesCoordWidth, axesCoordHeight); //status beside coordinates
  areas.feedOverride= DisplayArea(0,axesAreaHeight+axesCoordHeight+20,64,feedRateHeight);// Feed over gets half of width
  areas.spindleOverride= DisplayArea(64,axesAreaHeight+axesCoordHeight+20,64,feedRateHeight);// spindle over gets other half of width
  //areas.encoderLabel[0] = DisplayArea(0, axesAreaHeight, encoderColumnWidth, encoderLabelAreaHeight);
  //areas.encoderLabel[1] = DisplayArea(encoderColumnWidth + 1, axesAreaHeight, encoderColumnWidth, encoderLabelAreaHeight);
  //areas.encoderLabel[2] = DisplayArea((encoderColumnWidth * 2) + 2, axesAreaHeight, encoderColumnWidth, encoderLabelAreaHeight);
  //areas.encoderValue[0] = DisplayArea(0, axesAreaHeight + encoderLabelAreaHeight, encoderColumnWidth, encoderValueAreaHeight[0] );
  //areas.encoderValue[1] = DisplayArea(encoderColumnWidth + 1, axesAreaHeight + encoderLabelAreaHeight, encoderColumnWidth, encoderValueAreaHeight[0] );
  //areas.encoderValue[2] = DisplayArea((encoderColumnWidth * 2) + 2, axesAreaHeight + encoderLabelAreaHeight, encoderColumnWidth, encoderValueAreaHeight[0] );

  //areas.buttonLabels[0] = DisplayArea( 0, areas.encoderValue[0].b() + 1, buttonColumnWidth, displayHeight - areas.encoderValue[0].b() - 1);
  //areas.buttonLabels[1] = DisplayArea( buttonColumnWidth + 1, areas.encoderValue[0].b() + 1, buttonColumnWidth, displayHeight - areas.encoderValue[0].b() - 1);
  //areas.buttonLabels[2] = DisplayArea( (buttonColumnWidth * 2) + 2, areas.encoderValue[0].b() + 1, buttonColumnWidth, displayHeight - areas.encoderValue[0].b() - 1);
  //areas.buttonLabels[3] = DisplayArea( (buttonColumnWidth * 3) + 3, areas.encoderValue[0].b() + 1, buttonColumnWidth, displayHeight - areas.encoderValue[0].b() - 1);
  //areas.buttonLabels[4] = DisplayArea( (buttonColumnWidth * 4) + 4, areas.encoderValue[0].b() + 1, buttonColumnWidth + 1, displayHeight - areas.encoderValue[0].b() - 1);

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

static void drawIconPlay(Coords_s cp, uint16_t c/*=WHITE*/, uint8_t h/*=26*/) {
    uint8_t w = h / 1.5;
    gfx.fillTriangle(
      cp.x-(w/2)+3, cp.y-(0),
      cp.x+(w/2)+3, cp.y+(h/2),
      cp.x-(w/2)+3, cp.y+(h),
      c
    );
}

static void drawPulse(Coords_s cp, uint8_t r, int c/*=WHITE*/) {
  gfx.fillCircle(cp.x+r, cp.y+r, r, c);
  gfx.fillCircle(cp.x+(r*3), cp.y+r, r, c);
  gfx.fillTriangle(cp.x, cp.y+r+1,
                   cp.x+(r*4), cp.y+r+1, 
                   cp.x+(r*2), cp.y+(r*3.5), 
                   c);
}

static void drawIconOneStep(Coords_s cp, uint16_t c/*=WHITE*/) {
    uint16_t x = cp.x;
    uint8_t y = cp.y;
    uint8_t s = 4; //step size
    gfx.setFont(NULL);
    gfx.setTextColor(c);
    gfx.setCursor(x - 1, cp.y+3);
    gfx.print("1");
    for ( uint8_t i = 0; i < 2; (i++) ) {
      gfx.drawLine(x + (s * i), y + (s * i), x + (s * i) + s, y + (s * i), c);
      gfx.drawLine(x + (s * i) + s, y + (s * i), x + (s * i) + s, y + (s * i) + s, c);
    }
    gfx.setTextColor(WHITE);
    gfx.setFont(NULL);
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

  if (packet->machine_state == STATE_HOLD){
    
    if(previous_packet->machine_state!=STATE_HOLD){
      //clear the feedrate section and write HOLDING text
      gfx.fillRect(areas.feedRate.x(), areas.feedRate.y(), areas.feedRate.w(), areas.feedRate.h(), BLACK );
      gfx.setFont(&Arimo_Regular_12);
      gfx.setCursor(areas.feedRate.x(), areas.feedRate.y()+areas.feedRate.h());
      //gfx.setCursor(areas.axesCoords.x(), areas.axesCoords.y());

      gfx.print("HOLDING");
    }
    return;
  }
  
  if(packet->machine_state == STATE_CYCLE){
    //if entering cycle mode, clear and redraw the text
    if(previous_packet->machine_state!=STATE_CYCLE){
      //clear the feedrate section and write text and set up the number
      gfx.fillRect(areas.feedRate.x(), areas.feedRate.y(), areas.feedRate.w(), areas.feedRate.h(), BLACK );
      feedrate_display.setFont(&Arimo_Regular_12);
      feedrate_display.setFormat(4,0);
      //feedrate_display.setPosition(areas.feedRate.x()+20,areas.feedRate.y());
      feedrate_display.setPosition(gfx.width()-feedrate_display.w(), areas.feedRate.y()+1);
      //feedrate_display.begin();  
      gfx.setFont(&Arimo_Regular_12);
      gfx.setCursor(areas.feedRate.x(), areas.feedRate.y()+areas.feedRate.h());
      //gfx.setCursor(areas.axesCoords.x(), areas.axesCoords.y());
      gfx.print("RUN");
    }
    feedrate_display.draw(packet->feed_rate,0);
    return;
  }


#if 0
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

  if(packet->machine_state == STATE_DISCONNECTED){
    //if entering cycle mode, clear and redraw the text
    if(previous_packet->machine_state!=STATE_DISCONNECTED){
      //clear the feedrate section and write text and set up the number
      gfx.fillRect(areas.feedRate.x(), areas.feedRate.y(), areas.feedRate.w(), areas.feedRate.h(), BLACK );
      feedrate_display.begin(&FreeMono9pt7b);
      feedrate_display.setFormat(3,0);
      //feedrate_display.setPosition(areas.feedRate.x()+20,areas.feedRate.y());
      feedrate_display.setPosition(feedRateWidth-(feedrate_display.w()), areas.feedRate.y()+1);
      gfx.setFont(&Arimo_Regular_12);
      //gfx.setCursor(areas.feedRate.x(), areas.feedRate.y()+areas.feedRate.h());
      gfx.setCursor(areas.feedRate.x(), areas.feedRate.y()+20);
      Coords_s icon = {areas.feedRate.x(), areas.feedRate.y()};
      //gfx.print("FD");
      gfx.drawRGBBitmap(areas.feedRate.x(), areas.feedRate.y(), disconnected, 20, 20);
      gfx.drawRGBBitmap(20, 20, runperson, 20, 20);
      gfx.drawRGBBitmap(40, 20, turtle, 20, 20);
      gfx.drawRGBBitmap(60, 20, onestep, 20, 20);
      gfx.drawRGBBitmap(80, 20, hare, 20, 20);
      gfx.drawRGBBitmap(100, 20, rpm_icon, 20, 20);
      gfx.drawRGBBitmap(0, 40, playbutton, 20, 20);
      gfx.drawRGBBitmap(20, 40, pausebutton, 20, 20);
      gfx.drawRGBBitmap(40, 40, stopbutton, 20, 20);
      gfx.drawRGBBitmap(60, 40, driller, 20, 20);
      gfx.drawRGBBitmap(80, 40, laser, 20, 20);
      gfx.drawRGBBitmap(100, 40, power_icon, 20, 20);
    }
    feedrate_display.draw(packet->feed_rate,0);
    return;
  }

}

static void draw_machine_status(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){

 gfx.setCursor(0, 100);
 gfx.setTextColor(WHITE);  
 gfx.setTextSize(1);
 gfx.println("Disconnected");



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

static void draw_overrides_rpm(machine_status_packet_t *previous_packet, machine_status_packet_t *packet){
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

      default :
        draw_feedrate(previous_packet, packet);
        draw_machine_status(previous_packet, packet);
        //draw_dro_readout(previous_packet, packet);
        draw_overrides_rpm(previous_packet, packet);                     
      break; //close default case
    }//close machine_state switch statement
  }//close screen mode switch statement
#endif  
  //prev_packet = *packet;
  previous_jogmode = current_jogmode;
  previous_screenmode = screenmode;
}//close draw main screen