from time import sleep
from pySerialTransfer import pySerialTransfer as txfer
from MachineStatusPacket import MachineStatusPacket, example_binary_data


class struct:
    z = '$'
    y = 4.5


arr = 'hello'


if __name__ == '__main__':
    try:
        testStruct = example_binary_data
        
        
        link = txfer.SerialTransfer('COM16')
        
        link.open()
        sleep(1)
    
        while True:
            sendSize = 0
            
            sendSize = link.tx_obj(testStruct, start_pos=sendSize)
            #sendSize = link.tx_obj(testStruct.y, start_pos=sendSize)
            #sendSize = link.tx_obj(arr, start_pos=sendSize)
            
            link.send(sendSize)
        
    except KeyboardInterrupt:
        link.close()