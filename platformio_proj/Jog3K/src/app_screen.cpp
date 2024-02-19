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
#include <tusb.h>
#include <Adafruit_NeoPixel.h>

#include "i2c_jogger.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>

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

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

const float gxLow  = 0.0;
const float gxHigh = 100.0;
const float gyLow  = -512.0;
const float gyHigh = 512.0;

#define LOOP_PERIOD 35 // Display updates every 35 ms

//#define SHOWJOG 1
//#define SHOWOVER 1
#define SHOWRAM 1
#define TWOWAY 0

#define SCREEN_ENABLED 0

#define OLED_SCREEN_FLIP 1

#define JOGLINE 7
#define JOGFONT FONT_8x8
#define TOPLINE 2
#define INFOLINE 0
#define BOTTOMLINE 7
#define INFOFONT FONT_8x8

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

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint8_t jog_color[] = {0,255,0};
uint8_t halt_color[] = {0,255,0};
uint8_t hold_color[] = {0,255,0};
uint8_t run_color[] = {0,255,0};
int32_t feed_color[] = {0,10000,0};
int32_t rpm_color[] = {0,10000,0};

// define I2C addresses to be used for this peripheral
static const uint I2C_SLAVE_ADDRESS = 0x49;
static const uint I2C_BAUDRATE = 1000000; // 100 kHz

// RPI Pico

char buf[8];

const uint8_t * flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

// Option 1: use any pins but a little slower
//Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);  
  // Option 2: must use the hardware SPI pins 
  // (for UNO thats sclk = 13 and sid = 11) and pin 10 must be 
  // an output. This is much faster - also required if you want
  // to use the microSD card (see the image drawing example)

Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

float p = 3.1415926;

void testlines(uint16_t color) {
   tft.fillScreen(BLACK);
   for (uint16_t x=0; x < tft.width()-1; x+=6) {
     tft.drawLine(0, 0, x, tft.height()-1, color);
   }
   for (uint16_t y=0; y < tft.height()-1; y+=6) {
     tft.drawLine(0, 0, tft.width()-1, y, color);
   }
   
   tft.fillScreen(BLACK);
   for (uint16_t x=0; x < tft.width()-1; x+=6) {
     tft.drawLine(tft.width()-1, 0, x, tft.height()-1, color);
   }
   for (uint16_t y=0; y < tft.height()-1; y+=6) {
     tft.drawLine(tft.width()-1, 0, 0, y, color);
   }
   
   tft.fillScreen(BLACK);
   for (uint16_t x=0; x < tft.width()-1; x+=6) {
     tft.drawLine(0, tft.height()-1, x, 0, color);
   }
   for (uint16_t y=0; y < tft.height()-1; y+=6) {
     tft.drawLine(0, tft.height()-1, tft.width()-1, y, color);
   }

   tft.fillScreen(BLACK);
   for (uint16_t x=0; x < tft.width()-1; x+=6) {
     tft.drawLine(tft.width()-1, tft.height()-1, x, 0, color);
   }
   for (uint16_t y=0; y < tft.height()-1; y+=6) {
     tft.drawLine(tft.width()-1, tft.height()-1, 0, y, color);
   }
   
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0,0);
  tft.setTextColor(color);
  tft.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
   tft.fillScreen(BLACK);
   for (uint16_t y=0; y < tft.height()-1; y+=5) {
     tft.drawFastHLine(0, y, tft.width()-1, color1);
   }
   for (uint16_t x=0; x < tft.width()-1; x+=5) {
     tft.drawFastVLine(x, 0, tft.height()-1, color2);
   }
}

void testdrawrects(uint16_t color) {
 tft.fillScreen(BLACK);
 for (uint16_t x=0; x < tft.height()-1; x+=6) {
   tft.drawRect((tft.width()-1)/2 -x/2, (tft.height()-1)/2 -x/2 , x, x, color);
 }
}

