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
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"
#include "ss_oled.h"

#include "bsp/board.h"

#include "pico/stdio.h"
#include "pico/time.h"
#include <tusb.h>
#include "Adafruit_NeoPixel.hpp"

#include "i2c_jogger.h"

//#define SHOWJOG 1
//#define SHOWOVER 1
#define SHOWRAM 1
#define TWOWAY 1

// RPI Pico

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int led_update_counter = 0;
int update_neopixel_leds = 0;
int screen_update_counter = 0;
int status_update_counter = 0;
int onboard_led_count = 0;
int jogmode = 0;
int kpstr_counter = 0;

// ram_addr is the current address to be used when writing / reading the RAM
// N.B. the address auto increments, as stored in 8 bit value it automatically rolls round when reaches 255

static struct
{
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
} context;

char charbuf[32];

uint8_t ram_addr = 0;
uint8_t status_addr = 0;
uint8_t key_pressed = 0;
uint8_t key_character = '\0';

/* these are simply used for convenience */
uint8_t halt_pressed = 0;
uint8_t hold_pressed = 0;
uint8_t cycle_start_pressed = 0;
uint8_t alt_hold_pressed = 0;
uint8_t alt_halt_pressed = 0;
uint8_t alt_cycle_start_pressed = 0;

uint8_t feed_over_up_pressed = 0;
uint8_t feed_over_down_pressed = 0;
uint8_t feed_over_reset_pressed = 0;

uint8_t spindle_over_up_pressed = 0;
uint8_t spindle_over_down_pressed = 0;
uint8_t spindle_over_reset_pressed = 0;

uint8_t jog_mod_pressed = 0;
uint8_t jog_mode_pressed = 0;
uint8_t jog_toggle_pressed = 0;

uint8_t mist_pressed = 0;
uint8_t flood_pressed = 0;
uint8_t spindle_pressed = 0;
uint8_t home_pressed = 0;

uint8_t direction_pressed = 0;
uint8_t previous_direction_pressed = 0;
uint8_t keysent = 0;

uint8_t alt_up_pressed = 0;
uint8_t alt_down_pressed = 0;
uint8_t alt_left_pressed = 0;
uint8_t alt_right_pressed = 0;
uint8_t alt_lower_pressed = 0;
uint8_t alt_raise_pressed = 0;
uint8_t alt_spindle_pressed = 0;
uint8_t alt_home_pressed = 0;
uint8_t alt_flood_pressed = 0;
uint8_t alt_mist_pressed = 0;
uint8_t reset_pressed = 0;
uint8_t unlock_pressed = 0;
uint8_t spinon_pressed = 0;

/* these are simply used for convenience */

