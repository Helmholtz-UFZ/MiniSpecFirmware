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
or COMn (n=0...) in the Device Manager in Windows. 

Then open a terminal and send the appropiate *User Commands* (see below).

**Reconect to a running board**
If one want to connect to a running system and send commands both jumper on **CN2** must be removed.

**Programm the board with AC6 aka. System Workbench for STM32**
If one want to program the board both jumper on **CN2** must be set and the NRST of the Nucleo
must be wired. Therfore connect Pin5 on CN4 with Pin14 on CN7.

Implementet User Comands
------------------------

measure
m

stream

end

getdata
gd

#DEBUG#

rtc?

ival?

itime?
i?

itime=N N=[54..100000]
i=N

format={0,1}

rtc=DATETIME
DATETIME="20YY-MM-DDThh:mm:ss"

ival=TIME
TIME="hh:mm:ss"
if 0:0:0 the interval is disabled.
as a special case 24:00:00 is allowed, to allow dayly measurements.




