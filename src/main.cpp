#include "FS.h"
#include "SPIFFS.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include "RTClib.h"
#include <HardwareSerial.h>

////

TaskHandle_t pumpOn;
TaskHandle_t humidityCheck;
TaskHandle_t humidityToggle;
TaskHandle_t ventToggle;
TaskHandle_t lightToggle;
TaskHandle_t clockCheck;
TaskHandle_t soilCheck;
SemaphoreHandle_t xMutex;
SemaphoreHandle_t xMutexWaterCheck;
SemaphoreHandle_t xMutexWaterRelay;
SemaphoreHandle_t xMutexHumidityCheck;
SemaphoreHandle_t xMutexHumidityRelay;
SemaphoreHandle_t xMutexSoilCheck;
SemaphoreHandle_t xMutexVentRelay;
SemaphoreHandle_t xMutexLightRelay;
SemaphoreHandle_t xMutexRTCAccess;
SemaphoreHandle_t xMutexSPIFFSAccess;
SemaphoreHandle_t xMutexSettingsRead;

#define FORMAT_SPIFFS_IF_FAILED true
#define DHTTYPE DHT21 // DHT 21 (AM2301) / DHT 22  (AM2302), AM2321 / DHT 11

#define waterSignalPin 14
#define DHTPin 18
#define soilSensorPin 34
#define humPin 25
#define humPin1 27
#define ventPin 26
#define lightPin 19
#define waterPin 13
#define waterSensorPin 23

char SERVER_NAME[] = "ESP32 Semargl";
char user[20] = "root";
char pass[20] = "pass";
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char months[12][5] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
float humValue;
float tempValue;
int16_t soilValue;
int8_t waterLevel;
unsigned long waterInterval;
int8_t dayNight;
DHT dht(DHTPin, DHTTYPE);
RTC_DS3231 rtc;
DateTime now;
char settings[400];
String rebLog;
String incomingSerial;

WiFiMulti WiFiM;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // ws сервер доступний за адресою ws://[esp ip]/ws



void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  xSemaphoreTake( xMutexSPIFFSAccess, portMAX_DELAY );
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  xSemaphoreGive( xMutexSPIFFSAccess );
}
void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\r\n", path);

  xSemaphoreTake( xMutexSPIFFSAccess, portMAX_DELAY );
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  xSemaphoreTake( xMutexSPIFFSAccess, portMAX_DELAY );
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
  xSemaphoreGive( xMutexSPIFFSAccess );
}
String readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  xSemaphoreTake( xMutexSPIFFSAccess, portMAX_DELAY );
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    xSemaphoreGive( xMutexSPIFFSAccess );
    return "error";
  }
  Serial.println("- read from file:");
  while (file.available()) {
    String line = file.readString();        //зчитує файл
    //Serial.println(line);
    xSemaphoreGive( xMutexSPIFFSAccess );
    return line;
  }
  xSemaphoreGive( xMutexSPIFFSAccess );
}

void pumpOnCode( void * pvParameters ) {
  Serial.print("pumpOn running on core ");
  Serial.println(xPortGetCoreID());
  unsigned long timeOld;
  for (;;) {
    xSemaphoreTake( xMutexWaterCheck, portMAX_DELAY );
    waterLevel = digitalRead(waterSensorPin);
    xSemaphoreGive( xMutexWaterCheck );
    if (waterLevel == 0) {
      Serial.println("Water level is low, watering off");
      digitalWrite(waterPin, LOW);
    } else {
      xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
      char local[sizeof(settings)];
      strncpy(local, settings, sizeof(settings));
      xSemaphoreGive( xMutexSettingsRead );
      DynamicJsonDocument localSettings(1024);
      deserializeJson(localSettings, local);


      JsonObject water = localSettings["water"];
      uint8_t waterToggle = water["toggle"];
      switch (waterToggle) {
        case 1: {
            DynamicJsonDocument waterInterval(100);
            auto waterIntervalError = deserializeJson(waterInterval, readFile(SPIFFS, "/waterinterval.txt"));
            if (!waterIntervalError) {
              timeOld = waterInterval["wateringTime"];
              uint8_t interval = water["interval"][0];
              uint8_t workTime = water["interval"][1];
              xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
              DateTime timeNow = now;
              xSemaphoreGive( xMutexRTCAccess );
              if (timeNow.unixtime() > (timeOld + interval * 60)) {
                xSemaphoreTake( xMutexWaterRelay, portMAX_DELAY );
                digitalWrite(waterPin, HIGH);
                Serial.println("pump ON");
                xSemaphoreGive( xMutexWaterRelay );

                DynamicJsonDocument json(1024);
                json["wateringTime"] = timeNow.unixtime();
                String buff;
                serializeJson(json, buff);
                char textToFile[buff.length()];
                buff.toCharArray(textToFile, buff.length() + 1);
                writeFile(SPIFFS, "/waterinterval.txt", textToFile);
                delay(workTime * 1000);
                xSemaphoreTake( xMutexWaterRelay, portMAX_DELAY );
                digitalWrite(waterPin, LOW);
                Serial.println("pump OFF");
                xSemaphoreGive( xMutexWaterRelay );
                delay(30000);
              }
            } else {
              Serial.print(F("deserializeJson() failed with code "));
              Serial.println(waterIntervalError.c_str());
              Serial.println("Can't read last watering time");

              xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
              DateTime timeNow = now;
              xSemaphoreGive( xMutexRTCAccess );
              Serial.print("Set last watering time to current time: ");
              Serial.print(timeNow.month(), DEC);
              Serial.print('/');
              Serial.print(timeNow.day(), DEC);
              Serial.print(" (");
              Serial.print(daysOfTheWeek[timeNow.dayOfTheWeek()]);
              Serial.print(") ");
              Serial.print(timeNow.hour(), DEC);
              Serial.print(':');
              Serial.println(timeNow.minute(), DEC);
              DynamicJsonDocument json(1024);
              json["wateringTime"] = timeNow.unixtime();
              String buff;
              serializeJson(json, buff);
              char textToFile[buff.length()];
              buff.toCharArray(textToFile, buff.length() + 1);
              writeFile(SPIFFS, "/waterinterval.txt", textToFile);
            }
          } break;
        case 2: {
            uint16_t humDetector = water["detector"];
            int localSoil;
            xSemaphoreTake( xMutexSoilCheck, portMAX_DELAY );
            localSoil = soilValue;
            Serial.print("localSoil: ");
            Serial.println(localSoil);
            Serial.print("humInterval: ");
            Serial.println(humDetector);
            xSemaphoreGive( xMutexSoilCheck );
            if (humDetector < localSoil) {
              xSemaphoreTake( xMutexWaterRelay, portMAX_DELAY );
              digitalWrite(waterPin, HIGH);
              Serial.println("pump ON");
              xSemaphoreGive( xMutexWaterRelay );
              delay(7000);
              xSemaphoreTake( xMutexWaterRelay, portMAX_DELAY );
              digitalWrite(waterPin, LOW);
              Serial.println("pump OFF");
              xSemaphoreGive( xMutexWaterRelay );
              delay(30000);
            }
          } break;
        case 3: {
            xSemaphoreTake( xMutexWaterRelay, portMAX_DELAY );
            digitalWrite(waterPin, LOW);
            Serial.println("pump OFF");
            xSemaphoreGive( xMutexWaterRelay );
          }
      }
    }
    delay(10000);
  }
}

