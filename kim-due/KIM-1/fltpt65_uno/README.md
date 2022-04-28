The version of this code was taken from

https://github.com/obsolescence/kim_uno/tree/master/sources/KIM%20Uno%206502%20ROM%20sources/fltpt65%20for%20KIM%20Uno


The original build instructions appear to be incomplete, but I have included
them here for historical reasons.

1. cba fltpt65_uno
1. hex2bin fltpt65_uno.hex
1. use a text editor to strip out the hex lines at the end of the file that have 0x3xx addresses.
1. use a hex editor like HxC and save as C source code to paste in to cpu.c

