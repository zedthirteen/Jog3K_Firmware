/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "bsp/board.h"
#include "tusb.h"
#include "i2c_jogger.h"
#include "jog2k_app.h"

/* From https://www.kernel.org/doc/html/latest/input/gamepad.html
          ____________________________              __
         / [__ZL__]          [__ZR__] \               |
        / [__ TL __]        [__ TR __] \              | Front Triggers
     __/________________________________\__         __|
    /                                  _   \          |
   /      /\           __             (N)   \         |
  /       ||      __  |MO|  __     _       _ \        | Main Pad
 |    <===DP===> |SE|      |ST|   (W) -|- (E) |       |
  \       ||    ___          ___       _     /        |
  /\      \/   /   \        /   \     (S)   /\      __|
 /  \________ | LS  | ____ |  RS | ________/  \       |
|         /  \ \___/ /    \ \___/ /  \         |      | Control Sticks
|        /    \_____/      \_____/    \        |    __|
|       /                              \       |
 \_____/                                \_____/

     |________|______|    |______|___________|
       D-Pad    Left       Right   Action Pad
               Stick       Stick

                 |_____________|
                    Menu Pad

  Most gamepads have the following features:
  - Action-Pad 4 buttons in diamonds-shape (on the right side) NORTH, SOUTH, WEST and EAST.
  - D-Pad (Direction-pad) 4 buttons (on the left side) that point up, down, left and right.
  - Menu-Pad Different constellations, but most-times 2 buttons: SELECT - START.
  - Analog-Sticks provide freely moveable sticks to control directions, Analog-sticks may also
  provide a digital button if you press them.
  - Triggers are located on the upper-side of the pad in vertical direction. The upper buttons
  are normally named Left- and Right-Triggers, the lower buttons Z-Left and Z-Right.
  - Rumble Many devices provide force-feedback features. But are mostly just simple rumble motors.

halt_pressed | (hold_pressed << 1) | (cycle_start_pressed << 2) | (spindle_pressed << 3) |
(mist_pressed << 4) | (flood_pressed << 5) | (home_pressed << 6) | (spindle_over_down_pressed << 7) |
(spindle_over_reset_pressed << 8) | (spindle_over_up_pressed << 9) | (feed_over_down_pressed << 10) |
(feed_over_reset_pressed << 11) | (feed_over_up_pressed << 12) | (alt_halt_pressed << 13) |
(alt_hold_pressed << 14) | (alt_home_pressed << 15) | (alt_cycle_start_pressed << 16) | (alt_spindle_pressed << 17) |
(alt_flood_pressed << 18) | (alt_mist_pressed << 19) | (alt_up_pressed << 20) | (alt_down_pressed << 21) | (alt_left_pressed << 22) |
(alt_right_pressed << 23) | (alt_raise_pressed << 24) | (alt_lower_pressed << 25);


struct report
{
    uint32_t buttons;
    uint8_t joy0;
    uint8_t joy1;
    uint8_t joy2;
    uint8_t joy3;
} report;
*/

typedef struct TU_ATTR_PACKED
{
  struct
  {
    uint32_t buttons;
    uint8_t joy0;
    uint8_t joy1;
    uint8_t joy2;
    uint8_t joy3;
  };
} expatria_jog2k_report_t;

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

Memory_context context;
static Machine_status_packet *packet = (Machine_status_packet*) context.mem;

//static Jogger_status_packet usb_display_packet;
//void * usb_report = &usb_display_packet;

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

// check if 2 reports are different enough
bool diff_report(expatria_jog2k_report_t const* rpt1, expatria_jog2k_report_t const* rpt2)
{
  bool result;

  // check the result with mem compare
  //result |= memcmp(&rpt1, &rpt2, sizeof(expatria_jog2k_report_t));

  result = (rpt1->buttons != rpt2->buttons) || (rpt1->joy0 != rpt2->joy0) || (rpt1->joy1 != rpt2->joy1) || (rpt1->joy2 != rpt2->joy2) || (rpt1->joy3 != rpt2->joy3);

  return result;
}

