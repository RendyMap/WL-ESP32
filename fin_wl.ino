
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif



//Provide the token generation process info.
#include <addons/TokenHelper.h>


//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "TP-LINK_0664" 
#define WIFI_PASSWORD "63920444"



#define WIFI_SSID "Run" 
#define WIFI_PASSWORD "full123."


//For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "AIzaSyALns2KadY2lJwH7nE2a6K76g8TGpQc_B8"

/* 3. Define the RTDB URL */
#define DATABASE_URL "esp32wl-default-rtdb.firebaseio.com/test" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "thendomapila@gmail.com"
#define USER_PASSWORD "Tmstar1."

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

#define TdsSensorPin 33
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0,tdsValue = 0;
float averageVoltage = 0,temperature = 25;

#define echoPin 25
#define trigPin 26
#define ONE_WIRE_BUS 27


int relay = 32; //relay pin used
long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement
int ldist;
String ps="";
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2); 





void setup()
{

  Serial.begin(115200);
  

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    lcd.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  //Or use legacy authenticate method
  //config.database_url = DATABASE_URL;
  //config.signer.tokens.legacy_token = "<database secret>";

  //////////////////////////////////////////////////////////////////////////////////////////////
  //Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  //otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  Firebase.begin(&config, &auth);

  //Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);
 delay(5000);
  



  
  lcd.begin();  
  lcd.backlight();
   pinMode(TdsSensorPin,INPUT);
   pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
   pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
   pinMode(relay,OUTPUT);
    sensors.begin();

  
  lcd.print("TDS");
  lcd.setCursor(5,0);
  lcd.print("TEMP");
  lcd.setCursor(11,0);
  lcd.print("WaL");  
  delay(2000); 

   
}

int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
      for (i = 0; i < iFilterLen - j - 1; i++) 
          {
        if (bTab[i] > bTab[i + 1]) 
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}
  

