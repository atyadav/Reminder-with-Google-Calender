/*

  Copyright <2018> <Andreas Spiess>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.


  Based on the HTTPS library of Sujay Phadke ( https://github.com/electronicsguy/ESP8266/tree/master/HTTPSRedirect )

*/



#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
// #include <credentials.h>

/*
  The credentials.h file at least has to contain:
  char mySSID[]="your SSID";
  char myPASSWORD[]="your Password";
  const char *GScriptIdRead = "............"; //replace with you gscript id for reading the calendar
  const char *GScriptIdWrite = "..........."; //replace with you gscript id for writing the calendar
  It has to be placed in the libraries folder
  If you do not want a credentials file. delete the line: #include <credentials.h>
*/


//Connection Settings
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort = 443;

unsigned long entryCalender, entryPrintStatus, entryInterrupt, heartBeatEntry, heartBeatLedEntry;
String url;
int tempSensorValue;
bool statusFlag;

#define UPDATETIME 1800000

#ifdef CREDENTIALS
const char*  ssid = mySSID;
const char* password = myPASSWORD;
//const char *GScriptIdRead = GoogleScriptIdRead;
const char *GScriptIdWrite = GoogleScriptIdWrite;
#else
//Network credentials
const char*  ssid = "Airtel_9819822079";
const char* password = "air83659"; //replace with your password
//Google Script ID
const char *GScriptIdSendMail = "AKfycbxSVNQPgeG6r-_I-ZV0TnVNYJqYUvOKB8-HloSE7XyRUqru3-RRo1w"; //replace with you gscript id for reading the calendar
const char *GScriptIdWrite = "AKfycbyap1gEXPcrEjZ1oKklTduobBSbRUX-DV4vkK-3TA5XnWJ44rShx6Kw"; //replace with you gscript id for writing the calendar
#endif
//script.google.com/macros/s/AKfycbxSVNQPgeG6r-_I-ZV0TnVNYJqYUvOKB8-HloSE7XyRUqru3-RRo1w/exec

#define NBR_EVENTS 4


//String  possibleEvents[NBR_EVENTS] = {"Laundry", "Meal",  "Telephone", "Shop"};
String  possibleEvents[NBR_EVENTS] = {"Milk_Delivered"};
//byte  LEDpins[NBR_EVENTS]    = {D2, D7, D4, D8};  // connect LEDs to these pins or change pin number here
byte  switchPins[NBR_EVENTS] = {A0};  // connect switches to these pins or change pin number here
bool switchPressed[NBR_EVENTS];
boolean beat = false;
int beatLED = 0;

enum taskStatus {
  none,
  due,
  done
};

taskStatus taskStatus[NBR_EVENTS];
HTTPSRedirect* client = nullptr;

String calendarData = "";
bool calenderUpToDate;

//Connect to wifi
void connectToWifi() {
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    ESP.reset();
  }
  Serial.println("Connected to Google");
}



void createEvent(String title,String Status) {
  // Serial.println("Start Write Request");

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }
  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    ESP.reset();
  }
  if (Status=="Delivered")
  {
      // Create event on Google Calendar
    String url = String("/macros/s/") + GScriptIdWrite + "/exec" + "?title=" + title;
    client->GET(url, host);
    //  Serial.println(url);
    Serial.println("Write Event created ");
  }  
  else{    
    //Send Email 
    String url = String("/macros/s/") + GScriptIdSendMail + "/exec"+ "?SensorValue=" + title;
    client->GET(url, host);
    //  Serial.println(url);
    Serial.println("Email Sent");
  }
  

  calenderUpToDate = false;
}




bool eventHere(int task) {
  if (calendarData.indexOf(possibleEvents[task], 0) >= 0 ) {
    //    Serial.print("Task found ");
    //    Serial.println(task);
    return true;
  } else {
    //   Serial.print("Task not found ");
    //   Serial.println(task);
    return false;
  }
}



void setup() {
  Serial.begin(115200);
  Serial.println("Reminder_V2");

  connectToWifi();
  //getCalendar();
  entryCalender = millis();
  tempSensorValue = 1024;
  statusFlag = true;
}


void loop() 
{

  int sensorValue = analogRead(A0);
 
  
  // print out the value you read:
  Serial.write("Sensor Value : ");
  Serial.println(sensorValue);
  //Serial.write("Start of Timer : ");
  //Serial.println(entryCalender);
  //Serial.write("Current Timer : ");
  //Serial.println(millis());
  Serial.write("Temp Sensor Value : ");
  Serial.println(tempSensorValue);
  
  int diffValue=sensorValue-tempSensorValue;

  Serial.write("Diif Value : ");
  Serial.println(diffValue);
  
//  if (diffValue > 25)
//  {
//    Serial.println("Delivery collected");
//    statusFlag = false;
//  }
//  if ((diffValue < 0) and (diffValue > -25))
//  {
//    Serial.println("Delivered in box");
//    statusFlag = true;
//    
//  }
  

  //Inform about delivery got delivered
  
  //if ((millis() > entryCalender + UPDATETIME) and (sensorValue < 1000))

  if (abs(diffValue) > 25) 
  {
    
      if ((sensorValue < 1000))
      {
        //getCalendar();
        entryCalender = millis();
        
        createEvent(String(sensorValue),"Delivered");
        //Serial.println("Event created");
        createEvent(String(sensorValue),String(statusFlag));
        //Serial.println("Mail Sent");
    
        Serial.write("New Timer : ");
        //Serial.println(entryCalender);
       }
       
      if ((sensorValue > 1000) and diffValue!=0 )
      {
          //Case when delivery get collected
          createEvent(String(sensorValue),String(statusFlag));
      }
  } 
   
   tempSensorValue = sensorValue;
   delay(5000);        // delay in between reads for stability
  
}
