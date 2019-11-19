/*
   La Panca project
   sketch written by Francesco Pasino a.k.a. Paso
   14 oct 2019

   TOTEST:
   - periodic RPi power on
   - standby mode

   TODO:
   - battery calculation
   - secure battery shutdown
   - extend debug
   - warning for ok shutdown
   - testing and testing and testing
   -
*/

#include <ArduinoJson.h>
#include <Adafruit_BME680.h>
#include <SD.h>
#include <RTCZero.h>

// pinout
#define BATTERY_PIN A1
#define SOLAR_PIN A2
#define BUTTON_PIN 4
#define RELE_PIN 20 // A5
#define DEBUG_LED_PIN 3 // or LED_BUILTIN to test

// useful variables
const size_t capacity = 2 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5); // JSON capacity
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

// power management
#define PWR_OFF 0
#define PWR_BOOT 1
#define PWR_ON 2
#define PWR_SDWN 3
#define BAT_TH 10 // volts
// timers in milliseconds
#define TIMER_BOOT 60000
#define TIMER_ON 120000
#define TIMER_SDWN 20000
int power_state = PWR_OFF;
uint32_t power_timer = 0;

uint32_t last_poweron = 0;
#define RECURSIVE_ON 86400 // 60s * 60m * 24h

/*************************************************************************
   setup()
 ************************************************************************/
void setup() {
  // JSON setup /////////////////////////////////////////////////
  status_subjson = data_json.createNestedObject("status"); // nested object in JSON object
  weather_subjson = data_json.createNestedObject("weather"); // nested object in JSON object

  // Serial setup ////////////////////////////////////////////////
  Serial.begin(115200);
  Serial.println("La Panca");

  // I/O setup ///////////////////////////////////////////////////
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), powerON, RISING);
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW);
  pinMode(DEBUG_LED_PIN, OUTPUT);

  // Sensors setup /////////////////////////////////////////////// TODO
  bme.begin(); // BME680 via I2C
  // Set up oversampling and filter initialization (uncomment to change default value)
  // bme.setTemperatureOversampling(BME680_OS_8X);  // BME680_OS_NONE to disable
  // bme.setHumidityOversampling(BME680_OS_2X);  // BME680_OS_NONE to disable
  // bme.setPressureOversampling(BME680_OS_4X);  // BME680_OS_NONE to disable
  // bme.setIIRFilterSize(BME680_FILTER_SIZE_3);  // BME680_FILTER_SIZE_0 to disable
  // bme.setGasHeater(320, 150); // 320*C for 150 ms, 0*C or 0 ms to disable

  // SD card setup ///////////////////////////////////////////////
  while (!SD.begin(SDCARD_SS_PIN)) { // Onboard SD Card) {
    bool v = millis() % 600 > 300;
    digitalWrite(DEBUG_LED_PIN, v);
    printError("SD doesn't found!");

    delay(80);
  }

  // RTC setup /////////////////////////////////////////////////// TODO
  rtc.begin();
  // set custom wrong time
  // rtc.setTime(16, 33, 00); // hours, minutes, seconds
  // rtc.setDate(23, 9, 19); // day, month, year
  // set backup routing
  //rtc.setAlarmTime(00, 00, 00);
  rtc.enableAlarm(rtc.MATCH_SS); // Every Minute
  rtc.attachInterrupt(readData);
  //rtc.standbyMode(); // standby mode to reduce the consume
  powerON();
}

/*************************************************************************
   loop()
 ************************************************************************/
void loop() {
  delay(100); // little delay to avoid bad Serial response (useful?)

  dataManagement();
  powerManagement();
  serialCommunication();

  errorManagement();

  if (power_state == PWR_OFF) { // if RPi is off
    // rtc.standbyMode(); // standby mode to reduce the consume TOTEST
  }
}
/*************************************************************************
   Data Management
 ************************************************************************/
/*
    Read battery
*/
float readBattery() {
  return 12; // TODO
  //return analogRead(BATTERY_PIN) * 15.0 / 1023.0;
}
/*
   Backup data on SD card or send it to RPi
*/
void dataManagement() {
  if (read_data) {
    read_data = false;
    updateData();
    backupData();
  }
}
/*
   Called by  timer interrupt
*/
void readData() {
  read_data = true;
  if (rtc.getEpoch() - last_poweron > RECURSIVE_ON) powerON();
}

/*
   Update data in json
*/
void updateData() {
  // time
  data_json["time"] = rtc.getEpoch();
  // status
  status_subjson["battery"] = readBattery();
  status_subjson["solar"] = analogRead(SOLAR_PIN); // TODO * 15 / 1023
  // weather
  bme.performReading(); // update wheather sensor
  weather_subjson["temperature"] = bme.temperature;
  weather_subjson["humidity"] = bme.humidity;
  weather_subjson["pressure"] = bme.pressure;
  weather_subjson["gas_resistance"] = bme.gas_resistance;
}
/*
   Save data on SD card
*/
bool backupData() { // backup data on SD card
  // Create a folder to store data, name is YY-MM-DD
  sprintf(dir_name, "%02d-%02d-%02d",  rtc.getYear(), rtc.getMonth(), rtc.getDay());
  SD.mkdir(dir_name);
  // Create a file to store data, name is YY-MM-DD/hh-mm.LOG
  sprintf(file_name, "%s/%02d-%02d.LOG", dir_name, rtc.getHours(), rtc.getMinutes());
  File data_file = SD.open(file_name, FILE_WRITE);
  // if the file is available, write to it:
  if (data_file) {
    // save JSON on file
    serializeJson(data_json, data_file);
    data_file.close();
  }
}

