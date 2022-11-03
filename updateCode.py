import os
import time

os.system("cd")
os.system("sudo rm -r BeltonC_LED")
os.system("git clone https://github.com/mattymactutor/BeltonC_LED")
time.sleep(2)
os.system("cd /home/pi/BeltonC_LED")
print("Building the make file with qmake...");
os.system("qmake -o Makefile BeltonC_LEDController.pro")
time.sleep(0.5);
print("Making new program...");
os.system("make")
time.sleep(0.5);
print("Running now...")
os.system("./BeltonC_LEDController")

