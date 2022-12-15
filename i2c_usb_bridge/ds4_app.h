#ifndef __DS4_APP_H__
#define __DS4_APP_H__

// check if device is Sony DualShock 4
inline bool is_sony_ds4(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0x054c && (pid == 0x09cc || pid == 0x05c4)) // Sony DualShock4 
           || (vid == 0x0f0d && pid == 0x005e)                 // Hori FC4 
           || (vid == 0x0f0d && pid == 0x00ee)                 // Hori PS4 Mini (PS4-099U) 
           || (vid == 0x1f4f && pid == 0x1002)                 // ASW GG xrd controller
         );
}
void process_sony_ds4(uint8_t const* report, uint16_t len);

#if defined(_LINUX_) && defined(__cplusplus)
}
#endif // _LINUX_

#endif // 

