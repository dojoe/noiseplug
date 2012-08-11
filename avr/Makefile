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
	