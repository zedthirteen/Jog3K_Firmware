#include <Arduino.h>
#include <Wire.h>
#include <SerialTransfer.h>
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 12
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 13
#endif
/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* This example demonstrates use of native controller as host (TinyUSB Host)
 * Note:
 * - For most mcu with only 1 native usb, Serial is not available. We will use Serial1 instead
 *
 * Host example will get device descriptors of attached devices and print it out as follows:
      Device 1: ID 046d:c52f
      Device Descriptor:
        bLength             18
        bDescriptorType     1
        bcdUSB              0200
        bDeviceClass        0
        bDeviceSubClass     0
        bDeviceProtocol     0
        bMaxPacketSize0     8
        idVendor            0x046d
        idProduct           0xc52f
        bcdDevice           2200
        iManufacturer       1     Logitech
        iProduct            2     USB Receiver
        iSerialNumber       0
        bNumConfigurations  1
 *
 */

#include "Adafruit_TinyUSB.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"
#include "pico/time.h"
#include "i2c_jogger.h"

// RPI Pico

int rc;

volatile bool timer_fired = false;

static bool transfer_active;

SerialTransfer packetTransfer;

float num = 0;

status_context_t prev_status_context, status_context, count_context;

uint16_t mem_address = 0;
uint16_t mem_address_written = 0;

machine_status_packet_t prev_statuspacket = {};

machine_status_packet_t *statuspacket = (machine_status_packet_t*) status_context.mem;
machine_status_packet_t *previous_statuspacket = &prev_statuspacket;

pendant_count_packet_t prev_countpacket = {};

pendant_count_packet_t *countpacket = (pendant_count_packet_t*) count_context.mem;
pendant_count_packet_t *previous_countpacket = &prev_countpacket;

uint8_t simulation_mode;
static void process_simulation_mode(void);

void led_blinking_task(void)
{
  static const uint32_t interval_ms = 1000;
  static uint32_t start_ms = 0;
  static unsigned long mils = 0;

  static bool led_state = false;

  // Blink every interval ms
  mils=millis();
  if ( (mils - start_ms) < interval_ms) return; // not enough time
  start_ms += interval_ms;

  gpio_put(REDLED, led_state);
  led_state = 1 - led_state; // toggle
}

bool tick_timer_callback(struct repeating_timer *t) {
    
    return true;
}

#ifndef USE_TINYUSB_HOST
  #error This example requires usb stack configured as host in "Tools -> USB Stack -> Adafruit TinyUSB Host"
#endif

// Language ID: English
#define LANGUAGE_ID 0x0409

typedef struct {
  tusb_desc_device_t desc_device;
  uint16_t manufacturer[32];
  uint16_t product[48];
  uint16_t serial[16];
  bool mounted;
} dev_info_t;

// CFG_TUH_DEVICE_MAX is defined by tusb_config header
dev_info_t dev_info[CFG_TUH_DEVICE_MAX] = { 0 };

Adafruit_USBH_Host USBHost;
Adafruit_USBH_CDC SerialHost;

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
extern "C" {

// Invoked when a device with CDC interface is mounted
// idx is index of cdc interface in the internal pool.
void tuh_cdc_mount_cb(uint8_t idx) {
  // bind SerialHost object to this interface index
  SerialHost.mount(idx);
  Serial1.println("SerialHost is connected to a new CDC device");
}

// Invoked when a device with CDC interface is unmounted
void tuh_cdc_umount_cb(uint8_t idx) {
  SerialHost.umount(idx);
  Serial1.println("SerialHost is disconnected");
}

}

// forward Seral <-> SerialHost
void forward_serial(void) {
  uint8_t buf[64];

  // SerialHost -> Serial
  if (SerialHost.connected() && SerialHost.available()) {
    size_t count = SerialHost.read(buf, sizeof(buf));
    Serial1.write(buf, count);
    Serial1.flush();
  }
}

//--------------------------------------------------------------------+
// setup() & loop()
//--------------------------------------------------------------------+
void setup() {
  Serial1.setRX(13);
  Serial1.setTX(12);
  Serial1.begin(115200);
  Serial1.println("TinyUSB Native Host: Device Info Example");

  init_i2c_responder();
  gpio_init(REDLED);
  gpio_set_dir(REDLED, GPIO_OUT);

  // Init USB Host on native controller roothub port0
  USBHost.begin(0);
  SerialHost.begin(0,0);
  //packetTransfer.begin(SerialHost);
  packetTransfer.begin(SerialHost,true, Serial1, 50);
  simulation_mode = 0;

}