void testfillrects(uint16_t color1, uint16_t color2) {
 tft.fillScreen(BLACK);
 for (uint16_t x=tft.height()-1; x > 6; x-=6) {
   tft.fillRect((tft.width()-1)/2 -x/2, (tft.height()-1)/2 -x/2 , x, x, color1);
   tft.drawRect((tft.width()-1)/2 -x/2, (tft.height()-1)/2 -x/2 , x, x, color2);
 }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (uint8_t x=radius; x < tft.width()-1; x+=radius*2) {
    for (uint8_t y=radius; y < tft.height()-1; y+=radius*2) {
      tft.fillCircle(x, y, radius, color);
    }
  }  
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (uint8_t x=0; x < tft.width()-1+radius; x+=radius*2) {
    for (uint8_t y=0; y < tft.height()-1+radius; y+=radius*2) {
      tft.drawCircle(x, y, radius, color);
    }
  }  
}

void testtriangles() {
  tft.fillScreen(BLACK);
  int color = 0xF800;
  int t;
  int w = tft.width()/2;
  int x = tft.height();
  int y = 0;
  int z = tft.width();
  for(t = 0 ; t <= 15; t+=1) {
    tft.drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  tft.fillScreen(BLACK);
  int color = 100;
  
  int x = 0;
  int y = 0;
  int w = tft.width();
  int h = tft.height();
  for(int i = 0 ; i <= 24; i++) {
    tft.drawRoundRect(x, y, w, h, 5, color);
    x+=2;
    y+=3;
    w-=4;
    h-=6;
    color+=1100;
  }
}

void tftPrintTest() {
  tft.fillScreen(BLACK);
  tft.setCursor(0, 5);
  tft.setTextColor(RED);  
  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.println("Hello World!");
  tft.setTextColor(BLUE);
  tft.setTextSize(3);
  tft.print(1234.567);
  delay(1500);
  tft.setCursor(0, 5);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(0);
  tft.println("Hello World!");
  tft.setTextSize(1);
  tft.setTextColor(GREEN);
  tft.print(p, 6);
  tft.println(" Want pi?");
  tft.println(" ");
  tft.print(8675309, HEX); // print 8,675,309 out in HEX!
  tft.println(" Print HEX!");
  tft.println(" ");
  tft.setTextColor(WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(MAGENTA);
  tft.print(millis() / 1000);
  tft.setTextColor(WHITE);
  tft.print(" seconds.");
}

void mediabuttons() {
 // play
  tft.fillScreen(BLACK);
  tft.fillRoundRect(25, 10, 78, 60, 8, WHITE);
  tft.fillTriangle(42, 20, 42, 60, 90, 40, RED);
  delay(500);
  // pause
  tft.fillRoundRect(25, 90, 78, 60, 8, WHITE);
  tft.fillRoundRect(39, 98, 20, 45, 5, GREEN);
  tft.fillRoundRect(69, 98, 20, 45, 5, GREEN);
  delay(500);
  // play color
  tft.fillTriangle(42, 20, 42, 60, 90, 40, BLUE);
  delay(50);
  // pause color
  tft.fillRoundRect(39, 98, 20, 45, 5, RED);
  tft.fillRoundRect(69, 98, 20, 45, 5, RED);
  // play color
  tft.fillTriangle(42, 20, 42, 60, 90, 40, GREEN);
}

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
    tft.fillRect(0, tft.height() * c / 8, tft.width(), tft.height() / 8,
      pgm_read_word(&colors[c]));
  }
}

