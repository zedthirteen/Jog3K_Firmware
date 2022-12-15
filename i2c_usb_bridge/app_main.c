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

#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 12
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 13
#endif

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"

#include "pico/time.h"

#include "bsp/board.h"
#include "tusb.h"

#include <i2c_fifo.h>
#include <i2c_slave.h>

#include "i2c_jogger.h"

//#define SHOWJOG 1
//#define SHOWOVER 1
#define SHOWRAM 1
#define TWOWAY 1

#define TICK_TIMER_PERIOD 10
#define ROLLOVER_DELAY_PERIOD 7

// RPI Pico

int rc;

volatile bool timer_fired = false;

bool tick_timer_callback(struct repeating_timer *t) {
    
    return true;
}

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
void led_blinking_task(void);

extern void cdc_task(void);
extern void hid_app_task(void);

// Main loop - initilises system and then loops while interrupts get on with processing the data

void core1_main()
{
    tusb_init();
    while(!tusb_inited());

    while (1)
    {
        // Do stuff here on core 1
        tuh_task();
        hid_app_task();
    }
}

int main() {

  stdio_init_all();

  board_init();

  init_i2c_responder();

  gpio_init(REDLED);
  gpio_set_dir(REDLED, GPIO_OUT);
  gpio_init(GREENLED);
  gpio_set_dir(GREENLED, GPIO_OUT);
  gpio_put(ONBOARD_LED, 1);
  gpio_put(REDLED, 1);
  sleep_ms(250);
  gpio_put(ONBOARD_LED, 0);
  gpio_put(REDLED, 0);
  sleep_ms(250);
  gpio_put(ONBOARD_LED, 1);
  gpio_put(REDLED, 1);

  printf("TinyUSB Host   19   MSC HID Example\r\n");

  struct repeating_timer timer;
  add_repeating_timer_ms(TICK_TIMER_PERIOD, tick_timer_callback, NULL, &timer);

  //multicore_launch_core1(core1_main);

  tusb_init();
      
      // Main loop handles the LED, everything else handled in interrupts
  while (true) {           
   
      led_blinking_task();
      hid_app_task();
      tuh_task();

      }//close main while loop
      return 0;
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  const uint32_t interval_ms = 1000;
  static uint32_t start_ms = 0;

  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  gpio_put(GREENLED, led_state);
  led_state = 1 - led_state; // toggle
}