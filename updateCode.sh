#!/bin/bash

echo "Removing old files..."
sudo rm -r BeltonC_LED
echo "Downloading new code files from Github..."
git clone https://github.com/mattymactutor/BeltonC_LED
cd BeltonC_LED
echo "Building qmake..."
sleep 1
qmake -o Makefile BeltonC_LEDController.pro
echo "Building runnable files..."
sleep 1
make
echo "\n\nFinished...closing in 15 seconds. You can now double click -Run LEDS Controller- on the desktop"
sleep 15
