Program/Connect to PC
---------------------
- NRST 
- CN2 Jumper
- Power On ? / Plugged In ?


Commands / Sending / Receiving
------------------------------

Problem: System does not react to commands, but i can see System output.
- is lightsleep mode enabled?
- commands need the /r (CR) terminator: eg. `measure\r` or `m\r`
- check term settings: 1152000 @ 8-N-1
- is RX/TX wired
- 
