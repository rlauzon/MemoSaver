# MemoSaver
An Arduino-based "hard drive" for the Text application the Tandy 100/102

# Hardware explanation

The base is an Arduino microcontroller.  I needed more memory than the Arduino Uno, so I went with the Arduino Mega to get enough SRAM.  The SD shield plus SoftwareSerial libraries just wouldn't fit in the Uno's small memory.

The microSD shield was a no-brainer.  The only thing with that is that I had to solder the SPI headers on since the microSD shield was made for the Uno and the usual pin outs were not compatible with the Mega.  The SPI header, though, lined up. [MicroSD Shield](https://www.sparkfun.com/products/12761)

The biggest pain was the RS-232 shifter.  RS-232 operates from 3.3V to 12V.  So we need something to 1. shift DOWN the voltage (because sending more than 5V down the Arduino's TTL lines would fry it) and 2. to shift UP the TTL voltage to something that my T102 would like to see.  After a few failures, I went with the [RS232 to 5V TTL Converter](http://www.serialcomm.com/serial_rs232_converters/rs232_rs485_to_ttl_converters/rs232_to_5v_ttl_converter/rs232_to_5v_ttl.product_general_info.aspx)
This shifter worked nice and did the hardware flow control cross over for me, so I could use a standard through 9-25 pin serial cable.  No Frankenstein cable needed.

Arduino pins used:
* 7, Gnd - Drive activity light
* A6, Gnd - Boot monentary switch
* 62 (A8) - Shifter TX
* 63 (A9) - Shifter RX
* SPI header for SD shield

Unavailable pins:
* 10-13
* 50-53
* 8 (card select)

Notes:
The Mega puts the SPI pins on 50-53 instead of 10-13.  So you need to solder the SPI header to the MicroSD Shield in order to move the pins.  As a result, pins 50-53 on the Mega are unavailable.  Also, because of how the MicroSD Shield is made, pins 10-13 are unavailable as well - if you use the stacking headers.  You can probably get around this by not using the stacking headers for pins D8-D13 and connecting stuff directly to the Mega and not through the MicroSD Shield.

Not all pins on the Mega and Mega 2560 support change interrupts, so only the following can be used for RX:
10, 11, 12, 13, 50, 51, 52, 53, 62, 63, 64, 65, 66, 67, 68, 69
So, since 10-13 and 50-53 are unavailable, I had to put the shifter on pins 62 and 63.

# Software explanation

You enter the Text application on the M100/102 and type in a text file.

The first line of the text file needs to be a MemoSaver marker - ## - followed by a command.

Commands:

 ##Save <filename>
 
 ##Load <filename>
 
 ##List
 
 ##Del <filename>

"Save" will save the text file to the SD card on the Arduino, status will be sent the next time the button is pressed.

"Load" will tell the Arduino to send the file the next time the button is pressed.

"List" will send a list of files on the Arduino when the button is pressed.

"Del" will delete a file on the SD card.  Status will be returned the next time the button is pressed.

Results are *not* stacked.  If you send a "Del" command, then send a "List" command, the results of the "Del" command are lost.

Example 1:
Go into Text and start a new text file.  Call it whatever you want.

Enter this text:

 ##List<press enter>
 
Press F3 (Save) and for the file name use "com:58n1e"

Press F2 (Load) and for the file name, again use "com:58n1e"

Press the button on the Arduino side to tell it to start sending.

The list of files on the SD card will be loaded into the current text file.

The file names will be listed along with their size in bytes.

Example 2:

Go into Text and start a new text file.  Call it whatever you want.

Enter this text:

##load file1.dat<press enter>

Press F3 (Save) and for the file name use "com:58n1e"

Press F2 (Load) and for the file name, again use "com:58n1e"

Press the button on the Arduino side to tell it to start sending.

The contents of file1.dat from the SD card will be loaded into the current text file.

Example 3:

Create a new text file with some content, or use a current text file.

Enter this text AS THE FIRST LINE OF THE text file:

##save newfile.dat<press enter>

Press F3 (Save) and for the file name use "com:58n1e"

Optionally, you can press F2 (Load) and for the file name, again use "com:58n1e"

Press the button on the Arduino side to tell it to start sending.

The results of the save operation will be loaded in to the current text file.

Example 4:

Go into Text and start a new text file.  Call it whatever you want.

Enter this text:

##del file1.dat<press enter>

Press F3 (Save) and for the file name use "com:58n1e"

Press F2 (Load) and for the file name, again use "com:58n1e"

Press the button on the Arduino side to tell it to start sending.

The results of the delete operation will be loaded into the current text file.


In case you didn't see, the speed is limited to 1200 BPS.  I tried higher BPS but something would stop the transmission from the 100/102 side after about 80 bytes.  Lowering the speed to 1200 BPS was the only way to fix it.
