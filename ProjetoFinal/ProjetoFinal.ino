/*********
  Gourav Das
  Complete project details at https://hackernoon.com/cloud-home-automation-series-part-2-use-aws-device-shadow-to-control-esp32-with-arduino-code-tq9a37aj
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
/* modified by STK 01/may/2024 */

#include <AWS_IOT.h>
#include <WiFi.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ArduinoJson.h> //Download latest ArduinoJson 6 Version library only by Benoît Blanchon from Manage Libraries

AWS_IOT hornbill;
#define ledPin 25 // LED Pin
#define LUM_PIN 34 // Luminosity Sensor Pin
int lum = 0;
char buffer[256];

char WIFI_SSID[]="Oliver";
char WIFI_PASSWORD[]="Mili0507";
char HOST_ADDRESS[]="a1v892lowvzlbv-ats.iot.us-east-1.amazonaws.com";
char CLIENT_ID[]=               "my-esp32"; 
char TOPIC_NAME[]=              "house/room/luminosity";
char SHADOW_GET[]=              "$aws/things/my-esp32/shadow/get/accepted";
char SENT_GET[]=                "$aws/things/my-esp32/shadow/get";
char SHADOW_UPDATE[]=           "$aws/things/my-esp32/shadow/update";
char SHADOW_UPDATE_DOCUMENTS[]= "$aws/things/my-esp32/shadow/update/documents";
char SHADOW_UPDATE_ACCEPTED[]=  "$aws/things/my-esp32/shadow/update/accepted";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;
String epochTime;

int status = WL_IDLE_STATUS;
int tick=0,msgCount=0,msgReceived = 0;

unsigned long myTime, mypreviousTime, interval;

char payload[512];
char reportpayload[512];
char rcvdPayload[512];

char power_desired[8];

void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
    strncpy(rcvdPayload,payLoad,payloadLen);
    rcvdPayload[payloadLen] = 0;


   Serial.print("handler: Message arrived in topic: ");
   Serial.println(topicName);

    msgReceived = 1;
}

void updateShadow (int power)
{ 
  sprintf(reportpayload,"{\"state\": {\"reported\": {\"power\": \%d}}}",power);
  delay(3000);   
    if(hornbill.publish(SHADOW_UPDATE,reportpayload) == 0)
      {       
        Serial.print("Publish Message:");
        Serial.println(reportpayload);
      }
    else
      {
        Serial.println("Publish failed");
        Serial.println(reportpayload);   
      }  
} 

void send_luminosity(){
    while(!timeClient.update()) 
    {
        timeClient.forceUpdate();
    }

    // The formattedDate comes with the following format:
    // 2018-05-28T16:00:13Z
    // We need to extract date and time
    formattedDate = timeClient.getFormattedDate();
    //Serial.println(formattedDate);

    // Extract date
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    //Serial.print("DATE: ");
    //Serial.println(dayStamp);
    // Extract time
    timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
    //Serial.print("HOUR: ");
    //Serial.println(timeStamp);

    epochTime = timeClient.getEpochTime();
    Serial.println(epochTime);

    // Reading temperature or humidity takes about 250 milliseconds!
    // Reading luminosity 
    lum = analogRead(LUM_PIN);
    lum = lum * 100 / 4096;

    Serial.println(formattedDate);

    JsonDocument doc;
    doc["serialNumber"] = "SN-D7F3C8947867";
    doc["dateTime"] = formattedDate.c_str();
    doc["activated"] = true;
    doc["device"] = "my-esp32";
    doc["type"] = "MySmartIoTDevice";
    //doc["payload"] = 
    JsonObject payload = doc.createNestedObject("payload");
    payload["luminosity"] = lum;

    //serializeJsonPretty(doc, Serial);
    serializeJson(doc, buffer);

    if(hornbill.publish(TOPIC_NAME, buffer) == 0)   // Publish the message(date, Temp and humidity)
    {        
        Serial.print("Publish Message:");   
        Serial.println(buffer);
        serializeJsonPretty(doc, Serial);
    }
    else
    {
        Serial.println("Publish failed");
    }
}



void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);

    delay(1000);  

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
   
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("\nConnecting");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(WIFI_SSID);
        delay(5000);// wait 5 seconds for connection:
    }

    Serial.println("Connected to wifi");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());

    timeClient.begin();
    // Set offset time in seconds to adjust for your timezone, for example:
    // GMT -3 = -10800 (SAO PAULO)
    timeClient.setTimeOffset(-10800);

    if(hornbill.connect(HOST_ADDRESS,CLIENT_ID)== 0) //Connect to AWS IoT COre
    {
        Serial.println("Connected to AWS");
        delay(5000);

        if(0 == hornbill.subscribe(SHADOW_UPDATE_ACCEPTED, mySubCallBackHandler)) //Subscribe to Accepted GET Shadow Service
        {
            Serial.println("Subscribe Successfull");
        }
        else
        {
            Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
            while(1);
        }
    }
    else
    {
        Serial.println("AWS connection failed, Check the HOST Address");
        while(1);
    }
  
    interval = 10000; //send DHT11 values time interval in milliseconds 
    myTime=millis();
    mypreviousTime = myTime;

    delay(1000);

}    

void loop() {
  
  myTime = millis();
  if (myTime - mypreviousTime >= interval) {
    // save the last time a message was sent
      mypreviousTime = myTime;
      send_luminosity();
    }
  else {
	  if(msgReceived == 1){
		    msgReceived = 0;
		    StaticJsonDocument<256> doc;
		    deserializeJson(doc, rcvdPayload);
		    if (doc.isNull()) {
			      Serial.println("parseObject() failed");
			      return;
			    }
      Serial.println("STK Subscribe");
      JsonObject state = doc["state"];
      bool state_desired = state.containsKey("desired");

      if (state_desired == 1 ) {

        int power_desired = doc["state"]["desired"]["power"];
     
        if(power_desired == 1 )   digitalWrite(ledPin, HIGH);
        else if (power_desired == 0) digitalWrite(ledPin, LOW);
        
        int power_reported = power_desired;

        updateShadow(power_reported);  

      }
	  }
  }
}

