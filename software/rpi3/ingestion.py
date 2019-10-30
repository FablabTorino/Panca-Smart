#!/usr/bin/python3

import serial
import calendar
import time
import json
import io

#ser = serial.Serial('/dev/pts/2')
ser = serial.Serial('/dev/ttyACM0')
ser.flushInput()
ser_bytes = None
time.sleep(3)

#Aggiorna data arduino
ts = calendar.timegm(time.gmtime())
#print(ts)

# TODO Data request when needed
#ser.write(("data\n".encode()))
ser.write(str.encode("time " + str(ts) +"\n"))

while True:
     ser_bytes = ser.readline()
     try:
        f = json.loads(ser_bytes)
        with open('data.json', 'a') as outfile:
               json.dump(f, outfile)
        #print (f)

     except json.JSONDecodeError as e:
         print (ser_bytes.decode("utf-8"))
#         #pass
#         print (ser_bytes)
#         errors = open('data.exceptions', 'a')
#         errors.write = (ser_bytes)
#         errors.close()

     except UnicodeDecodeError:
       pass
#     except:
#         pass
#         print (type(ser_bytes))
#         print (ser_bytes.decode("utf-8"))