void init_multimedia (void){

  if (*flash_target_contents != 0xff)
    screenflip = *flash_target_contents;

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(NEO_BRIGHTNESS);
  
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();   // Send the updated pixel colors to the hardware.
  sleep_ms(50);

  int i;
  for (i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i,pixels.Color(255, 0, 0));
    pixels.show();   // Send the updated pixel colors to the hardware.
    sleep_ms(50);
  }

  sleep_ms(50);


   for (i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i,pixels.Color(0, 255, 0));
    pixels.show();   // Send the updated pixel colors to the hardware.
  }
  sleep_ms(50);
  pixels.show();   // Send the updated pixel colors to the hardware.
  sleep_ms(50);

  /*OLED code*/
  // Option 2: must use the hardware SPI pins 
  // (for UNO thats sclk = 13 and sid = 11) and pin 10 must be 
  // an output. This is much faster - also required if you want
  // to use the microSD card (see the image drawing example)
  SPI.setCS(SPI_CSn_PIN);
  SPI.setTX(SPI_TX_PIN);
  SPI.setSCK(SPI_CLK_PIN);
  // Init Display
    

    tft.begin();

    // You can optionally rotate the display by running the line below.
    // Note that a value of 0 means no rotation, 1 means 90 clockwise,
    // 2 means 180 degrees clockwise, and 3 means 270 degrees clockwise.
    //tft.setRotation(1);
    // NOTE: The test pattern at the start will NOT be rotated!  The code
    // for rendering the test pattern talks directly to the display and
    // ignores any rotation.

    uint16_t time = millis();
    tft.fillRect(0, 0, 128, 128, BLACK);
    time = millis() - time;
            
    lcdTestPattern();
    delay(100);
    
    tft.invert(true);
    delay(100);
    tft.invert(false);
    delay(100);

    tft.fillScreen(BLACK);
    testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", WHITE);
    delay(100);

    // tft print function!
    tftPrintTest();
    delay(100);
    
    //a single pixel
    tft.drawPixel(tft.width()/2, tft.height()/2, GREEN);
    delay(100);

    // line draw test
    testlines(YELLOW);
    delay(100);    
  
    // optimized lines
    testfastlines(RED, BLUE);
    delay(100);    


    testdrawrects(GREEN);
    delay(100);

    testfillrects(YELLOW, MAGENTA);
    delay(100);

    tft.fillScreen(BLACK);
    testfillcircles(10, BLUE);
    testdrawcircles(10, WHITE);
    delay(100);
    
    testroundrects();
    delay(100);
    
    testtriangles();
    delay(100);

    

}