static void assign_button_values (uint32_t buttons){
  halt_pressed               = (( buttons & (1 << (0) ) ) ? 1 : 0 );
  hold_pressed               = (( buttons & (1 << (1) ) ) ? 1 : 0 );
  cycle_start_pressed        = (( buttons & (1 << (2) ) ) ? 1 : 0 );
  spindle_pressed            = (( buttons & (1 << (3) ) ) ? 1 : 0 );
  mist_pressed               = (( buttons & (1 << (4) ) ) ? 1 : 0 );
  flood_pressed              = (( buttons & (1 << (5) ) ) ? 1 : 0 );
  home_pressed               = (( buttons & (1 << (6) ) ) ? 1 : 0 );
  spindle_over_down_pressed  = (( buttons & (1 << (7) ) ) ? 1 : 0 );
  spindle_over_reset_pressed = (( buttons & (1 << (8) ) ) ? 1 : 0 );
  spindle_over_up_pressed    = (( buttons & (1 << (9) ) ) ? 1 : 0 );
  feed_over_down_pressed     = (( buttons & (1 << (10) ) ) ? 1 : 0 );
  feed_over_reset_pressed    = (( buttons & (1 << (11) ) ) ? 1 : 0 );
  feed_over_up_pressed       = (( buttons & (1 << (12) ) ) ? 1 : 0 );
  alt_halt_pressed           = (( buttons & (1 << (13) ) ) ? 1 : 0 );
  alt_hold_pressed           = (( buttons & (1 << (14) ) ) ? 1 : 0 );
  alt_home_pressed           = (( buttons & (1 << (15) ) ) ? 1 : 0 );
  alt_cycle_start_pressed    = (( buttons & (1 << (16) ) ) ? 1 : 0 );
  alt_spindle_pressed        = (( buttons & (1 << (17) ) ) ? 1 : 0 );
  alt_flood_pressed          = (( buttons & (1 << (18) ) ) ? 1 : 0 );
  alt_mist_pressed           = (( buttons & (1 << (19) ) ) ? 1 : 0 );
  alt_up_pressed             = (( buttons & (1 << (20) ) ) ? 1 : 0 );
  alt_down_pressed           = (( buttons & (1 << (21) ) ) ? 1 : 0 );
  alt_left_pressed           = (( buttons & (1 << (22) ) ) ? 1 : 0 );
  alt_right_pressed          = (( buttons & (1 << (23) ) ) ? 1 : 0 );
  alt_raise_pressed          = (( buttons & (1 << (24) ) ) ? 1 : 0 );
  alt_lower_pressed          = (( buttons & (1 << (25) ) ) ? 1 : 0 );
}


uint32_t *version_pointer = (uint32_t*) &context.mem[sizeof(context.mem) - 4];
Machine_status_packet *packet = (Machine_status_packet*) context.mem;
Machine_status_packet *previous_packet = &prev_packet;

Pendant_count_packet buffered_count_packet;
Pendant_count_packet *count_packet = (Pendant_count_packet*) &context.mem[sizeof(Machine_status_packet)];
int32_t step_increment = 100;

Pendant_memory_map *pendant_memory_ptr = (Pendant_memory_map*) &context.mem[0];

int character_sent;

uint8_t keypad_signal_read (uint8_t character, bool clearpin, bool update_status) {
  int timeout = I2C_TIMEOUT_VALUE;
  command_error = 0;

  while (context.mem_address_written != false && context.mem_address>0);

  return 0;
}

//replaces old function and translates buttons to bit values.
void keypad_sendchar (char c){

  count_packet->buttons = count_packet->buttons | (halt_pressed               << (0) );
  count_packet->buttons = count_packet->buttons | (hold_pressed               << (1) );
  count_packet->buttons = count_packet->buttons | (cycle_start_pressed        << (2) );
  count_packet->buttons = count_packet->buttons | (spindle_pressed            << (3) );
  count_packet->buttons = count_packet->buttons | (mist_pressed               << (4) );
  count_packet->buttons = count_packet->buttons | (flood_pressed              << (5) );
  count_packet->buttons = count_packet->buttons | (home_pressed               << (6) );
  //count_packet->buttons = count_packet->buttons | (spindle_over_down_pressed  << (7) );
  //count_packet->buttons = count_packet->buttons | (spindle_over_reset_pressed << (8) );
  //count_packet->buttons = count_packet->buttons | (spindle_over_up_pressed    << (9) );
  //count_packet->buttons = count_packet->buttons | (feed_over_down_pressed     << (10) );
  //count_packet->buttons = count_packet->buttons | (feed_over_reset_pressed    << (11) );
  //count_packet->buttons = count_packet->buttons | (feed_over_up_pressed       << (12) );
  count_packet->buttons = count_packet->buttons | (alt_halt_pressed           << (13) );
  count_packet->buttons = count_packet->buttons | (alt_hold_pressed           << (14) );
  count_packet->buttons = count_packet->buttons | (alt_home_pressed           << (15) );
  count_packet->buttons = count_packet->buttons | (alt_cycle_start_pressed    << (16) );
  count_packet->buttons = count_packet->buttons | (alt_spindle_pressed        << (17) );
  count_packet->buttons = count_packet->buttons | (alt_flood_pressed          << (18) );
  count_packet->buttons = count_packet->buttons | (alt_mist_pressed           << (19) );
  count_packet->buttons = count_packet->buttons | (alt_up_pressed             << (20) );
  count_packet->buttons = count_packet->buttons | (alt_down_pressed           << (21) );
  count_packet->buttons = count_packet->buttons | (alt_left_pressed           << (22) );
  count_packet->buttons = count_packet->buttons | (alt_right_pressed          << (23) );
  count_packet->buttons = count_packet->buttons | (alt_raise_pressed          << (24) );
  count_packet->buttons = count_packet->buttons | (alt_lower_pressed          << (25) );

  sprintf(charbuf, "BTN: %d ", count_packet->buttons);
  draw_string(charbuf);
  count_packet->buttons = 0;
  //gpio_put(KPSTR_PIN, true);
  sleep_ms(250);   
  //gpio_put(KPSTR_PIN, false);

}

