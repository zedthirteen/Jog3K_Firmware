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
//#include <Wire.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"

#include "pico/time.h"
#include "pico/stdio.h"

#include <i2c_fifo.h>
#include <i2c_slave.h>

#include "i2c_jogger.h"

// define I2C addresses to be used for this peripheral
static const uint I2C_SLAVE_ADDRESS = 0x49;
static const uint I2C_BAUDRATE = 400000; // 100 kHz

// GPIO pins to use for I2C SLAVE
#define HALT_PIN 25
#define KPSTR_PIN 26
static const uint I2C_SLAVE_SDA_PIN = 0;
static const uint I2C_SLAVE_SCL_PIN = 1;

//flash defines
#define FLASH_TARGET_OFFSET (256 * 1024)

const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

// ram_addr is the current address to be used when writing / reading the RAM
// N.B. the address auto increments, as stored in 8 bit value it automatically rolls round when reaches 255

char buf[8];

uint8_t ram_addr = 0;
uint8_t status_addr = 0;
uint8_t key_pressed = 0;

//char *ram_ptr = (char*) &context.mem[0];
int character_sent;
int command_error = 0;

// Our handler is called from the I2C ISR, so it must complete quickly. Blocking calls /
// printing to stdio may interfere with interrupt handling.
static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {

    switch (event) {
    case I2C_SLAVE_RECEIVE: // master has written some data - goes to the outgoing status packet
        if (mem_address_written < 2 ) {
            // writes always start with the memory address
            if(mem_address_written == 0)
                status_context.mem_address |= (uint16_t)i2c_read_byte(i2c) << 8;              
            else {
                status_context.mem_address = i2c_read_byte(i2c);                
            }
            mem_address_written++;
        } else {
            // save into memory
            status_context.mem[status_context.mem_address] = i2c_read_byte(i2c);
            status_context.mem_address++;
            mem_address_written++;
        }
        break;
    case I2C_SLAVE_REQUEST: // master is requesting data - comes from the count packet
        // load from memory
        i2c_write_byte(i2c, count_context.mem[count_context.mem_address]);
        count_context.mem_address++;
        break;
    case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
            mem_address_written = 0;
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

     i2c_init(i2c0, I2C_BAUDRATE);
    // configure I2C0 for slave mode
    i2c_slave_init(i2c0, I2C_SLAVE_ADDRESS, &i2c_slave_handler);
}


uint8_t keypad_sendcount (bool clearpin) {
  //maybe use key_pressed variable to avoid spamming?
  int timeout = I2C_TIMEOUT_VALUE;

  command_error = 0;

  //make sure no transmission is active
  //while (mem_address_written != 0 && mem_address>0);
  while (mem_address_written != 0 && status_context.mem_address>0 &&count_context.mem_address>0);

    //context.mem[0] = character;
    count_context.mem_address = 0;
    gpio_put(KPSTR_PIN, false);
    sleep_us(200);
    while (count_context.mem_address == 0 && timeout){
      sleep_us(200);      
      timeout = timeout - 1;}

    if(!timeout)
      command_error = 1;
    //sleep_ms(2);
    if (clearpin){
      sleep_us(10);
      gpio_put(KPSTR_PIN, true);
    }

  return true;
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

void i2c_task (void){
    static uint8_t strbuf[1024];
    static uint8_t countbuf[1024];
    int recSize = mem_address;

    static int ms, prev_ms;
    
    ms = millis();

    bool send_data_now = false;    
    //all the task needs to do is to check the count packet and signal the host if it changes
    //may add debounce here in future?
    //keep the pin held down as long as the count packet is changing, lift it as soon as it is ==

    recSize = sizeof(pendant_count_packet_t);
    bool count_packet_changed = false;
    count_packet_changed = memcmp(count_context.mem, countbuf, recSize) != 0;

    if(count_packet_changed){

        if(ms < prev_ms+20)
            return;
#if 0
        Serial1.print("\033c");
        Serial1.println("Size");
        Serial1.println(recSize, DEC);  
        Serial1.println("\nreceive data");   
        for (size_t i = 0; i < recSize; i++) {
            // Print each byte as a two-digit hexadecimal number
            if (count_context.mem[i] < 16) {
            Serial1.print("0"); // Print leading zero for single digit numbers
            }
            Serial1.print(count_context.mem[i], HEX); // Print byte in hexadecimal format
            Serial1.print(" "); // Print space between bytes
            
            // Insert a line break after every 16 bytes for readability
            if ((i + 1) % 16 == 0) {
            Serial1.println();
            }
        }
        // Print a final line break if necessary
        if (recSize % 16 != 0) {
            Serial1.println();
        }
#endif
        key_pressed = 1;
        keypad_sendcount(1);
        memcpy(countbuf, count_context.mem, recSize);
    }



      //int recSize = sizeof(machine_status_packet_t);


    //make sure no transmission is active
    while ((mem_address_written > 0 ) && mem_address>0); 
      
      recSize = sizeof(machine_status_packet_t);
      send_data_now = false;
      send_data_now = memcmp(status_context.mem, strbuf, recSize) != 0;

      if(send_data_now){
#if 0        
        if(!count_packet_changed)
            Serial1.print("\033c");
        Serial1.println("Size");
        Serial1.println(recSize, DEC);      
        Serial1.println("\nAddr");
        Serial1.println(mem_address, DEC);             
        Serial1.println("\nreceive data");

        for (size_t i = 0; i < recSize; i++) {
            // Print each byte as a two-digit hexadecimal number
            if (status_context.mem[i] < 16) {
            Serial1.print("0"); // Print leading zero for single digit numbers
            }
            Serial1.print(status_context.mem[i], HEX); // Print byte in hexadecimal format
            Serial1.print(" "); // Print space between bytes
            
            // Insert a line break after every 16 bytes for readability
            if ((i + 1) % 16 == 0) {
            Serial1.println();
            }
        }
        // Print a final line break if necessary
        if (recSize % 16 != 0) {
            Serial1.println();
        }

        Serial1.print("machine_state    : ");
        Serial1.println(statuspacket->machine_state, DEC);
        Serial1.print("machine_substate : ");
        Serial1.println(statuspacket->machine_substate, DEC);        
        Serial1.print("home_state       : ");
        Serial1.println(statuspacket->home_state.value, DEC);
        Serial1.print("feed_override    : ");
        Serial1.println(statuspacket->feed_override, DEC);
        Serial1.print("spindle_override : ");
        Serial1.println(statuspacket->spindle_override, DEC);        
        Serial1.print("spindle_stop     : ");
        Serial1.println(statuspacket->spindle_stop, DEC);    
        Serial1.print("spindle_state    : ");
        Serial1.println(statuspacket->spindle_state.value, DEC); 
        Serial1.print("spindle_rpm      : ");
        Serial1.println(statuspacket->spindle_rpm, DEC);        
        Serial1.print("feed_rate        : ");
        Serial1.println(statuspacket->feed_rate, DEC);    
        Serial1.print("coolant_state    : ");
        Serial1.println(statuspacket->coolant_state.value, DEC);  
        Serial1.print("jog_mode         : ");
        Serial1.println(statuspacket->jog_mode.value, DEC);        
        Serial1.print("ctrl_signals     : ");
        Serial1.println(statuspacket->signals.value, DEC);    
        Serial1.print("jog_stepsize     : ");
        Serial1.println(statuspacket->jog_stepsize, DEC);        
        Serial1.print("current_wcs      : ");
        Serial1.println(statuspacket->current_wcs, DEC);        
        Serial1.print("limits           : ");
        Serial1.println(statuspacket->limits.value, DEC);    
        Serial1.print("status_code     : ");
        Serial1.println(statuspacket->status_code, DEC);
        Serial1.print("machine_modes   : ");
        Serial1.println(statuspacket->machine_modes.value, DEC);        
        Serial1.print("X_axis          : ");
        Serial1.println(statuspacket->coordinate.x, DEC);    
        Serial1.print("Y_axis          : ");
        Serial1.println(statuspacket->coordinate.y, DEC);  
        Serial1.print("Z_axis          : ");
        Serial1.println(statuspacket->coordinate.z, DEC);  
        Serial1.print("A_axis          : ");
        Serial1.println(statuspacket->coordinate.a, DEC);    
        Serial1.print("message_type    : ");
        Serial1.println(statuspacket->msgtype, DEC); 
        //Serial1.print("message         : ");
        //Serial1.print(statuspacket->msg);                                        
                                        
        sleep_us(50000);
#endif
        memcpy(strbuf, status_context.mem, recSize);            
      }

    prev_ms = ms;
}

void init_i2c_responder (void){

  //Machine_status_packet *previous_packet = &prev_packet;
  uint8_t key_character = '\0';

  gpio_init(KPSTR_PIN);
  gpio_put(KPSTR_PIN, true);
  gpio_set_dir(KPSTR_PIN, GPIO_OUT);

  gpio_init(HALT_PIN);
  gpio_put(HALT_PIN, true);
  gpio_set_dir(HALT_PIN, GPIO_OUT);

  gpio_init(HALTBUTTON);
  gpio_set_dir(HALTBUTTON, GPIO_IN);
  gpio_set_pulls(HALTBUTTON,true,false);


  gpio_init(ONBOARD_LED);
  gpio_set_dir(ONBOARD_LED, GPIO_OUT);

    // Setup I2C0 as slave (peripheral)
  Serial1.printf("Setup I2C\r\n");
  setup_slave();
  sleep_ms(50);
  //Wire.setSDA(I2C_SLAVE_SDA_PIN);
  //Wire.setSCL(I2C_SLAVE_SCL_PIN);
  //Wire.onReceive(i2c_receive_handler);
  //Wire.onRequest(i2c_request_handler);
  //Wire.onFinish(i2c_finish_handler);
  //Wire.begin();
  //packet->machine_state = 255;
  //key_character = CMD_FEED_HOLD;
  //keypad_sendchar (key_character, 1);

}