void process_expatria_jog2k(uint8_t const* report, uint16_t len)
{
  
  //static Jogger_status_packet usb_display_packet;
  //extern Memory_context context;
  //static void * usb_report = &usb_display_packet;
  //const uint32_t interval_ms = 25;
  //static uint32_t start_ms = 0;

  uint32_t button_values;

  // previous report used to compare for changes
  static expatria_jog2k_report_t prev_report = { 0 };

  uint8_t const report_id = report[0];
  report++;
  len--;

  // all buttons state is stored in ID 1
  if (report_id == 1)
  {
    expatria_jog2k_report_t jog2k_report;
    memcpy(&jog2k_report, report, sizeof(jog2k_report));

    // counter is +1, assign to make it easier to compare 2 report
    //prev_report.counter = ds4_report.counter;

    // only print if changes since it is polled ~ 5ms
    // Since count+1 after each report and  x, y, z, rz fluctuate within 1 or 2
    // We need more than memcmp to check if report is different enough
    if ( diff_report(&prev_report, &jog2k_report) )
    {
      assign_button_values(jog2k_report.buttons);

      printf("(J0, J1, J2, J3) = (%u, %u, %u, %u)\r\n", jog2k_report.joy0, jog2k_report.joy1, jog2k_report.joy2, jog2k_report.joy3);

      printf("\r\n");

      if(halt_pressed){printf("halt_pressed");}
      else if (hold_pressed){printf("hold_pressed");}
      else if (cycle_start_pressed){printf("cycle_start_pressed");}
      else if (spindle_pressed){printf("spindle_pressed");}
      else if (mist_pressed){printf("mist_pressed");}
      else if (flood_pressed){printf("flood_pressed");}
      else if (home_pressed){printf("home_pressed");}
      else if (spindle_over_down_pressed){printf("spindle_over_down_pressed");}
      else if (spindle_over_reset_pressed){printf("spindle_over_reset_pressed");}
      else if (spindle_over_up_pressed){printf("spindle_over_up_pressed");}
      else if (feed_over_down_pressed){printf("feed_over_down_pressed");}
      else if (feed_over_reset_pressed){printf("feed_over_reset_pressed");}
      else if (feed_over_up_pressed){printf("feed_over_up_pressed");}
      else if (alt_halt_pressed){printf("alt_halt_pressed");}
      else if (alt_hold_pressed){printf("alt_hold_pressed");}
      else if (alt_home_pressed){printf("alt_home_pressed");}
      else if (alt_cycle_start_pressed){printf("alt_cycle_start_pressed");}
      else if (alt_spindle_pressed){printf("alt_spindle_pressed");}
      else if (alt_flood_pressed){printf("alt_flood_pressed");}
      else if (alt_mist_pressed){printf("alt_mist_pressed");}
      else if (alt_up_pressed){printf("alt_up_pressed");}
      else if (alt_down_pressed){printf("alt_down_pressed");}
      else if (alt_left_pressed){printf("alt_left_pressed");}
      else if (alt_right_pressed){printf("alt_right_pressed");}
      else if (alt_raise_pressed){printf("alt_raise_pressed");}
      else if (alt_lower_pressed){printf("alt_lower_pressed");}

    }
    prev_report = jog2k_report;

  #if 0
  
  extern Memory_context context;  

  const uint32_t interval_ms = 25;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

    usb_display_packet.address = 0;
    usb_display_packet.machine_state = 2;
    usb_display_packet.alarm = 0;
    usb_display_packet.home_state = 1;
    usb_display_packet.feed_override = 1;
    usb_display_packet.spindle_override = 0;
    usb_display_packet.spindle_stop = 0;
    usb_display_packet.coolant_state = 0;
    usb_display_packet.spindle_rpm = 1000;
    usb_display_packet.feed_rate = 456;
    usb_display_packet.x_coordinate = 0;
    usb_display_packet.y_coordinate = 0;
    usb_display_packet.z_coordinate = 0;
    usb_display_packet.a_coordinate = 0;
    usb_display_packet.jog_stepsize = 0.25;
    usb_display_packet.current_wcs = 0;

    tuh_hid_set_report(1, 0, 2,HID_REPORT_TYPE_OUTPUT, usb_report, sizeof(Jogger_status_packet));
  
  #endif       
  }else{
    printf("got report on ID: %d LEN: %d\r\n", report_id, len);
  }
}