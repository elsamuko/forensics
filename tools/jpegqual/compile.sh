
#!/bin/bash


 
gcc $(pkg-config --cflags gtk+-2.0) jpeg-quality.c jpeg-quality.h jpegqual.c -o jpegqual -ljpeg $(pkg-config --libs gtk+-2.0)
