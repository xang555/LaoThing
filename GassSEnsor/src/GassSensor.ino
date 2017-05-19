
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include "FirebasePath.h"

// Set firebae information
#define FIREBASE_HOST "laothing-d014b.firebaseio.com"
#define FIREBASE_AUTH "0twt7Oi5P0bJ55QSVZenjAgJuRXrXNbwc8YuR5ZT"
// set AP WIFI
#define AP_SSID "laothing-6357854354635200000"
#define AP_PASSWORD "12345678"
#define SETTING_MODE  D0
#define STATE_CONNECTION  D5
#define DHTPIN D4
#define DHTTYPE DHT22

String WIFI_SSID = "NODEMCU";
String WIFI_PASSWORD = "nodemcu";

static bool manual_connect = false; // is manual connection
static bool isCloseServer = false; // count lose connection
static bool isInitFirebase = false;

static int count_connection_lose = 0; // count lose connection

float m = -0.318; //Slope
float b = 1.133; //Y-Intercept
float R0 = 38.55; //Sensor Resistance in fresh air from previous code
int gas_sensor = A0; //Sensor pin

ESP8266WebServer server(80);
bool looping = true;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.asia.pool.ntp.org", 7*3600, 60000);
HTTPClient http;

void setup() {
Serial.begin(250000);
//set pin mode
pinMode(SETTING_MODE,INPUT);
pinMode(STATE_CONNECTION,OUTPUT);
pinMode(gas_sensor, INPUT); //Set gas sensor as input
ESP.eraseConfig();

} // setup


void loop() {

  static uint8_t state = 0;

  switch(state) {

  case 0 :
    Serial.println("check Mode");
    checkMode(state); //check mode
  break;

  case 1 :
    //auto connect wifi and firebase
    Serial.println("auto connect wifi and firebase");
   state = AutoConnectWifiAndFirebase();


  break;

  case 2 :
  //connect wifi
     Serial.println("connect wifi and firebase");
     state = ConnectwifiAndFirebase();

  break;

  case 3 :
  //handle with firebase
   Serial.println("handle firebase");
   handleFirebaseController(state);

  break;

  default :
  state = 0;

  }

  delay(500);

} // loop

void checkMode(uint8_t& state) {

uint8_t mode = digitalRead(SETTING_MODE); // read mode
int time_click_count = 0;

while (mode == HIGH) {

  time_click_count+=100;

  if (time_click_count == 2000) {

    digitalWrite(D1,LOW); //trun off L1
    digitalWrite(D2,LOW);//trun off L2
    digitalWrite(D3,LOW); //trun off L1
    digitalWrite(D4,LOW);//trun off L2

    digitalWrite(STATE_CONNECTION,HIGH);//trun on state LED
    delay(200);
    digitalWrite(STATE_CONNECTION,LOW);//trun off state LED
    delay(200);
    digitalWrite(STATE_CONNECTION,HIGH);//trun on state LED
    delay(200);
    digitalWrite(STATE_CONNECTION,LOW);//trun off state LED

    looping = true;
    state = CreateServerApi(); // create server api for setting mode

    return;

  }

mode = digitalRead(SETTING_MODE); // read mode

delay(100);

}

state = 1;

} // check mode


bool isEnterOfflineMode(){

uint8_t mode = digitalRead(SETTING_MODE); // read mode
if (mode == HIGH) {
  return true;
}

return false;

} // siwtch mode


uint8_t AutoConnectWifiAndFirebase(){

  WiFi.setAutoConnect(true);
  Serial.print("connecting");

  while (WiFi.status() != WL_CONNECTED) {
    if (isEnterOfflineMode()) {
         return 0;
    }
    Serial.print(".");
    delay(500);
  }

initwhenConnectedWifiAndFirebase(); // connected wifi

  return 3; //handle Firebase

} //auto connect wifi and Firebase

uint8_t CreateServerApi() {

          WiFi.softAP(AP_SSID,AP_PASSWORD);
          Serial.print("Ap Wifi IP address: ");
          Serial.println(WiFi.softAPIP());

          server.on("/setting",HTTP_POST,handleSetting);

          server.onNotFound([](){
            server.send(200,"text/html","<h2>Not Found</h2>");
            });

           server.on("/",[](){
             server.send(200,"text/html","<h2>Your Connected ...</h2>");
            });

          server.begin();
          Serial.println("HTTP server started");

            while(looping) {
            server.handleClient();
            Serial.println("handle clien");
            delayMicroseconds(100);
           }

       if (isCloseServer) {
           return 1;
         }

         return 2;

} //create server api for mode setting wifi



