#ifndef __JOG2K_APP_H__
#define __JOG2K_APP_H__

typedef struct 
{
    uint8_t address;               // offset 0, size 1
    uint8_t machine_state;         // offset 1, size 1
    uint8_t alarm;                 // offset 2, size 1
    uint8_t home_state;            // offset 3, size 1
    uint8_t feed_override;         // offset 4, size 1
    uint8_t spindle_override;      // offset 5, size 1
    uint8_t spindle_stop;          // offset 6, size 1
    uint8_t coolant_state;         // offset 7, size 1
    int spindle_rpm;               // offset 8, size 4
    float feed_rate;               // offset 12, size 4
    float x_coordinate;            // offset 16, size 4
    float y_coordinate;            // offset 20, size 4
    float z_coordinate;            // offset 24, size 4
    float a_coordinate;            // offset 28, size 4
    float jog_stepsize;            // offset 32, size 4
    uint8_t jog_mode;              // includes both modifier as well as mode //offset 36, size 1
    uint8_t current_wcs;           // active WCS or MCS modal state //offset 37, size 1
} Jogger_status_packet;            // 40 bytes when aligned by compiler - 38 bytes of data.

// check if device is jog2k
inline bool is_expatria_jog2k(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0xcafe && (pid == 0x4004)) // Jog2k HID 
         );
}

void process_expatria_jog2k(uint8_t const* report, uint16_t len);

#if defined(_LINUX_) && defined(__cplusplus)
}
#endif // _LINUX_

#endif // 