void loop()
{
       ps= Firebase.getString(fbdo,"test/pump");
      if(ps=="1")
      {
        Serial.print("Pump On");
        digitalWrite(relay,HIGH);
      }else if(ps=="0")
      {
        Serial.print("Pump Off");
        digitalWrite(relay,LOW);
      }
   
     


    
    

  static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.println("TDS Value:");
      Serial.println(tdsValue);
      Serial.println("ppm");
      lcd.setCursor(0,1);
      lcd.print(tdsValue);
      delay(5000);
     

      
  //Flash string (PROGMEM and  (FPSTR), String, C/C++ string, const char, char array, string literal are supported
  //in all Firebase and FirebaseJson functions, unless F() macro is not supported.
   }
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    //Serial.printf("Set bool... %s\n", Firebase.setBool(fbdo, "/test/bool", count % 2 == 0) ? "ok" : fbdo.errorReason().c_str());

   //  Serial.printf("Get bool... %s\n", Firebase.getBool(fbdo, "/test/bool") ? fbdo.to<bool>() ? "true" : "false" : fbdo.errorReason().c_str());

   //  bool bVal;
   // Serial.printf("Get bool ref... %s\n", Firebase.getBool(fbdo, "/test/bool", &bVal) ? bVal ? "true" : "false" : fbdo.errorReason().c_str());

    //Serial.printf("Set int... %s\n", Firebase.setInt(fbdo, "/test/int", count) ? "ok" : fbdo.errorReason().c_str());

   // Serial.printf("Get int... %s\n", Firebase.getInt(fbdo, "/test/int") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());

    //int iVal = 0;
    //Serial.printf("Get int ref... %s\n", Firebase.getInt(fbdo, "/test/int", &iVal) ? String(iVal).c_str() : fbdo.errorReason().c_str());

    //Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, "/test/float", count + 10.2) ? "ok" : fbdo.errorReason().c_str());

    //Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/test/float") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());

    //Serial.printf("Set double... %s\n", Firebase.setDouble(fbdo, "/test/double", count + 35.517549723765) ? "ok" : fbdo.errorReason().c_str());

    //Serial.printf("Get double... %s\n", Firebase.getDouble(fbdo, "/test/double") ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());

    //Serial.printf("Set LEVEL... %s\n", Firebase.setString(fbdo, "/test/LEVEL", waterLevel) ? "ok" : fbdo.errorReason().c_str());

    //Serial.printf("Get LEVEL... %s\n", Firebase.getString(fbdo, "/test/LEVEL") ? fbdo.to<const char *>() : fbdo.errorReason().c_str());

    
    /*  if(distance=11)
      { 
        
        Serial.println("Water Level:0%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",0) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());

      }
      else if(distance=10)
      { Serial.println("Water Level: 10%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",10) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
        digitalWrite(relay, HIGH);
      }
      else if(distance=9)
      { 
        Serial.println("Water Level:20%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",20) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
      }
      else if(distance=8)
      { Serial.println("Water Level:30%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",30.0) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
      }
      else if(distance=7)
      { Serial.println("Water Level:40%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",40.0) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
      }
      else if(distance=6)
      { Serial.println("Water Level:50%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",50) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
      }
      else if(distance=5)
      { Serial.println("Water Level:60%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",60 )? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
      }
      else if(distance=4)
      { Serial.println("Water Level:70%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",70) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
      }
      else if(distance=3)
      { Serial.println("Water Level:80%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",80) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
      }
      else if(distance=2)
      { Serial.println("Water Level:90%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",90) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
      }
      else if(distance=1)
      { Serial.println("Water Level:100%");
        Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",0) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
        
       digitalWrite(relay, LOW);
      }*/

  
            digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      // Reads the echoPin, returns the sound wave travel time in microseconds
      duration = pulseIn(echoPin, HIGH);
      // Calculating the distance
      distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
      // Displays the distance on the Serial Monitor
      Serial.print("Distance: ");
      Serial.print(distance);
      Serial.println(" cm");
  
     delay(5000);


   
     if(distance != ldist)
     {
      switch(distance)
      {
      

        case 20:
         Serial.print("Water Level: 0%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",0) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
   
         lcd.setCursor(11,1);
         lcd.print("10");
         break;
         case 19:
         Serial.print("Water Level:20%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",20) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("20");
         break;

        case 18:
         Serial.print("Water Level:20%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",20) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("20");
         break;

         case 17:
         Serial.print("Water Level:30%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",30) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("20");
         break;

        case 16:
         Serial.println("Water Level:30%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",30.0) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("30");
         break;
         case 15:
         Serial.print("Water Level:40%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",40) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("40");
         break;

        case 14:
         Serial.println("Water Level:40%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",40) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
          lcd.setCursor(11,1);
          lcd.print("40");
         break;

         case 13:
         Serial.print("Water Level:50%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",50) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("20");
         break;

        case 12:
         Serial.println("Water Level:50%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",50) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("50");
         break;

         case 11:
         Serial.print("Water Level:60%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",60) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("60");
         break;

        case 10:
         Serial.println("Water Level:60%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",60) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         lcd.setCursor(11,1);
         lcd.print("60");
         break;
        case 9:
         Serial.println("Water Level:70%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",70) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
          lcd.setCursor(11,1);
          lcd.print("70");
         break;

         case 8:
         Serial.println("Water Level:70%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",70) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
          lcd.setCursor(11,1);
          lcd.print("70");
         break;
        case 7:
         Serial.println("Water Level:80%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",80) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
          lcd.setCursor(11,1);
          lcd.print("80");
         break;

         case 6:
         Serial.println("Water Level:80%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",80) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
          lcd.setCursor(11,1);
          lcd.print("80");
         break;

        case 5:
         Serial.println("Water Level:90%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",100) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
          lcd.setCursor(11,1);
          lcd.print("90");

          digitalWrite(relay, LOW);
         break;
        case 4:
         Serial.println("Water Level:100%");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",100) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
         

          lcd.setCursor(11,1);
          lcd.print("100");
         break;

         default:

         Serial.println("Water Level:ND");
         Serial.printf("Set WL... %s\n", Firebase.setFloat(fbdo, "/test/WL",-0.00) ? "ok" : fbdo.errorReason().c_str());
         Serial.printf("Get WL... %s\n", Firebase.getFloat(fbdo, "/test/WL") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
        
         lcd.setCursor(11,1);
         lcd.print("ND");
         
        
      }
       ldist=distance;
     }

      Serial.printf("Set PPM %s\n", Firebase.setFloat(fbdo, "/test/PPM",tdsValue ) ? "ok" : fbdo.errorReason().c_str());
      Serial.printf("Get PPM... %s\n", Firebase.getFloat(fbdo, "/test/PPM") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());

      
    
       Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if(tempC != DEVICE_DISCONNECTED_C) 
  {

    lcd.setCursor(5,1);
    lcd.print(tempC);
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);
    Serial.printf("Set Temp %s\n", Firebase.setFloat(fbdo, "/test/TEMP",tempC ) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Get Temp... %s\n", Firebase.getFloat(fbdo, "/test/Temp") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    
  } 
  else
  {
    Serial.println("Error: Could not read temperature data");
  }

  
       ps= Firebase.getString(fbdo,"test/pump");
      if(ps=="1")
      {
        Serial.print("Pump On");
        digitalWrite(relay,HIGH);
      }else if(ps=="0")
      {
        Serial.print("Pump Off");
        digitalWrite(relay,LOW);
      }

      delay(500);
   



  

   
    
    

    //For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
    FirebaseJson json;

    if (count == 0)
    {
      json.set("value/round/" + String(count), "cool!");
      json.set("vaue/ts/.sv", "timestamp");
      Serial.printf("Set json... %s\n", Firebase.set(fbdo, "/test/json", json) ? "ok" : fbdo.errorReason().c_str());
    }
    else
    {
      json.add(String(count), "smart!");
      Serial.printf("Update node... %s\n", Firebase.updateNode(fbdo, "/test/json/value/round", json) ? "ok" : fbdo.errorReason().c_str());
    }

    Serial.println();

    //For generic set/get functions.

    //For generic set, use Firebase.set(fbdo, <path>, <any variable or value>)

    //For generic get, use Firebase.get(fbdo, <path>).
    //And check its type with fbdo.dataType() or fbdo.dataTypeEnum() and
    //cast the value from it e.g. fbdo.to<int>(), fbdo.to<std::string>().

    //The function, fbdo.dataType() returns types String e.g. string, boolean,
    //int, float, double, json, array, blob, file and null.

    //The function, fbdo.dataTypeEnum() returns type enum (number) e.g. fb_esp_rtdb_data_type_null (1),
    //fb_esp_rtdb_data_type_integer, fb_esp_rtdb_data_type_float, fb_esp_rtdb_data_type_double,
    //fb_esp_rtdb_data_type_boolean, fb_esp_rtdb_data_type_string, fb_esp_rtdb_data_type_json,
    //fb_esp_rtdb_data_type_array, fb_esp_rtdb_data_type_blob, and fb_esp_rtdb_data_type_file (10)

    count++;
  }

}
/// PLEASE AVOID THIS ////

