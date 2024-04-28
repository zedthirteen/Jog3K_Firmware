import time
from time import sleep
from pySerialTransfer import pySerialTransfer as txfer
from MachineStatusPacket import MachineStatusPacket, example_binary_data

import array, struct
from enum import Enum

class Status(Enum):
    CONTINUE        = 3
    NEW_DATA        = 2
    NO_DATA         = 1
    CRC_ERROR       = 0
    PAYLOAD_ERROR   = -1
    STOP_BYTE_ERROR = -2

# Define count packet structure
class countPacket:
    uptime = 0
    jog_mode = 0
    feed_over = 0
    spindle_over = 0
    rapid_over = 0
    buttons = 0
    feedrate = 0
    spindle_rpm = 0
    x_axis = 0
    y_axis = 0
    z_axis = 0
    a_axis = 0

y = countPacket
x = MachineStatusPacket.parse(bytes(MachineStatusPacket.sizeof()))

x.address = 1
x.machine_state = 5
x.machine_substate = 0
x.home_state = 0
x.feed_override = 0
x.spindle_override = 0
x.spindle_stop = 0
x.spindle_state = 0
x.spindle_rpm = 2000
x.feed_rate = 150.65
x.coolant_state = 3
x.jog_mode = 0x00
x.control_signals = 0
x.jog_stepsize = 1500
x.current_wcs = 0
x.axes_limits = 0
x.status_code = 0
x.machine_mode = 0
x.x_coordinate = 100.35
x.y_coordinate = 6482.322
x.z_coordinate = -100.987
x.a_coordinate = 0
x.message_type = 0

packed_byte_array = MachineStatusPacket.build(x)
# Convert parsedata back into the format of example_binary_data
converted = [hex(byte) for byte in packed_byte_array]

testStruct = packed_byte_array
string = array.array('B',testStruct)

link = txfer.SerialTransfer('COM16')

link.open()
#sleep(1)

# stuff the TX buffer (https://docs.python.org/3/library/struct.html#format-characters)
send_len = 0

send_len = link.tx_obj(x.address,       	    send_len, val_type_override='H')        
send_len = link.tx_obj(x.machine_state,        send_len, val_type_override='B')        
send_len = link.tx_obj(x.machine_substate,     send_len, val_type_override='B')        
send_len = link.tx_obj(x.home_state,           send_len, val_type_override='B')        
send_len = link.tx_obj(x.feed_override,        send_len, val_type_override='H')        
send_len = link.tx_obj(x.spindle_override,     send_len, val_type_override='H')        
send_len = link.tx_obj(x.spindle_stop,         send_len, val_type_override='B')        
send_len = link.tx_obj(x.spindle_state, 	   send_len, val_type_override='B')        
send_len = link.tx_obj(x.spindle_rpm,          send_len, val_type_override='i')        
send_len = link.tx_obj(x.feed_rate,       	   send_len, val_type_override='f')        
send_len = link.tx_obj(x.coolant_state,        send_len, val_type_override='B')        
send_len = link.tx_obj(x.jog_mode,             send_len, val_type_override='B')        
send_len = link.tx_obj(x.control_signals,      send_len, val_type_override='H')        
send_len = link.tx_obj(x.jog_stepsize,         send_len, val_type_override='f')        
send_len = link.tx_obj(x.current_wcs,    	   send_len, val_type_override='B')        
send_len = link.tx_obj(x.axes_limits,          send_len, val_type_override='B')        
send_len = link.tx_obj(x.status_code, 		   send_len, val_type_override='B')        
send_len = link.tx_obj(x.machine_mode,         send_len, val_type_override='B')        
send_len = link.tx_obj(x.x_coordinate,         send_len, val_type_override='f')        
send_len = link.tx_obj(x.y_coordinate,         send_len, val_type_override='f')        
send_len = link.tx_obj(x.z_coordinate,    	   send_len, val_type_override='f')       
send_len = link.tx_obj(x.a_coordinate,         send_len, val_type_override='f')      
send_len = link.tx_obj(x.message_type, 		   send_len, val_type_override='B')         

#for index in range(MachineStatusPacket.sizeof()):
#    link.txBuff[index] = testStruct[index]
    
#sendSize = link.tx_obj(string)
#sendSize = link.tx_obj(testStruct.y, start_pos=sendSize)
#sendSize = MachineStatusPacket.sizeof()
print(send_len)

# send the data
link.send(send_len)
#sleep(1)

print('check available')
print(link.available())

timeout = 2  # 100ms timeout
start_time = time.time()

while time.time() - start_time < timeout:
    if link.available():
        print('get data')
        recSize = 0
        print(recSize)
        y.uptime = link.rx_obj(obj_type='I', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['I']
        print(recSize)
        
        y.jog_mode = link.rx_obj(obj_type='B', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['B']
        print(recSize)
        
        y.feed_over = link.rx_obj(obj_type='i', start_pos=recSize)
        little_endian_bytes = struct.pack('<i', y.feed_over)
        y.feed_over = struct.unpack('>i', little_endian_bytes)[0]
        recSize += txfer.STRUCT_FORMAT_LENGTHS['i']
        print(recSize)
        
        y.spindle_over = link.rx_obj(obj_type='i', start_pos=recSize)
        little_endian_bytes = struct.pack('<i', y.spindle_over)
        y.spindle_over = struct.unpack('>i', little_endian_bytes)[0]
        recSize += txfer.STRUCT_FORMAT_LENGTHS['i']
        print(recSize)
        
        y.buttons = link.rx_obj(obj_type='I', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['I']
        print(recSize)
        
        y.feedrate = link.rx_obj(obj_type='f', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['f']
        print(recSize)
        
        y.spindle_rpm = link.rx_obj(obj_type='f', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['f']
        print(recSize)
        
        y.x_axis = link.rx_obj(obj_type='f', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['f']
        print(recSize)
        
        y.y_axis = link.rx_obj(obj_type='f', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['f']
        print(recSize)
        
        y.z_axis = link.rx_obj(obj_type='f', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['f']
        print(recSize)
        
        y.a_axis = link.rx_obj(obj_type='f', start_pos=recSize)
        recSize += txfer.STRUCT_FORMAT_LENGTHS['f']          
        
        print(recSize)
        break
        
    elif link.status <= 0:
        if link.status == Status.CRC_ERROR:
            print('ERROR: CRC_ERROR')
        elif link.status == Status.PAYLOAD_ERROR:
            print('ERROR: PAYLOAD_ERROR')
        elif link.status == Status.STOP_BYTE_ERROR:
            print('ERROR: STOP_BYTE_ERROR')
        else:
            print('ERROR: {}'.format(link.status.name))                
    
print('uptime: ' + str(y.uptime))
print('jog_mode: ' + str(y.jog_mode))
print('feed_over: ' + str(y.feed_over))
print('spindle_over: ' + str(y.spindle_over))
print('rapid_over: ' + str(y.rapid_over))
print('buttons: ' + str(y.buttons))
print('feedrate: ' + str(y.feedrate))
print('spindle_rpm: ' + str(y.spindle_rpm))
print('x_axis: ' + str(y.x_axis))
print('y_axis: ' + str(y.y_axis))
print('z_axis: ' + str(y.z_axis))
print('a_axis: ' + str(y.a_axis))

link.close()
