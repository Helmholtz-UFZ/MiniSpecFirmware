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

Communication with the Board
----------------------------
To send commands to the board, a *Virtual COM Port* via USB is used.
Connect the USB, and look for the Port `/dev/ttyACMn` (n=0...) under Linux,
or `COMn` (n=0...) in the Device Manager in Windows. 

Then open a terminal and send the appropiate *User Commands* (see below).

**Attention:** Check that both jumper on **CN2** are removed, and the 
NRST is not connected (see also next section). 

Program the board
------------------

To programm the MCU on nukleo board use the tool *AC6* aka. *System Workbench for STM32*.

To successfully connect the board both jumper on **CN2** must be set and the NRST Signal between
the upper cuttable Part to the actual Nucleo Board must be wired. 
Therefor connect Pin5 on CN4 with Pin14 on CN7 with a simple flying wire.


Implementet User Comands
------------------------

Command 	| Short | Parameter	| Description 	                                       |
--------------- | ----- | ------------- | ---------------------------------------------------- |
**measure** 	| **m**	|		| Make a simgle measurement. Return the values or errorcode |
**stream**  	|	|               | Stream measurement and data. |
**end** 		| 	|               | End stream mode. |
**getdata** 	| **gd** 	|               | Return the data or errorcode of the last measurment. |
--------------- | ----- | ------------- | ---------------------------------------------------- |
**rtc?**   		| 	|               | Get the current Real-Time of the System. |
**ival?** 		| 	|               | Get the automatic measurement interval. `00:00:00` means the interval is disabled. |
**itime?** 		| **i?** 	|               | Get the current intergration time of the sensor in micro seconds [us] |
--------------- | ----- | ------------- | ---------------------------------------------------- |
**rtc=DATE** 	|	| DATE="20YY-MM-DDThh:mm:ss" | Set the Real-Time-Clock and the Calendar to the given Date and Time. Daylightsaving is not used. |
**ival=TIME** 	| 	| TIME="hh:mm:ss" | Set the automatic measurement interval. `00:00:00` disables feature. The maximum is `24:00:00`. |
**itime=N** 	| **i=N**	| N=[54..100000]| Set the integration time of the sensor in micro seconds [us] |
**format=B**	| 	| B={0,1}	| Set the output format to 0=Binary, or to 1=ASCII |
--------------- | ----- | ------------- | ---------------------------------------------------- |
**#debug**  	| 	|               | Enable debug prints. |
**#test**   	| **#t**	|               | (If enabled in Code) Run a special test function. |




