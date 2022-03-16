# TMS9929A/TMS9928A/TMS9918A VDP Alternative VRAM Solution
I rediscovered my own TMS9929A experience from the 1980's, and decided to recreate a TMS9929A setup using the latest optimal video output.  

As part of this journey I designed an improved alternative Video RAM solution, and wrote an Arduino Sketch to fully test the improved VRAM solution with multiple pass, multiple bit-pattern tests.

My alternative VRAM solution comprises just 4 ICs (totaling just 70 pins), significantly less than the original 128 pins (8 ICs) or the 102 pins (5 ICs) of a 2010 published Static RAM solution.

Next step will be implementing the successfully tested circuit onto a Minimalist Europe Card Bus (MECB) board, to allow me to play with this VDP with any 8-bit CPU or memory map that I wish to explore.

The TMS99xx_VDP_Test folder contains the Arduino VDP & VRAM testing sketch, which I also walk-through in the linked Youtube video. 

For more information and a Youtube design & testing walk-through video:
https://digicoolthings.com/tms9929a-vdp-rediscovery-and-alternative-vram-solution

