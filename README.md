# PicoCray
A system for running distributed code over multiple PI Picos 

## Wiring
Wire all of the picos in parallel connecting GPIO 10,11,12,13 and GND to all of the picos. 
Each pico needs powering separatly either from the USB BUS or 3V3 pin. 
Make the I2C bus as short as possible and you will need one pair of 4k7 Ohm resistors to +3v3 on GPIO 10 and 11 ideall in the Last Processor in the chain.
See Connections.txt and Picowiring.pdf

The Code is the same for all Pico's, to specify a controller add a wire from GPIO 22 to GND 
There must be (only) one controller in each cluster.

## Display
The Resulting Mandle is shown on a TJCTM24024-SPI 2.4 inch Touch 320x240 LCD display 
Wiring details are in the Connections.txt

## Touch 
A simple touch driver os provided to select the part of the mandle to zoom into

## Programming. 
Initially dump the uf2 file directly onto the picos USB drive as normal. 
As long as the code is running correctly you can use the USB serial speed change to set the pico into program mode. 

## Code
The calculations are sent as "Lumps" each lump is (currently) 120 calculations
Currently the picos will run multiple Mandlebrot calculations for LUMP(number) of steps from a starting double x and y co-ordinate and return LUMP integers of the itteration count, for each.
There are unions in common.h to structure the data sent/received. 
Data is sent and received as arrays of uint8_t by the controller to the Processors (proc's) via I2C

## Master/Slave Controller/Processor
I have left the code using these terms as the PI I2C slave examples use them, and if I change them, it would make a huge amount of work each time the examples were updated. 
I have changed the names of the picos to Controller/Proc (Processor) as this is not related to I2C operation.

## Proof of Concept. 
This code is only a proof of concept. For many uses, it actually runs slower than running on a single pico. This is a horses for courses situation. 
Currently I hope to develop this into a zoomable mandle display, where at greater zoom levels the calculation of ever smaller doubles will outweigh what is acheivable with a single Pico. Time will tell. 
Basically this project is a "Solution waiting for a Problem". 

## How it works.
Does it ? :)
 
On power up the Picos look at GPIO 22 if its low, if so that Pico becomes a Controller. Otherwise the pico is set as a Processor and is assigned an I2C address of 0x17 (yes, all of them) 

### Allocation of I2C addresses
When the Processor starts, it waits a random amount of time and them looks at the Assert line (GPIO 13) 
If this line is low the slave Asserts it high, and enables its I2C on 0x17
If the Assert line is high the Processor does nothing and waits a random amount of time. 
If the Processor has Asserted, the Controller will see a live device appear on I2C 0x17 and it will send a code via I2C to change the I2C address to the next free processor address. ( starting at 0x20)
On changing it's address the Processor releases the Assert line and waits for "questions" by setting its status byte to READY

### Running code. 
The code to run is compiled on all Processors. 
The Controller will find the first free Processor, and send it a "Question" of variables. These variables are set in a question union, and sent as the unions  associated array of uint8_t via I2C to the processor. The Controller records which questions have been sent to the Processor, so it can put the answers in to order when they are received.
The Go status flag is set by the Controller on the processor via I2C.
The Processor runs its code (currently a mandlebot running LUMP times for LUMP XY "Questions" ). The processor stores the LUMP answers  (an array unint16_t) as an coexisting array of uint8_t in to the I2C slaves accessable memory, and sets the DONE flag in the Processors Status byte. 
The Controller polls the Processors status for Done flags via I2C, and fetches the Answers via I2C as a uint8_t array converted to uint16_t's and stored into results pointed to by the processor it was assigned to. The Controller then sets the Prosessors Status flag to READY
This continues until all of the Questions have been asked, and all of the Processors are showing READY. 

### Display 
Outputs the resultant Mandle to 240x320 display using the ili9341
Defining USE_DMA in the ili9341.h changes the code to complete in a frame buffer then DMA to the display
if USE_DMA isn't configured, the display is added to as it arrives from the processors. 

###I2C
I've pushed the I2C to 3Mhz, its a trade off between speed and errors. Keep the wires short and terminate at both ends with 4k7's on both lines for best results.

### Test Code Mandlebrot. 
Currently the Pico Cray runs a distributed version of the mandlebot program. The calculation is done in 120 point "lumps" sent to each procesor in turn. Each processor splits the calculations into odd and even x positions to split them between cores. The results are sent back into the I2C memory and a done flag is set when both cores have completed. 
Taking a touch input allows you to zoom into the mandlebot to increasing "depth" until the precision of the double runs out (no frilly bits around the edges of the plot) 

### Palettes 
For the mandle display there are a number of palettes ranging from 16 colours to 128 these need to be compiled in (controller only) change the remarked includes at the top of common.h


# Warning
This code is all Proof of principle. Much of it works. Some of it gives reasonable results. Does it work, not for all cases. Does it give a good mandlebrot output. Well Yes

Basically if you are going to use this code to pilot a rocket, you will end up smashing into a planet at mach something rediculous. 
You have been warned. 


