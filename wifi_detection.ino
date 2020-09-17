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
  int prevConnectionState=0;
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
  if ((WiFi.status() == WL_NO_SSID_AVAIL)&&(prevConnectionState==WL_CONNECTED)) {
	prevTimeConnected=currTime;
  prevConnectionState=WL_NO_SSID_AVAIL;
  timeConnected=0;
  }
  else if (WiFi.status() == WL_CONNECTED) {
        prevConnectionState=WL_CONNECTED;
        prevTimeConnected=0;
        }
  if ((WiFi.status() == WL_CONNECTED)&&(prevConnectionState==WL_NO_SSID_AVAIL)){
    timeConnected=currTime;
  }
  
  if (currTime>prevTime+2000){
  Serial.println("Connected for ");Serial.println((currTime-timeConnected)/1000);Serial.println("sec.");
  prevTime=currTime;
  }
}