bool tick_timer_callback(struct repeating_timer *t) {

    static uint32_t start_ms = 0;    

    count_packet->uptime += TICK_TIMER_PERIOD;
    if (onboard_led_count == 0){
    onboard_led_count = 20;  //thi ssets the heartbeat
    }else{
    onboard_led_count = onboard_led_count - 1;
    }

    screen_update_counter--;
    if (screen_update_counter < 0)
      screen_update_counter = 0;

    if(direction_pressed && current_jogmode != STEP){

        if(gpio_get(UPBUTTON))
          buffered_count_packet.y_axis += step_increment;
        if(gpio_get(DOWNBUTTON))
          buffered_count_packet.y_axis -= step_increment;
        if(gpio_get(RIGHTBUTTON))
          buffered_count_packet.x_axis += step_increment;
        if(gpio_get(LEFTBUTTON))
          buffered_count_packet.x_axis -= step_increment;
        if(gpio_get(RAISEBUTTON))
          buffered_count_packet.z_axis += step_increment;

        if(alt_lower_pressed){
          if(gpio_get(LOWERBUTTON))
            buffered_count_packet.a_axis -= step_increment; 
        }else {
          if(gpio_get(LOWERBUTTON))
            buffered_count_packet.z_axis -= step_increment;
        }

        if(alt_raise_pressed){
          if(gpio_get(RAISEBUTTON))
            buffered_count_packet.a_axis += step_increment; 
        }else {
          if(gpio_get(RAISEBUTTON))
            buffered_count_packet.z_axis += step_increment;
        }          

        if (!context.mem_address_written){
        count_packet->x_axis = buffered_count_packet.x_axis;
        count_packet->y_axis = buffered_count_packet.y_axis;
        count_packet->z_axis = buffered_count_packet.z_axis;
        count_packet->a_axis = buffered_count_packet.a_axis;
        }
        kpstr_counter = KPSTR_DELAY;

    } else if (direction_pressed && current_jogmode == STEP && !previous_direction_pressed){
      //only execute in step mode on rising edge of direction pressed signal
        if(gpio_get(UPBUTTON))
          buffered_count_packet.y_axis += step_increment;
        if(gpio_get(DOWNBUTTON))
          buffered_count_packet.y_axis -= step_increment;
        if(gpio_get(RIGHTBUTTON))
          buffered_count_packet.x_axis += step_increment;
        if(gpio_get(LEFTBUTTON))
          buffered_count_packet.x_axis -= step_increment;
        if(gpio_get(RAISEBUTTON))
          buffered_count_packet.z_axis += step_increment;

        if(alt_lower_pressed){
          if(gpio_get(LOWERBUTTON))
            buffered_count_packet.a_axis -= step_increment; 
        }else {
          if(gpio_get(LOWERBUTTON))
            buffered_count_packet.z_axis -= step_increment;
        }

        if(alt_raise_pressed){
          if(gpio_get(RAISEBUTTON))
            buffered_count_packet.a_axis += step_increment; 
        }else {
          if(gpio_get(RAISEBUTTON))
            buffered_count_packet.z_axis += step_increment;
        }        
        if (!context.mem_address_written){
        count_packet->x_axis = buffered_count_packet.x_axis;
        count_packet->y_axis = buffered_count_packet.y_axis;
        count_packet->z_axis = buffered_count_packet.z_axis;
        count_packet->a_axis = buffered_count_packet.a_axis;
        }
        kpstr_counter = KPSTR_DELAY;          
    } else{
      //do nothing
    }

    previous_direction_pressed = direction_pressed;

    if(kpstr_counter){
      kpstr_counter--;
      if(kpstr_counter < 0)
        kpstr_counter = 0;
      //gpio_put(KPSTR_PIN, true);
    } else {
      //gpio_put(KPSTR_PIN, false);
    }

    return true;
}

