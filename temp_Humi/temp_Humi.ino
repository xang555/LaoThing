#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include "FirebasePath.h"
#include <DHT.h>

// Set firebae information
#define FIREBASE_HOST "laothing-d014b.firebaseio.com"
#define FIREBASE_AUTH "0twt7Oi5P0bJ55QSVZenjAgJuRXrXNbwc8YuR5ZT"
// set AP WIFI
#define AP_SSID "TempHumi10001"
#define AP_PASSWORD "12345678"
#define SETTING_MODE  D0
#define STATE_CONNECTION  D5
#define DHTPIN D4
#define DHTTYPE DHT22

String WIFI_SSID = "";
String WIFI_PASSWORD = "";

static bool manual_connect = false; // is manual connection
static bool isCloseServer = false; // count lose connection
static bool isInitFirebase = false;

static int count_connection_lose = 0; // count lose connection

IPAddress ip(192,168,4,1);
ESP8266WebServer server(ip,80);
bool looping = true;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.asia.pool.ntp.org", 7*3600, 60000);
DHT dht(DHTPIN, DHTTYPE);

void setup() {
Serial.begin(250000);
//set pin mode
pinMode(SETTING_MODE,INPUT);
pinMode(STATE_CONNECTION,OUTPUT);
dht.begin();
WiFi.softAPdisconnect(true);
} // setup


void loop() {

  static uint8_t state = 0;

  switch(state) {

  case 0 :
    checkMode(state); //check mode
  break;

  case 1 :
    //auto connect wifi and firebase
   state = AutoConnectWifiAndFirebase();
  break;
 
  case 2 :
  //connect wifi
     state = ConnectwifiAndFirebase();

  break;

  case 3 :
  //handle with firebase
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

    digitalWrite(STATE_CONNECTION,HIGH);//trun on state LED
    delay(100);
    digitalWrite(STATE_CONNECTION,LOW);//trun off state LED
    delay(100);
    digitalWrite(STATE_CONNECTION,HIGH);//trun on state LED
    delay(100);
    digitalWrite(STATE_CONNECTION,LOW);//trun off state LED

    looping = true; // loop create wifi
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

  WiFi.softAPdisconnect(true);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.reconnect();
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

            WiFi.enableSTA(false);
            WiFi.mode(WIFI_AP);
            WiFi.softAP(AP_SSID,AP_PASSWORD);
            server.on("/setting",HTTP_POST,handleSetting);
            server.onNotFound([](){
            server.send(200,"text/html","<h2>Not Found</h2>");
            });
           server.on("/",[](){
             server.send(200,"text/html","<h2>Your Connected ...</h2>");
            });
            server.begin();
            while(looping) {
            server.handleClient();
            Serial.println("handle clien");
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
  ESP.eraseConfig();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  //disconnect wifi
  WiFi.softAPdisconnect(true);
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
  handleTempandHumiSensor(); // send sensor value to Firebase
  checkConnection(); //check connection
  delay(100);

}

} // handle working in Firebase

void AnswerUplink() {

static uint8_t uplink = 0;
int state_uplink= Firebase.getInt(uplink_path);

// handle error
if (Firebase.failed()) {
    Serial.print("get status uplink failed:");
    Serial.println(Firebase.error());
    count_connection_lose++;
    return;
}
count_connection_lose =0;
if (state_uplink != uplink) {
int ack = state_uplink;
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

void handleTempandHumiSensor() {

  static float humi_prev = 0.0;
  static float hic_prev = 0.0;
  static int hour_prev = 0;
  static int minute_prev = 0;
  static int day_prev = 0;

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

if (humi_prev != h) {
  Firebase.setFloat(humi_value_path, h);
  if (!handleFirebaseError("Set humi to Firebase error")) {
      return;
  }
  humi_prev = h;
}

if (hic_prev != hic) {
  Firebase.setFloat(temp_value_path, hic);
  if (!handleFirebaseError("Set temp to Firebase error")) {
    return;
  }
  hic_prev = hic;
}

String FullDateTime ="";

if (day_prev != day()) {

  String mday = "";
  if (day() < 10) {
  mday +=0;
  }
  mday +=day();
  
  String mmonth = "";
  if (month() < 10) {
    mmonth +=0;
  }
  mmonth+=month();

FullDateTime += mday;
FullDateTime +="-";
FullDateTime +=mmonth;
FullDateTime += "-";
FullDateTime +=year();

Firebase.setString(update_fulldatetime_path,FullDateTime);
if (!handleFirebaseError("Set FullDateTime to Firebase error")) {
  return;
}

day_prev = day();

}


if (hour_prev != hour())  {
  Firebase.setInt(update_time_unit_hour_path, hour());
  if (!handleFirebaseError("Set time hour to Firebase error")) {
    return;
  }
  hour_prev = hour();
}

if (minute_prev != minute()) {
  Firebase.setInt(update_time_unit_minute_path, minute());
  if (!handleFirebaseError("Set time minute to Firebase error")) {
    return;
  }
  minute_prev = minute();
}

} //handle Sensor

bool handleFirebaseError(char* msg) {

  // handle error
  if (Firebase.failed()) {
      Serial.print(msg);
      Serial.println(Firebase.error());
      count_connection_lose++;
      return false;
  }
  
count_connection_lose =0;
return true;
} //handel firebase error

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