void handleSetting(){

  String Str_ssid = server.arg("ssid");
  String Str_password = server.arg("passwd");

  WIFI_SSID = Str_ssid;
  WIFI_PASSWORD = Str_password;
  server.send(200,"application/json","{ \"stat\" : \"ok\" }");
  delay(500);
  looping = false;
  isCloseServer = false;
  WiFi.softAPdisconnect(true);
  server.close();

} // handle setting mode

uint8_t ConnectwifiAndFirebase() {

     const char* ssid ;
     const char* passwd ;

  ssid = WIFI_SSID.c_str();
  passwd = WIFI_PASSWORD.c_str();

  Serial.println(ssid);
  Serial.println(passwd);

  //disconnect wifi
  WiFi.disconnect(true);

  // connect to wifi.
  WiFi.begin(ssid, passwd);
  Serial.print("connecting");

  while (WiFi.status() != WL_CONNECTED) {

    if (isEnterOfflineMode()) {
         return 0;
    }

    Serial.print(".");
    delay(500);
  }

initwhenConnectedWifiAndFirebase(); // connected wifi

  return 3;

} //connect wifi and Firebase


void initwhenConnectedWifiAndFirebase() {

  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  timeClient.begin(); //time server
  setSyncProvider(getTimeNow);
  setSyncInterval(1000 * 60 * 3600);

  if (!isInitFirebase) {
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    isInitFirebase = true;
  }

} // init when connected to wifi  and Firebase


void handleFirebaseController (uint8_t& state) {

while (true) {

  if (isEnterOfflineMode()) {
      state = 0 ;
      return;
  }

  AnswerUplink();  // check device is active
  handleGasSensor(); // send sensor value to Firebase
  checkConnection(); //check connection
  delay(100);

}

} // handle working in Firebase

void AnswerUplink() {

static uint8_t uplink = 0;
uint8_t state_uplink= Firebase.getInt(uplink_path);

// handle error
if (Firebase.failed()) {
    Serial.print("get status uplink failed:");
    Serial.println(Firebase.error());
    count_connection_lose++;
    return;
}

if (state_uplink != uplink) {
uint8_t ack = state_uplink;
Firebase.setInt(ack_path,ack);
// handle error
if (Firebase.failed()) {
    Serial.print("set status ack failed:");
    Serial.println(Firebase.error());
    count_connection_lose++;
    return;
}

count_connection_lose = 0;

uplink = state_uplink;

}

} // check if have uplink signal

/*------------- Handle Temp and Humi Sensor ------------*/

void handleGasSensor() {

  double ppm = getSensorValue();
  Serial.println(ppm);
 static int repeatTime = 0;
 static bool isNotify = false;

  if (ppm > 2000) {

    if (repeatTime > 100) {
      isNotify = false;
    }

    if (!isNotify) {
        AlertNotification(); //send fcm
        isNotify = true;
        repeatTime = 0;
    }

    repeatTime ++;

    return;
  }

  isNotify = false;

} //handle Sensor

void AlertNotification() {

Firebase.pushString(alert_listener_path, DEVICE_ID);
// handle error
if (Firebase.failed()) {
    Serial.print("push alert failed:");
    Serial.println(Firebase.error());
    count_connection_lose++;
    return;
}

count_connection_lose = 0;

} //alert notification


double getSensorValue(){

  float sensor_volt; //Define variable for sensor voltage
  float RS_gas; //Define variable for sensor resistance
  float ratio; //Define variable for ratio
  float sensorValue = analogRead(gas_sensor); //Read analog values of sensor

  sensor_volt = sensorValue * (5.0 / 1024.0); //Convert average to voltage
  RS_gas = ((5.0 * 20.0) / sensor_volt) - 20.0; //Calculate RS in fresh air
  ratio = RS_gas / R0;   // Get ratio RS_gas/RS_air

  double ppm_log = (log10(ratio) - b) / m; //Get ppm value in linear scale according to the the ratio value
  double ppm = pow(10, ppm_log); //Convert ppm value to log scale
  //double percentage = ppm / 10000; //Convert to percentage

  return ppm;

} // get gass sensor



void checkConnection() {

  if (count_connection_lose > 10) {
    digitalWrite(STATE_CONNECTION, LOW);
  }else{
    digitalWrite(STATE_CONNECTION, HIGH);
  }

} //check connection to internet

long getTimeNow(){

  long ntp_time = 0;

   while (ntp_time < 1000000) {

     if (isEnterOfflineMode()) {
          return 0;
     }

    if (count_connection_lose > 15) {
      break;
     }

     timeClient.update();
     ntp_time = timeClient.getEpochTime();

     ++count_connection_lose;
     Serial.println("get time ...");
     delay(100);

   }

  return ntp_time;

} // get timestramp