void update_neopixels(Machine_status_packet *previous_packet, Machine_status_packet *packet){

  //set override LEDS
  if (packet->feed_override > 100)
    feed_color[0] = ((packet->feed_override - 100) * 255)/100;
  else
    feed_color[0] = 0;
  if (packet->feed_override > 100)
    feed_color[1] = 255 - (((packet->feed_override - 100) * 255)/100);
  else
    feed_color[1] = 255- (((100-packet->feed_override) * 255)/100);
  if(packet->feed_override < 100)
    feed_color[2] = ((100 - packet->feed_override) * 255)/100;
  else
    feed_color[2] = 0;

  if (packet->spindle_override > 100)
    rpm_color[0] = ((packet->spindle_override - 100) * 255)/100;
  else
    rpm_color[0] = 0;
  if (packet->spindle_override > 100)
    rpm_color[1] = 255 - (((packet->spindle_override - 100) * 255)/100);
  else
    rpm_color[1] = 255- (((100-packet->spindle_override) * 255)/100);
  if(packet->spindle_override < 100)
    rpm_color[2] = ((100 - packet->spindle_override) * 255)/100;
  else
    rpm_color[2] = 0;    

  pixels.setPixelColor(FEEDLED,pixels.Color((uint8_t) feed_color[0], (uint8_t) feed_color[1], (uint8_t) feed_color[2]));
  pixels.setPixelColor(SPINLED,pixels.Color(rpm_color[0], rpm_color[1], rpm_color[2]));

  //set home LED
  if(packet->home_state)
    pixels.setPixelColor(HOMELED,pixels.Color(0, 255, 0));
  else
    pixels.setPixelColor(HOMELED,pixels.Color(200, 135, 0));

  //set spindleoff LED
  if(packet->spindle_rpm > 0)
    pixels.setPixelColor(SPINDLELED,pixels.Color(255, 75, 0));
  else
    pixels.setPixelColor(SPINDLELED,pixels.Color(75, 255, 130));

  //set Coolant LED
  if(packet->coolant_state.value)
    pixels.setPixelColor(COOLED,pixels.Color(0, 100, 255));
  else
    pixels.setPixelColor(COOLED,pixels.Color(0, 0, 100));  

  //preload jog LED colors depending on speed
  switch (current_jogmode) {
  case FAST :
    jog_color[0] = 255; jog_color[1] = 0; jog_color[2] = 0; //RGB
    break;
  case SLOW : 
    jog_color[0] = 0; jog_color[1] = 255; jog_color[2] = 0; //RGB      
    break;
  case STEP : 
    jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 255; //RGB      
    break;
  default :
    //jog_color[0] = 255; jog_color[1] = 255; jog_color[2] = 255; //RGB
  break;      
  }//close jogmode

  switch (packet->machine_state){
    case STATE_IDLE :
    //no change to jog colors
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB   
    break; //close idle case

    case STATE_HOLD :
    //no jog during hold to jog colors
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB
      jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 0; //RGB     
    break; //close idle case

    case STATE_TOOL_CHANGE : 
    //no change to jog colors
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB   
    break;//close tool change case 

    case STATE_JOG :
        //Indicate jog in progress
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB        
      jog_color[0] = 255; jog_color[1] = 150; jog_color[2] = 0; //RGB
    break;//close jog case

    case STATE_HOMING :
        //No jogging during homing
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB        
      jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 0; //RGB    
    break;//close homing case

    case STATE_CYCLE :
      //No jogging during job
      run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB        
      jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 0; //RGB     

    break;//close cycle state

    case STATE_ALARM :
          //handle alarm state at bottom
      jog_color[0] = 0; jog_color[1] = 0; jog_color[2] = 0; //RGB
      run_color[0] = 255; run_color[1] = 0; run_color[2] = 0; //RGB
      hold_color[0] = 255; hold_color[1] = 0; hold_color[2] = 0; //RGB   
      halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB 

      //also override above colors for maximum alarm 
      pixels.setPixelColor(SPINDLELED,pixels.Color(255, 0, 0));
      pixels.setPixelColor(COOLED,pixels.Color(255, 0, 0));
      pixels.setPixelColor(HOMELED,pixels.Color(255, 0, 0));
      pixels.setPixelColor(SPINLED,pixels.Color(255, 0, 0));
      pixels.setPixelColor(FEEDLED,pixels.Color(255, 0, 0));
    break;//close alarm state

    default :  //this is active when there is a non-interactive controller
      //run_color[0] = 0; run_color[1] = 255; run_color[2] = 0; //RGB
      //hold_color[0] = 255; hold_color[1] = 150; hold_color[2] = 0; //RGB
      //halt_color[0] = 255; halt_color[1] = 0; halt_color[2] = 0; //RGB   

    break;//close alarm state
  }//close machine_state switch statement

  if(screenmode == JOG_MODIFY){
    //some overrides for alternate functions
    //violet = (138, 43, 226)
    //jog_color[0] = 138; jog_color[1] = 43; jog_color[2] = 226;
    //run_color[0] = 138; run_color[1] = 43; run_color[2] = 226; //RGB
    //hold_color[0] = 138; hold_color[1] = 43; hold_color[2] = 226; //RGB   
    //halt_color[0] = 0; halt_color[1] = 0; halt_color[2] = 0; //RGB 
    pixels.setPixelColor(COOLED,pixels.Color(138, 43, 226));
    pixels.setPixelColor(HOMELED,pixels.Color(138, 43, 226));
    //pixels.setPixelColor(FEEDLED,pixels.Color(0, 0, 0));
    //pixels.setPixelColor(SPINLED,pixels.Color(0, 0, 0));
  }

  //set jog LED values
  if(screenmode == JOG_MODIFY)
    pixels.setPixelColor(JOGLED,pixels.Color(138, 43, 226));
  else
    pixels.setPixelColor(JOGLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));

  if(packet->a_coordinate==0xFFFFFFFF && screenmode == JOG_MODIFY)
    pixels.setPixelColor(RAISELED,pixels.Color(138, 43, 226));
  else
    pixels.setPixelColor(RAISELED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(HALTLED,pixels.Color(halt_color[0], halt_color[1], halt_color[2]));
  pixels.setPixelColor(HOLDLED,pixels.Color(hold_color[0], hold_color[1], hold_color[2]));
  pixels.setPixelColor(RUNLED,pixels.Color(run_color[0], run_color[1], run_color[2]));

  pixels.show();
}

