#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "LittleFS.h"
#include "FS.h"
#include <Arduino.h>

RTC_DS1307 rtc;
DateTime C_time;

OneWire oneWire(D6);  // for the DS18B20 temperature sensor pin connect 
DallasTemperature TemSensor(&oneWire);

LiquidCrystal_I2C lcd(0x27, 16, 2);  // lcd initialize
int8_t btnClickPin = D7;              // input pin for button click
int8_t RTCsqrPin = D5;                // input pin for RTC Square wave pulse count (1Hz frequency)

// Variable Define
JsonDocument OutgoingData ;
long duration;               // store the total duration in second received from pc
String msg;                 // store the mode of microcontroller to perform, which received from pc
int sample;                  // store the sampling time in second received from pc
int8_t modeSelect = -1;      // store the selected mode index
String CSV_Data;             // store the csv data string
long sampleCount = 0;        // store total number of remaining sample counts
volatile int16_t sampleFlag; /* store the total number of pulse get from RTC at every second
                 if sampleFlag is equal to sample time, its trigger the data sample capturing */
bool btnFlag = false;        // button flag set during data log to exit data-log mode by user to press button through button intrrupt
volatile unsigned long last_INTR_Trigger_sqr = 0;
volatile unsigned long last_INTR_Trigger_btn = 0;

// ---------------------------- Functions Declaration Start -------------------------

bool CurrentTime(void);             // function for get current time
String ReadBuiltInTemSensor(void);  //function for read the temperature value from sensor return the CSV string
String ReadAnalogSensor(void); 
void DataReceiveACK_Print(void);    // print the data get received properly to lcd
void DataReceiveError_Print(void);  // print the data not get received properly to lcd
bool DataLogEndPrint(void);
bool dataWrite_File(String);        // function for write the data to txt file
void RTCsqr(void);              // function for 

//----------------------------- Intrrupt function define -------------------------

void IRAM_ATTR RTCsqrCount() {
  unsigned long INTR_trigger_sqr = millis();
  if (INTR_trigger_sqr - last_INTR_Trigger_sqr > 400) {
    sampleFlag += 1;
    // Serial.println("RTCsqr intrrupt");
  }
  last_INTR_Trigger_sqr = INTR_trigger_sqr;
}
void IRAM_ATTR btnFlagSet() {
  unsigned long INTR_trigger_btn = millis();
  if (INTR_trigger_btn - last_INTR_Trigger_btn > 400) {
    btnFlag = true;
    // Serial.println("btnFlagSet intrrupt");
  }
  last_INTR_Trigger_btn = INTR_trigger_btn;
}


// --------------------------------- Setup Start ----------------------------
void setup() {

  // Serial Communication start
  Serial.begin(115200);
  delay(2);

  pinMode(btnClickPin, INPUT_PULLUP);
  pinMode(RTCsqrPin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);


  // Serial.println("start.......");
  // LittleFS File system initialize
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
  }
  // Temperature Sensor initialize
  TemSensor.begin();
  delay(2);

  lcd.init();       // lcd initialize
  lcd.backlight();  // Turn on the LCD backlight
  lcd.print("..DATA-LOGGER..");

  Wire.begin();
  RTCsqr();

#ifndef ESP8266
  while (!Serial)
    ;  // wait for serial port to connect. Needed for native USB
#endif

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    // abort();
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  deserializeJson(OutgoingData, "{\"msg\":\"msg\"}");

  File csvFile = LittleFS.open("/SensorData.txt", "a");
  csvFile.close();

  File rstFile = LittleFS.open("/rst.txt", "a");
  rstFile.print(String(ESP.getResetReason()).c_str());
  rstFile.close();

  delay(2000);


// Serial.println("Reset reason: " + String(ESP.getResetReason()));

}

// -------------------------------- Loop Start ------------------------------

