/*Credits:
- to solara70 on Instructables for SMS service code: https://www.instructables.com/id/How-to-Send-SMS-Text-Messages-From-Your-Arduino-ES/
- to various Github users for various code for the wifi connection
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include <NTPClient.h>
#include <RTCZero.h>  // include the library
#include <WiFiUdp.h>
#include "secrets.h"

// Setup variables for wifi connection
  const char *ssid = SECRET_SSID;
  const char *password = SECRET_PASS;
   
// Define NTP Client and RTC library to get time
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org");
  RTCZero rtc;           // make an instance of the library
  #define GMT 2          //set timezone, for NL this is GMT +2

/*//Setup variables for SMS service
const char _sKapow_Host[] ="kapow.co.uk";
const int  _iKapow_Port =80;

// SMS Service User Account Details:
const char _sKapow_User[]     = SECRET_SMS_USER;
const char _sKapow_Password[] = SECRET_SMS_PASS;  
char _sKapow_Mobile[]="0031646527480";  //For now my personal phone number*/

//  Setup variables for on-board time management
  unsigned long currTime=0;
  unsigned long prevTimeConnected=0;
  unsigned long prevTime=0;
  unsigned long timeConnected=0;
  unsigned long startTime=0;
  int day=1;
  int hour=1;
  int prevHour=0;
  int totalTimeLastHour=0; //time for which device was connected to wifi in minutes the last hour
  int prevConnectionState=WL_CONNECTED;
  int currConnectionState=WL_CONNECTED;

  //Initiliaze Wifi client library
  WiFiClient client;
  
//  Setup variables for data storage of last week
int storageWeek[7][24]={0}; //total time per hour that user spend outside in minutes

void setup() {
   Serial.begin(115200);
   pinMode(LED_BUILTIN,OUTPUT);
   digitalWrite(LED_BUILTIN,LOW);
  // Setup wifi
   WiFi.begin(ssid, password);
  Serial.println("OUTDOOR \n\n4WBB0 Engineering Design\n\n");
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
     digitalWrite(LED_BUILTIN,HIGH);
  //Set time 
  currTime=millis();
  //Prepare storage array
  for (int d=0;d<=6;d++){
    for(int h=0;h<=23;h++){
      storageWeek[d][h]=-1;
    }
  }
  
  Serial.println();
  //WiFi is connected, print it's status
  printWifiStatus();
  // Setup NTP and RTC
  rtc.begin();
  timeClient.begin();
  // Set offset time in seconds to adjust for timezone, which is GMT +2 in the Netherlands
  timeClient.setTimeOffset(7200);
  //Get Epoch time and set it to the RTC
  while(!WiFi.getTime());
     digitalWrite(LED_BUILTIN,LOW);
     delay(500);
   rtc.setEpoch(WiFi.getTime());
   startTime=((rtc.getHours()+GMT)*3600+rtc.getMinutes()*60+rtc.getSeconds())*1000;
}

void loop() {
  currTime=millis()+startTime;
  
  int currHour=(rtc.getHours()+GMT);
  if (hour!=currHour){
    storeTime();
    prevHour=hour;
    hour=currHour;
  }
  if (hour==0&&prevHour==23) {
    day+=1;
    prevHour=0;
  }
  if (day==8){
    uploadData();
    resetStorage();
  }
  currConnectionState=WiFi.status();
  //printStorage();

  // Check for wifi connection status
  if ((currConnectionState != WL_CONNECTED)&&(prevConnectionState==WL_CONNECTED)) {
	    prevTimeConnected=currTime;
      prevConnectionState=WL_NO_SSID_AVAIL;
      timeConnected=0;
      Serial.print("Disconnected since ");
      Serial.print(round(currTime/1000));
      Serial.println(" s after boot.");
      printStorage();
      Serial.print(rtc.getHours()+GMT);Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.println(rtc.getSeconds());
  }
  if (currConnectionState == WL_CONNECTED) {
       digitalWrite(LED_BUILTIN,HIGH);
      if (prevConnectionState!=WL_CONNECTED){
          timeConnected=currTime;
          prevConnectionState=WL_CONNECTED;
          Serial.print("Reconnected at ");
          Serial.print(round(currTime/1000));
          Serial.println(" s after boot.");
          totalTimeLastHour+=round((currTime-prevTimeConnected)/1000/60);
          Serial.print("Total time outside: "); Serial.println(totalTimeLastHour);
  }
        prevTimeConnected=0;
        if (currTime>prevTime+10000){
          int tempTime = (currTime-timeConnected)/1000;
          Serial.print("Connected for ");
          Serial.print(round(tempTime));
          Serial.println(" s.");
            prevTime=currTime;
  }
  }
  if (currConnectionState != WL_CONNECTED) {
       digitalWrite(LED_BUILTIN,LOW);
        if (currTime>prevTime+30000){
            Serial.print("Disconnected for ");
            Serial.print(round((currTime-prevTimeConnected)/1000));
            Serial.println(" s.");
            reconnectWifi();    //try to reconnect every 30 seconds
            prevTime=currTime;
  }
  

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

void storeTime(void){
  if (storageWeek[day][hour-1]==-1){
   storageWeek[day][hour-1]=totalTimeLastHour;
   totalTimeLastHour=0;
  }
}
void printStorage(){
  for (int d=0;d<=6;d++){
    Serial.print("Day ");Serial.print(d);
    for(int h=0;h<=23;h++){
      Serial.print(" "); Serial.print(storageWeek[d][h]);
    }
    Serial.println();
  }
}

void uploadData(){}
void resetStorage(){}

/*bool SendSmsKapow(char* sMobile, char* sMessage)
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
}*/
