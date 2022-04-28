Kim Due - a Kim-1 Clone
=======================

The original Kim Uno was a wonderful step forward in emulation and
pushed the Arduino Nano to its limits, in terms of both memory and speed.

I am thankful for that project.

Today we have some new hardware available that bring about new options.

I present, the Kim Due

Differences
-----------

The original Kim Uno had to make a number of trade offs to bring the best
experience avaiable with such little resources.  With only 2K of RAM, the
Uno/Nano could only emulate the basic Kim-1 with no additional features.

The Arduino Due runs on a 32-bit ARM machine and offers an amazing 98K
of RAM.   Why not take advantage of that?

    * 5K of KIM RAM
    * Hardware support that offers full LED bit mapping.
      * The KIM-UNO only partly enables the LED display, which prevents 
        many games from working
    * Optional use of an I2C LED bar, which greatly simplifies wiring
      (though the bar runs slower).



Credits
-------

Original source:

https://obsolescence.wixsite.com/obsolescence/kim-uno-summary-c1uuh 

Release to branch this project was granted via email:


>There's no license for the KIM Uno, it is freeware/open source/whatever
>you like to do with it. But thank you for checking, so - yes. Formally
>confirmed: do whatever you want with it! If you make an extended version
>(like an AIM?) I'll buy the kit for sure!
>
>Kind regards,
>
>Oscar.

As far as I can tell, Mike Chambers released a stripped down version of
6502 emulator for use on the Arduino. Although his Arudino release did not
contain a license, it seems to be based off earlier code with a very
permissive licence and I want to assure he gets proper credit here.


```
/* Fake6502 CPU emulator core v1.1 *******************
 * (c)2011 Mike Chambers (miker00lz@gmail.com)       *
 *****************************************************
 * v1.1 - Small bugfix in BIT opcode, but it was the *
 *        difference between a few games in my NES   *
 *        emulator working and being broken!         *
 *        I went through the rest carefully again    *
 *        after fixing it just to make sure I didn't *
 *        have any other typos! (Dec. 17, 2011)      *
 *                                                   *
 * v1.0 - First release (Nov. 24, 2011)              *
 *****************************************************
 * LICENSE: This source code is released into the    *
 * public domain, but if you use it please do give   *
 * credit. I put a lot of effort into writing this!  *
 *                                                   *
```