void loop() {

  // variable define here because reduce RAM overload


  if (!Serial.available()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   UART MODE");
    lcd.setCursor(0, 1);
    lcd.print("Buad-Rate:115200");
    delay(1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Waiting For Data");
    lcd.setCursor(0, 1);
    lcd.print("  ............");
    delay(1000);
  }

  else if (Serial.available()) {
  //  long j = millis();
    JsonDocument IncomingData;
    String jsonData = Serial.readString();
    // Serial.println(jsonData);
    DeserializationError error = deserializeJson(IncomingData, jsonData);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    }

    msg = IncomingData["msg"].as<String>();
    sample = IncomingData["sample"];
    duration = IncomingData["duration"];
    
  // if (msg == "rstFile") { 
  //    File rstFile = LittleFS.open("/rst.txt", "r");
  //     Serial.println(rstFile.read());
  //     rstFile.close();
  // }



    if (msg == "File Transfer Mode") {     
     // Write code for file transfer to pc
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("File Transfering");
      // long k =millis();
      // Serial.println(j-k);
      OutgoingData["msg"]="ACK";
      serializeJson(OutgoingData, Serial);
      while(!Serial.available()){ yield();}
      String jsonData = Serial.readString();
      DeserializationError error = deserializeJson(OutgoingData, jsonData);
       if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());}
      String data=OutgoingData["msg"].as<String>();
      if (data == "R") {
      File csvFile = LittleFS.open("/SensorData.txt", "r");
          while(1){
                String fileContent = csvFile.readStringUntil('\n');
                OutgoingData["msg"]=fileContent;
                serializeJson(OutgoingData, Serial);
                Serial.write("\r\n");
                Serial.flush();
                    if (fileContent=="END"){break;}
                yield();
           }
          csvFile.close();
             csvFile = LittleFS.open("/SensorData.txt", "a");
            if (csvFile) {
              csvFile.print("START\n");
              csvFile.print("DATA DOESN'T EXIT..\n");
              csvFile.print("END\n");
              csvFile.close();
                                }
          lcd.setCursor(0, 1);
          lcd.print(" ..Completed...");
          delay(10000);
       }
       else {
        OutgoingData["msg"]="NACK";
        serializeJson(OutgoingData, Serial);
        DataReceiveError_Print();
       }
      modeSelect = -1;
    }

    else if (msg == "Temperature Data-Log") {
      if (sample && duration) {
        Serial.write("ACK\n");
        // long k =millis();
        // Serial.println(j-k);
        // Write code for Temperature data log
        DataReceiveACK_Print();
        delay(10);
        modeSelect = 1;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temperature Log");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sample T:");
        lcd.setCursor(9, 0);
        lcd.print(sample);
        lcd.setCursor(0, 1);
        
        if(duration >= 60 ){
          lcd.print("Duration(M):");
          int i = int(duration / 60);
          lcd.setCursor(12, 1);
          lcd.print(i);}

        else{
          lcd.print("Duration(S):");
          lcd.setCursor(12, 1);
          lcd.print(duration);
        }

        LittleFS.remove("/SensorData.txt");
        delay(3000);
      }

      else {
        Serial.write("NACK\n");
        DataReceiveError_Print();
      }
    }

    else if (msg == "Analog Input Data-Log") {
      if (sample && duration) {
        Serial.write("ACK\n");
        // Write code for Analog data log
        DataReceiveACK_Print();
        delay(10);
        modeSelect = 2;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Analog Input Log");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sample T:");
        lcd.setCursor(9, 0);
        lcd.print(sample);
        lcd.setCursor(0, 1);

         if(duration >= 60 ){
          lcd.print("Duration(M):");
          int i = int(duration / 60);
          lcd.setCursor(12, 1);
          lcd.print(i);}

        else{
          lcd.print("Duration(S):");
          lcd.setCursor(12, 1);
          lcd.print(duration);
        }

        LittleFS.remove("/SensorData.txt");
        delay(3000);

      } else {
        Serial.write("NACK\n");
        DataReceiveError_Print();
      }
    }

    else {
      Serial.write("NACK\n");
      DataReceiveError_Print();
    }
  }



  // ---------------------- code start to run selected mode -----------------------------

  if (!(modeSelect == -1)) {

    sampleCount = int(duration / sample);
    // Serial.println(sampleCount);
    sampleFlag = 0;
    btnFlag = false;

    //code for asking to user for start the data log
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Press Button");
    lcd.setCursor(0, 1);
    lcd.print("To Start");

     digitalWrite(LED_BUILTIN, 0);
     delay(100);
    digitalWrite(LED_BUILTIN, 1);
    // Serial.println("waiting btn press");
    while (!(digitalRead(btnClickPin) == 0)) {
      yield();
      continue;
    }
    delay(1000);
    // Serial.println("--- btn press");
    // --------code for check the selected mode and start the data logging after user press button ------
    
    // temperature data log
    if (modeSelect == 1) {
      // write code for Temperature data log
      // Serial.println("temperature mode");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temperature Log");
      lcd.setCursor(0, 1);
      lcd.print("Count:");
      lcd.setCursor(6, 1);
      lcd.print(sampleCount);

       File csvFile = LittleFS.open("/SensorData.txt", "a");
          if (csvFile) {
             csvFile.print("START\n");
            //  Serial.println("In the START Write");
             
             csvFile.close();
                 }

      attachInterrupt(digitalPinToInterrupt(btnClickPin), btnFlagSet, FALLING);
      attachInterrupt(digitalPinToInterrupt(RTCsqrPin), RTCsqrCount, FALLING);
      // Serial.println("Intrrupt attach");
      while (1) {
        // continuously store the data log in file until total sample completed
        if ((sampleFlag == sample) && !(sampleCount <= 0)) {
          digitalWrite(LED_BUILTIN, 0);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Temperature Log");
          lcd.setCursor(0, 1);
          lcd.print("Count:");
          sampleCount -= 1;
          lcd.setCursor(6, 1);
          lcd.print(sampleCount);
          // Serial.println("1 condition");
          CurrentTime();
          // Serial.println("step 2");
          CSV_Data = ReadBuiltInTemSensor();
          // Serial.println(CSV_Data);
          // Serial.println("step 3");
          dataWrite_File(CSV_Data);
          // Serial.println("step 4");
          sampleFlag = 0;
          // Serial.println("step 5");
          yield();
        }
        digitalWrite(LED_BUILTIN, 1);
        // condition for check to total sample completed it exit the data log mode
        if (sampleCount <= 0) {
          // Serial.println("2 condition");
          File csvFile = LittleFS.open("/SensorData.txt", "a");
          if (csvFile) {
             csvFile.print("END\n");
             csvFile.close();
                 }
          DataLogEndPrint();
          sampleFlag = 0;
          break;
        }
        // condition for check the user want exit or not
        if (btnFlag == true) {
          // Serial.println("3 condition");
          detachInterrupt(digitalPinToInterrupt(RTCsqrPin));  // Get off the RTC Square pulse intrrupt avoid error in code
          btnFlag = false;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("You want To Exit");
          lcd.setCursor(0, 1);
          lcd.print(" [Press Button] ");
          delay(9000);  // wait 9 second to second click of usser for exit
          yield();
          if (btnFlag == true) {
            btnFlag = false;
            File csvFile = LittleFS.open("/SensorData.txt", "a");
             if (csvFile) {
             csvFile.print("END\n");
             csvFile.close();
                 }
            break;
          }

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Temperature Log");
          lcd.setCursor(0, 1);
          lcd.print("Count:");
          attachInterrupt(digitalPinToInterrupt(RTCsqrPin), RTCsqrCount, FALLING);  // Get on the RTC Square pulse intrrupt to run again normally
        }
      yield();
      }
      detachInterrupt(digitalPinToInterrupt(btnClickPin));
      detachInterrupt(digitalPinToInterrupt(RTCsqrPin));
      modeSelect = -1;
    }
    // analog data log
    else if (modeSelect == 2) {
      // write code for Analog data log
      // Serial.println("Analog data mode");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Analog Value Log");
      lcd.setCursor(0, 1);
      lcd.print("Count:");
      lcd.setCursor(6, 1);
      lcd.print(sampleCount);

      File csvFile = LittleFS.open("/SensorData.txt", "a");
          if (csvFile) {
             csvFile.print("START\n");
             csvFile.close();
                 }

      attachInterrupt(digitalPinToInterrupt(btnClickPin), btnFlagSet, FALLING);
      attachInterrupt(digitalPinToInterrupt(RTCsqrPin), RTCsqrCount, FALLING);
      // Serial.println("Intrrupt attach");
      while (1) {
        // continuously store the data log in file until total sample completed
        if ((sampleFlag == sample) && !(sampleCount <= 0)) {
          digitalWrite(LED_BUILTIN, 0);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Analog Value Log");
          lcd.setCursor(0, 1);
          lcd.print("Count:");
          sampleCount -= 1;
          lcd.setCursor(6, 1);
          lcd.print(sampleCount);
          // Serial.println("1 condition");
          CurrentTime();
          // Serial.println("step 2");
          CSV_Data = ReadAnalogSensor();
          // Serial.println("step 3");
          dataWrite_File(CSV_Data);
          // Serial.println("step 4");
          sampleFlag = 0;
          // Serial.println("step 5");
          yield();
        }
        digitalWrite(LED_BUILTIN, 1);
        // condition for check to total sample completed it exit the data log mode
        if (sampleCount <= 0) {
          // Serial.println("2 condition");
          DataLogEndPrint();
          sampleFlag = 0;
          File csvFile = LittleFS.open("/SensorData.txt", "a");
          if (csvFile) {
             csvFile.print("END\n");
             csvFile.close();
                 }
          break;
        }
        // condition for check the user want exit or not
        if (btnFlag == true) {
          // Serial.println("3 condition");
          detachInterrupt(digitalPinToInterrupt(RTCsqrPin));  // Get off the RTC Square pulse intrrupt avoid error in code
          btnFlag = false;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("You want To Exit");
          lcd.setCursor(0, 1);
          lcd.print(" [Press Button] ");
          delay(9000);  // wait 9 second to second click of usser for exit
          yield();
          if (btnFlag == true) {
            btnFlag = false;
          File csvFile = LittleFS.open("/SensorData.txt", "a");
          if (csvFile) {
             csvFile.print("END\n");
             csvFile.close();
                 }
            break;
          }

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Analog Value Log");
          lcd.setCursor(0, 1);
          lcd.print("Count:");
          attachInterrupt(digitalPinToInterrupt(RTCsqrPin), RTCsqrCount, FALLING);  // Get on the RTC Square pulse intrrupt to run again normally
        }
        yield();
      }
      detachInterrupt(digitalPinToInterrupt(btnClickPin));
      detachInterrupt(digitalPinToInterrupt(RTCsqrPin));
      modeSelect = -1;
    }

    else {
      modeSelect = -1;
      DataReceiveError_Print();
    }
  }
}




