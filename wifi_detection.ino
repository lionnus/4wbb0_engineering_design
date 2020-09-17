#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <ESP8266WebServer.h>


// Setup variables for wifi connection
  const char *ssid = "Lionnus";
  const char *password = "hannahisdom";
// Define NTP Client to get time
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org");

//  Setup variables for on-board time management
  double currTime=millis();
  double prevTimeConnected=0;
  double prevTime=0;
  int prevConnectionState=WL_CONNECTED;
  int currConnectionState=WL_CONNECTED;
  double timeConnected=0;
//  Setup variables for data storage


void setup() {
   Serial.begin(115200);
  // Setup wifi
   WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  // Setup NTP
    timeClient.begin();
  // Set offset time in seconds to adjust for timezone, which is GMT +2 in the Netherlands
  timeClient.setTimeOffset(7200);
}

void loop() {
  currTime=millis();
  currConnectionState=WiFi.status();
  timeClient.update();
   /* 
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);  

  int currentHour = timeClient.getHours();
  Serial.print("Hour: ");
  Serial.println(currentHour);  

  int currentMinute = timeClient.getMinutes();
  Serial.print("Minutes: ");
  Serial.println(currentMinute); 
  */
  // Check for wifi connection status
  if ((currConnectionState == WL_NO_SSID_AVAIL)&&(prevConnectionState==WL_CONNECTED)) {
	prevTimeConnected=currTime;
  prevConnectionState=WL_NO_SSID_AVAIL;
  timeConnected=0;
  Serial.printf("Disconnected since %d s after boot\n",round(currTime/1000));
  }
  if (currConnectionState == WL_CONNECTED) {
      if (prevConnectionState==WL_NO_SSID_AVAIL){
          timeConnected=currTime;
          prevConnectionState=WL_CONNECTED;
          Serial.printf("Reconnected at %d s after boot\n",round(currTime/1000));
  }
        prevTimeConnected=0;
        if (currTime>prevTime+2000){
          int tempTime = (currTime-timeConnected)/1000;
          Serial.printf("Connected for %d s.\n",round(tempTime));
            prevTime=currTime;
  }
  }
  if (currConnectionState == WL_NO_SSID_AVAIL) {
        if (currTime>prevTime+2000){
            Serial.printf("Disconnected for %d s\n",round((currTime-prevTimeConnected)/1000));
            prevTime=currTime;
  }
  }
}
