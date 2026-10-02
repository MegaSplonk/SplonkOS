# SplonkOS
Firmware for MegaSplonk Project


### ATTINY:

To verify that the attiny can communicate
```
avrdude -p attiny1614 -c serialupdi -P/dev/ttyUSB0 -vv -b 57600
```

To set the clock fuse

```
avrdude -p attiny1614 -c serialupdi -P/dev/ttyUSB0 -vv -b 57600 -U w:fuse2:0x2
```

to go fuck yourself. go ask emis :D