// --------------- Functions Start ---------------

bool CurrentTime(void) {
  C_time = rtc.now();
  return true;
}

void DataReceiveACK_Print(void) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(".Data Received..");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode Initialize");
  for (int i = 0; i < 16; i++) {
    yield();
    lcd.setCursor(i, 1);
    lcd.print(".");
    delay(300);
  }
}

void DataReceiveError_Print(void) {
  for (int i = 0; i < 3; i++) {
    yield();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("..Data Error....");
    lcd.setCursor(0, 1);
    lcd.print("..Send Again....");
    delay(2000);
  }
}


bool DataLogEndPrint(void) {
  detachInterrupt(digitalPinToInterrupt(btnClickPin));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(".. Completed ...");
  lcd.setCursor(0, 1);
  lcd.print("Press OK To EXIT");
  while (1) {
    yield();
    if (digitalRead(btnClickPin) == 0) {
      break;
    }
  }
  delay(300);
  attachInterrupt(digitalPinToInterrupt(btnClickPin), btnFlagSet, FALLING);
  return true;

}

String ReadBuiltInTemSensor(void) {
  TemSensor.requestTemperatures();
  delay(1);
  float value = TemSensor.getTempCByIndex(0);

  return (String(C_time.timestamp(DateTime::TIMESTAMP_DATE)) + "," + String(C_time.timestamp(DateTime::TIMESTAMP_TIME)) + "," + String(value) + "\n");
}

String ReadAnalogSensor(void) {

  float value = analogRead(2);

  return (String(C_time.timestamp(DateTime::TIMESTAMP_DATE)) + "," + String(C_time.timestamp(DateTime::TIMESTAMP_TIME)) + "," + String(value) + "\n");
}

bool dataWrite_File(String data) {
  //  write code for write data to file
  File csvFile = LittleFS.open("/SensorData.txt", "a");
  if (csvFile) {
    csvFile.print(data);
    // Serial.println("Data write in file");
    csvFile.close();
    }
    else{
      return false;
    }
  // Serial.println(data);
  return true;
}

void RTCsqr(void) {
  Wire.beginTransmission(0x68);
  Wire.write(7);
  Wire.write(0x10);
  Wire.endTransmission();
}
