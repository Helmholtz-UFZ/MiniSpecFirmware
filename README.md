
<a href="https://www.ufz.de/index.php?en=33573">
    <img src="https://github.com/user-attachments/assets/1473d383-34d8-4ce1-82cc-d16e7eddfa3b" width="400"/>
</a>

<a href="https://www.ufz.de/index.php?en=45348">
    <img src="https://github.com/user-attachments/assets/57f0a57d-9991-4641-bbf8-b070567cca83" align="right" width="220"/>
</a>



Micro Spectrometer Firmware for STM32
=====================================

This repository provide the firmware for the *Nucleo Board* by ST. 
Together with the custom PCB (version `2.1`) this firmware can 
be used to control a Hamamatsu C12880MA Mini-Spectrometer.  

The hardware development of the cutom PCB can be found here:
https://github.com/Helmholtz-UFZ/MiniSpecHardware

Feature Overview
----------------
* simple single measurement 
* multi-measurement (MM) with up to `N` repetitions for each of `M` configurable integration times
* SD card support for measurements and configuration
* RTC with possible battery backup (battery not included ;) )
* configurable integration time, with auto-adjust support
* alarm-driven MMs in periodic intervals with or without start- and end-time
* pin-triggered MMs
* different sleep modes for ultimative power-save
* simple on-the-fly configuration and communication via UART 