// Main loop - initilises system and then loops while interrupts get on with processing the data

int main() {

  stdio_init_all();
  //int command_error = 0;
  //float step_calc = 0;

  //gpio_init(KPSTR_PIN);
  //gpio_set_dir(KPSTR_PIN, GPIO_OUT);

  gpio_init(HALTBUTTON);
  gpio_set_dir(HALTBUTTON, GPIO_IN);
  gpio_set_pulls(HALTBUTTON,true,false);

  gpio_init(UPBUTTON);
  gpio_set_dir(UPBUTTON, GPIO_IN);
  gpio_set_pulls(UPBUTTON,true,false);

  gpio_init(DOWNBUTTON);
  gpio_set_dir(DOWNBUTTON, GPIO_IN);
  gpio_set_pulls(DOWNBUTTON,true,false);

  gpio_init(LEFTBUTTON);
  gpio_set_dir(LEFTBUTTON, GPIO_IN);
  gpio_set_pulls(LEFTBUTTON,true,false);

  gpio_init(RIGHTBUTTON);
  gpio_set_dir(RIGHTBUTTON, GPIO_IN);
  gpio_set_pulls(RIGHTBUTTON,true,false);

  gpio_init(LOWERBUTTON);
  gpio_set_dir(LOWERBUTTON, GPIO_IN);
  gpio_set_pulls(LOWERBUTTON,true,false);

  gpio_init(RAISEBUTTON);
  gpio_set_dir(RAISEBUTTON, GPIO_IN);
  gpio_set_pulls(RAISEBUTTON,true,false);

  gpio_init(JOG_SELECT);
  gpio_set_dir(JOG_SELECT, GPIO_IN);
  gpio_set_pulls(JOG_SELECT,true,false);

  gpio_init(FEEDOVER_RESET);
  gpio_set_dir(FEEDOVER_RESET, GPIO_IN);
  gpio_set_pulls(FEEDOVER_RESET,true,false);

  gpio_init(SPINDLEBUTTON);
  gpio_set_dir(SPINDLEBUTTON, GPIO_IN);
  gpio_set_pulls(SPINDLEBUTTON,true,false);

  gpio_init(RUNBUTTON);
  gpio_set_dir(RUNBUTTON, GPIO_IN);
  gpio_set_pulls(RUNBUTTON,true,false);

  gpio_init(HOLDBUTTON);
  gpio_set_dir(HOLDBUTTON, GPIO_IN);
  gpio_set_pulls(HOLDBUTTON,true,false);

  gpio_init(MISTBUTTON);
  gpio_set_dir(MISTBUTTON, GPIO_IN);
  gpio_set_pulls(MISTBUTTON,true,false);

  gpio_init(FLOODBUTTON);
  gpio_set_dir(FLOODBUTTON, GPIO_IN);
  gpio_set_pulls(FLOODBUTTON,true,false);

  gpio_init(HOMEBUTTON);
  gpio_set_dir(HOMEBUTTON, GPIO_IN);
  gpio_set_pulls(HOMEBUTTON,true,false);

  gpio_init(SPINOVER_RESET);
  gpio_set_dir(SPINOVER_RESET, GPIO_IN);
  gpio_set_pulls(SPINOVER_RESET,true,false);

  struct repeating_timer timer;
  add_repeating_timer_ms(TICK_TIMER_PERIOD, tick_timer_callback, NULL, &timer);
  
init_multimedia();

packet->machine_state = 255;
screenmode = DEFAULT; 
*version_pointer = PROTOCOL_VERSION;

//set the counts to zero
count_packet->x_axis = 0;
count_packet->y_axis = 0;
count_packet->z_axis = 0;
count_packet->a_axis = 0;
count_packet->feed_over = 100;
count_packet->spindle_over = 100;
count_packet->rapid_over = 100;
count_packet->uptime = 0;

    // Main loop handles the buttons, everything else handled in interrupts
    while (true) {

        //if the machine state is 255 (never been connected)
        //pulse the KPSTR every interval to let the host know pendant is connected.

        current_jogmode = static_cast<Jogmode>(packet->jog_mode >> 4);
        if(current_jogmode == STEP){
          step_calc = packet->jog_stepsize * 1000;
          step_increment = (int32_t) round(step_calc);
        } else {
          step_calc = (packet->jog_stepsize*1000)/60000 * TICK_TIMER_PERIOD;
          step_increment = (int32_t) round(step_calc);
        }

        if (screen_update_counter <= 0){
          draw_main_screen(previous_packet, packet, 0);
          update_neopixels(previous_packet, packet);
          screen_update_counter = SCREEN_UPDATE_PERIOD;          
        }  

                if(packet->machine_state == 255){
          if (count_packet->uptime % 1000 == 0){
            //gpio_put(KPSTR_PIN, true);
            sleep_ms(50);
            //gpio_put(KPSTR_PIN, false);
          }
        continue;
        }

        //BUTTON READING ***********************************************************************
        //these first three buttons take effect on press, not lift.
        if(gpio_get(HALTBUTTON)){   
          halt_pressed = 1;
          keypad_sendchar(key_character);
          if(!jog_toggle_pressed){
            while(gpio_get(HALTBUTTON))
              sleep_ms(250);
          }
          halt_pressed = 0;
        } else if (gpio_get(HOLDBUTTON)){
          if(!jog_toggle_pressed){             
          hold_pressed = 1;
          keypad_sendchar(key_character);
          }
          while(gpio_get(HOLDBUTTON))
            sleep_us(100);
          //gpio_put(KPSTR_PIN, false);
          hold_pressed = 0;                      
        } else if (gpio_get(RUNBUTTON)){
          if(!jog_toggle_pressed){             
          cycle_start_pressed = 1;
          keypad_sendchar(key_character);
          }
          while(gpio_get(RUNBUTTON))
            sleep_us(100);
          //gpio_put(KPSTR_PIN, false);
          cycle_start_pressed = 0;    
        //misc commands.  These activate on lift
        } else if (gpio_get(SPINOVER_RESET)){  
          spindle_over_reset_pressed = 1;            
        } else if (gpio_get(FEEDOVER_RESET)){  
          feed_over_reset_pressed = 1;              
        } else if (gpio_get(HOMEBUTTON)){  
          home_pressed = 1;        
        } else if (gpio_get(MISTBUTTON)){  
          mist_pressed = 1;
        } else if (gpio_get(FLOODBUTTON)){  
          flood_pressed = 1;   
        } else if (gpio_get(SPINDLEBUTTON)){ 
          spindle_pressed = 1;
        } else if (gpio_get(JOG_SELECT)){  //Toggle Jog modes
          jog_toggle_pressed = 1;
        } else if (!jog_toggle_pressed &&//only read jog actions when jog toggle is released.
                   gpio_get(UPBUTTON) ||
                   gpio_get(RIGHTBUTTON) ||
                   gpio_get(DOWNBUTTON) ||
                   gpio_get(LEFTBUTTON) ||
                   gpio_get(RAISEBUTTON) ||
                   gpio_get(LOWERBUTTON) ){
          direction_pressed = 1;
          screenmode = JOGGING;     
        } else {    
            direction_pressed = 0;
            if(!count_packet->buttons);
              //gpio_put(KPSTR_PIN, false); //make sure stobe is clear when no button is pending.
            if(screenmode != JOG_MODIFY)
              switch (packet->machine_state){
                case STATE_ALARM:
                  screenmode = ALARM;
                  break;
                case STATE_CYCLE:
                  screenmode = RUN;
                  break;                                 
                case STATE_HOLD:
                  screenmode = HOLD;
                  break;                
                case STATE_TOOL_CHANGE:
                  screenmode = TOOL_CHANGE;
                  break;                
                case STATE_IDLE:
                  screenmode = DEFAULT;
                  break;                
                case STATE_HOMING:
                  screenmode = HOMING;
                  break;                
                case STATE_JOG:
                  screenmode = JOGGING;
                default:
                  screenmode = DEFAULT;             
              }//close switch (packet->machine_state)
        }       
        //close button reads
//SINGLE BUTTON PRESSES ***********************************************************************
//Alternate functions ***********************************************************************
        if (jog_toggle_pressed){  //Pure modifier button.          
          if (gpio_get(JOG_SELECT)){//keyis still helddown, check alternate keys.
            screenmode = JOG_MODIFY;
            update_neopixels(previous_packet, packet);
            if (gpio_get(FLOODBUTTON)){
              alt_flood_pressed = 1;
            }
            if (gpio_get(MISTBUTTON)){
              alt_mist_pressed = 1;
            }
            if (gpio_get(LEFTBUTTON)){
              alt_left_pressed = 1;
            } 
            if (gpio_get(RIGHTBUTTON)){
              alt_right_pressed = 1;
            } 
            if (gpio_get(UPBUTTON)){
              alt_up_pressed = 1;
            } 
            if (gpio_get(DOWNBUTTON)){
              alt_down_pressed = 1;
            } 
            if (gpio_get(RAISEBUTTON)){
              alt_raise_pressed = 1;
            } 
            if (gpio_get(LOWERBUTTON)){
              alt_lower_pressed = 1;
            }
            if (gpio_get(HOMEBUTTON)){
              alt_home_pressed = 1;
            }  
            if (gpio_get(SPINDLEBUTTON)){
              alt_spindle_pressed = 1;
            }  
            if (gpio_get(HOLDBUTTON)){
              alt_hold_pressed = 1;
            }  
            if (gpio_get(RUNBUTTON)){
              alt_cycle_start_pressed = 1;
            }
            if (gpio_get(HALTBUTTON)){
              alt_halt_pressed = 1;              
            }                                                                                                                                       
          }//close jog toggle pressed.
        }//close jog button pressed statement
//Single functions ***********************************************************************
        if (jog_toggle_pressed) {
          if (gpio_get(JOG_SELECT)){}//button is still pressed, do nothing
          else{
            jog_toggle_pressed = 0;
            screenmode = DEFAULT;                     
          }
        }
        if (feed_over_reset_pressed) {
          if (gpio_get(FEEDOVER_RESET)){}//button is still pressed, do nothing
          else{
            count_packet->feed_over = 100;
            kpstr_counter = KPSTR_DELAY;
            feed_over_reset_pressed = 0;            
          }
        }
        if (spindle_over_reset_pressed) {
          if (gpio_get(SPINOVER_RESET)){}//button is still pressed, do nothing
          else{
            count_packet->spindle_over = 100;
            kpstr_counter = KPSTR_DELAY;
            spindle_over_reset_pressed = 0;        
          }
        }
        if (mist_pressed) {
          if (gpio_get(MISTBUTTON)){}//button is still pressed, do nothing
          else{
            if(!jog_toggle_pressed){
            keypad_sendchar(key_character);
            }
            mist_pressed = 0;         
          }
        }
        if (flood_pressed) {
          if (gpio_get(FLOODBUTTON)){}//button is still pressed, do nothing
          else{
            if(!jog_toggle_pressed){
            keypad_sendchar(key_character);
            }
            flood_pressed = 0;        
          }                    
        }
        if (spindle_pressed) {
          if (gpio_get(SPINDLEBUTTON)){}//button is still pressed, do nothing
          else{
            if(!jog_toggle_pressed){
            keypad_sendchar(key_character);
            }
            spindle_pressed = 0;        
          }                    
        } 
        if (home_pressed) {
          if (gpio_get(HOMEBUTTON)){}//button is still pressed, do nothing
          else{
            if(!jog_toggle_pressed){      
            keypad_sendchar(key_character);
          }
            home_pressed = 0;                
          }                    
        }
        if (alt_flood_pressed) {
          if (gpio_get(FLOODBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_flood_pressed = 0;
            sleep_ms(10);                     
          }
        }
        if (alt_mist_pressed) {
          if (gpio_get(MISTBUTTON)){}//button is still pressed, do nothing
          else{      
          keypad_sendchar(key_character);
          //gpio_put(KPSTR_PIN, false);
          alt_mist_pressed = 0;
          sleep_ms(10);               
          }
        }
        if (alt_left_pressed){
          if (gpio_get(LEFTBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_left_pressed = 0;
            sleep_ms(10);
        }}
        if (alt_right_pressed){
          if (gpio_get(RIGHTBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_right_pressed = 0;
            sleep_ms(10);
        }}
        if (alt_up_pressed){
          if (gpio_get(UPBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_up_pressed = 0;
            sleep_ms(10);
        }}
        if (alt_down_pressed){
          if (gpio_get(DOWNBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_down_pressed = 0;
            sleep_ms(10);
        }}
        if (alt_lower_pressed){
          if (gpio_get(LOWERBUTTON)){
          }//button is still pressed, Jog A Axis
          else{
            //key raised, switch back to alt mode
            keypad_sendchar(key_character);
            alt_lower_pressed = 0;
            sleep_ms(10);
        }}
        if (alt_raise_pressed){
          if (gpio_get(RAISEBUTTON)){
          }//button is still pressed, Jog A Axis//button is still pressed, Jog A axis
          else{
            keypad_sendchar(key_character);
            alt_raise_pressed = 0;
            sleep_ms(10);
        }}        
        if (alt_spindle_pressed){
          if (gpio_get(SPINDLEBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_spindle_pressed = 0;
            sleep_ms(10);
        }}
        if (alt_home_pressed){
          if (gpio_get(HOMEBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_home_pressed = 0;
            sleep_ms(10);
        }}
        if (alt_hold_pressed){
          if (gpio_get(HOLDBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_hold_pressed = 0;
            sleep_ms(10);
        }}
        if (alt_cycle_start_pressed){
          if (gpio_get(RUNBUTTON)){}//button is still pressed, do nothing
          else{
            keypad_sendchar(key_character);
            alt_cycle_start_pressed = 0;
            sleep_ms(10);
        }}  
        if (alt_halt_pressed){
          if (gpio_get(HALTBUTTON)){}//button is still pressed, do nothing
          else{
            //erase and write memory location based on current value of screenflip.
            screenflip = !screenflip;
            uint32_t status = save_and_disable_interrupts();
            flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
            restore_interrupts(status);
            sleep_ms(250);
            flash_range_program(FLASH_TARGET_OFFSET, (uint8_t*)&screenflip, 1);
            alt_halt_pressed = 0;
        }}                                                                                                                                                  
    }//close main while loop
    return 0;
}