void humidityCheckCode( void * pvParameters ) {
  Serial.print("humidityCheck running on core ");
  Serial.println(xPortGetCoreID());
  uint8_t count = 0;
  float hum;
  float temp;
  for (;;) {
    xSemaphoreTake( xMutexHumidityCheck, portMAX_DELAY );
    hum = dht.readHumidity();
    temp = dht.readTemperature();
    if (count >= 10) {
      Serial.println("To many failed reads from DHT sensor!");
    }
    if (isnan(hum) || isnan(temp)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      count++;
      Serial.print("Count :");
      Serial.println(count);
    } else {
      count = 0;
      humValue = hum;
      tempValue = temp;
    }
    xSemaphoreGive( xMutexHumidityCheck );
    delay(3000);
  }
}

void humidityToggleCode( void * pvParameters ) {
  Serial.print("humidityToggle running on core ");
  Serial.println(xPortGetCoreID());
  float hum;
  unsigned long timeOld = 0;
  for (;;) {
    xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
    char local[sizeof(settings)];
    strncpy(local, settings, sizeof(settings));
    xSemaphoreGive( xMutexSettingsRead );
    DynamicJsonDocument localSettings(1024);
    deserializeJson(localSettings, local);

    JsonObject humidity = localSettings["hum"];
    uint8_t humidityToggle = humidity["toggle"];
    switch (humidityToggle) {
      case 1: {
          uint8_t interval = humidity["interval"][0];
          uint8_t workTime = humidity["interval"][1];
          xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
          int timeNow = now.unixtime();
          xSemaphoreGive( xMutexRTCAccess );
          if (timeOld == 0) {
            xSemaphoreTake( xMutexHumidityRelay, portMAX_DELAY );
            digitalWrite(humPin, LOW);
            digitalWrite(humPin1, LOW);
            xSemaphoreGive( xMutexHumidityRelay );
            Serial.println("Hum on!");
            timeOld = timeNow;
          } else {
            if (timeNow < timeOld + (workTime * 60)) {
              xSemaphoreTake( xMutexHumidityRelay, portMAX_DELAY );
              digitalWrite(humPin, LOW);
              digitalWrite(humPin1, LOW);
              xSemaphoreGive( xMutexHumidityRelay );
              Serial.println("Hum working...");
            } else if (timeNow > timeOld + (workTime * 60)) {
              if (timeNow < timeOld + (interval * 60)) {
                xSemaphoreTake( xMutexHumidityRelay, portMAX_DELAY );
                digitalWrite(humPin, HIGH);
                digitalWrite(humPin1, HIGH);
                xSemaphoreGive( xMutexHumidityRelay );
                Serial.println("Hum interval has passed");
              } else {
                timeOld = 0;
                Serial.println("Hum finished");
              }
            }
          }
        } break;
      case 2: {
          timeOld = 0;
          uint8_t intervalMin = humidity["detector"][0];
          uint8_t intervalMax = humidity["detector"][1];
          xSemaphoreTake( xMutexHumidityCheck, portMAX_DELAY );
          hum = humValue;
          xSemaphoreGive( xMutexHumidityCheck );
          if (hum > intervalMax) {
            xSemaphoreTake( xMutexHumidityRelay, portMAX_DELAY );
            digitalWrite(humPin, HIGH);
            digitalWrite(humPin1, HIGH);
            xSemaphoreGive( xMutexHumidityRelay );
            Serial.println("Hum off");
          } else if (hum < intervalMin) {
            xSemaphoreTake( xMutexHumidityRelay, portMAX_DELAY );
            digitalWrite(humPin, LOW);
            digitalWrite(humPin1, LOW);
            xSemaphoreGive( xMutexHumidityRelay );
            Serial.println("Hum on");
          }
        } break;
      case 3: {
          timeOld = 0;
          xSemaphoreTake( xMutexHumidityRelay, portMAX_DELAY );
          digitalWrite(humPin, HIGH);
          digitalWrite(humPin1, HIGH);
          xSemaphoreGive( xMutexHumidityRelay );
          Serial.println("Hum always off!");
        } break;
    }
    delay(10000);
  }
}

