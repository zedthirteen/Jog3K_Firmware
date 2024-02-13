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

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"

#include "pico/time.h"

#include <i2c_fifo.h>
#include <i2c_slave.h>

#include "i2c_jogger.h"


// define I2C addresses to be used for this peripheral
static const uint I2C_SLAVE_ADDRESS = 0x49;
static const uint I2C_BAUDRATE = 100000; // 100 kHz

// GPIO pins to use for I2C SLAVE
#if JOG_MODULE == 1
#define HALT_PIN 28
#define KPSTR_PIN 29
static const uint I2C_SLAVE_SDA_PIN = 0;
static const uint I2C_SLAVE_SCL_PIN = 1;
#else
#define HALT_PIN 0
#define KPSTR_PIN 10
static const uint I2C_SLAVE_SDA_PIN = 4;
static const uint I2C_SLAVE_SCL_PIN = 5;
#endif

//flash defines
#define FLASH_TARGET_OFFSET (256 * 1024)

const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

// ram_addr is the current address to be used when writing / reading the RAM
// N.B. the address auto increments, as stored in 8 bit value it automatically rolls round when reaches 255

/*struct
{
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
} context;*/

char buf[8];

uint8_t ram_addr = 0;
uint8_t status_addr = 0;
uint8_t key_pressed = 0;
//extern uint8_t key_character;

Memory_context context;

Machine_status_packet *packet = (Machine_status_packet*) context.mem;
Machine_status_packet prev_packet;
Machine_status_packet *previous_packet = &prev_packet;

char *ram_ptr = (char*) &context.mem[0];
int character_sent;
int command_error = 0;

// Our handler is called from the I2C ISR, so it must complete quickly. Blocking calls /
// printing to stdio may interfere with interrupt handling.
static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: // master has written some data
        if (!context.mem_address_written) {
            // writes always start with the memory address
            context.mem_address = i2c_read_byte(i2c);
            context.mem_address_written = true;
        } else {
            // save into memory
            context.mem[context.mem_address] = i2c_read_byte(i2c);
            context.mem_address++;
        }
        break;
    case I2C_SLAVE_REQUEST: // master is requesting data
        // load from memory
        i2c_write_byte(i2c, context.mem[context.mem_address]);
        context.mem_address++;
        break;
    case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
        context.mem_address_written = false;
        break;
    default:
        break;
    }
}

static void setup_slave() {
    gpio_init(I2C_SLAVE_SDA_PIN);
    gpio_set_function(I2C_SLAVE_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SDA_PIN);

    gpio_init(I2C_SLAVE_SCL_PIN);
    gpio_set_function(I2C_SLAVE_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SCL_PIN);

    #if JOG_MODULE == 1
    i2c_init(i2c0, I2C_BAUDRATE);
    // configure I2C0 for slave mode
    i2c_slave_init(i2c0, I2C_SLAVE_ADDRESS, &i2c_slave_handler);
    #else
    i2c_init(i2c1, I2C_BAUDRATE);
    // configure I2C1 for slave mode
    i2c_slave_init(i2c1, I2C_SLAVE_ADDRESS, &i2c_slave_handler);
    #endif
}

uint8_t keypad_sendchar (uint8_t character, bool clearpin) {
  //maybe use key_pressed variable to avoid spamming?
  int timeout = I2C_TIMEOUT_VALUE;

  command_error = 0;

  gpio_put(ONBOARD_LED, 0);

  while (context.mem_address_written != false && context.mem_address>0);

    context.mem[0] = character;
    context.mem_address = 0;
    //gpio_put(KPSTR_PIN, false);
    sleep_us(100);
    gpio_put(KPSTR_PIN, false);
    sleep_us(1000);
    while (context.mem_address == 0 && timeout){
      sleep_us(1000);      
      timeout = timeout - 1;}

    if(!timeout)
      command_error = 1;
    //sleep_ms(2);
    if (clearpin){
      sleep_ms(5);
      gpio_put(KPSTR_PIN, true);
    }
  gpio_put(ONBOARD_LED, 1);
  return true;
};

void kpstr_clear (void) {
  gpio_put(KPSTR_PIN, true);
  sleep_ms(5);
};

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

void init_i2c_responder (void){

  Machine_status_packet *previous_packet = &prev_packet;
  uint8_t key_character = '\0';

  gpio_init(KPSTR_PIN);
  gpio_set_dir(KPSTR_PIN, GPIO_OUT);

    // Setup I2C0 as slave (peripheral)
  Serial1.printf("Setup I2C\r\n");
  setup_slave();
  packet->machine_state = 255;
  key_character = '?';
  keypad_sendchar (key_character, 1);

}