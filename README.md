# PicoCray
A system for running distributed code over multiple PI Picos 

## Wiring
Wire all of the picos in parallel connecting GPIO 10,11,12,13 and GND to all of the picos. 
Each pico needs powering separatly either from the USB BUS or 3V3 pin. 
Make the I2C bus as short as possible and you will need one pair of 4k7 Ohm resistors to +3v3 on GPIO 10 and 11

The Code is the same for all Pico's to specify a controller, add a wire from GPIO 22 to gnd 
There must be one controller in each cluster.

## Programming. 
Initially dump the uf2 file directly onto the picos USB drive as normal. 
As long as the code is running correctly you can use the USB serial speed change to set the pico into program mode. 

## Code
Currently the picos will run a Mandlebrot calculation for 10 double x,y co-ordinates and return 10 integers of teh count for 1-10
there are unions in common.h to structure the data sent/received. 
Date is sent and received as arrays of uint8_t by the controller to the Processors (proc's) 

## Master/Slave Controller/Processor
I have left the code using these terms as the PI I2C slave examples use them, and if I change them it would make a huge amount of work each time they were updated. 
I have changed the names of the pico to Controller/proc (Processor) as this is not related to I2C operation

## Proof of Concept. 
This code is only a proof of concept. For many uses, it actually runs slower than running on a single pico. This is a horses for courses situation. 
Currently I hope to develop this into a zoomable mandle display, where at greater zoom levels the calculation of ever smaller doubles will outweigh what is acheivable with a single Pico. Time will tell. 
Basically this project is a "Solution waiting for a Problem". 

## How it works.
Does it ? :) 
On power up the Picos look at GPIO 22 if its low, then that Pico becomes a Controller. Otherwise the pico is a Processor and is assigned an I2C address of 0x17 (yes all of them) 

### Allocation of I2C addresses
When the Processor starts, it waits a random amount of time and them looks at the Assert line (GPIO 13) 
If this line is low the slave Asserts it high, and enables its I2C
If the Assert line is high the Processor waits a random amount of time. 
If the Processor has Asserted, the Controller will see a live device on I2C 0x17 and it will send a code to change the I2C address to the next free processor address. ( starting at 0x20)
On chaning it's address the Processor releases the Assert line and waits for "questions" by setting the status byte to READY

### Running code. 
The code to run is compiled on all Processors. 
The Controller will find the first free Processor, and send it a Question of variables. These variables are set in a question union, and sent as the unions  associated array of uint8_t to the processor. The Controller records which questions have been sent to the Processor, so it can put the answers in order when they are received.
The Go status flag is set by the Controller on the processor via I2C.
The Processor runs its code (currently 10 times for the 10 XY questions for the mandlebrot code. The processor stores the 10 answers  (an array unint16_t) as an array of uint8_t in the I2C accessable memory, and sets the DONE flag in the Processors Status byte. 
The Controller polls the Processors for Done flags and fetches the Answers via I2C as a uint8_t array converted to uint16_t's and stored into results pointed to by the processor it was assigned to. The Controller then sets the Prosessors Status flag to READY
This continues until all of the Questions have been asked, and all of the Processors are showing READY. 