bool sendData() { // TODO
  File root = SD.open("/");
  while (true) {
    File dir =  root.openNextFile();
    if (dir) { // no more files
      if (dir.isDirectory()) {
        while (true) {
          File dataFile = dir.openNextFile();
          if (dataFile) {
            if (!dataFile.isDirectory()) {
              if (dataFile) {
                Serial.print(dir.name());
                Serial.print("/");
                Serial.println(dataFile.name());
              }
              while (dataFile.available()) {
                Serial.print(char(dataFile.read()));
              }
              Serial.println();
              dataFile.close();
            } // if a file
          } else break;
        } // while for each file in dir
      } // if a dir
    } else break;
  } // while for each dir in root
}

/*************************************************************************
   Serial Communication
 ************************************************************************/

void serialCommunication() {
  if (Serial) {
    input_string = "";
    while (Serial.available()) {
      // get the new byte:
      char inChar = (char)Serial.read();
      // if the incoming character is a newline execute the command
      // else add it to the inputString
      if (inChar == '\n') {
        // while (Serial.available()) Serial.read();
        parse_cmd();
        input_string = "";
        break;
      } else {
        input_string += inChar;
      }
    }
  }
}

/*
  Execute input string command | TODO
*/
void parse_cmd() {
  isAlive();

  Serial.println("COMMAND: " + input_string);

  if (input_string == "") { // get data from SD card
    // pass
  }
  else if (input_string == "data") { // get data from SD card
    sendData();
  }
  else if (input_string == "poweroff") { // start poweroff routine
    powerOFF();
  }
  // delete log file
  else if (input_string.startsWith("delete")) { // "delete YY-MM-DD/hh-mm.LOG"
    String file2del = input_string.substring(6);
    file2del.trim();
    if (SD.exists(file2del)) {
      if (SD.remove(file2del)) {
        printDebug("Delete " + file2del + ":\tsuccess!");
        String dir2del = file2del.substring(0, file2del.indexOf('/'));
        if (SD.rmdir(dir2del)) printDebug("Delete " + dir2del + ":\tsuccess!");
        else printDebug("Delete " + dir2del + ":\tfail!");
      }
      else printDebug("Delete " + file2del + ":\tfail!");
    }
    else printDebug("Delete " + file2del + ":\tdoesn't exist!");
  }
  // update datatime
  else if (input_string.startsWith("time")) { // "time 1571759341"
    rtc.setEpoch(input_string.substring(4).toInt());
    // debug
    char new_datetime[] = "YY-MM-DD hh:mm";
    sprintf(new_datetime, "%02d-%02d-%02d %02d:%02d", rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes());
    printDebug("Time update: " + String(new_datetime));
  }
  else {
    printDebug("Command not found!");
  }
}

/*************************************************************************
   Power Management
 ************************************************************************/

/*
   power routine
*/
void powerManagement() {
  switch (power_state) {
    case PWR_OFF:
      // pass
      break;
    case PWR_BOOT:
      if (millis() - power_timer > TIMER_BOOT) { // if poweron fail
        powerOFF();
      }
      break;
    case PWR_ON:
      if (millis() - power_timer > TIMER_ON) { // to avoid fails from RPi
        powerOFF();
      }
      break;
    case PWR_SDWN:
      if (millis() - power_timer > TIMER_SDWN) { // timer for shutdown
        digitalWrite(DEBUG_LED_PIN, LOW); // led off
        digitalWrite(RELE_PIN, LOW); // RPi off
        power_state = PWR_OFF;
      }
      break;
    default:
      break;
  }
}

void powerON() {
  if (power_state == PWR_OFF) {
    if (readBattery() > BAT_TH) {
      digitalWrite(RELE_PIN, HIGH); // RPi on
      power_timer = millis();
      power_state = PWR_BOOT;
    }
  }
}
void powerOFF() {
  power_timer = millis();
  power_state = PWR_SDWN;
}

void isAlive() {
  last_poweron = rtc.getEpoch();
  power_timer = millis(); // heartbeat
  power_state = PWR_ON;
}

/*************************************************************************
   other functions
 ************************************************************************/
void printDebug(String string) {
  DynamicJsonDocument debug_json(JSON_OBJECT_SIZE(8));
  debug_json["DEBUG"] = string;
  serializeJson(debug_json, Serial);
  Serial.println();
  Serial.println(power_state);
  // Serial.println("DEBUG: " + string);
}
void printError(String string) {
  DynamicJsonDocument debug_json(JSON_OBJECT_SIZE(2));
  debug_json["ERROR"] = string;
  serializeJson(debug_json, Serial);
  Serial.println();
  // Serial.println("ERROR: " + string);
}
/*
   debug function | TODO
*/
void errorManagement() {
  switch (power_state) {
    case PWR_OFF:
      // pass
      break;
    case PWR_BOOT:
      digitalWrite(DEBUG_LED_PIN, millis() % 1000 > 500); // led blink
      break;
    case PWR_ON:
      digitalWrite(DEBUG_LED_PIN, HIGH); // led on
      break;
    case PWR_SDWN:
      digitalWrite(DEBUG_LED_PIN, millis() % 800 > 400); // led blink
      break;
    default:
      //pass
      break;
  }
}
