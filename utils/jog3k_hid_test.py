import hid
import time
import struct
import sys
import serial
from enum import Enum


# config options
looptime = 0.01  # loop time for HID communication loop
oled_interval = 1  # send oled data every n loops
state_change_interval = 200  # cycle through test states every n loops

# counters
oled_interval_counter = 0
state_change_counter = 0

# enum for state change testing

# define STATE_NOT_CONNECTED 0 // Not receiving USB connected flag. Display 'Not Connected' message
# define STATE_ALARM 1         //!< In alarm state. Locks out all g-code processes. Allows settings access.
# define STATE_CYCLE 2         //!< Cycle is running or motions are being executed.
# define STATE_HOLD 3          //!< Active feed hold
# define STATE_TOOL_CHANGE 4   //!< Manual tool change, similar to #STATE_HOLD - but stops spindle and allows jogging.
# define STATE_IDLE 5          //!< Must be zero. No flags.
# define STATE_HOMING 6        //!< Performing homing cycle
# define STATE_JOG 7           //!< Jogging mode.
# define STATE_NOT_HOMED 8     // Machine not homed, most LEDs off.
# define STATE_MACHINE_OFF 9   // Machine power off - likely LCNC only?


class State(Enum):
    NOT_CONNECTED = 0
    ALARM = 1
    CYCLE = 2
    HOLD = 3
    TOOL_CHANGE = 4
    IDLE = 5
    HOMING = 6
    JOG = 7
    NOT_HOMED = 8
    MACHINE_OFF = 9


state = State.NOT_CONNECTED
# state = State.DRO_TEST

# HID report IDs
button_reportid = 1  # HID Repord ID for button data
# oled_reportid = [2]  # HID Report ID for oled data

# HID config options
button_read_bytes = 9
encoder_read_bytes = 21

# Button status variables
halt_button = 0
hold_button = 0
cycle_start_button = 0
spindle_button = 0
mist_button = 0
flood_button = 0
home_button = 0
spindle_over_down_button = 0
spindle_over_reset_button = 0
spindle_over_up_button = 0
feed_over_down_button = 0
feed_over_reset_button = 0
feed_over_up_button = 0
jog_sel_button = 0
alt_halt_button = 0
alt_hold_button = 0
alt_home_button = 0
alt_cycle_start_button = 0
alt_spindle_button = 0
alt_flood_button = 0
alt_mist_button = 0
alt_up_button = 0
alt_down_button = 0
alt_left_button = 0
alt_right_pressed_button = 0
alt_raise_button = 0


# jog axes; 128 = centered(ish)
jog_x = 128
jog_y = 128
jog_z = 128
jog_speed = 0

# jog encoders
jog_encoder_x = 0
jog_encoder_y = 0
jog_encoder_z = 0

# Status flag variables, used for LED states
lcnc_connected = 1
ALARM_active = 0
IDLE = 0
machine_homed = 0
program_paused = 0
spindle_on = 0
coolant_on = 0
mist_on = 0
program_running = 0
flags = [0x00, 0x00]

xpos = -9123.456
ypos = 456.789
zpos = 789.012
tool = 99999


# def to_bytes(n, length, endianess='big'):
#    h = '%x' % n
#    s = ('0'*(len(h) % 2) + h).zfill(length*2).decode('hex')
#    return s if endianess == 'big' else s[::-1]


# decrement display invertval to keep setting intuitive. Counts to 0.
oled_interval -= 1


def float_to_hex(f):
    return hex(struct.unpack('<I', struct.pack('<f', f))[0])


# def float_to_bytes(f):
#    if sys.version_info[0] == 2:
#        return [ord(ch) for ch in list(struct.pack('<q', struct.unpack('<q', struct.pack('d', f))[0]))]
#    else:
#        # to_bytes only exists in Python 3
#        return list(int(struct.unpack('<q', struct.pack('d', f))[0]).to_bytes(8, 'little', signed=True))

def float_to_bytes(f):
    if sys.version_info[0] == 2:
        return [ord(ch) for ch in list(struct.pack('<q', struct.unpack('<q', struct.pack('d', f))[0]))]
    else:
        # to_bytes only exists in Python 3
        return list(int(struct.unpack('<l', struct.pack('f', f))[0]).to_bytes(4, 'little', signed=True))


def uint_to_bytes(i):
    if sys.version_info[0] == 2:
        return [ord(ch) for ch in list(struct.pack('<I', struct.unpack('<I', struct.pack('I', i))[0]))]
    else:
        # to_bytes only exists in Python 3
        return list(int(struct.unpack('<I', struct.pack('I', i))[0]).to_bytes(4, 'little', signed=False))


