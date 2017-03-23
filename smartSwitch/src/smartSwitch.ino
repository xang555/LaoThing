#include <Arduino.h>


#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// Set firebae information
#define FIREBASE_HOST "laothing-d014b.firebaseio.com"
#define FIREBASE_AUTH "0twt7Oi5P0bJ55QSVZenjAgJuRXrXNbwc8YuR5ZT"
// set AP WIFI
#define AP_SSID "laothing-837548734"
#define AP_PASSWORD "1234567890"
#define SETTING_MODE  D0

String WIFI_SSID = "NODEMCU";
String WIFI_PASSWORD = "nodemcu";

const String DEVICE_ID = "837548734"; //set DEVICE ID
const String uplink_path = DEVICE_ID + "/active/uplink"; // set uplink path
const String ack_path = DEVICE_ID + "/active/ack"; // set ack path
const String status_channel_one_path = DEVICE_ID + "/status/L1"; // set channle one path
const String status_channel_two_path = DEVICE_ID + "/status/L2"; // set channle two path
const String status_channel_three_path = DEVICE_ID + "/status/L3"; // set channle three path
const String status_channel_four_path = DEVICE_ID + "/status/L4"; // set channle four path

static uint8_t state_mode = 0; // 0 is autoconct ; 1 is setting mode
static bool init_mode = true; // is init mode

ESP8266WebServer server(80);
bool looping = true;

void setup() {
Serial.begin(115200);
//set pin mode
pinMode(D1,OUTPUT);
pinMode(D2,OUTPUT);
pinMode(SETTING_MODE,INPUT);

} // setup


void loop() {

  static uint8_t staterun = 0;
  static uint8_t state = 0;

  checkMode(); //check mode

  switch(state) {

  case 0 :
    //setting wifi mode
   state = SettingMode();
   Serial.println("setting mode");

  break;

  case 1 :
  //connect wifi
  state = ConnectwifiAndFirebase();
   Serial.println("connect wifi and firebase");
  break;

  case 2 :
  //handle with firebase
   handleFirebaseController();
   Serial.println("handle firebase");
  break;

  default :
  state = 0;

  }

  delay(500);

} // loop

void checkMode() {

  if (!init_mode) {

    uint8_t mode = digitalRead(SETTING_MODE); // read mode

    if (state_mode != mode) {

      digitalWrite(D1,LOW); //trun off L1
      digitalWrite(D2,LOW);//trun off L2

      ESP.reset(); //reset

    }

  }else {
    state_mode = digitalRead(SETTING_MODE);  //init state mode
    init_mode = false; // next step
  }

} // check mode


uint8_t SettingMode(){
   Serial.println("Enter Mode : ");
   while(true) {
    uint8_t state = digitalRead(SETTING_MODE);
    if(state == HIGH) { // if mode is setting wifi connection
    CreateServerApi(); // create server api for setting mode
    return 1;
    }
    if(state == LOW) { // if mode is connect to firebase
     return AutoConnectWifiAndFirebase();
    }

  delay(100);

   } // looping setting

} // setting mode


uint8_t AutoConnectWifiAndFirebase(){

  WiFi.setAutoConnect(true);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    checkMode(); //check mode
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); //connect to Firebase

  return 2; //next state
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
            checkMode(); //check mode
            server.handleClient();
            Serial.println("handle clien");
            delayMicroseconds(100);
           }

} //create server api for mode setting wifi

void handleSetting(){

String Str_ssid = server.arg("ssid");
String Str_password = server.arg("passwd");

  WIFI_SSID = Str_ssid;
  WIFI_PASSWORD = Str_password;
  server.send(200,"application/json","{ \"stat\" : \"ok\" }");
  delay(500);
   looping = false;
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

  // connect to wifi.
  WiFi.begin(ssid, passwd);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  return 2;

} //connect wifi and Firebase


void handleFirebaseController () {

  AnswerUplink();  // check device is active
  delay(500);
  handleSwitchChannelOne(); // handle switch L1
  delay(500);
  handleSwitchChannelTwo(); //handle switch L2
  delay(500);

} // handle command switchLigth in Firebase

void AnswerUplink() {

static uint8_t uplink = 0;
uint8_t state_uplink= Firebase.getInt(uplink_path);

// handle error
if (Firebase.failed()) {
    Serial.print("get status uplink failed:");
    Serial.println(Firebase.error());
    return;
}

if (state_uplink != uplink) {
uint8_t ack = rand();
Firebase.setInt(ack_path,ack);
// handle error
if (Firebase.failed()) {
    Serial.print("set status ack failed:");
    Serial.println(Firebase.error());
    return;
}

uplink = state_uplink;

}

} // check if have uplink signal

void handleSwitchChannelOne() {

    static uint8_t isactive_L1 = 0;
  //get status swwitch L1
  uint8_t state_L1 = Firebase.getInt(status_channel_one_path);

  // handle error
  if (Firebase.failed()) {
      Serial.print("get status switch L1 failed:");
      Serial.println(Firebase.error());
      return;
  }

  if (isactive_L1 != state_L1) {
    digitalWrite(D1,state_L1);
    isactive_L1 = state_L1;
  }

} // handle switch L1


void handleSwitchChannelTwo() {

    static uint8_t isactive_L2 = 0;
  //get status swwitch L2
  uint8_t state_L2 = Firebase.getInt(status_channel_two_path);

  // handle error
  if (Firebase.failed()) {
      Serial.print("get status switch L2 failed:");
      Serial.println(Firebase.error());
      return;
  }

  if (isactive_L2 != state_L2) {
    digitalWrite(D2,state_L2);
    isactive_L2 = state_L2;
  }

} // handle switch L2