void ventToggleCode( void * pvParameters ) {

  Serial.print("ventToggle running on core ");
  Serial.println(xPortGetCoreID());
  float temp;
  float hum;
  unsigned long timeOld = 0;

  for (;;) {
    xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
    char local[sizeof(settings)];
    strncpy(local, settings, sizeof(settings));
    xSemaphoreGive( xMutexSettingsRead );
    DynamicJsonDocument localSettings(1024);
    deserializeJson(localSettings, local);

    JsonObject vent;
    xSemaphoreTake( xMutexLightRelay, portMAX_DELAY );
    int localDayNight = dayNight;
    xSemaphoreGive( xMutexLightRelay );

    switch (localDayNight) {
      case 0: {
          Serial.println("vent Night ///////////////////");
          vent = localSettings["ventNight"];
        } break;
      case 1: {
          Serial.println("vent Day /////////////////");
          vent = localSettings["vent"];
        } break;
    }
    uint8_t ventToggle = vent["toggle"];

    Serial.print("ventToggle: ");
    Serial.println(ventToggle);

    xSemaphoreTake( xMutexHumidityCheck, portMAX_DELAY );
    temp = tempValue;
    hum = humValue;
    xSemaphoreGive( xMutexHumidityCheck );
    switch (ventToggle) {
      case 1: {
          //          xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
          //          int timeNow = now.unixtime();
          //          xSemaphoreGive( xMutexRTCAccess );
          //          if (timeOld == 0) {
          //            xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
          //            digitalWrite(ventPin, LOW);
          //            xSemaphoreGive( xMutexVentRelay );
          //            Serial.println("Vent on!");
          //            timeOld = timeNow;
          //          } else {
          //            if (timeNow < timeOld + (workTime * 60)) {
          //              xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
          //              digitalWrite(ventPin, LOW);
          //              xSemaphoreGive( xMutexVentRelay );
          //              Serial.println("Vent working...");
          //            } else if (timeNow > timeOld + (workTime * 60)) {
          //              if (timeNow > timeOld + (interval * 60)) {
          //                timeOld = 0;
          //                Serial.println("Interval has passed");
          //              } else {
          //                xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
          //                digitalWrite(ventPin, HIGH);
          //                xSemaphoreGive( xMutexVentRelay );
          //                Serial.println("Vent finished");
          //              }
          //            }
          //          }
          uint8_t interval = vent["interval"][0];
          uint8_t workTime = vent["interval"][1];
          xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
          unsigned long timeNow = now.unixtime();
          xSemaphoreGive( xMutexRTCAccess );
          if (timeOld == 0) {
            xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
            digitalWrite(ventPin, LOW);
            xSemaphoreGive( xMutexVentRelay );
            Serial.println("Vent on!");
            timeOld = timeNow;
          } else if (timeOld != NULL || timeOld < timeNow) {
            if (timeNow < timeOld + (workTime * 60)) {
              xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
              digitalWrite(ventPin, LOW);
              xSemaphoreGive( xMutexVentRelay );
              Serial.println("Vent working...");
            } else if (timeNow > timeOld + (workTime * 60)) {
              if (timeNow < timeOld + (interval * 60)) {
                xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
                digitalWrite(ventPin, HIGH);
                xSemaphoreGive( xMutexVentRelay );
                Serial.println(timeNow);
                Serial.println(timeOld + (interval * 60));
                Serial.println("Vent interval has passed");
              } else {
                timeOld = 0;
                Serial.println("Vent finished");
              }
            }
          } else {
            timeOld = 0;
          }
        } break;
      case 2: {
          timeOld = 0;
          uint8_t humCheck = vent["detector"][0];
          uint8_t humDiap = vent["detector"][1];
          uint8_t tempCheck = vent["detector"][2];
          uint8_t tempDiap = vent["detector"][3];
          if (humCheck == 1 && tempCheck == 1) {
            bool ventOn = 0;
            if (hum > humDiap) {
              ventOn = 1;
              Serial.println("Hum is high, vent on!");
            }
            if (temp > tempDiap) {
              ventOn = 1;
              Serial.println("Temp is high, vent on!");
            }
            if (ventOn) {
              xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
              digitalWrite(ventPin, LOW);
              xSemaphoreGive( xMutexVentRelay );
            } else {
              Serial.println("Hum & temp are low, vent off!");
              xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
              digitalWrite(ventPin, HIGH);
              xSemaphoreGive( xMutexVentRelay );
            }
          } else if (humCheck == 1 && tempCheck == 0) {
            if (hum > humDiap) {
              Serial.println("Hum is high, vent on!");
              xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
              digitalWrite(ventPin, LOW);
              xSemaphoreGive( xMutexVentRelay );
            } else {
              Serial.println("Hum is low, vent off!");
              xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
              digitalWrite(ventPin, HIGH);
              xSemaphoreGive( xMutexVentRelay );
            }
          } else if (tempCheck == 1 && humCheck == 0) {
            if (temp > tempDiap) {
              Serial.println("Temp is high, vent on!");
              xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
              digitalWrite(ventPin, LOW);
              xSemaphoreGive( xMutexVentRelay );
            } else {
              Serial.println("Temp is low, vent off!");
              xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
              digitalWrite(ventPin, HIGH);
              xSemaphoreGive( xMutexVentRelay );
            }
          } else if (tempCheck == 0 && humCheck == 0) {
            Serial.println("Range is not set, vent off!");
            xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
            digitalWrite(ventPin, HIGH);
            xSemaphoreGive( xMutexVentRelay );
          }
        } break;
      case 3: {
          timeOld = 0;
          xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
          digitalWrite(ventPin, LOW);
          xSemaphoreGive( xMutexVentRelay );
          Serial.println("Vent always on!");
        } break;
      case 4: {
          timeOld = 0;
          xSemaphoreTake( xMutexVentRelay, portMAX_DELAY );
          digitalWrite(ventPin, HIGH);
          xSemaphoreGive( xMutexVentRelay );
          Serial.println("Vent always off!");
        } break;
    }
    delay(4000);
  }
}

void lightToggleCode( void * pvParameters ) {
  Serial.print("lightToggle running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
    char local[sizeof(settings)];
    strncpy(local, settings, sizeof(settings));
    xSemaphoreGive( xMutexSettingsRead );
    DynamicJsonDocument localSettings(1024);
    deserializeJson(localSettings, local);

    JsonObject light = localSettings["light"];
    uint8_t lightToggle = light["toggle"];

    switch (lightToggle) {
      case 1: {
          int16_t interval1 = light["interval"][0];
          int16_t interval2 = light["interval"][1];
          xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
          DateTime timeNow = now;
          xSemaphoreGive( xMutexRTCAccess );
          int16_t intervalTime = timeNow.hour() * 60 + timeNow.minute();
          Serial.print("interval 1: ");
          Serial.println(interval1 / 60);
          Serial.print("intervalTime ");
          Serial.println(intervalTime / 60);
          Serial.print("interval 2: ");
          Serial.println(interval2 / 60);
          if (interval1 < interval2) {
            if (intervalTime >= interval1 && intervalTime <= interval2) {
              xSemaphoreTake( xMutexLightRelay, portMAX_DELAY );
              digitalWrite(lightPin, LOW);
              dayNight = 1;
              xSemaphoreGive( xMutexLightRelay );
              Serial.println("Light on11111!");
            } else if (intervalTime < interval1  || intervalTime > interval2) {
              xSemaphoreTake( xMutexLightRelay, portMAX_DELAY );
              digitalWrite(lightPin, HIGH);
              dayNight = 0;
              xSemaphoreGive( xMutexLightRelay );
              Serial.println("Light off22222!");
            }
          } else if (interval1 > interval2) {
            if (intervalTime >= interval1 && intervalTime <= 24 * 60) {
              xSemaphoreTake( xMutexLightRelay, portMAX_DELAY );
              digitalWrite(lightPin, LOW);
              dayNight = 1;
              xSemaphoreGive( xMutexLightRelay );
              Serial.println("Light on33333!");
            } else if (intervalTime < interval1 && intervalTime > interval2) {
              xSemaphoreTake( xMutexLightRelay, portMAX_DELAY );
              digitalWrite(lightPin, HIGH);
              dayNight = 0;
              xSemaphoreGive( xMutexLightRelay );
              Serial.println("Light off44444!");
            } else if (intervalTime > 0 && intervalTime <= interval2) {
              xSemaphoreTake( xMutexLightRelay, portMAX_DELAY );
              digitalWrite(lightPin, LOW);
              dayNight = 1;
              xSemaphoreGive( xMutexLightRelay );
              Serial.println("Light on55555!");
            }
          }
        } break;
      case 2: {
          xSemaphoreTake( xMutexLightRelay, portMAX_DELAY );
          digitalWrite(lightPin, LOW);
          dayNight = 1;
          xSemaphoreGive( xMutexLightRelay );
          Serial.println("Light on6666666!");
        } break;
      case 3: {
          xSemaphoreTake( xMutexLightRelay, portMAX_DELAY );
          digitalWrite(lightPin, HIGH);
          dayNight = 0;
          xSemaphoreGive( xMutexLightRelay );
          Serial.println("Light off7777777!");
        } break;
    }
    delay(10000);
  }
}

