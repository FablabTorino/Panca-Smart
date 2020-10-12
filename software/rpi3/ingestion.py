#!/usr/bin/python3

import serial
import calendar
import time
import logging
import os

import kupiki
from arduserial import ArduSerial

"""
config 
"""
SERIAL_PORT = '/dev/ttyACM0'

logging.basicConfig(format='%(asctime)s : %(threadName)s : %(message)s',
                    # filename="/var/log/ingestion.log",
                    level=logging.DEBUG)

POWEROFF_TIMER = 600  # in seconds
LOCK_FILE = '/.lock_ingestion'  # if file exist avoid poweroff on exit

"""
constants
"""
KEY_DEBUG = 'DEBUG'
KEY_ERROR = 'ERROR'
KEY_TIME = 'time'
KEY_STATUS = 'status'
KEY_WEATHER = 'weather'

"""
init global variables
"""
_boot = True  # set False after initial setup
_run = True  # set False to exit
_poweroff = True  # set False to avoid poweroff after exit
ser = ArduSerial(SERIAL_PORT)  # class for serial communication
starting_time = time.time()

time.sleep(1)


"""
start loop
"""
while _run:
    try:
        if _boot:
            logging.info("boot")
            # update arduino datetime
            ts = calendar.timegm(time.gmtime())
            ser.command("time", str(ts))
            ser.command("data")
            # TODO Data request
            _boot = False

        json_arrived = ser.update()

        if json_arrived:
            # debug message
            _debug = json_arrived.get(KEY_DEBUG)
            # error message
            _error = json_arrived.get(KEY_ERROR)
            # log message
            _time = json_arrived.get(KEY_TIME)
            _status = json_arrived.get(KEY_STATUS)
            _weather = json_arrived.get(KEY_WEATHER)

            if _debug:
                logging.debug(_debug)
            if _error:
                logging.error(_error)

            if _time and _status and _weather:
                logging.info(json_arrived)
                # TODO save log

        """
        Se è accesa da più di POWER_OFF secondi e non c'è nessun host collegato
        spegne la rpi
        """
        if starting_time - time.time() > POWEROFF_TIMER:
            if not kupiki.get_hosts():
                if os.path.exists(LOCK_FILE):
                    _poweroff = False
                else:
                    ser.send("poweroff")
                _run = False

    except KeyboardInterrupt:
        _run = False
        _poweroff = False
    except serial.serialutil.SerialException as e:
        logging.exception(e)
        ser = ArduSerial(SERIAL_PORT)
    except BaseException as e:
        logging.exception(e)

if _poweroff and False:
    logging.info("poweroff")
    os.system("sudo poweroff")

exit(2)
