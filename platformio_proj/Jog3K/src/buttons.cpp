#include <Arduino.h>

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "pico/stdio.h"

#include "i2c_jogger.h"

/* these are simply used for convenience */

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

uint8_t up_pressed = 0;
uint8_t down_pressed = 0;
uint8_t left_pressed = 0;
uint8_t right_pressed = 0;
uint8_t raise_pressed = 0;
uint8_t lower_pressed = 0;

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

uint32_t buttons;

void init_buttons(void){
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

  gpio_init(JOG_SELECT2);
  gpio_set_dir(JOG_SELECT2, GPIO_IN);
  gpio_set_pulls(JOG_SELECT2,true,false);

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
}

void readButtons(void){

  buttons = 0;

  if (!gpio_get(JOG_SELECT2) || !gpio_get(JOG_SELECT)){
    buttons           = ( buttons | (!gpio_get(HALTBUTTON) << (15) ) );
    buttons           = ( buttons | (!gpio_get(HOLDBUTTON) << (16) ) );
    buttons           = ( buttons | (!gpio_get(HOMEBUTTON) << (17) ) );
    buttons           = ( buttons | (!gpio_get(RUNBUTTON) << (18) ) );
    buttons           = ( buttons | (!gpio_get(SPINDLEBUTTON) << (19) ) );
    buttons           = ( buttons | (!gpio_get(FLOODBUTTON) << (20) ) );
    buttons           = ( buttons | (!gpio_get(MISTBUTTON) << (21) ) );
    buttons           = ( buttons | (!gpio_get(UPBUTTON) << (22) ) );
    buttons           = ( buttons | (!gpio_get(DOWNBUTTON) << (23) ) );
    buttons           = ( buttons | (!gpio_get(LEFTBUTTON) << (24) ) );
    buttons           = ( buttons | (!gpio_get(RIGHTBUTTON) << (25) ) );
    buttons           = ( buttons | (!gpio_get(RAISEBUTTON) << (26) ) );
    buttons           = ( buttons | (!gpio_get(LOWERBUTTON) << (27) ) );

  } else {
    buttons           = ( buttons | (!gpio_get(HALTBUTTON) << (0) ) );
    buttons           = ( buttons | (!gpio_get(HOLDBUTTON) << (1) ) );
    buttons           = ( buttons | (!gpio_get(RUNBUTTON) << (2) ) );
    buttons           = ( buttons | (!gpio_get(SPINDLEBUTTON) << (3) ) );
    buttons           = ( buttons | (!gpio_get(MISTBUTTON) << (4) ) );
    buttons           = ( buttons | (!gpio_get(FLOODBUTTON) << (5) ) );
    buttons           = ( buttons | (!gpio_get(HOMEBUTTON) << (6) ) );
    buttons           = ( buttons | (!gpio_get(SPINOVER_RESET) << (7) ) );
    buttons           = ( buttons | (!gpio_get(FEEDOVER_RESET) << (8) ) );
    buttons           = ( buttons | (!gpio_get(UPBUTTON) << (9) ) );
    buttons           = ( buttons | (!gpio_get(DOWNBUTTON) << (10) ) );
    buttons           = ( buttons | (!gpio_get(LEFTBUTTON) << (11) ) );
    buttons           = ( buttons | (!gpio_get(RIGHTBUTTON) << (12) ) );
    buttons           = ( buttons | (!gpio_get(RAISEBUTTON) << (13) ) );
    buttons           = ( buttons | (!gpio_get(LOWERBUTTON) << (14) ) );
  }

  //Serial1.println("Buttons: ");
  //Serial1.println(buttons, HEX);

}
