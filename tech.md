Some tech details about the Noiseplug
=====================================

A fan, Fleck, asked me for details about how the audio output works. I was feeling especially rambly that night, so I wrote the Great American Novel in reply. As this might be of interest to others, I thought I'd also put it here.

Note that whenever I refer to a "faster clock", I'm referring to the fact that he's trying to get it to run on an ATtiny13A, which has a 9.7 MHz internal oscillator as opposed to the 8 MHz one in the ATtiny9 I used.

On 10.06.2013 13:43, Fleck wrote:

> big problem for now is that I don't understand - how sounds are made!
> Maybe you could point out some links/manuals/tutorials where I can
> learn step by step - how do I create 440Hz for example sq.wave and
> triangle or sine wave notes... Cause I don't get it, timer int runs
> 37500 times a second, far beyond ear limits, so... What we do to hear
> sound?

Hah, that's actually something I can explain!

PWM-based audio generation
--------------------------

The basic idea (you probably already figured this out) is that I generate the sound using PWM; similar to how you would dim a LED, just with a faster changing output value.

It's correct that I set up the timer to run at 32K PWM periods per second (37,5K on your chip), which would correspond to a sample rate of 32 kHz if I were to change the sample value (OCR0A) after each PWM period[1].

Now, I do not intend to run at that high a sample rate, nor do I have enough processing power to generate that many samples in real time, so I divide the sampling rate by four simply by only changing OCR0A every four PWM periods. This gives me a sample rate of 8 kHz, with a theoretical Nyquist frequency of 4 kHz.

Why am I not just running a slower PWM, you ask? Well, the main reason is that the PWM, being essentially a square wave, introduces its own set of distortions and overtones. The faster I run the PWM, the higher up (and therefore out of the audible range) my overtones. If I ran the PWM at normal 8 kHz, it would cause a constant, high-pitched beep all over the music.
 
Another arguable advantage of this technique is that this very basic sample rate conversion (if you want to call it that), using a simple step function, introduces all kinds of aliasing harmonics. While normally unwanted, this aliasing is what gives the Noiseplug its treble-rich sound although it's only producing samples at 8 kHz. This is also why the C prototype version sounds so dull compared to the actual plug -- it's missing the aliasing because the 8kHz are properly upsampled to whatever the soundcard can output and are put through the card's reconstruction filter.

How it works in code
--------------------

For the details, please take a look at the very basic framework code
from the commit I linked to: https://github.com/dop3j0e/noiseplug/commit/62c720ab1bfa62de564a630cafb3bb62ce006703

As you can see, the interrupt routine does nothing but increment int_ctr modulo 4. The main loop below sleeps until int_ctr reaches zero, then starts computing a new sample and outputs it to OCR0A when it's done. This makes sure that every four interrupts, a new sample is generated.

Also, there's a 24-bit loop counter, i, that increments on each sample. This is the basic system time that drives most of the song[2]. The test code in the framework simply uses the second byte of that counter to generate the next sample, which gives you a sawtooth wave that increments every 256 samples, wrapping back to zero every 64K samples. You might notice that this gives you a frequency of 1/8 Hz ;) So you won't hear anything, but if you hook up your scope to the PWM pin, you should be able to actually see a PWM wave that's slowly incrementing with a period of 8 seconds (less with your higher clock frequency).

I used a scope for debugging a lot; you'll notice the two instructions setting and clearing bit two of PORTB, before and after the sample calculation. This is pure debug code so I could see on my scope how long my sample generation code was taking.

How I generate samples
----------------------

As for generating actual square or triangle waves, lft's seminar might be a good start: http://www.linusakesson.net/music/elements/index.php

The oscillators in the Noiseplug are simple 16-bit accumulators. I choose which note they play by choosing the value I add to them each sample. I generate the sample by shifting and masking the accumulator values -- a good example is the voice_bass() function in the C prototype:

	unsigned char ret = ((bassosc >> 8) & 0x7F) + ((flangeosc >> 8) & 0x7F);

I have two oscillators, slightly detuned for a flanger effect. I take the almost-highest seven bits of the accumulators for a sample value, giving me a nice sawtooth wave. Taking bits near the MSB gives me slowly changing values, and shifting by eight means I can just take the upper byte of the two-byte value instead of shifting.

For a square wave, I just pick a single bit from the oscillator and choose a sample value of zero or non-zero based on that bit:

	return (arp_osc & (1 << 12)) ? 0 : 35;

I can tune the voice in octaves by simply changing which bit to pick.

Basic note values
-----------------

As for what value to add for a C, C#, D and so on, I used a table of basic note frequencies and converted them using a spreadsheet -- see win/nodes.ods in the Github repo for that. Column B is the note frequency in Hz, C is the corresponding period in samples, based on my sample rate of 8 kHz. In D, I multiplied the periods with 2 or 4 to pitch them down one or two octaves respectively, giving me a two-octave scale. In column E, finally, I get my "how much to add to the accumulator" value by asking: "If I had an accumulator that wraps after 16K, how much would I have to add per sample to make it wrap every N samples?". I could have gotten there much easier had I known from the start what I was doing, but there you have it =)

Hint: As your clock is running faster, try changing the 8K in the formulas for column C to 9375. That should give you the Noiseplug's original pitch back.

Guess in which table in the firmware these note values ended up ;)

That's all, hope it helps!
  Joachim

[1] Note that even that sample rate would give you a Nyquist
frequency of 16 kHz, which is actually within the audible range.

[2] In the final version, I reserved three registers solely for i because I was running out of RAM, so you won't find any explicit memory location called "i" there.
