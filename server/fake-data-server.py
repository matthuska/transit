#!/usr/bin/env python2.7

import serial
import time
import sys
from datetime import datetime

port = '/dev/ttyACM0'
ard = serial.Serial(port,9600)
infile = open("fakedata.csv")
for l in infile:
    newdata = l.strip()
    ard.write((newdata + "\n").encode())
    print newdata
    time.sleep(1)