def bytes_to_signed_int(data):
    return int(struct.unpack('<i', bytearray(data))[0])


def set_flags():
    # Set/clear status flag bits
    if(lcnc_connected):
        flags[0] |= (1 << 0)  # set bit 0
    else:
        flags[0] &= ~ (1 << 0)  # clear bit 0

    if(ALARM_active):
        flags[0] |= (1 << 1)  # set bit 1
    else:
        flags[0] &= ~ (1 << 1)  # clear bit 1

    if(IDLE):
        flags[0] |= (1 << 2)  # set bit 2
    else:
        flags[0] &= ~ (1 << 2)  # clear bit 2

    if(machine_homed):
        flags[0] |= (1 << 3)  # set bit 3
    else:
        flags[0] &= ~ (1 << 3)  # clear bit 3

    if(program_paused):
        flags[0] |= (1 << 4)  # set bit 4
    else:
        flags[0] &= ~ (1 << 4)  # clear bit 4

    if(spindle_on):
        flags[0] |= (1 << 5)  # set bit 5
    else:
        flags[0] &= ~ (1 << 5)  # clear bit 5

    if(coolant_on):
        flags[0] |= (1 << 6)  # set bit 6
    else:
        flags[0] &= ~ (1 << 6)  # clear bit 6

    if(mist_on):
        flags[0] |= (1 << 7)  # set bit 7
    else:
        flags[0] &= ~ (1 << 7)  # clear bit 7

    if(program_running):
        flags[1] |= (1 << 0)  # set bit 0
    else:
        flags[1] &= ~ (1 << 0)  # clear bit 0


# typedef struct Machine_status_packet
# {
#    uint8_t address;
#    uint8_t machine_state;
#    uint8_t alarm;
#    uint8_t home_state;
#    uint8_t feed_override;
#    uint8_t spindle_override;
#    uint8_t spindle_stop;
#    int spindle_rpm;
#    float feed_rate;
#    coolant_state_t coolant_state;
#    uint8_t jog_mode; // includes both modifier as well as mode
#    float jog_stepsize;
#    coord_system_id_t current_wcs; // active WCS or MCS modal state
#    float x_coordinate;
#    float y_coordinate;
#    float z_coordinate;
#    float a_coordinate;
# } Machine_status_packet;

# define STATE_NOT_CONNECTED 0 // Not receiving USB connected flag. Display 'Not Connected' message
# define STATE_ALARM 1         //!< In alarm state. Locks out all g-code processes. Allows settings access.
# define STATE_CYCLE 2         //!< Cycle is running or motions are being executed.
# define STATE_HOLD 3          //!< Active feed hold
# define STATE_TOOL_CHANGE 4   //!< Manual tool change, similar to #STATE_HOLD - but stops spindle and allows jogging.
# define STATE_IDLE 5          //!< Must be zero. No flags.
# define STATE_HOMING 6        //!< Performing homing cycle
# define STATE_JOG 7           //!< Jogging mode.
# define STATE_NOT_HOMED 8     // Machine not homed, most LEDs off.
# define STATE_MACHINE_OFF 9   // Machine power off - likely LCNC only?

# status packet variables
address = 0x01
machine_state = 0x00
alarm = 0x00
home_state = 0x01
feed_override = 100
spindle_override = 100
spindle_stop = 0x00
spindle_rpm = 24000
feed_rate = 1234.567
coolant_state = 0x00
jog_mode = 0x00
jog_stepsize = 1.000
current_wcs = 0x00
x_coordinate = 123.456
y_coordinate = 2345.567
z_coordinate = 3456.678
a_coordinate = 4567.890
# a_coordinate = 0xFFFFFFFF

# state = State.HOLD
# state_change_interval = 20000000  # cycle through test states every n loops

# print("Status packet size = " + str(len(machine_status_packet)) + " bytes")

