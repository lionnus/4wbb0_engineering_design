/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

#define TIME_ON 1 //time for which WiFi turns on in minutes
#define TIME_OFF 1  //time for which WiFi turns off in minutes

/* Set these to your desired credentials. */
char *ssid = "4WBB0 Test";
char *password = "lionnuskesting";
unsigned long int currTime = 0;
unsigned long int prevTime = 0;

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void handleRoot() {
	server.send(200, "text/html", "<h1>You are connected</h1>");
}

void setup() {
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print("Configuring access point...");
	/* You can remove the password parameter if you want the AP to be open. */
  WiFi.mode(WIFI_AP);
   pinMode(LED_BUILTIN, OUTPUT);

	 if (WiFi.softAP(ssid,password)){
    digitalWrite(LED_BUILTIN, HIGH);
  }
    else {
    digitalWrite(LED_BUILTIN,LOW);
  }

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.on("/", handleRoot);
	server.begin();
	Serial.println("HTTP server started");
  Serial.print("WiFi status:");
WiFi.printDiag(Serial);
 
}

void loop() {
  currTime=millis();

  if (prevTime+(2*60*1000)<currTime) {
    Serial.println("Turning off WiFi.");
    WiFi.softAPdisconnect(true);  //Turn off AP
    Serial.println("WiFi disconnected, waiting for the specified delay.");
    delay(1*60*1000);//delay for half an hour
    WiFi.mode(WIFI_AP);

      if (WiFi.softAP(ssid, password)){
    digitalWrite(LED_BUILTIN, HIGH);
  }
    else {
    digitalWrite(LED_BUILTIN,LOW);
  }
    Serial.println("WiFi soft Access Point re-enabled.");
    WiFi.printDiag(Serial);
    currTime=millis();
    prevTime=currTime;
  }
	server.handleClient();
}
