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

- Default USART-Interface is now Uart4 (befor was Uart1). Use CN7 Pin1 for TX and CN7 Pin2 for RX.
    Use thes to connect them to CN3 rx to tx and tx to rx for Usb-Virtual-COM-Port communication.
- NRST (SB12) is not connected by default anymore. For Programming/Debugging connect manually: 
    connect CN4 Pin5 with CN7 Pin14
- SD Card is working now. Use a formated Fat32 SD card. CSV-Files are generated.


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




