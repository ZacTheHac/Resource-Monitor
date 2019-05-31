# Resource Monitor
A Python script and Arduino software to capture the current system utilization and display it on a WS2811 RGB LED array.
Simply upload the .ino to the arduino, connect the LED array to pin 12, and run the python script. The CPU, RAM, GPU0, and VRAM utilization will be displayed.

***

## Arduino Software
By default, the software assumes an 18x4 array is attached on pin 12.
The display is interpolated so as to smooth out the time between updates, but this also means that the graph does lag behind slightly.

## Python Script
Accepts 2 arguments: 
* The time between updates in seconds (By default 1)
* The com port to send the data out through (By default will find and use the first Arduino on the system). Setting "None" disables the serial communication.