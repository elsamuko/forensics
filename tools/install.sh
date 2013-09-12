#!/bin/bash

echo "installing error level analysis"
gimptool-2.0 --install-script elsamuko-error-level-analysis.scm > /dev/null

echo "installing up down analysis"
gimptool-2.0 --install-script elsamuko-up-down.scm > /dev/null

echo "installing copy move"
CC=g++ CFLAGS="-O3 -msse2" LIBS=-lpthread gimptool-2.0 --install elsamuko-copy-move.cpp > /dev/null

echo "installing saturation"
gimptool-2.0 --install elsamuko-saturation.c > /dev/null

echo "installing LAB analysis"
CC=g++ LIBS="-lX11" CFLAGS="-O3" gimptool-2.0 --install elsamuko-lab-analysis.c > /dev/null

echo "installing HSV analysis"
gimptool-2.0 --install elsamuko-hsv-analysis.c > /dev/null