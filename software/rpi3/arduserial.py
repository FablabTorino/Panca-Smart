import serial
import json
import logging


def str2json(msg: str) -> dict:
    if not msg:
        return {}
    try:
        return json.loads(msg)
    except json.decoder.JSONDecodeError as e:
        logging.exception(e)
        logging.error("msg: _{}_".format(msg))
        return {}


def json2str(jsn: dict) -> str:
    if not jsn:
        return ''
    try:
        return json.dumps(jsn)
    except TypeError:
        return ''


class ArduSerial(serial.Serial):

    def __init__(self, port, baudrate=115200, timeout=0.2,
                 write_timeout=0.04):
        try:
            super().__init__(port, baudrate, timeout=timeout,
                             write_timeout=write_timeout)
            self.flush()
        except serial.serialutil.SerialException:
            logging.error("Serial '{}' not found".format(port))

    def receive(self):
        if self.is_alive():
            return self.readline().decode().rstrip()
        else:
            return False

    def send(self, text: str) -> bool:
        if self.is_alive():
            try:
                logging.debug("Send: '{}'".format(text))
                self.write(text.encode())
                return True
            except serial.SerialTimeoutException:
                return False
        else:
            return False

    def cleanup(self):
        if self.is_alive():
            while self.read():
                pass

    def is_alive(self) -> bool:
        return self.isOpen()

    def command(self, program: str, parameter: str = '') -> bool:
        if parameter:
            return self.send(program + " " + parameter)
        else:
            self.send(program)

    def update(self) -> dict:
        return str2json(self.receive())