void transmit_data(void){
  // use this variable to keep track of how many
  // bytes we're stuffing in the transmit buffer
  uint16_t sendSize = 0;
  static uint8_t strbuf[1024];
  bool send_data_now;

  if(transfer_active)
    return;

  static const uint32_t interval_ms = 20;
  static uint32_t start_ms = 0;
  static unsigned long mils = 0;

  mils=millis();
  if ( (mils - start_ms) < interval_ms) return; // not enough time
  start_ms += interval_ms;

  //copy data into the output buffer, only send it if there is a change
  //send_data_now = true;
  send_data_now = memcmp(status_context.mem, prev_status_context.mem, sizeof(machine_status_packet_t)) != 0;

  //may need to guard this copy.
  if(send_data_now)
    memcpy(prev_status_context.mem, status_context.mem, sizeof(machine_status_packet_t));

  //copy data into the output buffer
  for (size_t i = 0; i < sizeof(machine_status_packet_t); i++) {
    strbuf[i] = status_context.mem[i];
  }

  
  if (!packetTransfer.available() && SerialHost.connected() && (send_data_now == true)){
    transfer_active = 1;
    gpio_put(REDLED, 1);
    ///////////////////////////////////////// Stuff buffer with struct
    Serial1.flush();
    packetTransfer.reset();
    sendSize = packetTransfer.txObj(strbuf, sendSize, sizeof(machine_status_packet_t));
    SerialHost.flush();
    ///////////////////////////////////////// Send buffer
    packetTransfer.sendData(sendSize);
    SerialHost.flush();
    Serial1.flush();
    packetTransfer.reset();
    
    #if 0
    Serial1.print("\033c");

    Serial1.println("machine_status_packet_t\n");
    Serial1.println(sizeof(machine_status_packet_t), DEC);

    Serial1.println("send data");
    Serial1.println(sendSize, DEC);

    for (size_t i = 0; i < sendSize; i++) {
      // Print each byte as a two-digit hexadecimal number
      if (prev_status_context.mem[i] < 16) {
        Serial1.print("0"); // Print leading zero for single digit numbers
      }
      Serial1.print(prev_status_context.mem[i], HEX); // Print byte in hexadecimal format
      Serial1.print(" "); // Print space between bytes
      
      // Insert a line break after every 16 bytes for readability
      if ((i + 1) % 16 == 0) {
        Serial1.println();
      }
    }
    // Print a final line break if necessary
    if (sendSize % 16 != 0) {
      Serial1.println();
    }
    #endif

  }
  gpio_put(REDLED, 0);
  transfer_active = 0;
}

void receive_data(void){

  static uint8_t strbuf[1024];
  if(transfer_active)
    return;
  transfer_active = 1;
  if (SerialHost.connected() && SerialHost.available()) {

    if(packetTransfer.available())
    {
      gpio_put(GREENLED, 1);
      // use this variable to keep track of how many
      // bytes we've processed from the receive buffer
      uint16_t recSize = 0;

      //Serial1.println("receive data\n");
      //Serial1.println(sizeof(machine_status_packet_t), DEC);

      recSize = packetTransfer.rxObj(strbuf, recSize );

      #if 0
      Serial1.print("\033c");

      Serial1.println("pendant_count_packet_t_size\n");
      Serial1.println(sizeof(pendant_count_packet_t), DEC);

      Serial1.println("receive data");
      Serial1.println(recSize, DEC);

      for (size_t i = 0; i < recSize; i++) {
        // Print each byte as a two-digit hexadecimal number
        if (strbuf[i] < 16) {
          Serial1.print("0"); // Print leading zero for single digit numbers
        }
        Serial1.print(strbuf[i], HEX); // Print byte in hexadecimal format
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

      //copy data into the count packet
      for (size_t i = 0; i < sizeof(pendant_count_packet_t); i++) {
        count_context.mem[i] = strbuf[i];
      }
      packetTransfer.reset();
    }
  }
  transfer_active = 0;
  gpio_put(GREENLED, 0);
}


void loop() {
  //led_blinking_task();
  USBHost.task();
  receive_data();
  SerialHost.flush();
  transmit_data();
  SerialHost.flush();
  //forward_serial();
  Serial1.flush();
  i2c_task();
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
void print_device_descriptor(tuh_xfer_t *xfer);

void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len);

void print_lsusb(void) {
  bool no_device = true;
  for (uint8_t daddr = 1; daddr < CFG_TUH_DEVICE_MAX + 1; daddr++) {
    // TODO can use tuh_mounted(daddr), but tinyusb has an bug
    // use local connected flag instead
    dev_info_t *dev = &dev_info[daddr - 1];
    if (dev->mounted) {
      Serial1.printf("Device %u: ID %04x:%04x %s %s\r\n", daddr,
                    dev->desc_device.idVendor, dev->desc_device.idProduct,
                    (char *) dev->manufacturer, (char *) dev->product);

      no_device = false;
    }
  }

  if (no_device) {
    Serial1.println("No device connected (except hub)");
  }
}

// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr) {
  Serial1.printf("Device attached, address = %d\r\n", daddr);

  dev_info_t *dev = &dev_info[daddr - 1];
  dev->mounted = true;

  // Get Device Descriptor
  tuh_descriptor_get_device(daddr, &dev->desc_device, 18, print_device_descriptor, 0);
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr) {
  Serial1.printf("Device removed, address = %d\r\n", daddr);
  dev_info_t *dev = &dev_info[daddr - 1];
  dev->mounted = false;

  // print device summary
  print_lsusb();
}

