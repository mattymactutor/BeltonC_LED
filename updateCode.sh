#1/bin/bash

cd BeltonC_LED
echo "Downloading new code files from Github..."
git pull
echo "Building qmake..."
sleep 1
qmake -o Makefile BeltonC_LEDController.pro
echo "Building runnable files..."
sleep 1
make
echo "Finished...closing in 15 seconds. You can now double click -Run LEDS Controller- on the desktop"
sleep 15
