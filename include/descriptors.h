#include "common/tusb_common.h"
#include "device/usbd.h"

// Descriptor with following:
// | Button Map (4 bytes) |  X | Y | Z | Rz | (2 bytes) | X Encoder Counts (4 bytes) | Y Encoder Counts (4 bytes) | Z Encoder Counts (4 bytes)

#define REPORT_DESC_GAMEPAD(...)                           \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
        HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),              \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),        \
        __VA_ARGS__                                        \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),             \
        HID_USAGE_MIN(0),                                  \
        HID_USAGE_MAX(32),                                 \
        HID_LOGICAL_MIN(0),                                \
        HID_LOGICAL_MAX(1),                                \
        HID_REPORT_COUNT(32),                              \
        HID_REPORT_SIZE(1),                                \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),            \
        HID_LOGICAL_MIN(0x00),                             \
        HID_LOGICAL_MAX_N(0x00FF, 2),                      \
        HID_USAGE(HID_USAGE_DESKTOP_X),                    \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                    \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                    \
        HID_USAGE(HID_USAGE_DESKTOP_RZ),                   \
        HID_REPORT_COUNT(4),                               \
        HID_REPORT_SIZE(8),                                \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_COLLECTION_END

#define REPORT_DESC_STATUS(...)                             \
    HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),             \
        HID_USAGE(0x00),                                    \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),         \
        __VA_ARGS__                                         \
        HID_USAGE_PAGE(HID_USAGE_PAGE_ALPHA_DISPLAY),       \
        HID_USAGE(0x00),                                    \
        HID_REPORT_COUNT(40),                               \
        HID_REPORT_SIZE(8),                                 \
        HID_LOGICAL_MIN(0x0),                               \
        HID_LOGICAL_MAX(0xff),                              \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_COLLECTION_END

#define REPORT_DESC_ENCODER(...)                            \
    HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),             \
        HID_USAGE(0x00),                                    \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),         \
        __VA_ARGS__                                         \
        HID_USAGE_PAGE(HID_USAGE_PAGE_ALPHA_DISPLAY),       \
        HID_USAGE(0x00),                                    \
        HID_REPORT_COUNT(12),                               \
        HID_REPORT_SIZE(8),                                 \
        HID_LOGICAL_MIN(0x0),                               \
        HID_LOGICAL_MAX(0xff),                              \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_COLLECTION_END
