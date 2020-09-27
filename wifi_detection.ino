#include <SPI.h>
#include <WiFiNINA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


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

  //Initiliaze Wifi client library
  WiFiClient client;
  
//  Setup variables for data storage
struct data {
  int weekNumber;        //Save the number of the week
  int useHour[7][24];   //For every day of week and every hour of week save the hourly time spend outside in minutes
  int totTime;          //Total time in minutes that user has spend outside in the week
}week1;

void setup() {
   Serial.begin(115200);
  // Setup wifi
   WiFi.begin(ssid, password);
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  //WiFi is connected, print it's status
  printWifiStatus();
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
  Serial.print("Disconnected since ");
  Serial.println(round(currTime/1000));
  }
  if (currConnectionState == WL_CONNECTED) {
      if (prevConnectionState!=WL_CONNECTED){
          timeConnected=currTime;
          prevConnectionState=WL_CONNECTED;
          Serial.print("Reconnected at ");
          Serial.println(round(currTime/1000));
  }
        prevTimeConnected=0;
        if (currTime>prevTime+2000){
          int tempTime = (currTime-timeConnected)/1000;
          Serial.print("Connected for ");
          Serial.println(round(tempTime));
            prevTime=currTime;
  }
  }
  if (currConnectionState != WL_CONNECTED) {
        if (currTime>prevTime+2000){
            Serial.print("Disconnected for ");
            Serial.println(round((currTime-prevTimeConnected)/1000));
            prevTime=currTime;
  }
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
