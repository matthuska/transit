#!/usr/bin/env python2.7

import urllib, json
import serial
import time
import sys
from datetime import datetime
#ard = serial.Serial('/dev/ttyACM0',9600)
ard = serial.Serial('/dev/ttyACM3',9600)

def download():
    # TODO error handling
    # this sometimes(?) does not work :(
    params = urllib.urlencode({'stopPointId': '940GZZLUKNB', 'app_id': 'dee78f0e', 'app_key': 'b06f1894af8c343cb0b3714b35987968'})
    #params = urllib.urlencode({'direction': 'inbound', 'stopPointId': '940GZZLUASL', 'app_id': 'dee78f0e', 'app_key': 'b06f1894af8c343cb0b3714b35987968'})
    # params = urllib.urlencode({'direction': 'inbound', 'stopPointId': '940GZZLUASL'})
    filehandle = urllib.urlopen('https://api.tfl.gov.uk/Line/piccadilly/Arrivals?%s' % params)
    result = filehandle.read()
    data = json.loads(result)
    lo,hi = sys.maxint,-sys.maxint-1
    for item in data:
       #print len(data),lo
       if (item['timeToStation'] < lo and item['direction'] == 'outbound'):
          lo,timestamp,vehicle = item['timeToStation'],item['timestamp'],item['vehicleId']
    time = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S.%fZ')
    return str(lo) + " " + str(int((time - datetime(2015,10,24)).total_seconds()*1000)) + " " + str(vehicle)

while True:
    try:
        newdata = download()
        print(newdata)
        sys.stdout.flush()
        #ard.write((newdata + "\n").encode())
    except:
        pass
    time.sleep(1)
