# !/bin/bash

PROG=main
MCU=atmega128

avr-gcc -std=c99 -O0 -g -mmcu=$MCU -c src/$PROG.c -DSOUND_ON
avr-gcc -g -mmcu=$MCU $PROG.o -o $PROG.elf
avr-objcopy -j .text -j .data -O ihex $PROG.elf $PROG.hex

sudo /home/embedsys/local/bin/avrdude -p m128 -c jtag1 -e -U flash:w:$PROG.hex -P /dev/ttyUSB0
