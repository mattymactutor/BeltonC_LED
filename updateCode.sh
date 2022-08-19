#1/bin/bash

cd BeltonC_LED
echo "Building qmake..."
sleep 1
qmake -o Makefile BeltonC_LEDController.pro
echo "Building runnable files..."
sleep 1
make
echo "Finished...closing in 15 seconds"
sleep 15