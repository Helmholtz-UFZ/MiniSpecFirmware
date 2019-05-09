Micro Spectrometer Firmware for STM32
=====================================

This repository provide the firmware for the *Nucleo Board* by ST, 
which is used by the *Microspectrometer Sensor Board*. This firmware can 
be used by the hardware version 2.0, and 2.1, so far all HW-versions that use
the Nucleo Board.

The hardware development is done in own repositories, one for each version.

HW_Version 2.0:
https://git.ufz.de/MET/WG6/Sensornetworks/Micro_Spectrometer/microspecsensorboard_v2_0

HW_Version 2.1:
https://git.ufz.de/MET/WG6/Sensornetworks/Micro_Spectrometer/microspecsensorboard_v2_1

Differences in usage with HW-Versions-Change 2.0 to 2.1
-------------------------------------

- Default USART-Interface is Uart1. Use PB7 for RX and PB6 for TX
    Use thes to connect them to CN3 rx to tx and tx to rx for Usb-Virtual-COM-Port communication.
- NRST (SB12) is not connected by default anymore. For Programming/Debugging connect manually: 
    connect CN4 Pin5 with CN7 Pin14
- SD Card is working now. Use a formated Fat32 SD card. CSV-Files are generated.
- DIP-Switch is connected physically to pins PA2,PA3 and PB15
- ParallelPort Data D3 is switched from PA2 to PA10
- ParallelPort Data D2 is switched from PA3 to PA8
- ADC-RESET signal removed
- ADC-PD signal removed

Communication with the Board
----------------------------
To send commands to the board, a *Virtual COM Port* via USB is used.
Connect the USB, and look for the Port `/dev/ttyACMn` (n=0...) under Linux,
or `COMn` (n=0...) in the Device Manager in Windows. 

Then open a terminal and send the appropiate *User Commands* (see below).

**Attention:** Check that both jumper on **CN2** are removed, and the 
NRST is not connected (see also next section) otherwise the board is reset on connect or 
the communication is corrupted. 


Program the board
------------------

**Copy-Paste Image**

The simplest method is to flash a binary image to the board. 
Therefore connect the Usb-Cable to the PC and wait until the board is
recognized as Device (Mass Storage like an ordinary USB flash drive). Then 
simply copy the imgage to this drive. During copy the LED (LD1) on the ST-Link 
should ficker greenish/yellowish and stop blinking if the flashing is done. 
Now you can start have fun :)



**with AC6 Tool**

To programm the MCU on nucleo board use the tool *AC6* aka. *System Workbench for STM32*.
To successfully connect the board **both jumper on CN2 must be set** and the NRST Signal between
the upper cuttable Part to the actual Nucleo Board must be wired. See *Differences in usage with HW-Versions-Change 2.0 to 2.1* for the correct Pins.



Implementet User Comands and Usage
------------------------

**Single Measurement**: As a human use `format=1` make the output readable. Use `i=` to set integration time and use `m` to measure and receive the data immedeatly. Use `gd` to receive the data again. As a machine use `format=0` and make yr admin contact us.

**Stream Measurement - Countiniously measure**: Set the format and the intergartion time as in *Single Measurement*. Use `stream` to start the stream and use `end` to end it.

**Automatic Measure by time with SD-Card**: 
A Alarm is set, different integration times and a iteration N. If then the alarm occur a multi-measurement is performed. 
For each set integration time, N measurements are made and the results are stored to the SD card. 
For generating the repeating alarm two modes are possible: *start-end* or *endless*. 
In *start-end-mode* one can choose the start- and end time and the interval. The alarm occur at start and then every time the interval
is reached until the end time. Between end and start nothing will happen. The end time needs to be *later* than the start time!
In the *endless-mode* the alarm occure every time the interval is reached - very simple - start and end are ignored. 
**This is how you do it...:**
* Set the iterations per integration time with `N=`.  
* Set the first integration time with `i=`. 
* Set the index to **1** by using `ii=1`. 
* Now you can set the next integration time with `i=` again. 
* Continue with the last steps (`ii=2`,`i=`,`ii=3`,...) until the number of integration times you want to set is reached, the maximum is 32. 
* May you want to check the result with `c?`. Also you can enable debug prints with `debug` and run a test multimeasurement with `mm`. 
The data from this test is not written to SD card.

Now set the **timings**. 
* First set the RTC with `rtc=`.
* Now choose your mode..
  * *endless* mode, use `ival=2,IVAL` for example `ival=2,00:15:00` to set the alarm to every 15 minutes and y're done.
  * *start-end* mode use `ival=1,IVAL,START,END`. e.g. `ival=1,00:10:00,04:30:00,18:00:00` - this set a alarm to every 10 minutes
if the time (RTC) is between half-past four and six-O'Clock evening. 
The times are inclusive so a measurement will happen at 4:30(!), 4:40, ... , 17:50, 18:00(!).

Further Notes: 
* To recall the settings to yr mind use `c?`
* With `ival=0` one can disable the interval - no measurements will occur. 
* Everytime `ival` is called and executed, the current multi-measure-config is written to the SD in a file called config. This includes
all integration times, the iteration value (N), the mode, the start and end time and the interval.
* If the board resets the config file is read. Unfortunally the RTC-time is impossible to recover.

Command                        | Short             | Brief description                                                     |
--------------------           | -----             | ------------------------------------------------------------          |
**measure**                    | **m**             | Make a simgle measurement. Return the values or errorcode             |
**multimeasure**               | **mm**            | Make a multi measurement. For manual testing use with debug on.       | 
**stream**                     |                   | Stream measurement and data.                                          |
**end**                        |                   | End stream mode.                                                      |
**getdata**                    | **gd**            | Return the data or errorcode of the very last measurment.             |
**rtc?**                       |                   | Get the current Real-Time of the System.                              |
**ival?**                      |                   | Get the current interval and mode in the format MODE,IVAL,START,END.  |
**config?**                    | **c?**            | Print current config info. For humans only.                           |
**itime?**                     | **i?**            | Get the current intergration time of the sensor in micro seconds [us] |
**itimeindex?**                | **ii?**           | Get the current index for setting the intergration time               |
**itime=[54..100000]**         | **i=[54..100000]**| Set the integration time of the sensor in micro seconds [us]          |
**itimeindex=[0..31]**         | **ii=[1..31]**    | Set the index for setting the integration time                        |
**iterations=[0..31]**         | **N=[1..31]**     | Set the repetitions of a measurement                                  |
**format={0\|1}**              |                   | Set the output format to 0=Binary, or to 1=ASCII                      |
**rtc=20YY-MM-DDThh:mm:ss**    |                   | Set the Real-Time-Clock and the Calendar. No Daylightsaving is used.  |
**ival=MODE,IVAL,START,END**   |                   | Set the regular automatic measurement.                                | 
**debug**                      |                   | Toggle debug prints on or off.                                        |

where
 * **MODE  ={0\|1\|2}**: 0:off, 1:start-end-mode, 2:endless-mode
 * **IVAL  =hh:mm:ss**: can omitted if MODE is 0
 * **START =hh:mm:ss**: can omitted if MODE is 0 or 1
 * **END   =hh:mm:ss**: can omitted if MODE is 0 or 1