void print_device_descriptor(tuh_xfer_t *xfer) {
  if (XFER_RESULT_SUCCESS != xfer->result) {
    Serial1.printf("Failed to get device descriptor\r\n");
    return;
  }

  uint8_t const daddr = xfer->daddr;
  dev_info_t *dev = &dev_info[daddr - 1];
  tusb_desc_device_t *desc = &dev->desc_device;

  Serial1.printf("Device %u: ID %04x:%04x\r\n", daddr, desc->idVendor, desc->idProduct);
  Serial1.printf("Device Descriptor:\r\n");
  Serial1.printf("  bLength             %u\r\n"     , desc->bLength);
  Serial1.printf("  bDescriptorType     %u\r\n"     , desc->bDescriptorType);
  Serial1.printf("  bcdUSB              %04x\r\n"   , desc->bcdUSB);
  Serial1.printf("  bDeviceClass        %u\r\n"     , desc->bDeviceClass);
  Serial1.printf("  bDeviceSubClass     %u\r\n"     , desc->bDeviceSubClass);
  Serial1.printf("  bDeviceProtocol     %u\r\n"     , desc->bDeviceProtocol);
  Serial1.printf("  bMaxPacketSize0     %u\r\n"     , desc->bMaxPacketSize0);
  Serial1.printf("  idVendor            0x%04x\r\n" , desc->idVendor);
  Serial1.printf("  idProduct           0x%04x\r\n" , desc->idProduct);
  Serial1.printf("  bcdDevice           %04x\r\n"   , desc->bcdDevice);

  // Get String descriptor using Sync API
  Serial1.printf("  iManufacturer       %u     ", desc->iManufacturer);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_manufacturer_string_sync(daddr, LANGUAGE_ID, dev->manufacturer, sizeof(dev->manufacturer))) {
    utf16_to_utf8(dev->manufacturer, sizeof(dev->manufacturer));
    Serial1.printf((char *) dev->manufacturer);
  }
  Serial1.printf("\r\n");

  Serial1.printf("  iProduct            %u     ", desc->iProduct);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, dev->product, sizeof(dev->product))) {
    utf16_to_utf8(dev->product, sizeof(dev->product));
    Serial1.printf((char *) dev->product);
  }
  Serial1.printf("\r\n");

  Serial1.printf("  iSerialNumber       %u     ", desc->iSerialNumber);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, dev->serial, sizeof(dev->serial))) {
    utf16_to_utf8(dev->serial, sizeof(dev->serial));
    Serial1.printf((char *) dev->serial);
  }
  Serial1.printf("\r\n");

  Serial1.printf("  bNumConfigurations  %u\r\n", desc->bNumConfigurations);

  // print device summary
  print_lsusb();
}

//--------------------------------------------------------------------+
// String Descriptor Helper
//--------------------------------------------------------------------+

