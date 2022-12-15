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

#include "ds4_app.h"
#include "jog2k_app.h"
#include "i2c_jogger.h"

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

bool mounted = 0;
bool send_busy = 0;
bool report_received = 0;
uint8_t jog_dev_addr;
uint8_t jog_instance;
uint32_t sequence = 0;
uint32_t rpt_sequence = 0;

static Jogger_status_packet usb_display_packet;
void * usb_report = &usb_display_packet;

void hid_app_task(void)
{
  #if 1
  
  extern Memory_context context;  

  const uint32_t interval_ms = 25;
  static uint32_t start_ms = 0;

  if (!mounted) return;

  if ( board_millis() - start_ms < interval_ms)
    return;    
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

  //printf("send %d ", sequence);
  
  tuh_hid_set_report(jog_dev_addr, jog_instance, 2,HID_REPORT_TYPE_OUTPUT, usb_report, sizeof(Jogger_status_packet));

/*
    tusb_control_request_t const request =
  {
    .bmRequestType_bit =
    {
      .recipient = TUSB_REQ_RCPT_INTERFACE,
      .type      = TUSB_REQ_TYPE_CLASS,
      .direction = TUSB_DIR_OUT
    },
    .bRequest = HID_REQ_CONTROL_SET_REPORT,
    .wValue   = tu_u16(report_type, report_id),
    .wIndex   = hid_itf->itf_num,
    .wLength  = len
  };

  tuh_xfer_t xfer =
  {
    .daddr       = dev_addr,
    .ep_addr     = 0,
    .setup       = &request,
    .buffer      = report,
    .complete_cb = set_report_complete,
    .user_data   = 0
  };

  //TU_ASSERT( tuh_control_xfer(&xfer) );
  tuh_edpt_xfer(&xfer);
  */
  #endif
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  (void)desc_report;
  (void)desc_len;
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  printf("VID = %04x, PID = %04x\r\n", vid, pid);

  // Sony DualShock 4 [CUH-ZCT2x]
  if ( is_sony_ds4(dev_addr) )
  {
    printf("DS4 connected\r\n");
    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      printf("Error: cannot request to receive report\r\n");
    }
  } else if (is_expatria_jog2k(dev_addr)){
    printf("Jog2k connected\r\n");

    printf("dvice address:  %d \r\n", dev_addr);
    printf("dvice instance: %d \r\n", instance);
    jog_dev_addr = dev_addr;
    jog_instance = instance;
    mounted = 1;

    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      printf("Error: cannot request to receive report\r\n");
    }
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
  mounted = 0;

}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if ( is_sony_ds4(dev_addr) )
  {
    process_sony_ds4(report, len);  //process the report and indicate which buttons are pressed.
  } else if (is_expatria_jog2k(dev_addr))
  {
    process_expatria_jog2k(report, len);
  }
  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    printf("Error: cannot request to receive report\r\n");
  }
}

#if 0
 //Invoked when sent report to device successfully via interrupt endpoint
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
  if(len)
    printf("clear %d\r\n", sequence);
  sequence++;  
}

#endif