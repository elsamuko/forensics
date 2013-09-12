#!/bin/bash


if [ -f /tmp/dump.html ]; then
    rm /tmp/dump.html
fi

exiftool -htmldump "$1" > /tmp/dump.html; firefox /tmp/dump.html


