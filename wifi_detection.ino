/*Credits:
- to solara70 on Instructables for SMS service code: https://www.instructables.com/id/How-to-Send-SMS-Text-Messages-From-Your-Arduino-ES/
- to various Github users for various code for the wifi connection
*/

#include "Firebase_Arduino_WiFiNINA.h"
#include <RTCZero.h>  // include the library
#include "secrets.h"
//Setup defined values
#define RECONDELAY 10000 
#define RECONTRYDELAY 10000

// Setup variables for wifi connection
  const char *ssid = SECRET_SSID;
  const char *password = SECRET_PASS;
   
// Define NTP Client and RTC library to get time
  RTCZero rtc;           // make an instance of the library
  #define GMT 2          //set timezone, for NL this is GMT +2
  //Setup Firebase database
  FirebaseData firebaseData;
  char currDate[] = "01-01-1970";
  char dataString[] = "users/1/times/01-01-1970";

/*//Setup variables for SMS service
const char _sKapow_Host[] ="kapow.co.uk";
const int  _iKapow_Port =80;

// SMS Service User Account Details:
const char _sKapow_User[]     = SECRET_SMS_USER;
const char _sKapow_Password[] = SECRET_SMS_PASS;  
char _sKapow_Mobile[]="0031646527480";  //For now my personal phone number*/

//  Setup variables for on-board time management
  unsigned long int currTime=0;
  unsigned long int prevTimeConnected=0;
  unsigned long int prevTime=0;
  unsigned long int timeConnected=0;
  unsigned long int startTime=0;
  unsigned long int outdoorTime=0;
  int day=1;
  int hour=1;
  int prevHour=0;
  int totalTimeLastHour=0; //time for which device was connected to wifi in minutes the last hour
  int prevConnectionState=WL_CONNECTED;
  int currConnectionState=WL_CONNECTED;
  
//  Setup variables for data storage of last week
int storageWeek[7][24]={0}; //total time per hour that user spend outside in minutes

void setup() {
   Serial.begin(115200);
   pinMode(LED_BUILTIN,OUTPUT);
   digitalWrite(LED_BUILTIN,LOW);
  // Setup wifi
    Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  while(WiFi.begin(ssid,password)!=WL_CONNECTED);
  Serial.println("WiFi connected, connecting to Firebase...");
  Firebase.begin(SECRET_FIREBASE_PROJECT,SECRET_FIREBASE_CODE,ssid,password);
  Serial.println("Connected to Firebase \n\nOUTDOOR \n\n4WBB0 Engineering Design\n\n");
  {
    delay(500);
    Serial.print(".");
  }
     digitalWrite(LED_BUILTIN,HIGH);
  //Set local time 
  currTime=millis();
  prevTime=currTime;
  
  //Prepare storage array
  for (int d=0;d<=6;d++){
    for(int h=0;h<=23;h++){
      storageWeek[d][h]=-1;
    }
  }
  Serial.println();
  //WiFi is connected, print it's status
  printWifiStatus();
  // Setup RTC
  rtc.begin();
  //Get Epoch time and set it to the RTC
  while(!WiFi.getTime()){
   digitalWrite(LED_BUILTIN,LOW);
   delay(200);
   digitalWrite(LED_BUILTIN,HIGH);
   delay(200);
  }
  Serial.println("Got WiFi time");
  //Sync RTC with NTP
  rtc.setEpoch(WiFi.getTime()+GMT*60);
  //Setup time variables for code
   startTime=((rtc.getHours()+GMT)*3600+rtc.getMinutes()*60+rtc.getSeconds())*1000;
   sprintf(dataString,"users/1/times/%02d-%02d-20%02d",rtc.getDay(), rtc.getMonth(), rtc.getYear());
   //Retrieve value of outdoorTime for this date
     if (Firebase.getInt(firebaseData, dataString)) {
    //Success, then read the payload value
    //Make sure payload value returned from server is integer
    if (firebaseData.dataType() == "int")) {
      outdoorTime = firebaseData.intData();
    }
  } else {
    //Failed to get outdoorTime print error detail
    Serial.println(firebaseData.errorReason());
  }
   Serial.print("Retrieved current outdoorTime: "); Serial.println(outdoorTime);
   digitalWrite(LED_BUILTIN,LOW);
   delay(500);
  prevTimeConnected=millis();
   //Optimize power usage
   WiFi.lowPowerMode();
  }

void loop() {
  currTime=millis();
  
  int currHour=(rtc.getHours()+GMT);
  if (hour!=currHour){
    storeTime();
    prevHour=hour;
    hour=currHour;
  }
  if (hour==24&&prevHour<24) {
    day++;
    if (day%2) rtc.setEpoch(WiFi.getTime()+GMT*2);
    prevHour=0;
    if (WiFi.status()==WL_CONNECTED){
      uploadData();
      outdoorTime=0;  //time user spend outside back to 0 mins
    }
    else {
      //what to do if wifi not available at midnight?
    }
    sprintf(dataString,"users/1/times/%02d-%02d-20%02d",rtc.getDay(), rtc.getMonth(), rtc.getYear());
  }
  if (day==8){
    //uploadData();
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
          //Update time tracking vars
          outdoorTime+=round((currTime-prevTimeConnected)/1000/60);
          totalTimeLastHour+=round((currTime-prevTimeConnected)/1000/60);
          //Store data to Firebase
          uploadData();
          Serial.print("Total time outside: "); Serial.println(outdoorTime);
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
       //Deep sleep for 1 minute?
        if (currTime>prevTime+RECONDELAY){
            Serial.print("Disconnected for ");
            Serial.print(round((currTime-prevTimeConnected)/1000));
            Serial.println(" s.");
            reconnectWifi();    //try to reconnect every RECONDELAY ms
            prevTime=currTime;
  }
  }
}
int reconnectWifi(void) {
  WiFi.disconnect();
  WiFi.end();
  WiFi.begin(ssid,password);
  delay(RECONTRYDELAY);
  if(WiFi.status()==WL_CONNECTED) {
    WiFi.lowPowerMode();
    return 1;
  }
  else {
    WiFi.end();
    return 0;
    ;
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
void uploadData(){
     Firebase.setInt(firebaseData,dataString, outdoorTime);
     Serial.println("Data stored to firebase");
  }
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
