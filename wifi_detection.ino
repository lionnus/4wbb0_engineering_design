/*Credits:
- to solara70 on Instructables for SMS service code: https://www.instructables.com/id/How-to-Send-SMS-Text-Messages-From-Your-Arduino-ES/
- to various Github users for various code for the wifi connection
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "secrets.h"

// Setup variables for wifi connection
  const char *ssid = SECRET_SSID;
  const char *password = SECRET_PASS;
   
// Define NTP Client to get time
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Setup variables for SMS service
const char _sKapow_Host[] ="kapow.co.uk";
const int  _iKapow_Port =80;

// SMS Service User Account Details:
const char _sKapow_User[]     = SECRET_SMS_USER;
const char _sKapow_Password[] = SECRET_SMS_PASS;  
char _sKapow_Mobile[]="0031646527480";  //For now my personal phone number

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
  if ((currConnectionState != WL_CONNECTED)&&(prevConnectionState==WL_CONNECTED)) {
	    prevTimeConnected=currTime;
      prevConnectionState=WL_NO_SSID_AVAIL;
      timeConnected=0;
      Serial.print("Disconnected since ");
      Serial.print(round(currTime/1000));
      Serial.println(" ms after boot.");
  }
  if (currConnectionState == WL_CONNECTED) {
      if (prevConnectionState!=WL_CONNECTED){
          timeConnected=currTime;
          prevConnectionState=WL_CONNECTED;
          Serial.print("Reconnected at ");
          Serial.print(round(currTime/1000));
          Serial.println(" ms after boot.");
  }
        prevTimeConnected=0;
        if (currTime>prevTime+2000){
          int tempTime = (currTime-timeConnected)/1000;
          Serial.print("Connected for ");
          Serial.print(round(tempTime));
          Serial.println(" ms.");
            prevTime=currTime;
  }
  }
  if (currConnectionState != WL_CONNECTED) {
        if (currTime>prevTime+2000){
            Serial.print("Disconnected for ");
            Serial.print(round((currTime-prevTimeConnected)/1000));
            Serial.println(" ms.");
            prevTime=currTime;
  }
  reconnectWifi();
  }
}

void reconnectWifi() {
  WiFi.disconnect();
  WiFi.end();
  WiFi.begin(ssid,password);
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

bool SendSmsKapow(char* sMobile, char* sMessage)
{
  WiFiClient clientSms;

  int iAttempts=0;
  int iMaxAttempts=10;
  Serial.print("Connecting to KAPOW host");
  while (!clientSms.connect(_sKapow_Host, _iKapow_Port)) {
    Serial.print(".");
    iAttempts++;
    if (iAttempts > iMaxAttempts) {
      Serial.println("\nFailed to Connect to KAPOW");
      return true;
    }
    delay(1000);
  }
  Serial.println("\nConnected to KAPOW");
  delay(1000);

  Serial.println("Sending HTTP request to KAPOW:");

  //An example GET request would be:
  //http://www.kapow.co.uk/scripts/sendsms.php?username=test&password=test&mobile=07777123456&sms=Test+message

  char sHttp[500]= "";
  strcat(sHttp, "GET /scripts/sendsms.php?username=");
  strcat(sHttp, _sKapow_User);
  strcat(sHttp, "&password=");
  strcat(sHttp, _sKapow_Password);
  strcat(sHttp, "&mobile=");
  strcat(sHttp, sMobile);
  strcat(sHttp, "&sms=");
  strcat(sHttp, sMessage);
  strcat(sHttp, "&returnid=TRUE\n\n");
    
  Serial.println(sHttp);
  clientSms.print(sHttp);

  Serial.println("Waiting for response (10 secs)...");
  delay(10 * 1000);
  
  char  sReply[100] = "";
  int   iPos = 0;

  while (clientSms.available()) {
    char c = clientSms.read();
    Serial.print(c);
    sReply[iPos] = c;
    iPos++;
    if (iPos == 99) break;
  }
  sReply[iPos] = '\0';

  // check if reply contains OK
  bool bResult = (strstr(sReply, "OK") != NULL);

  if (bResult)
    Serial.println("\nSMS: Succesfully sent");
  else
    Serial.println("\nSMS: Failed to Send");

  if (!clientSms.connected()) {
    Serial.println("Disconnecting from KAPOW");
    clientSms.stop();
  }

  return bResult;
}
