/*
   With this library an ESP8266 can ping a remote machine and know if it's reachable.
   It provides some basic measurements on ping messages (avg response time).
*/
//#ifndef UNIT_TEST
#include <PanasonicHeatpumpIR.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

//#include <IRutils.h>
#define IR_SEND_PIN D2
#define DHT11_PIN D7
#define DHTTYPE DHT11
DHT dht(DHT11_PIN, DHTTYPE);
IRSenderBitBang irSender(D2);
int ledPin = D6;
WiFiServer server(8089);
float valt;
float valh;
int tempPin = 1;
int temp = 1;
float cel;
int oldval = -1;
IPAddress ip(192, 168, 1, 28);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);
const char* ssid     = "pna";
const char* password = "kaffekop";
PanasonicDKEHeatpumpIR *heatpumpIR;
const IPAddress remote_ip(192, 168, 1, 254);

unsigned long startTime = 0;
const long interval = 900000;  // 1 second = 1000 millis
bool timeFlag = false;
bool startS = false;
void connect_host() {
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected with ip ");
  Serial.println(WiFi.localIP());

  Serial.print("Pinging ip ");
  Serial.println(remote_ip);
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

}

void ping() {
  if (Ping.ping(remote_ip)) {
    Serial.println("Success!!");
    startS = true;
    timeFlag = true;
    startTime = millis(); // Begin timing sequence
  } else {
    Serial.println("Error :(");
    startS = false;
  }
}

void setup() {
  Serial.begin(9600);
  delay(10);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  //irsend.begin();
  // We start by connecting to a WiFi network
  heatpumpIR = new PanasonicDKEHeatpumpIR();
  Serial.println();
  Serial.println("Connecting to WiFi");
  connect_host();
  //startS==true;
  ping();

}

void loop() 
{
  String request="";
  unsigned long currentTime = millis();
  WiFiClient client = server.available();
  if (!client) {
    if (currentTime - startTime >= interval) // End timing sequence
    {
      Serial.println("start to ping again....");
      ping();
      if (startS == true) {
        Serial.println("Success again!!");
        //Serial.println("Do something here");
      }
      else {
        Serial.println("Not success, try to connect and ping again..");
        connect_host();
        ping();
      }
      // do stuff here when time is up
    }
    return;
  }
  if (client.connected()) {
    Serial.println("new client");
    delay(100);
    if (client.available()) {
      delay(10);
      request = client.readStringUntil('\r');
      Serial.println(request);
    } else {
      client.flush();
      Serial.print("disconnected");
      delay(10);
      return;
    }
  }
  // Match the request
  int value = -1;
  if (request.indexOf("/LED=ON") != -1) {
    digitalWrite(ledPin, HIGH);
    value = HIGH;
    oldval = value;
  }
  if (request.indexOf("/LED=OFF") != -1) {
    digitalWrite(ledPin, LOW);
    value = LOW;
    oldval = value;
  }
  if (request.indexOf("/TH=TEMP") != -1) {
    valt = dht.readTemperature();
    valh = dht.readHumidity();
    temp = 2;
  }
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  //client.print("Welcome to Cuong's security webpage <br>");
  client.print("Airpump status is now: ");

  if (oldval == HIGH) {
    client.print("On. ");
    Serial.print("on");
    heatpumpIR->send(irSender, POWER_ON, MODE_HEAT, FAN_2, 24, VDIR_UP, HDIR_AUTO);
    oldval = -1;
  } else if (oldval == LOW) {
    client.print("Off. ");
    Serial.print("off");
    heatpumpIR->send(irSender, POWER_OFF, MODE_HEAT, FAN_2, 24, VDIR_UP, HDIR_AUTO);
    oldval = -1;
  }
  client.print("<br>Temperature: ");
  if (temp == 2) {
    client.print(valt);
    client.print("<br>Humidity: ");
    client.print(valh);
  }
  client.println("<br><br>");
  if (value == LOW) {
    client.println("Click <a href=\"/LED=ON\">here</a> Turn airpump ON<br>");
  } else {
    client.println("Click <a href=\"/LED=OFF\">here</a> Turn airpump OFF<br>");
  }
  client.println("Click <a href=\"/TH=TEMP\">here</a> to measure temperature<br>");
  client.println("</html>");
  client.flush();
  Serial.println("Client disconnected");
  Serial.println("");
}

