import GPUtil
import psutil
import serial
import serial.tools.list_ports
from threading import Thread
import time
import sys, warnings
import os

global graphSize
global blank
global filled
global ser

graphSize = 16
blank = "░"
filled = "▓"
ser=serial.Serial()
SerialCom = True


class Monitor(Thread):
    """Thread for handling async tasks, like updating the system load"""
    def __init__(self, delay):
        super(Monitor, self).__init__()
        self.stopped = False
        self.delay = delay
        self.start()

    def run(self):
        while not self.stopped:
            startTime = time.time()
            DoUpdate()
            elapsedTime = time.time()-startTime #checks how long that loop took, very useful if using blocking serial that may take a very long time to resolve.
            time.sleep(max(self.delay-elapsedTime,0.01)) #sleep the remaining time on the delay, or 0.01 seconds, whichever is longer

    def stop(self):
        """Stops the Async Monitor class thread from running"""
        self.stopped = True

def repeatCharacter(Char,reps):
    """Returns a string containing the spefied character the number of reps specified"""
    returnString = ""
    for _ in range(reps):
        returnString += Char
    return returnString

def DoUpdate():
    """Aquires, prints out, and sends over serial the current load of the system"""

    printbuffer="" #hold all the things we want to say until the end so the graph always stays in the same position
    #get all the stats we want, and convert them all into the same form. Specifically XX.XX
    try:
        CPU = round(psutil.cpu_percent(),2)
        RAM = round(dict(psutil.virtual_memory()._asdict()).get('percent'),2)
    except:
        printbuffer+="Error while retrieving CPU/RAM status.\n"
        CPU=0
        RAM=0
    try:
        GPUs = GPUtil.getGPUs()
        GPUavail = GPUtil.getAvailable(includeNan=False)
        NumOfGPUs = len(GPUs) #total number of GPUs, not that we'll show them all, since "consumer-grade" GPUs no longer report usage to nv-smi
        GPUmemArr = [0] * NumOfGPUs
        GPULoadArr = [0] * NumOfGPUs


        if len(GPUavail)<NumOfGPUs:
            printbuffer+="Some GPUs did not report their usage:"
            for i in GPUs:
                if not i.id in GPUavail:
                    printbuffer += " "+str(i.name)+" "
            printbuffer+="\n"

        for i in GPUavail:
            GPUmemArr[i]= round((GPUs[i].memoryUsed/GPUs[i].memoryTotal) * 100,2)
            GPULoadArr[i] = round(GPUs[i].load *100,2)

    except:
        printbuffer+="Error while loading GPU status (normal after something like sleep mode).\n"
        GPUmemArr = [0]
        GPULoadArr= [0]
        GPUavail = []


    os.system('cls')
    print("Press enter to quit.\n")#add an extra newline to space it out
    #Print out the bare numbers to the console
    print("CPU Avrg:   "+str(CPU)+"%")
    print("RAM Usage:  "+str(RAM)+"%")
    for i in GPUavail:
        print("GPU"+str(i)+" Usage: "+str(GPULoadArr[i])+"%")
        print("GPU"+str(i)+" mem:   "+str(GPUmemArr[i])+"%")

    #Create graphs for better "at-a-glance" functionality, and print them to the console
    #CPU graph
    print("CPU Load: "+GenerateGraph(CPU,graphSize,filled,blank))
    print("RAM Usage:"+GenerateGraph(RAM,graphSize,filled,blank))

    #GPU Graph
    for i in GPUavail:
        print("GPU"+str(i)+" Load:"+GenerateGraph(GPULoadArr[i],graphSize,filled,blank))
        print("GPU"+str(i)+" Mem: "+GenerateGraph(GPUmemArr[i],graphSize,filled,blank))

    print() #newline before extra info and all that
    #OpenComPort doesn't have to use the printbuffer since it's already at the end
    if SerialCom:
        if ser.is_open:
            ExportString = str(delay) + "," + str(CPU) + "," + str(RAM)
            for i in GPUavail:
                ExportString+= "," + str(GPULoadArr[i]) + "," + str(GPUmemArr[i])
            try:
                ser.write(ExportString.encode())
                printbuffer+=ser.readline().strip().decode()+"\n"
            except Exception as err:
                printbuffer+="There was an error communicating with the arduino. Port is open, but error returned: \n"+str(err)+"\n"
                OpenComPort() #try to reopen. It clearly was there. If the error is fixed later, let it reconnect
        else:
            printbuffer+="Serial Unavailable.\n"
            OpenComPort() #constantly search for the arduino
    else:
        printbuffer+="Serial functionality disabled.\n"
    
    print(printbuffer) #print out everything else at the end
    

def OpenComPort():
    """Attempts to open a serial port using the 2nd runtime argument first, Opening none if it is "None". Then falling back to looking for an arduino plugged into the system and sending to the first one found."""
    global ser
    global SerialCom
    #open serial comms
    try:
        COMPort = sys.argv[2]
        if COMPort == "None":
            SerialCom = False
            print("Serial functionality disabled.")
            return 
    except:
        COMPort = GetFirstArduino()
        #"COM5" is a common port for arduinos to take

    if COMPort != 0:
        try:
            ser = serial.Serial(COMPort, 9600, bytesize = 8, parity = 'O', stopbits = 2, timeout = 0)
        except Exception as ex:
            print("Error Resolving Serial Port: \n"+str(ex))
        else:
            while ser.is_open == False:
                print("Waiting For Serial...")
                #waste time
            print("Serial Open")
            #Exchange pleasantries
            print(ser.readline().strip().decode())

    return 

def GetFirstArduino():
    """Finds all open arduino comports, and returns the first one found, or 0 if none are found"""
    print("Finding Arduino...")
    arduino_ports = [
    p.device
    for p in serial.tools.list_ports.comports()
    if 'Arduino' in p.description  # may need tweaking to match new arduinos
    ]
    if not arduino_ports:
        print("No Arduino found")
        return 0
    if len(arduino_ports) > 1:
        warnings.warn('Multiple Arduinos found - using the first')
    print("Using Arduino on "+str(arduino_ports[0]))
    return arduino_ports[0]

def GenerateGraph(PercentFilled, size, filledChar, UnfilledChar):
    """Generates a string containing a representation of how large a percentage is.

    PercentFilled: XX.XX percent to make a display for

    size: the total length of the graph

    filledChar: the character to use for filled in sections

    UnfilledChar: the character to use as filler for the rest of the graph."""
    workString = ""
    filledNumber = 0
    filledNumber = int(round(PercentFilled/100*size,0))
    workString = repeatCharacter(filledChar,filledNumber)
    workString += repeatCharacter(UnfilledChar,size-filledNumber)
    return workString







#ACTUAL CODE STARTS HERE
#First argument is time between updates in seconds
try:
    delay = float(sys.argv[1])
except:
    delay = 1

#open serial comms
OpenComPort()

#open the resource monitor class
monitor = Monitor(delay)

while True:
    i = input("Press enter to quit.\n")
    if not i:
        break

#Main thread only progresses once the user hits enter, the secondary thread provides the main usage anyways

# Close monitor
monitor.stop()

if ser.is_open:
    ser.close()