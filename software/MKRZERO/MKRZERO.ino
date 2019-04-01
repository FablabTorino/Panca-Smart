#include <ArduinoJson.h>
#include <Adafruit_BME680.h>
#include <SD.h>
#include <RTCZero.h>

// pinout
#define BATTERY_PIN A1
#define SOLAR_PIN A2
#define BUTTON_PIN 1
#define RELE_PIN 2
#define DEBUG_R_PIN 3
#define DEBUG_G_PIN 4
#define DEBUG_B_PIN 5

// useful variables
const size_t capacity = JSON_OBJECT_SIZE(4) + 2*JSON_OBJECT_SIZE(5); // JSON capacity
String input_string = ""; // a String to hold incoming data
Adafruit_BME680 bme; // weather sensor with I2C protocol
RTCZero rtc; // internal real time clock
DynamicJsonDocument data_json(capacity); // JSON object
JsonObject status_subjson;
JsonObject weather_subjson;
char dir_name[] = "YY-MM-DD";
char file_name[] = "YY-MM-DD/hh-mm.LOG";
bool read_data = false;
bool time_is_wrong = false; // on statup the time is always wrong

/*************************************************************************
   setup()
 ************************************************************************/
void setup() {
  status_subjson = data_json.createNestedObject("status"); // nested object in JSON object
  weather_subjson = data_json.createNestedObject("weather"); // nested object in JSON object

  // Serial setup ////////////////////////////////////////////////
  Serial.begin(115200);
  Serial.println();

  // Sensors setup /////////////////////////////////////////////// TODO
  debug(bme.begin(), 55); // BME680 via I2C
  // Set up oversampling and filter initialization (uncomment to change default value)
  // bme.setTemperatureOversampling(BME680_OS_8X);  // BME680_OS_NONE to disable
  // bme.setHumidityOversampling(BME680_OS_2X);  // BME680_OS_NONE to disable
  // bme.setPressureOversampling(BME680_OS_4X);  // BME680_OS_NONE to disable
  // bme.setIIRFilterSize(BME680_FILTER_SIZE_3);  // BME680_FILTER_SIZE_0 to disable
  // bme.setGasHeater(320, 150); // 320*C for 150 ms, 0*C or 0 ms to disable

  // SD card setup ///////////////////////////////////////////////
  debug(SD.begin(SDCARD_SS_PIN), 50); // Onboard SD Card

  // RTC setup /////////////////////////////////////////////////// TODO
  rtc.begin();
  // set custom wrong time
  rtc.setTime(14, 20, 00); // hours, minutes, seconds
  rtc.setDate(01, 04, 19); // day, month, year
  data_json["wakeup time"] = rtc.getEpoch();
  // set backup routing
  //rtc.setAlarmTime(00, 00, 00);
  rtc.enableAlarm(rtc.MATCH_SS); // Every Minute
  rtc.attachInterrupt(readData);
  //rtc.standbyMode(); // standby mode to reduce the consume
}

/*************************************************************************
   loop()
 ************************************************************************/
void loop() {
  delay(100); // little dalay to avoid bad Serial response (useful?)

  if (read_data) {
    read_data = false;

    updateData();
    if (Serial) {
      serializeJson(data_json, Serial);
      Serial.println();
    }
    else {
      backupData();
    }
  }

  if (Serial) {
    input_string = "";
    while (Serial.available()) {
      // get the new byte:
      char inChar = (char)Serial.read();
      // if the incoming character is a newline execute the command
      // else add it to the inputString
      if (inChar == '\n') {
        parse_cmd();
        break;
      } else {
        input_string += inChar;
      }
    }
  }
  else {
    // rtc.standbyMode(); // standby mode to reduce the consume | TODO
  }
}
/*************************************************************************
   funtions
 ************************************************************************/

/*
   Backup data on SD card, called by interrupt
*/
void readData() {
  read_data = true;
}
bool backupData() {
  // Create a folder to store data, name is YY-MM-DD
  sprintf(dir_name, "%02d-%02d-%02d",  rtc.getYear(), rtc.getMonth(), rtc.getDay());
  debug(SD.mkdir(dir_name), 11);
  // Create a file to store data, name is YY-MM-DD/hh-mm.LOG
  sprintf(file_name, "%s/%02d-%02d.LOG", dir_name, rtc.getHours(), rtc.getMinutes());
  File data_file = SD.open(file_name, FILE_WRITE);
  // if the file is available, write to it:
  if (debug(data_file, 12)) {
    // save JSON on file
    serializeJson(data_json, data_file);
    data_file.close();
  }
}
/*
   Update data in json
*/
void updateData() {
  // time
  data_json["time"] = rtc.getEpoch();
  // status
  status_subjson["battery"] = 50; // TODO
  status_subjson["solar"] = 12.5; // TODO
  status_subjson["backup battery"] = analogRead(ADC_BATTERY)  * (4.3 / 1023.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
  // weather
  debug(bme.performReading(), 56); // update wheather sensor
  weather_subjson["temperature"] = bme.temperature;
  weather_subjson["humidity"] = bme.humidity;
  weather_subjson["pressure"] = bme.pressure;
  weather_subjson["gas_resistance"] = bme.gas_resistance;
}
/*
   debug function | TODO
*/
bool debug(bool success, uint8_t code) {
  switch (code) {
    case 0:
      break;
    case 50: // SD card begin
      break;
    case 55: // BME680 begin
      break;
    default:
      break;
  }
  return success;
}

/*
  Execute input string command | TODO
*/
void parse_cmd() {
  Serial.println(input_string);
}
