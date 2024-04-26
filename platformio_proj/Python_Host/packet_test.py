from time import sleep
from pySerialTransfer import pySerialTransfer as txfer
from MachineStatusPacket import MachineStatusPacket, example_binary_data

import array

x = MachineStatusPacket.parse(bytes(MachineStatusPacket.sizeof()))

x.address = 1
x.machine_state = 5
x.machine_substate = 0
x.home_state = 0xFF
x.feed_override = 125
x.spindle_override = 78
x.spindle_stop = 0
x.spindle_state = 0
x.spindle_rpm = 1235
x.feed_rate = 150.65
x.coolant_state = 1
x.jog_mode = 0
x.control_signals = 0
x.jog_stepsize = 1500
x.current_wcs = 0
x.axes_limits = 0
x.status_code = 0
x.machine_mode = 0
x.x_coordinate = 100.35
x.y_coordinate = 6587.12
x.z_coordinate = -1.25
x.a_coordinate = 0
x.message_type = 0

packed_byte_array = MachineStatusPacket.build(x)
# Convert parsedata back into the format of example_binary_data
converted = [hex(byte) for byte in packed_byte_array]

if __name__ == '__main__':
    try:
        testStruct = packed_byte_array
        string = array.array('B',testStruct)
        
        link = txfer.SerialTransfer('COM16')
        
        link.open()
        #sleep(1)
        
        # stuff the TX buffer (https://docs.python.org/3/library/struct.html#format-characters)
        send_len = 0
        print(send_len)
        send_len = link.tx_obj(x.address,       	    send_len, val_type_override='H')
        print(send_len)
        send_len = link.tx_obj(x.machine_state,        send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.machine_substate,     send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.home_state,           send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.feed_override,        send_len, val_type_override='H')
        print(send_len)
        send_len = link.tx_obj(x.spindle_override,     send_len, val_type_override='H')
        print(send_len)
        send_len = link.tx_obj(x.spindle_stop,         send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.spindle_state, 	   send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.spindle_rpm,          send_len, val_type_override='i')
        print(send_len)
        send_len = link.tx_obj(x.feed_rate,       	   send_len, val_type_override='f')
        print(send_len)
        send_len = link.tx_obj(x.coolant_state,        send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.jog_mode,             send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.control_signals,      send_len, val_type_override='I')
        print(send_len)
        send_len = link.tx_obj(x.jog_stepsize,         send_len, val_type_override='f')
        print(send_len)
        send_len = link.tx_obj(x.current_wcs,    	   send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.axes_limits,          send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.status_code, 		   send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.machine_mode,         send_len, val_type_override='B')
        print(send_len)
        send_len = link.tx_obj(x.x_coordinate,         send_len, val_type_override='f')
        print(send_len)
        send_len = link.tx_obj(x.y_coordinate,         send_len, val_type_override='f')
        print(send_len)
        send_len = link.tx_obj(x.z_coordinate,    	   send_len, val_type_override='f')
        print(send_len)
        send_len = link.tx_obj(x.a_coordinate,         send_len, val_type_override='f')
        print(send_len)
        send_len = link.tx_obj(x.message_type, 		   send_len, val_type_override='B')         
        
        #for index in range(MachineStatusPacket.sizeof()):
        #    link.txBuff[index] = testStruct[index]
            
        #sendSize = link.tx_obj(string)
        #sendSize = link.tx_obj(testStruct.y, start_pos=sendSize)
        #sendSize = MachineStatusPacket.sizeof()
        print(send_len)
        print(testStruct)
        
        # send the data
        link.send(send_len)
        
    except KeyboardInterrupt:
        link.close()