void activate_jogled(void){
  jog_color[0] = 255; jog_color[1] = 150; jog_color[2] = 0; //RGB
  pixels.setPixelColor(JOGLED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.setPixelColor(RAISELED,pixels.Color(jog_color[0], jog_color[1], jog_color[2]));
  pixels.show();
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

void draw_string(char * str){

#if SCREEN_ENABLED
oledFill(&oled, 0,1);
oledWriteString(&oled, 0,0,2,str, FONT_12x16, 0, 1);
sleep_ms(100);
oledFill(&oled, 0,1);
#else
sleep_ms(100);
#endif

}

static void draw_dro_readout(Machine_status_packet *previous_packet, Machine_status_packet *packet){

  #if SCREEN_ENABLED
  char charbuf[32];

  if (packet->current_wcs != previous_packet->current_wcs || screenmode != previous_screenmode){
    oledWriteString(&oled, 0,0,2,(char *)"                G", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0,-1,-1,map_coord_system(packet->current_wcs), FONT_6x8, 0, 1);
    oledWriteString(&oled, 0,-1,-1,(char *)"  ", FONT_6x8, 0, 1);
  }

  if(packet->x_coordinate != previous_packet->x_coordinate || 
      packet->y_coordinate != previous_packet->y_coordinate || 
      packet->z_coordinate != previous_packet->z_coordinate || 
      packet->a_coordinate != previous_packet->a_coordinate ||
      screenmode != previous_screenmode){
    sprintf(charbuf, "X %8.3F", packet->x_coordinate);
    oledWriteString(&oled, 0,0,2,charbuf, FONT_8x8, 0, 1);

      sprintf(charbuf, "Y %8.3F", packet->y_coordinate);
      oledWriteString(&oled, 0,0,3,charbuf, FONT_8x8, 0, 1);

      sprintf(charbuf, "Z %8.3F", packet->z_coordinate);
      oledWriteString(&oled, 0,0,4,charbuf, FONT_8x8, 0, 1);

    if(packet->a_coordinate < 65535){          
      sprintf(charbuf, "A %8.3F", packet->a_coordinate);
      oledWriteString(&oled, 0,0,5,charbuf, FONT_8x8, 0, 1);         
    }else{
      sprintf(charbuf, "           ", packet->a_coordinate);
      oledWriteString(&oled, 0,0,5,charbuf, FONT_8x8, 0, 1);            
    }
  }

  #endif
}

static void draw_feedrate(Machine_status_packet *previous_packet, Machine_status_packet *packet){

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
#if SCREEN_ENABLED
  int i = 0;
  int j = 0;
  char charbuf[32];

  int x, y;
  
  switch (screenmode){
  case JOG_MODIFY:
  //put hints about alternate button functions here. 
  if(screenmode != previous_screenmode){
    oledFill(&oled, 0,1);  //only clear screen if the mode changes.
    draw_alt_screen(previous_packet, packet);
  }
  break;
  default:  
    if(previous_screenmode == JOG_MODIFY){
      oledFill(&oled, 0,1);  //only clear screen if necessary.
    }

    if((previous_screenmode == ALARM || previous_screenmode == HOMING) &&
        (screenmode == DEFAULT ||
        screenmode == JOGGING ||
        screenmode == TOOL_CHANGE ||
        screenmode == RUN)){
      oledFill(&oled, 0,1);  //only clear screen if necessary.
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
          oledWriteString(&oled, 0,0,0,(char *)" *****************", FONT_6x8, 0, 1);
          oledWriteString(&oled, 0,0,7,(char *)" *****************", FONT_6x8, 0, 1);
          //no jog during hold
          oledWriteString(&oled, 0,0,4,(char *)"HOMING", JOGFONT, 0, 1);
        break; //close home case

        case STATE_ALARM : //no overrides during homing
          //oledFill(&oled, 0,1);
          oledWriteString(&oled, 0,0,0,(char *)" *****************", FONT_6x8, 0, 1);
          oledWriteString(&oled, 0,0,7,(char *)" *****************", FONT_6x8, 0, 1);
          //no jog during hold
          oledWriteString(&oled, 0,0,3,(char *)"ALARM", JOGFONT, 0, 1);
          sprintf(charbuf, "Code: %d ", packet->alarm);
          oledWriteString(&oled, 0,0,4,charbuf, INFOFONT, 0, 1);        
        break; //close home case                               
        default :
          oledWriteString(&oled, 0,0,0,(char *)" *****************", FONT_6x8, 0, 1);
          oledWriteString(&oled, 0,0,7,(char *)" Disconnected", FONT_6x8, 0, 1);
          //no jog during hold
          oledWriteString(&oled, 0,0,3,(char *)"STATE", JOGFONT, 0, 1);
          sprintf(charbuf, "Code: %d ", packet->machine_state);
          oledWriteString(&oled, 0,0,4,charbuf, INFOFONT, 0, 1);            
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
