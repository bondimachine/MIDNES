# MIDNES

Connect your NES / Famicom / Famiclone / Family Game to your MIDI controller thru a Pi Pico RP2040 and play your APU.

## Adapter

### Connection

Board pin out

```
GPIO2 -> DATA
GPIO3 -> LATCH
GPIO4 -> CLK
+5V -> +5V
GND -> GND
```

Connector pinout
```                         
                              +--------< DATA                
                              | +------> LATCH            +------------------ GND
                              | | +----> CLK              | 
                              | | |                       |             +---< DATA
        .-                    | | |                       |             |
 GND -- |O\              +-------------+               +-------------------+
 CLK <- |OO\ -- +5V       \ O O O O O /                 \ O O O O O O O O /
LATC <- |OO|               \ O O O O /                   \ O O O O O O O /
DATA -> |OO|                +-------+                     +-------------+
        '--'                 |   |                         |     |     |
                             |   +------ GND               |     |     +---- +5V
                             +---------- +5V               |     +---------: LATCH
                                                           +---------------> CLK
     NES port           9 pin famiclone port               15 pin famiclone port     
```

Connections from console side of the port. 

### Compiling

Programmed using [arduino-pico](https://github.com/earlephilhower/arduino-pico).

To compile choose Tools -> USB Stack -> "Adafruit TinyUSB Host".

You may need to update `tusb_config_rp2040.h` to enable USB MIDI Host support.

## Game ROM

TODO

