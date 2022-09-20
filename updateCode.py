import os
import time

os.system("cd")
os.system("cd /home/pi/BeltonC_LED")
print("Removing old files...")
os.system("make clean")
time.sleep(0.5)
print("Downloading new files...")
os.system("git pull");
time.sleep(0.5)
print("Building the make file with qmake...");
os.system("qmake -o Makefile BeltonC_LEDController.pro")
time.sleep(0.5);
print("Making new program...");
os.system("make")
time.sleep(0.5);
print("Running now...")
os.system("./BeltonC_LEDController")