static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len, uint8_t *utf8, size_t utf8_len) {
  // TODO: Check for runover.
  (void) utf8_len;
  // Get the UTF-16 length out of the data itself.

  for (size_t i = 0; i < utf16_len; i++) {
    uint16_t chr = utf16[i];
    if (chr < 0x80) {
      *utf8++ = chr & 0xff;
    } else if (chr < 0x800) {
      *utf8++ = (uint8_t) (0xC0 | (chr >> 6 & 0x1F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 0 & 0x3F));
    } else {
      // TODO: Verify surrogate.
      *utf8++ = (uint8_t) (0xE0 | (chr >> 12 & 0x0F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 6 & 0x3F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 0 & 0x3F));
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
}

// Count how many bytes a utf-16-le encoded string will take in utf-8.
static int _count_utf8_bytes(const uint16_t *buf, size_t len) {
  size_t total_bytes = 0;
  for (size_t i = 0; i < len; i++) {
    uint16_t chr = buf[i];
    if (chr < 0x80) {
      total_bytes += 1;
    } else if (chr < 0x800) {
      total_bytes += 2;
    } else {
      total_bytes += 3;
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
  return total_bytes;
}

void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len) {
  size_t utf16_len = ((temp_buf[0] & 0xff) - 2) / sizeof(uint16_t);
  size_t utf8_len = _count_utf8_bytes(temp_buf + 1, utf16_len);

  _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *) temp_buf, buf_len);
  ((uint8_t *) temp_buf)[utf8_len] = '\0';
}

static void process_simulation_mode(void){
 
  static float fast_stepsize = 15000;
  static float slow_stepsize = 1500;
  static float step_stepsize = 1;

  
  //Serial1.println("jog_mode: ");
  //Serial1.println(statuspacket->jog_mode.value, HEX);

  //during simulation mode the status packet is updated in response to local operations.
  //statuspacket->coordinate.x = countpacket->x_axis;
  //statuspacket->coordinate.y = countpacket->y_axis;
  //statuspacket->coordinate.z = countpacket->z_axis;
  //statuspacket->coordinate.a = countpacket->a_axis;

  statuspacket->feed_override = countpacket->feed_over;
  statuspacket->spindle_override = countpacket->spindle_over;

  if(statuspacket->machine_state == MachineState_Disconnected)
    statuspacket->machine_state = MachineState_Idle;

  statuspacket->jog_mode = countpacket->jog_mode;  
  //cacluate jog stepsize
  switch (statuspacket->jog_mode.mode){
    case JOGMODE_FAST :
        statuspacket->jog_stepsize = fast_stepsize; 
    break;
    case JOGMODE_SLOW :
        statuspacket->jog_stepsize = slow_stepsize;       
    break;
    case JOGMODE_STEP :
        statuspacket->jog_stepsize = step_stepsize;      
    break;        
  }

  //if(previous_countpacket->feedrate != countpacket->feedrate){
    statuspacket->jog_stepsize = statuspacket->jog_stepsize+countpacket->feedrate;
  //}  

  switch (statuspacket->jog_mode.modifier){
    case 0 :
      statuspacket->jog_stepsize = statuspacket->jog_stepsize;
    break;
    case 1 :
      statuspacket->jog_stepsize = statuspacket->jog_stepsize/10;
    break;
    case 2 :
      statuspacket->jog_stepsize = statuspacket->jog_stepsize/100;
    break;
    case 3 :
      statuspacket->jog_stepsize = statuspacket->jog_stepsize/1000;
    break;              
  } 

  switch (statuspacket->jog_mode.mode){
    case JOGMODE_FAST :
      statuspacket->coordinate.x = countpacket->x_axis*10;
      statuspacket->coordinate.y = countpacket->y_axis*10;
      statuspacket->coordinate.z = countpacket->z_axis*10;
      statuspacket->coordinate.a = countpacket->a_axis*10;      
    break;
    case JOGMODE_SLOW :
      statuspacket->coordinate.x = countpacket->x_axis;
      statuspacket->coordinate.y = countpacket->y_axis;
      statuspacket->coordinate.z = countpacket->z_axis;
      statuspacket->coordinate.a = countpacket->a_axis;         
    break;
    case JOGMODE_STEP :
      statuspacket->coordinate.x = countpacket->x_axis*statuspacket->jog_stepsize;
      statuspacket->coordinate.y = countpacket->y_axis*statuspacket->jog_stepsize;
      statuspacket->coordinate.z = countpacket->z_axis*statuspacket->jog_stepsize;
      statuspacket->coordinate.a = countpacket->a_axis*statuspacket->jog_stepsize;         
    break;        
  }
  
  if(previous_countpacket->spindle_rpm != countpacket->spindle_rpm)
    statuspacket->spindle_rpm = countpacket->spindle_rpm;

  *previous_countpacket = *countpacket;

  //buttons just set the state directly.  Jog buttons set jogging state.  Run, hold halt set their states (halt sets alarm) etc.
  //pressing alt-spindle sets tool change state.
}