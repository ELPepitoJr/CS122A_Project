#!/usr/bin/env python3
from lirc import RawConnection
from time import sleep
import spidev

def createSPI(device):
    spi = spidev.SpiDev()
    spi.open(0, device)
    spi.max_speed_hz = 1000000
    spi.mode = 0
    return spi

bus = 0
device = 0
lcdspi = spidev.SpiDev()
lcdspi.open(bus,device)
lcdspi.max_speed_hz = 1000000
lcdspi.mode = 0

device1 = 1
motorspi = spidev.SpiDev()
motorspi.open(0, device1)
motorspi.max_speed_hz = 1000000
motorspi.mode = 0



def ProcessIRRemote():

    #get IR command
    #keypress format = (hexcode, repeat_num, command_key, remote_id)
    try:
        keypress = conn.readline(.0001)
    except:
        keypress=""
              
    if (keypress != "" and keypress != None):
                
        data = keypress.split()
        sequence = data[1]
        command = data[2]
        
        #ignore command repeats
        if (sequence != "00"):
           return
        
        print(command)        
           
    return keypress

#define Global
conn = RawConnection()

print("Starting Up...")
num = 1

while True:         

    output = ProcessIRRemote()
    #lcdSPI = createSPI(0);
    if (output != ""):
        print(output)
        num = 1
        num1 = num
        num1 = int(num1)
        num = int(num)
        motorspi.writebytes([num1])
        lcdspi.writebytes([num])