Hardware nomenclature
---------------------
* **HW**: Hardware
* **FW**/**SW**: Firmware/Software (refer here to the same thing, although it is technically not fully correct) 
* **Microspectrometer Sensor Board** or simply **the System** : the whole thing
* **Nucleo Board**: the whole upper light bluish board with the *ST* logo
* **UFZ-PCB**: the whole lower dark green board with the *UFZ* logo
* **ST-Link**: the cuttable, smaller part of the nucleo board
* **the Sensor**: the actual micro-spectrometer on the UFZ-PCB


Important HW Pins
-------------------------------------
- Default USART (Uart1): Rx:`CN7 Pin21 (PB7)`, Tx:`CN10 Pin17 (PB6)` (for using the (USB) Virtual-COM-Port via the ST-Link, connect Rx to `CN3 Pin2 (tx)` and Tx to `CN3 Pin1 (tx)`) 
- NRST: `CN7 Pin14` (for programming) connect to `CN4 Pin5` 
- CMDS_EN/TRIGGER: `CN10 Pin18 (PB11)` (switch deep- and light-sleep / used as trigger in trigger-mode)
- two jumper on `CN2` - needed for programming 


Communication with a (already) running System
---------------------------------------------

**direct USART** 

For direct USART communication with the system connect to 
Rx (`CN7 Pin21 (PB7)`) and Tx (`CN10 Pin17 (PB6)`) 
to your USB-TTL-Converter or to any device that can send and receive TTL-Rs232 messages.

Disable the *Deep-Sleep-Function* by setting the *CMDS_EN*(`CN10 Pin18`) high by simply wire *3.3V* (eg.`CN7 Pin16`) to it. 

**USB**

If you want to connect to a already running system, firstly remove both jumper on **CN2**, and the disconnect the NRST (see also next section) otherwise the board is reset on connect.

If you want to use the integrated *Virtual COM Port* via USB 
wire the nucleo USART pins to the ST-Link: `CN7 Pin21 (PB7)` to `CN3 Pin2 (tx)` and `CN10 Pin17 (PB6)` to `CN3 Pin1 (tx)`.
Connect the USB, and look for the Port `/dev/ttyACMn` (n=0...) under Linux,
or `COMn` (n=0...) in the Device Manager in Windows. 

Disable the *Deep-Sleep-Function* by setting the *CMDS_EN*(`CN10 Pin18`) high by simply wire *3.3V* (eg.`CN7 Pin16`) to it. 

Open a terminal and use a serial-commandline-tool (cutecom, minicom, putty...). 
Use `115200 @ 8-N-1` (115200 baude, 8bit, no parity, 1stopbit).

If you did it right use the implemented *User Commands* (see below) to configure the system and/or do measurements. (May try `help` first to check if your connection works).


Programming the System / Flashing a new Image
---------------------------------------------

**Copy-Paste Image**

The simplest method is, to flash a binary image directly to the System. 
Therefore connect the Usb-Cable to the PC and wait until the System is
recognized as *Mass Storage Device* (like an ordinary USB flash drive). Then 
simply copy the image to this drive. During copy the LED (LD1) on the ST-Link 
should ficker greenish/yellowish and stop blinking if the flashing is done. 
Now you can start have fun :)


**with AC6 Tool**

To manually program the nucleo-board, use the tool *AC6* aka. *System Workbench for STM32*.
To successfully connect the board **both jumper** on `CN2` must be **set** and the `NRST` signal between
the ST-Link and the nucleo board must be wired (connect `CN4 Pin5` with `CN7 Pin14`) also
make sure that the board is (externally) powered.
See also *Differences in usage with HW-Versions-Change 2.0 to 2.1*. 


Usage Cookbook 
--------------


**Light vs. Deep Sleep Mode**: 

If the `CMDS_EN` pin is high the system switch to *light sleep mode* (LSM). 
Only in LSM the system can receive commands from the user, but the system has a higher power consumption than 
in *deep sleep mode* (DSM), which is designed for stand-alone, long-term driven usage. 
In DSM only a reduced set of actions and functions remains active or available: 
* RTC still active
* alarms still functional (if enabled) 
* the trigger is functional (if enabled) 
Of course the switch to LSM is always possible.


**Modes**: 

There are 4 Modes, which change the main behavior of the system:
* `0 - off`: the system do nothing without user-interaction
* `1 - ival1`: endless interval mode   - the system perform a periodic multi-measurement(MM) 
* `2 - ival2`: start-end interval mode - the system perform a periodic multi-measurement(MM) if the current time is between a stored start- and an end-timestamp.
* `3 - triggered`: triggered mode: the system perform a multi-measurement(MM) if the `TRIGGER`Pin becomes high.
The modes can be swiched by the `mode=` command. 


**Single measurement**: 

* As a human use `format=1` make the output readable. 
* Use `i=` to set integration time and use `m` to measure and receive the data immediately. 
* Use `gd` to receive the data again. As a machine use `format=0` and make yr admin contact us.
Note: data is not stored to SD


**Multi measurements**

* use `ii=` to set the index to an number
* use `i=` to set the integration time (at last choosen index position)
* repeat `ii=` and `i=` as many integration times you want to use (max. 32)
* use `N=` to set the number of repetitions per integration time
* (optional) check the config result with `c?`
* (optional) store your config to SD with `stcf`
* (optional) use `mm` to make a MM (measurement is not stored to SD)  

**Auto-adjust integration time**

To use auto-adjust:
* (optional) use `aa=` to set `UPPER` and `LOWER` 
* (optional) use `aa` to test auto-adjust params
* (optional) `stcf` to store the params to sd.
* use `i=-1` (negative value) to make the system automatically adjust the integration time shortly before a measurement is made.

Default values: `LOWER=33'000` and `UPPER=54'000`

Note: Bear in mind that auto-adjusting need a bit of time. Normally less then 4 (internally) measurements are needed, 
to find a acceptable integration time. In dark conditions this would need ~4 seconds. 
The worst case scenario is ~17sec (see the discussion of the problem in the end of this document).

*How it works:*

The auto-adjustion uses a *modified binary search* and works like this:
* a measurement is made (intial itime: ~500'000 us)
* the maximum value (`maxval`) in the data is searched
* tree cases can occur
	1. `maxval < LOWER`, adjust the itime to a higher value
	2. `LOWER < maxval < UPPER` a suitable itime was found
	3. `UPPER < maxval`, adjust the itime to a lower value
* in case 1. or case 3. we start over with the new itime
* after 16 iterations, we return the itime to prevent a infinite loop (e.g with fast swiching light conditions).

Note: `UPPER` and `LOWER` are both absolut values. The do not handle dark-current, nor saturation. 
The former normally lies around 6000 the latter around 60'000, but this can vary from sensor to sensor.
Hence these values should not be exceeded. Also do not set `UPPER` and `LOWER` very close, because this will 
increase mean search time. Calculate your params as following:
* `LOWER= darkcurrent + 0.5 (saturation - darkcurrent)`
* `UPPER= darkcurrent + 0.9 (saturation - darkcurrent)`, but `UPPER= 0.9 saturation` is also sufficient.


**Automatic time measurements with SD-card**: 

* set up a MM (see *Multi measurements* )
* set a mode:
  * use `mode=1,IVAL` for example `mode=1,00:15:00` to set the alarm to every 15 minutes or
  * use `mode=2,IVAL,START,END`. e.g. `mode=2,00:10:00,04:30:00,18:00:00` - to make a MM every 10 minutes if the time is between half-past four and six-O'Clock evening. The times are inclusive so a measurement will happen at 4:30(!), 4:40, ... , 17:50, 18:00(!).
* use `stcf` to store the jsut setup config to the SD card
* optionally disconnect `CMDS_EN` to enter deepsleep
Note: All measurements are stored to SD


**Triggered measurements with SD-Card**: 

* set up a MM (see *Multi Measurements* )
* use `mode=3` to enable trigger mode.
* use `stcf` to store the just setup config to the SD card
* disconnect `CMDS_EN` to enter deepsleep
* (external) set the `TRIGGER` Pin to high for at least `100 ms`. 

Note: All measurements are stored to SD

Note: The `TRIGGER`-Pin and the `CMDS_EN` Pin are the same !


**SD card**

In the config on the SD the following is stored:
* if format bin or ascii is used
* the current mode
* the debug level
* the `UPPER` and `LOWER` for auto-adjust
* 32 integration times
* the current integration time index 
* measurement repetitions aka. `N`
* start-time, end-time, interval

If you are satisfied with your timing and measurement configuration (check with `c?`), 
you can store the config on the SD card with `stcf`. 
These settings are automatically loaded on system-reset or power-loss. 
To see what is stored in the config on the SD use `c?sd`.
To manually read back the config that is stored on the sd and apply it to the running system (overwrite current config), use `rdcf`.


User Commands 
-------------

Command                      | Short           | Brief description                                                     |
--------------------         | -----           | ------------------------------------------------------------          |
`help`                       | `h`             | Print a even briefer help.                                                   |
`version`                    |                 | Print the current Firmware vewrsion |
`storeconf`                  | `stcf`          | Store the system config to the SD.                 |
`readconf`                   | `rdcf`          | Read and apply the SD-config to the System.                |
`measure`                    | `m`             | Make a simgle measurement. Return the values or errorcode             |
`multimeasure`               | `mm`            | Make a multi measurement.        | 
`getdata`                    | `gd`            | Return the data or errorcode of the very last measurment.             |
`auto-adjust`                | `aa`            | Test the automatic adjustion of the integration time             |
`itime?`                     | `i?`            | Get the intergration time for the current index position. in micro seconds [us]           |
`rtc?`                       |                 | Get the current Real-Time of the System.                              |
`config?`                    | `c?`            | Print current config info. For humans only.                           |
`config?sd`                  | `c?sd`          | Print the config that is currently stored on the SD. |
`debug=[0..3]`               | `dbg=[0..3]`    | Set the debug level. 0 - off, 1 - some, 2 - many, 3 - all |
`itime=[54..100000]`         | `i=[54..100000]`| Set the integration time (at the current index position) of the sensor in micro seconds [us], negative values set to auto-adjust.          |
`auto-adjust=L,U`            | `aa=L,U`        | Set the automatic adjustion parameters.             |
`itimeindex=[0..31]`         | `ii=[1..31]`    | Set the index for setting the integration time                        |
`iterations=[0..31]`         | `N=[1..31]`     | Set the repetitions of a measurement.                                 |
`format={0\|1}`              |                 | Set the output format to 0=Binary, or to 1=ASCII                      |
`rtc=20YY-MM-DDThh:mm:ss`    |                 | Set the Real-Time-Clock and the Calendar. No Daylightsaving is used. YY > 0 ! (see also Important Notes) |
`mode=MODE,IVAL,START,END`   |                 | Set the system mode |

where
 * `MODE`  = `0`: off, `1`: ival1, `2`: ival2, `3`: triggered
 * `IVAL`  =`hh:mm:ss`: omit if MODE is `0` or `3`
 * `START` =`hh:mm:ss`: omit if MODE is `0`,`1` or `3`
 * `END`   =`hh:mm:ss`: omit if MODE is `0`,`1` or `3`
 * `0 < dark-current < L < U < saturation < (2^16)-1`


Important Notes
---------------

**Never use the year 2000 for the RTC, in combination with a backup batterie**

A problem will arise, iff:
 * the RTC year was set to 2000
 * backup batterie on VBAT
 * a VDD power breakdown in the very same year (2000)

On power restore, the device checks if the RTC was initialized, by checking the year. 
As the last two digits of the year are 0, these are equal to the default value, 
which indicates, that the RTC wasn't initialiezed and it is done then, whereby the RTC will set to its default values.

Timing problems with auto-adjust integration time
------------------------------------------------
A short light burst during a auto-adjustment can lead to a very long adjustion cycle. 
Due the use of a binary search, up to 16 adjustments and therefore 16 measurements are done. 
Every measurement take at least as long as the used interation time (plus a little overhead of a few ms). 
If a light burst occur while the search just started in the upper half of 
the search interval [(max. itime)/2, max. itime], the
algorithm 'thinks' it should search lower. All further corrections are done upwards to compensate the burst.
In the end one initial measurement (~0.5s) and 17 measurements of nearly one second each, are made. 
 
worst case (~17 sec):
```
[20:15:32:446] dbg: sensor curr itime: 499973 us   // dark
[20:15:32:951] dbg: sensor curr itime: 1000000 us  // suddenly light -> down correction
[20:15:33:956] dbg: sensor curr itime: 750000 us   // dark again (till end) -> up correction
[20:15:34:711] dbg: sensor curr itime: 875000 us
[20:15:35:591] dbg: sensor curr itime: 937500 us
[20:15:36:533] dbg: sensor curr itime: 968750 us
[20:15:37:507] dbg: sensor curr itime: 984375 us
[20:15:38:496] dbg: sensor curr itime: 992187 us
[20:15:39:493] dbg: sensor curr itime: 996093 us
[20:15:40:493] dbg: sensor curr itime: 998046 us
[20:15:41:496] dbg: sensor curr itime: 999022 us
[20:15:42:499] dbg: sensor curr itime: 999510 us
[20:15:43:504] dbg: sensor curr itime: 999754 us
[20:15:44:508] dbg: sensor curr itime: 999876 us
[20:15:45:512] dbg: sensor curr itime: 999937 us
[20:15:46:516] dbg: sensor curr itime: 999967 us
[20:15:47:521] dbg: sensor curr itime: 999982 us
[20:15:48:525] dbg: sensor curr itime: 999989 us
[20:15:49:533] -> integration time = 999992 us	   // returnd result
```