try:
    print("Opening  HID device")

    h = hid.device()
    h.open(0xCAFE, 0x4005)  # VendorID/ProductID

    print("Found: " + h.get_manufacturer_string() + " " + h.get_product_string())

    print("Opening CDC device")
    s = serial.Serial(
        '/dev/serial/by-id/usb-Expatria_Jog3k_123456-if01', 115200, timeout=1)

    # enable non-blocking HID reads
    h.set_nonblocking(1)
    # wait
    time.sleep(0.05)

    # Read buttons
    print("Read button data")
    # byte 0 = report id, bytes 1 - 4 = buttons, bytes 5 - 8 = joysticks
    while True:
        now = time.time()  # get current time, we'll use this later to set loop time
        # two reports are read in random order. Read twice then figure out which is which.
        reports = [h.read(encoder_read_bytes + 1),
                   h.read(encoder_read_bytes + 1)]  # Report ID + data
        print(reports)

        if (reports[0][0] == 1):
            # button report is first
            button_report = reports[0]
            encoder_report = reports[1]
        else:
            # encoder report is first
            encoder_report = reports[0]
            button_report = reports[1]

        # we're using non-blocking reads to keep loop time in check; ensure we got both entire reports.
        if(len(button_report) == button_read_bytes and len(encoder_report) == encoder_read_bytes):
            print("Raw Button Report:")
            print(button_report)
            halt_button = button_report[1] & 0b00000001
            hold_button = button_report[1] & 0b00000010
            cycle_start_button = button_report[1] & 0b00000100
            spindle_button = button_report[1] & 0b00001000
            mist_button = button_report[1] & 0b00010000
            flood_button = button_report[1] & 0b00100000
            home_button = button_report[1] & 0b01000000
            spindle_over_down_button = button_report[1] & 0b10000000
            spindle_over_reset_button = button_report[2] & 0b00000001
            spindle_over_up_button = button_report[2] & 0b00000010
            feed_over_down_button = button_report[2] & 0b00000100
            feed_over_reset_button = button_report[2] & 0b00001000
            feed_over_up_button = button_report[2] & 0b00010000
            alt_halt_button = button_report[2] & 0b00100000
            alt_hold_button = button_report[2] & 0b01000000
            alt_home_button = button_report[2] & 0b10000000
            alt_cycle_start_button = button_report[3] & 0b00000001
            alt_spindle_button = button_report[3] & 0b00000010
            alt_flood_button = button_report[3] & 0b00000100
            alt_mist_button = button_report[3] & 0b00001000
            alt_up_button = button_report[3] & 0b00010000
            alt_down_button = button_report[3] & 0b00100000
            alt_left_button = button_report[3] & 0b01000000
            alt_right_pressed_button = button_report[3] & 0b10000000
            alt_raise_button = button_report[4] & 0b00000001
            alt_lower_button = button_report[4] & 0b00000010

            jog_x = button_report[5]
            jog_y = button_report[6]
            jog_z = button_report[7]
            jog_speed = button_report[8]
            jog_encoder_x = bytes_to_signed_int(encoder_report[1:5])
            jog_encoder_y = bytes_to_signed_int(encoder_report[5:9])
            jog_encoder_z = bytes_to_signed_int(encoder_report[9:13])

            # print debug data
            print("X: " + str(jog_x))
            print("Y: " + str(jog_y))
            print("Z: " + str(jog_z))
            print("Jog Speed: " + str(jog_speed))

            print("X Encoder: " + str(jog_encoder_x))
            print("Y Encoder: " + str(jog_encoder_y))
            print("Z Encoder: " + str(jog_encoder_z))

            if(halt_button):
                print("Halt")
            if(hold_button):
                print("Hold")
            if(cycle_start_button):
                print("Cycle Start")
            if(spindle_button):
                print("Spindle")
            if(mist_button):
                print("Mist")
            if(flood_button):
                print("Flood")
            if(home_button):
                print("Home")
            if(spindle_over_down_button):
                print("Spindle Override Down")
            if(spindle_over_reset_button):
                print("Spindle Override Reset")
            if(spindle_over_up_button):
                print("Spindle Override Up")
            if(feed_over_down_button):
                print("Feed Override Down")
            if(feed_over_reset_button):
                print("Feed Override Reset")
            if(feed_over_up_button):
                print("Feed Override Up")
            if(alt_halt_button):
                print("Alt Halt")
            if(alt_hold_button):
                print("Alt Hold")
            if(alt_home_button):
                print("Alt Home")
            if(alt_cycle_start_button):
                print("Alt Cycle Start")
            if(alt_spindle_button):
                print("Alt Spindle")
            if(alt_flood_button):
                print("Alt Flood")
            if(alt_mist_button):
                print("Alt Mist")
            if(alt_up_button):
                print("Alt Up")
            if(alt_down_button):
                print("Alt Down")
            if(alt_left_button):
                print("Alt Left")
            if(alt_right_pressed_button):
                print("Alt Right")
            if(alt_raise_button):
                print("Alt Raise")
            if(alt_lower_button):
                print("Alt Lower")

            if(oled_interval_counter == 0):
                # Create and send status packet
                print("Updating Status")
                # stock structure
                # machine_status_packet = [address, machine_state, alarm, home_state, feed_override, spindle_override, spindle_stop] + uint_to_bytes(spindle_rpm) + float_to_bytes(
                #    feed_rate) + [coolant_state, jog_mode] + float_to_bytes(jog_stepsize) + [current_wcs] + float_to_bytes(x_coordinate) + float_to_bytes(y_coordinate) + float_to_bytes(z_coordinate) + float_to_bytes(a_coordinate)

                # Data from Andrew that breaks HID
                # address = 0
                # machine_state = 2
                # alarm = 0
                # home_state = 1
                # feed_override = 1
                # spindle_override = 0
                # spindle_stop = 0
                # coolant_state = 0
                # spindle_rpm = 1000
                # feed_rate = 456
                # x_coordinate = 0
                # y_coordinate = 0
                # z_coordinate = 0
                # a_coordinate = 0
                # jog_stepsize = 0.25
                # current_wcs = 0

                # re-ordered structure
                machine_status_packet = [address, machine_state, alarm, home_state, feed_override, spindle_override, spindle_stop, coolant_state] + uint_to_bytes(spindle_rpm) + float_to_bytes(
                    feed_rate) + float_to_bytes(x_coordinate) + float_to_bytes(y_coordinate) + float_to_bytes(z_coordinate) + float_to_bytes(a_coordinate) + float_to_bytes(jog_stepsize) + [jog_mode] + [current_wcs] + [0x00, 0x00]
                # h.write(oled_reportid + machine_status_packet)
                s.write(machine_status_packet)

                oled_interval_counter = oled_interval
            else:
                oled_interval_counter -= 1

        if(state_change_counter == 0):
            print("Changing state")

            # state machine
            if (state == State.NOT_CONNECTED):
                print("State = NOT_CONNECTED")
                machine_state = 0x00
                x_coordinate = 0
                y_coordinate = 0
                z_coordinate = 0
                tool = 0
                state = State.ALARM
            elif (state == State.ALARM):
                print("State = ALARM")
                machine_state = 0x01  # ALARM
                state = State.CYCLE
            elif (state == State.CYCLE):
                print("State = CYCLE")
                machine_state = 0x02  # CYCLE
               # state = State.HOLD # We will do this in the DRO dummy data generation routine.
            elif (state == State.HOLD):
                print("State = HOLD")
                machine_state = 0x03  # HOLD
                state = State.IDLE
            elif (state == State.IDLE):
                print("State = IDLE")
                machine_state = 0x05  # TOOL_CHANGE
                state = State.TOOL_CHANGE
            elif (state == State.TOOL_CHANGE):
                print("State = TOOL_CHANGE")
                machine_state = 0x04  # TOOL_CHANGE
                state = State.HOMING
            elif (state == State.HOMING):
                print("State = HOMING")
                machine_state = 0x06  # HOMING
                state = State.JOG
            elif (state == State.JOG):
                print("State = JOG")
                machine_state = 0x07  # HOMING
                state = State.NOT_HOMED
            elif (state == State.NOT_HOMED):
                print("State = NOT_HOMED")
                machine_state = 0x08  # NOT_HOMED
                state = State.MACHINE_OFF
            elif (state == State.MACHINE_OFF):
                print("State = MACHINE_OFF")
                machine_state = 0x09  # MACHINE_OFF
                state = State.NOT_CONNECTED

            state_change_counter = state_change_interval
        else:
            state_change_counter -= 1

        # generate DRO test data
        if (state == State.CYCLE):
            x_coordinate = x_coordinate + 1.123
            y_coordinate = y_coordinate + 2.345
            z_coordinate = z_coordinate + 3.456
            if (tool >= 99999):
                tool = 1
                state = State.HOLD
            else:
                tool = tool + 234

        # determine how long we spend doing stuff
        elapsed = time.time() - now
        print("Tasks took " + str(elapsed) + "s")
        # wait for the remainder of the loop time. If we took longer than the loop time, don't sleep.
        time.sleep(max(0, looptime - elapsed))
        print("Loop time = " + str(time.time() - now) + "s")

        if (elapsed > looptime):
            print("Loop time exceeded!")
           # h.close()
           # s.close()
           # exit()


except IOError as ex:
    print(ex)
    print("JOG2k not found!")
    h.close()
    s.close()

except KeyboardInterrupt:
    h.close()
    s.close()
    raise SystemExit

except Exception as ex:
    template = "An exception of type {0} occurred. Arguments:\n{1!r}"
    message = template.format(type(ex).__name__, ex.args)
    print(message)


print("Closing the device")
h.close()
s.close()
print("Done")
