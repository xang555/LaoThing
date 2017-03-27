
/*
gass sensor MQ-5
*/

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <FirebaseCloudMessaging.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <utility>
#include <vector>

// Set firebae information
#define FIREBASE_HOST "laothing-d014b.firebaseio.com"
#define FIREBASE_AUTH "0twt7Oi5P0bJ55QSVZenjAgJuRXrXNbwc8YuR5ZT"
// set AP WIFI
#define AP_SSID "laothing-837548734"
#define AP_PASSWORD "1234567890"
#define SETTING_MODE  D0
#define SERVER_KEY  "AAAAT6-pZT0:APA91bEKMf0X-gK7Cg14NTvWh3Al2wRa2EhWnVrWjUv8Lr8uDX7HfOF4my2iUDw5Am181YKbMSqyppnq-G_lpy3uPF6jqw4ObxCe11ISoQ7qmzCh8WtpIttfTSkRLvRgd1rsOFNR218L"

String WIFI_SSID = "NODEMCU";
String WIFI_PASSWORD = "nodemcu";

const String DEVICE_ID = "874387434"; //set DEVICE ID

const String uplink_path = DEVICE_ID + "/active/uplink"; // set uplink path
const String ack_path = DEVICE_ID + "/active/ack"; // set ack path
const String update_time_path = DEVICE_ID + "/sensor/values/time";
const String update_sensorval_path = DEVICE_ID + "/sensor/values/sensorval";
const String alert_path = DEVICE_ID + "/sensor";

static uint8_t state_mode = 0; // 0 is autoconct ; 1 is setting mode
static bool init_mode = true; // is init mode

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.asia.pool.ntp.org", 7*3600, 10000);

ESP8266WebServer server(80);
bool looping = true;

void setup() {
Serial.begin(115200);

pinMode(SETTING_MODE,INPUT);

timeClient.begin(); //time server

} //set up


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
   handleFirebaseSensor();
   Serial.println("handle firebase");
  break;

  default :
  state = 0;

  }

  delay(500);


} //loop


void checkMode() {

  if (!init_mode) {
    uint8_t mode = digitalRead(SETTING_MODE); // read mode
    if (state_mode != mode) {
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
  Serial.print("connected : ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  return 2;

} //connect wifi and Firebase

void handleFirebaseSensor() {

  static float prev_ratioSensorvalue = 0;

  AnswerUplink(); // Answer uplink from mobile

  float ratioSensorvalue = getSensorValue();
  if (prev_ratioSensorvalue != ratioSensorvalue) {

    Firebase.setFloat(update_sensorval_path, ratioSensorvalue); //update sensor value
    // handle error
    if (Firebase.failed()) {
        Serial.print("update sensor value failed:");
        Serial.println(Firebase.error());
        return;
    }

    Firebase.setInt(update_time_path , getTimeNow()); //update time

    // handle error
    if (Firebase.failed()) {
        Serial.print("update time failed:");
        Serial.println(Firebase.error());
        return;
    }

    prev_ratioSensorvalue = ratioSensorvalue;

  }

} // handle data in firebase


void Notification(float sensorval) {

  if (sensorval>= 2 && sensorval <= 4) { // CO gass alert code 500
    //  alertMessage("500");
  }else if (sensorval >= 0 && sensorval <= 1) { // LPG and CH4 gass alert code 200
      //alertMessage("200");
  }

}


void alertMessage(std::string alert_code) {

  FirebaseCloudMessaging fcm(SERVER_KEY);
//   FirebaseCloudMessage fmsg;
//
//   fmsg.high_priority = true;
//   fmsg.time_to_live = 3600 * 2;
//
// std::vector<std::pair<std::string, std::string>> data;
// std::pair <std::string, std::string> pairmessage("err","200");
// pairmessage.first  = "err";
// pairmessage.second  = alert_code;
// data.push_back(pairmessage);
//
// fmsg.data = data;
//
// FirebaseObject sensors = Firebase.get(alert_path);
//
// // handle error
// if (Firebase.failed()) {
//     Serial.print("get sensor values failed:");
//     Serial.println(Firebase.error());
//     return;
// }
//
// JsonVariant registerIDs = sensors.getJsonVariant("alert");
//
// // handle error
// if (sensors.failed()) {
//     Serial.print("get alert failed:");
//     Serial.println(sensors.error());
//     return;
// }
//
// const JsonObject& ridsobject = registerIDs.asObject();
//
//  std::vector<std::string> rgid;
//
// for(auto Id : ridsobject){
//       std::string rId_val = ridsobject[Id.key];
//       rgid.push_back(rId_val);
// }
//
// FirebaseError error = fcm.SendMessageToUsers(rgid, fmsg);
//
// if (error) {
//    Serial.print("Error:");
//    Serial.print(error.code());
//    Serial.print(" :: ");
//    Serial.println(error.message().c_str());
//  } else {
//    Serial.println("Sent OK!");
// }


} //send alert


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


float getSensorValue(){

  float sensor_volt;
      float RS_air; //  Get the value of RS via in a clear air
      float R0;  // Get the value of R0 via in H2
      float sensorValue;

      /*--- Get a average data by testing 100 times ---*/
      for(int x = 0 ; x < 100 ; x++)
      {
          sensorValue = sensorValue + analogRead(A0);
      }
      sensorValue = sensorValue/100.0;
    /*-----------------------------------------------*/

      sensor_volt = sensorValue/1023 * 3.3;
      RS_air = (3.3 - sensor_volt)/sensor_volt; // omit *RL
      R0 = RS_air/6.5; // The ratio of RS/R0 is 6.5 in a clear air from Graph

      float ratio = RS_air / 0.34;

      return ratio;

} // get gass sensor

long getTimeNow(){
timeClient.update();

long timstramp = timeClient.getEpochTime();
return timstramp;
}

