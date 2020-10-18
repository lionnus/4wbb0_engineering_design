///////////////////////////////////////////
// 4WBB0 Engineering Design
//
// Arduino code for the Outdoor device, a device that tracks the time spend outside.
// Written for the Arduino Nano 33 IoT board, a low-power Arduino board with interated WiFi chip.
//
// Code written by Lionnus Kesting with help of various libraries and online repositories.
///////////////////////////////////////////

#include "Firebase_Arduino_WiFiNINA.h"
#include <RTCZero.h>  // include the library
#include "secrets.h"
//Setup defined values
#define RECONDELAY 10000 //Delay for how long WiFi doesn't try to reconnect, bigger for less power consumption
#define RECONTRYDELAY 10000 //Delay for how long WiFi is trying to reconnect, shorter for less power consumption

// Setup variables for wifi connection, retrieved from secrets.h or from non-volatile storage by using FlashStorage.h
  const char *ssid = SECRET_SSID;
  const char *password = SECRET_PASS; 
   
// Define RTC to get time
  RTCZero rtc;           // make an instance of the library
  #define GMT 2          //set timezone, for NL this is GMT +2
  //Setup Firebase database variables
  FirebaseData firebaseData;
  char currDate[] = "01-01-1970";   //string for date formatting
  char dataString[] = "users/1/times/01-01-1970"; //strin for Firebase variable location formatting

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
  int prevConnectionState=WL_CONNECTED; //previous state of the WiFi to detect a change and upload to Firebase
  int currConnectionState=WL_CONNECTED;
  
//  Setup variables for data storage of last week
int storageWeek[7][24]={0}; //total time per hour that user spend outside in minutes

void setup() {
   Serial.begin(115200);
   pinMode(LED_BUILTIN,OUTPUT);
   digitalWrite(LED_BUILTIN,LOW);
  // Setup WiFi connection
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
  //Set precise local time management
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
    //Success, then read the retrieved int value
    if (firebaseData.dataType() == "int") {
      outdoorTime = firebaseData.intData();
    }
  } else {
    //Failed to get outdoorTime, print error detail
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
  //Store outsideTime locally in the 2D array
  if (hour!=currHour){
    storeTime();
    prevHour=hour;
    hour=currHour;
  }
  //Check if it is past midnight and if a new day has started
  if (hour==24&&prevHour<24) {
    day++;
    if (day%2) rtc.setEpoch(WiFi.getTime()+GMT*2);
    prevHour=0;
    if (WiFi.status()==WL_CONNECTED){
      uploadData();    //upload data of previous day if not yet uploaded
      outdoorTime=0;  //time user spend outside back to 0 mins
    }
    else {
      //what to do if wifi not available at midnight
    }
    //Update the path to which data is stored with the new date
    sprintf(dataString,"users/1/times/%02d-%02d-20%02d",rtc.getDay(), rtc.getMonth(), rtc.getYear());
  }
  //Reset local 2D storage array after a week
  if (day==8){
    //uploadData();
    resetStorage();
  }
  //Check the status of the WiFi
  currConnectionState=WiFi.status();

  // Decide what to do with this new WiFi status
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
          //Store new data to Firebase
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
       //For optimal power usage: deep sleep for specified amount of time here
        if (currTime>prevTime+RECONDELAY){ //Only try to reconnect every RECONDELAY ms.
            Serial.print("Disconnected for ");
            Serial.print(round((currTime-prevTimeConnected)/1000));
            Serial.println(" s.");
            reconnectWifi();    //try to reconnect every RECONDELAY ms
            prevTime=currTime;
  }
  }
}
//Code which is used to check if the WiFi is available again
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
  // print the SSID of the network the device is attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
//Function to store the time locally in the 2D array
void storeTime(void){
  if (storageWeek[day][hour-1]==-1){
   storageWeek[day][hour-1]=totalTimeLastHour;
   totalTimeLastHour=0;
  }
}
//Function to print the 2D array 
void printStorage(){
  for (int d=0;d<=6;d++){
    Serial.print("Day ");Serial.print(d);
    for(int h=0;h<=23;h++){
      Serial.print(" "); Serial.print(storageWeek[d][h]);
    }
    Serial.println();
  }
}
//Code to upload the data to Firebase
void uploadData(){
     Firebase.setInt(firebaseData,dataString, outdoorTime);
     Serial.println("Data stored to firebase");
  }
  //Function to reset the 2D array at the end of the week
void resetStorage(){
    for (int d=0;d<=6;d++){
    for(int h=0;h<=23;h++){
      storageWeek[d][h]=-1;
    }
  }
  }
