# BeltonC_LED

# Setup

## Install the touchscreen driver
-Download and install the touch driver
```
git clone https://github.com/goodtft/LCD-show.git
chmod -R 755 LCD-show
cd LCD-show/
sudo ./LCD5-show
```

## Compile the GUI
-Install qt files on RPI

For rpi 4
```
sudo apt-get install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools
```
if your application uses QSerialPort you need to run. (* THIS PROJECT USES QSerialPort *)
```
sudo apt-get install libqt5serialport5
sudo apt-get install libqt5serialport5-dev
```

-Clone the repository on RPI4 which will create a DarianB_Vitamin folder and put all of the GIT files in it
```
git clone https://github.com/mattymactutor/BeltonC_LED
```

-use qmake to generate the makefile. 
In this case, the .pro file should be called DarianB_Vitamin.pro but replace that with the name of your .pro file.
```
qmake -o Makefile BeltonC_LEDController.pro
```

-now use make which will call the Makefile you just created
```
make
```
-Run the file
```
./BeltonC_LEDController
```

-After updates are made to the code use this to rebuild
```
git pull
make
```
