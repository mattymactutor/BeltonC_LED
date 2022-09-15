#!/bin/bash

echo "Removing old files..."
cd BeltonC_LED
make clean
echo "Downloading new code files from Github..."
git pull
echo "Building qmake..."
sleep 1
qmake -o Makefile BeltonC_LEDController.pro
echo "Building runnable files..."
sleep 1
make
echo "\n\nFinished...running now"
./BeltonC_LEDController
