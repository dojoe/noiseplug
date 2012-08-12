# This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
# To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
# or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

all: flash

noiseplug.o: noiseplug.S
	avr-gcc -Os -nostdlib -mmcu=attiny9 -o $@ -c $<

noiseplug.elf: noiseplug.o
	avr-gcc -Os -nostdlib -mmcu=attiny9 -o $@ $<

noiseplug.hex: noiseplug.elf
	avr-objcopy -O ihex $< $@

flash: noiseplug.hex
	avrdude -cavrisp2 -Pusb -pt8 -Uflash:w:$<:a

dump: noiseplug.elf
	avr-objdump -Sr $< | less
	