//Please avoid the following inappropriate and inefficient use cases
/**
 * 
 * 1. Call get repeatedly inside the loop without the appropriate timing for execution provided e.g. millis() or conditional checking,
 * where delay should be avoided.
 * 
 * Everytime get was called, the request header need to be sent to server which its size depends on the authentication method used, 
 * and costs your data usage.
 * 
 * Please use stream function instead for this use case.
 * 
 * 2. Using the single FirebaseData object to call different type functions as above example without the appropriate 
 * timing for execution provided in the loop i.e., repeatedly switching call between get and set functions.
 * 
 * In addition to costs the data usage, the delay will be involved as the session needs to be closed and opened too often
 * due to the HTTP method (GET, PUT, POST, PATCH and DELETE) was changed in the incoming request. 
 * 
 * 
 * Please reduce the use of swithing calls by store the multiple values to the JSON object and store it once on the database.
 * 
 * Or calling continuously "set" or "setAsync" functions without "get" called in between, and calling get continuously without set 
 * called in between.
 * 
 * If you needed to call arbitrary "get" and "set" based on condition or event, use another FirebaseData object to avoid the session 
 * closing and reopening.
 * 
 * 3. Use of delay or hidden delay or blocking operation to wait for hardware ready in the third party sensor libraries, together with stream functions e.g. Firebase.RTDB.readStream and fbdo.streamAvailable in the loop.
 * 
 * Please use non-blocking mode of sensor libraries (if available) or use millis instead of delay in your code.
 * 
 * 4. Blocking the token generation process.
 * 
 * Let the authentication token generation to run without blocking, the following code MUST BE AVOIDED.
 * 
 * while (!Firebase.ready()) <---- Don't do this in while loop
 * {
 *     delay(1000);
 * }
 * 
 */