void clockCode( void * pvParameters ) {
  Serial.print("Clock running on core ");
  Serial.println(xPortGetCoreID());
  uint8_t count = 0;
  uint16_t localSoil;
  soilValue = 1000;
  uint8_t failTime = 0;
  DateTime localTime;
  for (;;) {
    DateTime localNow;
    xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
    localNow = rtc.now();
    localTime = now;
    xSemaphoreGive( xMutexRTCAccess );

    if (localNow.unixtime() > (localTime.unixtime() + 400) || localNow.unixtime() < (localTime.unixtime() - 400)) {
      Serial.println("fail time read");
      Serial.println(localNow.unixtime());
      Serial.println(localTime.unixtime() + 30);
      failTime = failTime + 1;
    } else {
      xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
      now = localNow;
      xSemaphoreGive( xMutexRTCAccess );
      failTime = 0;
    }

    if (failTime > 10) {
      Serial.println("Set previous time");
      rtc.adjust(DateTime(localTime.year(), localTime.month(), localTime.day(), localTime.hour(), localTime.minute(), (localTime.second() + 10)));
    }

    if (count < 10) {
      count++;
    } else {
      Serial.println();
      Serial.print(localNow.year(), DEC);
      Serial.print('/');
      Serial.print(localNow.month(), DEC);
      Serial.print('/');
      Serial.print(localNow.day(), DEC);
      Serial.print(" (");
      Serial.print(daysOfTheWeek[localNow.dayOfTheWeek()]);
      Serial.print(") ");
      Serial.print(localNow.hour(), DEC);
      Serial.print(':');
      Serial.print(localNow.minute(), DEC);
      Serial.print(':');
      Serial.println(localNow.second(), DEC);

      xSemaphoreTake( xMutexSoilCheck, portMAX_DELAY );
      localSoil = soilValue;
      xSemaphoreGive( xMutexSoilCheck );
      Serial.print("Soil: ");
      Serial.println(localSoil);

      xSemaphoreTake( xMutexHumidityCheck, portMAX_DELAY );
      Serial.print("Temp: ");
      Serial.println(tempValue);
      Serial.print("Hum: ");
      Serial.println(humValue);
      xSemaphoreGive( xMutexHumidityCheck );

      xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
      Serial.print("Settings: ");
      Serial.println(settings);
      xSemaphoreGive( xMutexSettingsRead );
      count = 0;

      DynamicJsonDocument water(100);
      auto waterError = deserializeJson(water, readFile(SPIFFS, "/waterinterval.txt"));
      if (waterError) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(waterError.c_str());
      } else {
        waterInterval = water["wateringTime"];
        Serial.println(waterInterval);
        water["waterLevel"] = waterLevel;
      }
      water["type"] = 8;
      String buff;
      serializeJson(water, buff);
      Serial.println(buff);
      ws.textAll(buff);
      delay(200);
      ws.textAll(rebLog);


    }
    delay(1000);
  }
}

