# Resource Monitor
A Python script and Arduino software to capture the current system utilization and display it on a WS2811 RGB LED array.

Simply upload the .ino to the arduino, connect the LED array to pin 12, and run the python script. The CPU, RAM, GPU0, and VRAM utilization will be displayed.

The Serial communication format is:\
[The update interval in seconds],[CPU load],[RAM usage],[GPU load],[VRAM usage]\
With all usages being expressed as percentages with a minimum of 1 decimal place, and a maximum of 2: xx.xx (eg: 56% is sent as 56.0, 3.08% = 3.08, 1% = 1.0)\
the ,[GPU load],[VRAM usage] may be omitted or repeated depending on the number of GPUs detected by the python script
***

## Arduino Software
[ResourceMonitor.ino](https://github.com/ZacTheHac/Resource-Monitor/blob/master/ResourceMonitor.ino)

Assumes an 18x4 WS2811 LED array is attached on pin 12.

The display is interpolated so as to smooth out the time between updates, but this also means that the graph does lag behind slightly.

Dependencies:
  * [FastLED](https://github.com/FastLED/FastLED)

## Python Script
[ResourceMonitor.py](https://github.com/ZacTheHac/Resource-Monitor/blob/master/ResourceMonitor.py)

Writen for Python 3.7.2, tested on 3.6.8

***Note: Only GPUs listed by nvidia-smd are supported at this time.***

Accepts 2 command-line arguments: 
* The time between updates in seconds (By default 1)
* The com port to send the data out through (By default will find and use the first Arduino on the system). Setting "None" disables the serial communication.

Dependencies:
  * [GPUtil](https://github.com/anderskm/gputil)
  * [psutil](https://pypi.org/project/psutil/)
  * [pySerial](https://pythonhosted.org/pyserial/index.html)