void soilCheckCode( void * pvParameters ) {
  Serial.print("Soil Check running on core ");
  Serial.println(xPortGetCoreID());
  int localSoil[6] = {0, 0, 0, 0, 0, 0};
  uint8_t count = 0;
  uint8_t failread = 0;
  uint8_t soilZero = 0;
  for (;;) {
    if (localSoil[count] == 0) {
      Serial.println("There 1");
      for (int k = 0; k < 4; k++) {
        if (Serial2.available() > 0) {
          incomingSerial = Serial2.readStringUntil('\n');
          Serial.println(incomingSerial);
          DynamicJsonDocument serialRead(400);
          auto serialReadError = deserializeJson(serialRead, incomingSerial);
          if (!serialReadError) {
            int SerialSoilValue = serialRead["sV"];
            if (SerialSoilValue > 4095 || SerialSoilValue < 0) {
              Serial.println("Incorrect soil value");
              failread = failread + 1;
            } else {
              Serial.print("SerialSoilValue: ");
              Serial.println(SerialSoilValue);
              localSoil[count] = SerialSoilValue;
              failread = 0;
              break;
            }
          } else {
            Serial.println("Serial read error");
            failread = failread + 1;
          }
        } else {
          delay(500);
        }
      }
      delay(1000);
    } else {
      int16_t tempSoil = 0;
      int16_t midSoil = 0;
      uint8_t countZero = 0;
      for (uint8_t j = 0; j < sizeof(localSoil) / sizeof(localSoil[0]); j++) {
        if (localSoil[j] != 0) {
          midSoil = midSoil + localSoil[j];
          countZero = countZero + 1;
        }
      }
      if (countZero == 0) {
        countZero = 1;
      }
      midSoil = midSoil / countZero;
      for (int k = 0; k < 6; k++) {
        if (Serial2.available() > 0) {
          incomingSerial = Serial2.readStringUntil('\n');
          Serial.println(incomingSerial);
          DynamicJsonDocument serialRead(400);
          auto serialReadError = deserializeJson(serialRead, incomingSerial);
          if (!serialReadError) {
            int SerialSoilValue = serialRead["sV"];
            if (SerialSoilValue > 4095 || SerialSoilValue < 0) {
              Serial.println("Incorrect soil value");
              failread = failread + 1;
            } else {
              Serial.print("SerialSoilValue: ");
              Serial.println(SerialSoilValue);
              tempSoil = SerialSoilValue;
              failread = 0;
              break;
            }
          } else {
            Serial.println("Serial read error");
            failread = failread + 1;
          }
        } else {
          delay(500);
        }
      }
      if (tempSoil < midSoil + 70 && tempSoil > midSoil - 70) {
        localSoil[count] = tempSoil;
      } else {
        localSoil[count] = 0;
      }
    }
    int16_t midSoil = 0;
    uint8_t countZero = 0;
    for (uint8_t j = 0; j < sizeof(localSoil) / sizeof(localSoil[0]); j++) {
      if (localSoil[j] != 0) {
        midSoil = midSoil + localSoil[j];
        countZero = countZero + 1;
      }
    }
    if (countZero == 0) {
      countZero = 1;
    }
    if (midSoil != 0) {
      xSemaphoreTake( xMutexSoilCheck, portMAX_DELAY );
      soilValue = midSoil / countZero;
      xSemaphoreGive( xMutexSoilCheck );
      soilZero = 0;
    } else {
      if (soilZero > 3) {
        Serial.println("Too many fail reads of soil sensor! Soil sensor is broken");
        xSemaphoreTake( xMutexSoilCheck, portMAX_DELAY );
        soilValue = 0;
        xSemaphoreGive( xMutexSoilCheck );
        soilZero = 4;
      } else {
        soilZero = soilZero + 1;
      }
    }
    for (uint8_t j = 0; j < sizeof(localSoil) / sizeof(localSoil[0]); j++) {
      Serial.print("Soil Value ");
      Serial.print(j);
      Serial.print(": ");
      Serial.println(localSoil[j]);
    }
    if (failread > 15) {
      localSoil[count] = 0;
    } else {
      failread = 0;
    }
    Serial.print("Count: ");
    Serial.println(count);
    count = count + 1;
    if (count > sizeof(localSoil) / sizeof(localSoil[0]) - 1) {
      count = 0;
    }
    delay(2000);
  }
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    //client connected
    ets_printf("ws[%s][%u] connect\n", server->url(), client->id());
    //    DynamicJsonBuffer jsonBuffer;
    //    JsonObject& root = jsonBuffer.createObject();            //додає дані до json та відправляє при з'єднанні клієнту
    //    root["w"] = wVal;                                        //
    //    String data;                                             //
    //    root.printTo(data);
    //    client->text(data);

  } else if (type == WS_EVT_DISCONNECT) {
    //client disconnected
    ets_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if (type == WS_EVT_ERROR) {
    //error was received from the other end
    ets_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if (info->opcode == WS_TEXT) {
      //client->text("I got your text message");

      DynamicJsonDocument jsonBuffer(1024);
      deserializeJson(jsonBuffer, data);                      //отримує дані від клієнта у форматі json та парсить
      int dataType = jsonBuffer["type"];                            //перевіряє тип отриманих даних(1=показники та перемикачі,2=зміна користувача,3=зміна налаштувань WIFI)
      switch (dataType) {
        case 1: {

          } break;
        case 2: { // зміна паролю
            String checkUser = jsonBuffer["user"];                          // парсить логін та пароль
            String checkPass = jsonBuffer["pass"];                          //
            bool check = true;
            for (int i = 0; i <= checkUser.length() + 1; i++) {
              if (isAlphaNumeric(checkUser[i])) {                        // перевіряє логін на відсутність символів крім латинских літер та цифр
                bool check = false;
                break;
              }
            }
            for (int i = 0; i <= checkPass.length() + 1; i++) {
              if (isAlphaNumeric(checkPass[i])) {                        // перевіряє пароль на відсутність символів крім латинских літер та цифр
                bool check = false;
                break;
              }
            }
            if (checkUser != NULL && checkPass != NULL && check == true && checkUser.length() <= 20 && checkPass.length() <= 20) {
              checkUser.toCharArray(user, checkUser.length());
              checkPass.toCharArray(pass, checkPass.length());
              DynamicJsonDocument json(1024);
              json["user"] = checkUser;                              //серилізує логін та пароль у форматі json
              json["pass"] = checkPass;
              json["type"] = 2;
              String buff;
              serializeJson(json, buff);
              ws.text(client->id(), buff);                               //відправляє дані клієнту
              char textToFile[buff.length()];
              buff.toCharArray(textToFile, buff.length() + 1);
              writeFile(SPIFFS, "/login.txt", textToFile);               //зберігає логін та пароль до файлу
              delay(2000);
              ESP.restart();                                             //перезавантажує МК
            }
          } break;
        case 3: { // зміна налаштувань WiFi
            String checkSSID = jsonBuffer["ssid"];                             // парсить логін та пароль
            String checkPass = jsonBuffer["ssidpass"];
            bool check = true;
            for (int i = 0; i <= checkSSID.length() + 1; i++) {
              if (isAlphaNumeric(checkSSID[i])) {                        // перевіряє логін на відсутність символів крім латинских літер та цифр
                bool check = false;
                break;
              }
            }
            for (int i = 0; i <= checkPass.length() + 1; i++) {
              if (isAlphaNumeric(checkPass[i])) {                        // перевіряє пароль на відсутність символів крім латинских літер та цифр
                bool check = false;
                break;
              }
            }
            if (checkSSID != NULL && checkPass != NULL && check == true && checkSSID.length() <= 32 && checkPass.length() <= 32) {
              client->text(checkSSID + checkPass);
              char ssid[checkSSID.length()];
              char ssidpass[checkPass.length()];
              DynamicJsonDocument json(1024);
              json["type"] = 3;
              json["ssid"] = checkSSID;                              //серилізує логін та пароль у форматі json
              json["ssidpass"] = checkPass;
              String buff;
              serializeJson(json, buff);
              ws.text(client->id(), buff);
              char textToFile[buff.length()];
              buff.toCharArray(textToFile, buff.length() + 1);
              writeFile(SPIFFS, "/wifi.txt", textToFile);               //зберігає ssid та пароль до файлу
              delay(2000);
              ESP.restart();                                            //перезавантажує МК
            }
          } break;
        case 4: { //змінює налаштування дати й часу
            String setDate = jsonBuffer["date"];                                  // парсить дату
            String setTime = jsonBuffer["time"];                                  // парсить час
            String formatDate;
            formatDate = setDate.substring(0, 4);                           //форматує отриману дату у формат ммм.дд.рррр наприклад Jan-13-1989
            setDate.remove(0, 5);
            setDate = setDate + setDate.charAt(2) + formatDate;
            formatDate = months[setDate.substring(0, 2).toInt() - 1];
            setDate.remove(0, 2);
            setDate = formatDate + setDate;
            char textDate[setDate.length()];
            char textTime[setTime.length()];
            setDate.toCharArray(textDate, setDate.length() + 1);
            setTime.toCharArray(textTime, setTime.length() + 1);
            Serial.println("Date: ");
            Serial.println(textDate);
            Serial.println("Time: ");
            Serial.println(textTime);

            DynamicJsonDocument json(1024);
            json["type"] = 4;
            json["setDate"] = setDate;                      //серилізує дату у форматі json
            String buff;
            serializeJson(json, buff);
            ws.text(client->id(), buff);

            xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
            rtc.adjust(DateTime(textDate, textTime));                        //передаж дані у модуль rtc(DS3231 чи подібний)
            delay(500);
            ESP.restart();
            xSemaphoreGive( xMutexRTCAccess );
          } break;
        case 5: { // зчитує час з модуля RTC і віддає у разі запиту
            xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
            delay(500);
            DynamicJsonDocument json(1024);
            json["type"] = 5;
            json["serverClock"] = String(now.unixtime()) + "000";                    //додає до дати нулі мілісекунд, щоб він був у форматі unixtime та серилізує час у форматі json
            xSemaphoreGive( xMutexRTCAccess );
            String buff;
            serializeJson(json, buff);
            ws.text(client->id(), buff);
          } break;
        case 6: { // отримання налаштувань клієнтом
            DynamicJsonDocument json(1024);
            json["form"] = jsonBuffer["form"];
            json["type"] = jsonBuffer["type"];
            int dataForm = jsonBuffer["form"];
            switch (dataForm) {
              case 1: {
                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  char local[400];
                  strncpy(local, settings, sizeof(settings));
                  xSemaphoreGive( xMutexSettingsRead );
                  DynamicJsonDocument localSettings(1024);
                  deserializeJson(localSettings, local);

                  Serial.println(localSettings.memoryUsage());
                  json["ventNight"] = jsonBuffer["ventNight"];
                  json["hum"] = jsonBuffer["hum"];
                  localSettings["ventNight"] = jsonBuffer["ventNight"];
                  localSettings["hum"] = jsonBuffer["hum"];
                  Serial.println(localSettings.memoryUsage());
                  //                  JsonObject jsonVentNight = jsonBuffer["ventNight"];
                  //
                  //                  JsonObject ventNight = localSettings.createNestedObject("ventNight");
                  //                  ventNight["toggle"] = jsonVentNight["toggle"];
                  //
                  //                  JsonArray interval = ventNight.createNestedArray("interval");
                  //                  uint8_t interval1 = jsonVentNight["interval"][0];
                  //                  uint8_t interval2 = jsonVentNight["interval"][1];
                  //                  uint8_t intervalArray[2]= {interval1, interval2};
                  //                  copyArray(intervalArray, interval);
                  //
                  //                  JsonArray detector = ventNight.createNestedArray("detector");
                  //                  bool detector1 = jsonVentNight["detector"][0];
                  //                  uint8_t detector2 = jsonVentNight["detector"][1];
                  //                  bool detector3 = jsonVentNight["detector"][2];
                  //                  uint8_t detector4 = jsonVentNight["detector"][3];
                  //                  uint8_t detectorArray[4]= {detector1, detector2, detector3, detector4};
                  //                  copyArray(detectorArray, detector);

                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  serializeJson(localSettings, settings);
                  Serial.println(settings);
                  xSemaphoreGive( xMutexSettingsRead );
                } break;
              case 2: {
                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  char local[400];
                  strncpy(local, settings, sizeof(settings));
                  xSemaphoreGive( xMutexSettingsRead );
                  DynamicJsonDocument localSettings(1024);
                  deserializeJson(localSettings, local);

                  Serial.println(localSettings.memoryUsage());
                  json["vent"] = jsonBuffer["vent"];
                  json["hum"] = jsonBuffer["hum"];
                  localSettings["vent"] = jsonBuffer["vent"];
                  localSettings["hum"] = jsonBuffer["hum"];
                  Serial.println(localSettings.memoryUsage());

                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  serializeJson(localSettings, settings);
                  Serial.println(settings);
                  xSemaphoreGive( xMutexSettingsRead );
                } break;
              case 3: {
                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  char local[400];
                  strncpy(local, settings, sizeof(settings));
                  xSemaphoreGive( xMutexSettingsRead );
                  DynamicJsonDocument localSettings(1024);
                  deserializeJson(localSettings, local);

                  Serial.println(localSettings.memoryUsage());
                  json["light"] = jsonBuffer["light"];
                  localSettings["light"] = jsonBuffer["light"];
                  Serial.println(localSettings.memoryUsage());

                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  serializeJson(localSettings, settings);
                  Serial.println(settings);
                  xSemaphoreGive( xMutexSettingsRead );
                } break;
              case 4: {
                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  char local[400];
                  strncpy(local, settings, sizeof(settings));
                  xSemaphoreGive( xMutexSettingsRead );
                  DynamicJsonDocument localSettings(1500);
                  deserializeJson(localSettings, local);

                  Serial.println(localSettings.memoryUsage());
                  json["vent"] = jsonBuffer["vent"];
                  json["ventNight"] = jsonBuffer["ventNight"];
                  json["hum"] = jsonBuffer["hum"];
                  localSettings["vent"] = jsonBuffer["vent"];
                  localSettings["ventNight"] = jsonBuffer["ventNight"];
                  localSettings["hum"] = json["hum"];
                  Serial.println(localSettings.memoryUsage());

                  String localSettingsVent;
                  serializeJson(localSettings["vent"], localSettingsVent);
                  Serial.println(localSettingsVent);
                  String localSettingsVentNight;
                  serializeJson(localSettings["ventNight"], localSettingsVentNight);
                  Serial.println(localSettingsVentNight);
                  String localSettingsHum;
                  serializeJson(localSettings["hum"], localSettingsHum);
                  Serial.println(localSettingsHum);

                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  serializeJson(localSettings, settings);
                  Serial.println(settings);
                  xSemaphoreGive( xMutexSettingsRead );
                } break;
              case 5: {
                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  char local[400];
                  strncpy(local, settings, sizeof(settings));
                  xSemaphoreGive( xMutexSettingsRead );
                  DynamicJsonDocument localSettings(1024);
                  deserializeJson(localSettings, local);

                  Serial.println(localSettings.memoryUsage());
                  json["water"] = jsonBuffer["water"];
                  localSettings["water"] = jsonBuffer["water"];
                  Serial.println(localSettings.memoryUsage());

                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  serializeJson(localSettings, settings);
                  Serial.println(settings);
                  xSemaphoreGive( xMutexSettingsRead );
                } break;
              case 6: { //зберігає налаштування на флеш
                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  char local[400];
                  strncpy(local, settings, sizeof(settings));
                  xSemaphoreGive( xMutexSettingsRead );
                  DynamicJsonDocument localSettings(1500);
                  deserializeJson(localSettings, local);

                  Serial.println(localSettings.memoryUsage());
                  json["ventNight"] = jsonBuffer["ventNight"];
                  json["vent"] = jsonBuffer["vent"];
                  json["light"] = jsonBuffer["light"];
                  json["hum"] = jsonBuffer["hum"];
                  json["water"] = jsonBuffer["water"];
                  localSettings["ventNight"] = jsonBuffer["ventNight"];
                  localSettings["vent"] = jsonBuffer["vent"];
                  localSettings["light"] = jsonBuffer["light"];
                  localSettings["hum"] = jsonBuffer["hum"];
                  localSettings["water"] = jsonBuffer["water"];
                  Serial.println(localSettings.memoryUsage());

                  String buff;
                  serializeJson(localSettings, buff);
                  char textToFile[buff.length()];
                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  strncpy(settings, buff.c_str(), sizeof(buff.c_str()));
                  Serial.println(settings);
                  buff.toCharArray(textToFile, buff.length() + 1);
                  writeFile(SPIFFS, "/settings.txt", settings);
                  xSemaphoreGive( xMutexSettingsRead );

                } break;
              case 7: {
                  xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
                  char local[400];
                  strncpy(local, settings, sizeof(settings));
                  xSemaphoreGive( xMutexSettingsRead );
                  DynamicJsonDocument localSettings(1024);
                  deserializeJson(localSettings, local);
                  json["ventNight"] = localSettings["ventNight"];
                  json["vent"] = localSettings["vent"];
                  json["light"] = localSettings["light"];
                  json["hum"] = localSettings["hum"];
                  json["water"] = localSettings["water"];

                } break;
              default: {
                  json["type"] = 38;
                  json["error"] = "Shoto pishlo ne tak";
                } break;
            }
            String buff;
            serializeJson(json, buff);
            Serial.println(buff);
            ws.textAll(buff);
          } break;
        case 7: { // отримання показань датчиків та часу
            DynamicJsonDocument json(1024);
            json["type"] = 7;
            xSemaphoreTake( xMutexHumidityCheck, portMAX_DELAY );
            json["hum"] = humValue;
            json["temp"] = tempValue;
            xSemaphoreGive( xMutexHumidityCheck );
            xSemaphoreTake( xMutexSoilCheck, portMAX_DELAY );
            json["soil"] = soilValue;
            xSemaphoreGive( xMutexSoilCheck );
            xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
            json["serverClock"] = String(now.unixtime()) + "000";
            xSemaphoreGive( xMutexRTCAccess );
            String buff;
            serializeJson(json, buff);
            client->text(buff);
          } break;
        case 8: {

          } break;
        case 9: {

          } break;
        default: {

          } break;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(38400);
  Serial.println();
  Serial.println();
  Serial.println();

  pinMode(waterSignalPin, OUTPUT);
  digitalWrite(waterSignalPin, HIGH);
  pinMode(humPin, OUTPUT);
  pinMode(humPin1, OUTPUT);
  pinMode(waterPin, OUTPUT);
  pinMode(ventPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  pinMode(waterSensorPin, INPUT_PULLDOWN);
  //pinMode(waterPin, INPUT_PULLDOWN);
  //pinMode(soilSensorPin, INPUT_PULLDOWN);
  dht.begin();
  rtc.begin();
  delay(500);
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(2019, 3, 21, 11, 0, 0));
  }
  now = rtc.now();

  xMutex = xSemaphoreCreateMutex();
  xMutexWaterCheck = xSemaphoreCreateMutex();
  xMutexWaterRelay = xSemaphoreCreateMutex();
  xMutexHumidityCheck = xSemaphoreCreateMutex();
  xMutexHumidityRelay = xSemaphoreCreateMutex();
  xMutexVentRelay = xSemaphoreCreateMutex();
  xMutexLightRelay = xSemaphoreCreateMutex();
  xMutexRTCAccess = xSemaphoreCreateMutex();
  xMutexSPIFFSAccess = xSemaphoreCreateMutex();
  xMutexSettingsRead = xSemaphoreCreateMutex();
  xMutexSoilCheck = xSemaphoreCreateMutex();

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  listDir(SPIFFS, "/", 0);

  DynamicJsonDocument SPIFFSSettings(1024);
  auto settingsError = deserializeJson(SPIFFSSettings, readFile(SPIFFS, "/settings.txt"));
  Serial.println(settings);
  if (!settingsError) {
    Serial.println("Set settings from flash");
    xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
    char local[400] = "{}";
    strncpy(local, settings, 400);
    serializeJson(SPIFFSSettings, settings);
    Serial.println(settings);
    xSemaphoreGive( xMutexSettingsRead );
  } else {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(settingsError.c_str());
    Serial.println("Can't set settings from flash");
    Serial.println("Set hardcoded settings");
    xSemaphoreTake( xMutexSettingsRead, portMAX_DELAY );
    String local = "{\"ventNight\":{\"toggle\":1,\"interval\":[20,5],\"detector\":[1,60,1,24]},\"vent\":{\"toggle\":1,\"interval\":[8,2],\"detector\":[1,60,1,24]},\"light\":{\"toggle\":1,\"interval\":[200,860]},\"hum\":{\"toggle\":3,\"interval\":[20,5],\"detector\":[10,50]},\"water\":{\"toggle\":3,\"interval\":[15,6],\"detector\":1700}}";
    strncpy(settings, local.c_str(), sizeof(local.c_str()));
    Serial.println(settings);
    xSemaphoreGive( xMutexSettingsRead );
  }

  DynamicJsonDocument water(100);
  auto waterError = deserializeJson(water, readFile(SPIFFS, "/waterinterval.txt"));
  if (waterError) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(waterError.c_str());
  } else {
    waterInterval = water["wateringTime"];
    Serial.println(waterInterval);
  }

  DynamicJsonDocument reb(1500);
  auto rebootError = deserializeJson(reb, readFile(SPIFFS, "/reboot.txt"));
  if (rebootError) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(rebootError.c_str());
  } else {
    DynamicJsonDocument json(1500);
    xSemaphoreTake( xMutexRTCAccess, portMAX_DELAY );
    DateTime timeNow = now;
    xSemaphoreGive( xMutexRTCAccess );

    int nextLine = reb["nextLine"];
    JsonArray countLines = json.createNestedArray("countLines");
    json["countLines"] = reb["countLines"];
    int count = json["countLines"].size();
    Serial.println(count);
    String stringLocal = String(timeNow.year());
    stringLocal.concat('/');
    stringLocal.concat(timeNow.month());
    stringLocal.concat('/');
    stringLocal.concat(timeNow.day());
    stringLocal.concat(" (");
    stringLocal.concat(daysOfTheWeek[timeNow.dayOfTheWeek()]);
    stringLocal.concat(") ");
    stringLocal.concat(timeNow.hour());
    stringLocal.concat(':');
    stringLocal.concat(timeNow.minute());
    stringLocal.concat(':');
    stringLocal.concat(timeNow.second());
    json["countLines"][nextLine] = stringLocal;
    for (byte i = 0; i < json["countLines"].size(); i++) {
      String value = json["countLines"][i];
      Serial.println(value);
    }
    Serial.print("nextLine: ");
    Serial.println(nextLine);
    if (nextLine >= count) {
      nextLine = 0;
    } else {
      nextLine = nextLine + 1;
    }
    Serial.print("nextLine: ");
    Serial.println(nextLine);
    json["nextLine"] = nextLine;
    json["type"] = 9;
    String buff;
    serializeJson(json, buff);
    rebLog = buff;
    Serial.print("buff: ");
    Serial.println(rebLog);
    char textToFile[buff.length()];
    buff.toCharArray(textToFile, buff.length() + 1);
    writeFile(SPIFFS, "/reboot.txt", textToFile);               //зберігає логін та пароль до файлу
  }

  DynamicJsonDocument login(400);
  auto loginError = deserializeJson(login, readFile(SPIFFS, "/login.txt"));
  if (loginError) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(loginError.c_str());
    Serial.println("Can't set login from flash");
    Serial.println("Set hardcoded login and password");
  } else {
    String userCheck = login["user"];
    String passCheck = login["pass"];
    if (userCheck != NULL && passCheck != NULL) {
      userCheck.toCharArray(user, userCheck.length() + 1);
      passCheck.toCharArray(pass, passCheck.length() + 1);
    }
  }

  DynamicJsonDocument wifi(400);
  auto wifiError = deserializeJson(wifi, readFile(SPIFFS, "/wifi.txt"));
  if (wifiError) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(wifiError.c_str());
  } else {
    String ssidCheck = wifi["ssid"];
    String ssidPassCheck = wifi["ssidpass"];
    Serial.println(ssidCheck);
    Serial.println(ssidPassCheck);
    if (ssidCheck != NULL && ssidPassCheck != NULL) {
      char ssid[(ssidCheck.length() + 1)];
      char ssidpass[(ssidPassCheck.length() + 1)];
      ssidCheck.toCharArray(ssid, ssidCheck.length() + 1);
      ssidPassCheck.toCharArray(ssidpass, ssidPassCheck.length() + 1);
      WiFiM.addAP(ssid, ssidpass);
    }
  }

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  for (int i = 3; i > 0; i--) {
    WiFiM.run();
    delay(100);
    Serial.print("WiFi status :");
    Serial.println(WiFi.status());
    if (WiFi.status() == WL_CONNECTED) {
      break;
    } else {
      WiFi.config(192, 168, 100, 100);
      WiFi.softAP("Semargl", "root");
      Serial.println();
      Serial.print("Server IP address: ");
      Serial.println(WiFi.softAPIP());
      Serial.print("Server MAC address: ");
      Serial.println(WiFi.softAPmacAddress());
      Serial.println(WiFi.SSID());
      break;
    }
  }

  //доменне ім'я в локальній мережі
  if (MDNS.begin(SERVER_NAME)) {
    Serial.println("MDNS responder started");
  }


  xTaskCreatePinnedToCore(
    pumpOnCode,   /* Task function. */
    "pumpOn",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &pumpOn,      /* Task handle to keep track of created task */
    1);          /* pin task to core 0 */
  delay(100);

  xTaskCreatePinnedToCore(
    humidityCheckCode,   /* Task function. */
    "humidityCheck",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &humidityCheck,      /* Task handle to keep track of created task */
    tskNO_AFFINITY);          /* pin task to core 1 */
  delay(100);

  xTaskCreatePinnedToCore(
    humidityToggleCode,   /* Task function. */
    "humidityToggle",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &humidityToggle,      /* Task handle to keep track of created task */
    tskNO_AFFINITY);          /* pin task to core 1 */
  delay(100);

  xTaskCreatePinnedToCore(
    ventToggleCode,   /* Task function. */
    "ventToggle",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &ventToggle,      /* Task handle to keep track of created task */
    tskNO_AFFINITY);          /* pin task to core 1 */
  delay(100);

  xTaskCreatePinnedToCore(
    lightToggleCode,   /* Task function. */
    "lightToggle",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &lightToggle,      /* Task handle to keep track of created task */
    tskNO_AFFINITY);          /* pin task to core 1 */
  delay(100);

  xTaskCreatePinnedToCore(
    clockCode,   /* Task function. */
    "clockCheck",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &clockCheck,      /* Task handle to keep track of created task */
    1);          /* pin task to core 0 */
  delay(100);

  xTaskCreatePinnedToCore(
    soilCheckCode,   /* Task function. */
    "soilCheck",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &soilCheck,      /* Task handle to keep track of created task */
    tskNO_AFFINITY);          /* pin task to core 0 */
  delay(100);
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////// TASKS

  server.on("/" , HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(user, pass)) {
      return request->requestAuthentication();
    }
    String html = readFile(SPIFFS, "/index.html");
    request->send(200, "text/html", html);       // відсилає index.html з параметрами wVal та yVal для перемикачів
  });
  server.on("/bootstrap.min.css", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/bootstrap.min.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("bootstrap.min.css", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/bootstrap.min.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/bootstrap-slider.min.css", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/bootstrap-slider.min.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/bootstrap-slider.min.css", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/bootstrap-slider.min.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/bootstrap-slider.min.js", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/bootstrap-slider.min.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/style.css", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/style.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/popper.min.js", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/popper.min.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/bootstrap.min.js", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/bootstrap.min.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/jquery-3.3.1.min.js", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/jquery-3.3.1.min.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("bootstrap.min.js", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/bootstrap.min.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/JsPreLoad.js", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/JsPreLoad.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/JsAfterLoad.js", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/JsAfterLoad.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/BootstrapAfterLoadJS.js", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/BootstrapAfterLoadJS.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  ws.setAuthentication(user, pass);           //Авторизація
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.begin();

  // Додаємо сервіс до MDNS та NetBios
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
  //   NBNS.begin(SERVER_NAME);

}

void loop() {
}