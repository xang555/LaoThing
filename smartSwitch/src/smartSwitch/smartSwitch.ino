
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

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

const String scheduler_state_channel_one_path = DEVICE_ID + "/scheduler/L1/state"; // state L1
const String scheduler_status_channel_one_path = DEVICE_ID + "/scheduler/L1/status"; // set scheduler status change new one
const String scheduler_time_hour_channel_one_path = DEVICE_ID + "/scheduler/L1/time/hour";// set scheduler huor change new one
const String scheduler_time_minute_channel_one_path = DEVICE_ID + "/scheduler/L1/time/minute";// set scheduler minute change new one

const String scheduler_state_channel_tow_path = DEVICE_ID + "/scheduler/L2/state"; // state L2
const String scheduler_status_channel_tow_path = DEVICE_ID + "/scheduler/L2/status"; // set scheduler status change new tow
const String scheduler_time_hour_channel_tow_path = DEVICE_ID + "/scheduler/L2/time/hour";// set scheduler huor change new tow
const String scheduler_time_minute_channel_tow_path = DEVICE_ID + "/scheduler/L2/time/minute";// set scheduler minute change new tow

const String scheduler_state_channel_three_path = DEVICE_ID + "/scheduler/L3/state"; // state L3
const String scheduler_status_channel_three_path = DEVICE_ID + "/scheduler/L3/status";// set scheduler status change new three
const String scheduler_time_channel_three_path = DEVICE_ID + "/scheduler/L3/time";// set scheduler time change new three
const String scheduler_time_hour_channel_three_path = DEVICE_ID + "/scheduler/L3/time/hour";// set scheduler huor change new three
const String scheduler_time_minute_channel_three_path = DEVICE_ID + "/scheduler/L3/time/minute";// set scheduler minute change new three

const String scheduler_state_channel_four_path = DEVICE_ID + "/scheduler/L4/state"; // state L4
const String scheduler_status_channel_four_path = DEVICE_ID + "/scheduler/L4/status";// set scheduler status change new four
const String scheduler_time_channel_four_path = DEVICE_ID + "/scheduler/L4/time";// set scheduler time change new four
const String scheduler_time_hour_channel_four_path = DEVICE_ID + "/scheduler/L4/time/hour";// set scheduler huor change new four
const String scheduler_time_minute_channel_four_path = DEVICE_ID + "/scheduler/L4/time/minute";// set scheduler minute change new four


static uint8_t state_mode = 0; // 0 is autoconct ; 1 is setting mode
static bool init_mode = true; // is init mode

/*----------- init for l1 --------------*/
static int init_day_for_l1 = 30; // init day
static int init_month_for_l1 = 3; // init month
static int init_yare_for_l1 = 2017; //init yare

/*----------- init for l2 --------------*/
static int init_day_for_l2 = 30; // init day
static int init_month_for_l2 = 3; // init month
static int init_yare_for_l2 = 2017; //init yare

/*----------- init for l1 --------------*/
static int init_day_for_l3 = 30; // init day
static int init_month_for_l3 = 3; // init month
static int init_yare_for_l3 = 2017; //init yare

/*----------- init for l1 --------------*/
static int init_day_for_l4 = 30; // init day
static int init_month_for_l4 = 3; // init month
static int init_yare_for_l4 = 2017; //init yare



ESP8266WebServer server(80);
bool looping = true;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.asia.pool.ntp.org", 7*3600, 60000);

void setup() {
Serial.begin(115200);
//set pin mode
pinMode(D1,OUTPUT);
pinMode(D2,OUTPUT);
pinMode(D3,OUTPUT);
pinMode(D4,OUTPUT);
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

  timeClient.begin(); //time server

  setSyncProvider(getTimeNow);
  setSyncInterval(3600);
  initCurentDateTimeForSwitchOne(); //init date time for l1
  initCurentDateTimeForSwitchtow(); //init date time for l2
  initCurentDateTimeForSwitchthree(); //init date time for l3
  initCurentDateTimeForSwitchfour(); //init date time for l4
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); //connect to Firebase

  return 2; //next state
} //auto connect wifi and Firebase


uint8_t CreateServerApi() {

          WiFi.softAP(AP_SSID,AP_PASSWORD);
          Serial.print("Ap Wifi IP address: ");
          Serial.println(WiFi.softAPIP());

          server.on("/setting",HTTP_POST,handleSetting);
          server.on("/controller", HTTP_POST, handleSwitchController);
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

void handleSwitchController() {

  String switchname = server.arg("swn");
  uint8_t cmd = server.arg("cmd").toInt();

  if (switchname == "L1") {
    digitalWrite(D1,cmd);
  }else if (switchname == "L2") {
    digitalWrite(D2,cmd);
  }else if (switchname == "L3") {
    digitalWrite(D3,cmd);
  }else if (switchname == "L4") {
    digitalWrite(D4,cmd);
  }

  server.send(200,"application/json","{ \"state\" : \""+ String(cmd) +"\" }");

} // handle switch in mode offline

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

  timeClient.begin(); //time server
  setSyncProvider(getTimeNow);
  setSyncInterval(3600);

  initCurentDateTimeForSwitchOne(); //init date time for l1
  initCurentDateTimeForSwitchtow(); //init date time for l2
  initCurentDateTimeForSwitchthree(); //init date time for l3
  initCurentDateTimeForSwitchfour(); //init date time for l4

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  return 2;

} //connect wifi and Firebase


void handleFirebaseController () {

  AnswerUplink();  // check device is active
  delay(100);
  handleSwitchChannelOne(); // handle switch L1
  delay(100);
  handleSwitchChannelTwo(); //handle switch L2
  delay(100);
  scheduler_switch_L1(); //scheduler l1
  delay(100);
  scheduler_switch_L2(); //scheduler l2
  delay(100);
  scheduler_switch_L3(); //scheduler l3
  delay(100);
  scheduler_switch_L4(); //scheduler l4
  delay(100);

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

/*------------------------ scheduler function --------------------------------------------*/

void initCurentDateTimeForSwitchOne() {

init_day_for_l1 = day();
init_month_for_l1 = month();
init_yare_for_l1 = year() ;

} // init curent date time

void initCurentDateTimeForSwitchtow() {

init_day_for_l2 = day();
init_month_for_l2 = month();
init_yare_for_l2 = year() ;

} // init curent date time

void initCurentDateTimeForSwitchthree() {

init_day_for_l3 = day();
init_month_for_l3 = month();
init_yare_for_l3 = year() ;

} // init curent date time

void initCurentDateTimeForSwitchfour() {

init_day_for_l4 = day();
init_month_for_l4 = month();
init_yare_for_l4 = year() ;

} // init curent date time


void scheduler_switch_L1() {

  Serial.println("int l1");

  static bool scheduler_l1_isactived = false;

  bool l1_state = Firebase.getBool(scheduler_state_channel_one_path); // get sate one
  // handle error
  if (Firebase.failed()) {
      Serial.print("get state switch L1 failed:");
      Serial.println(Firebase.error());
      return;
  }

  if (l1_state) {

    if (!scheduler_l1_isactived) { //check scheduler is actived

          Serial.println("int scheduler l1 check actived");

      int scheduler_hour = Firebase.getInt(scheduler_time_hour_channel_one_path);
      // handle error
      if (Firebase.failed()) {
          Serial.print("get hour switch L1 failed:");
          Serial.println(Firebase.error());
          return;
      }

      int scheduler_minute = Firebase.getInt(scheduler_time_minute_channel_one_path);
      // handle error
      if (Firebase.failed()) {
          Serial.print("get minute switch L1 failed:");
          Serial.println(Firebase.error());
          return;
      }

      if (hour() >= scheduler_hour && minute() >= scheduler_minute) {

          Serial.println("switch on|of");

        uint8_t status = Firebase.getInt(scheduler_status_channel_one_path);
        // handle error
        if (Firebase.failed()) {
            Serial.print("get status switch L1 failed:");
            Serial.println(Firebase.error());
            return;
        }

        digitalWrite(D1,status);  // send signal to delay one
        scheduler_l1_isactived = true; //is actived

        Firebase.setInt(status_channel_one_path,status); //state status to Firebase

        // handle error
        if (Firebase.failed()) {
            Serial.print("set status switch L1 failed:");
            Serial.println(Firebase.error());
            return;
        }

      } //check if scheduler is match


    } else{

        Serial.println("scheduler l1 is actived");

      if (day() != init_day_for_l1 && month() != init_month_for_l1 && year() != init_yare_for_l1) {
          scheduler_l1_isactived = false;
          initCurentDateTimeForSwitchOne();
      }

    }

  }  //check state


} //scheduler ligth one


void scheduler_switch_L2() {

  Serial.println("int l2");

  static bool scheduler_l2_isactived = false;

  bool l2_state = Firebase.getBool(scheduler_state_channel_tow_path); // get sate one
  // handle error
  if (Firebase.failed()) {
      Serial.print("get state switch L2 failed:");
      Serial.println(Firebase.error());
      return;
  }

  if (l2_state) {

    if (!scheduler_l2_isactived) { //check scheduler is actived

          Serial.println("int scheduler l2 check actived");

      int scheduler_hour = Firebase.getInt(scheduler_time_hour_channel_tow_path);
      // handle error
      if (Firebase.failed()) {
          Serial.print("get hour switch L2 failed:");
          Serial.println(Firebase.error());
          return;
      }

      int scheduler_minute = Firebase.getInt(scheduler_time_minute_channel_tow_path);
      // handle error
      if (Firebase.failed()) {
          Serial.print("get minute switch L2 failed:");
          Serial.println(Firebase.error());
          return;
      }

      if (hour() >= scheduler_hour && minute() >= scheduler_minute) {

          Serial.println("switch l2 on|of");

        uint8_t status = Firebase.getInt(scheduler_status_channel_tow_path);
        // handle error
        if (Firebase.failed()) {
            Serial.print("get status switch L2 failed:");
            Serial.println(Firebase.error());
            return;
        }

        digitalWrite(D2,status);  // send signal to delay one
        scheduler_l2_isactived = true; //is actived

        Firebase.setInt(status_channel_two_path,status); //state status to Firebase

        // handle error
        if (Firebase.failed()) {
            Serial.print("set status switch L2 failed:");
            Serial.println(Firebase.error());
            return;
        }

      } //check if scheduler is match


    } else{

        Serial.println("scheduler l2 is actived");

      if (day() != init_day_for_l2 && month() != init_month_for_l2 && year() != init_yare_for_l2) {
          scheduler_l2_isactived = false;
          initCurentDateTimeForSwitchtow();
      }

    }

  }  //check state


} //scheduler ligth tow




void scheduler_switch_L3() {

  Serial.println("int l3");

  static bool scheduler_l3_isactived = false;

  bool l3_state = Firebase.getBool(scheduler_state_channel_three_path); // get sate one
  // handle error
  if (Firebase.failed()) {
      Serial.print("get state switch L3 failed:");
      Serial.println(Firebase.error());
      return;
  }

  if (l3_state) {

    if (!scheduler_l3_isactived) { //check scheduler is actived

          Serial.println("int scheduler l3 check actived");

      int scheduler_hour = Firebase.getInt(scheduler_time_hour_channel_three_path);
      // handle error
      if (Firebase.failed()) {
          Serial.print("get hour switch L3 failed:");
          Serial.println(Firebase.error());
          return;
      }

      int scheduler_minute = Firebase.getInt(scheduler_time_minute_channel_three_path);
      // handle error
      if (Firebase.failed()) {
          Serial.print("get minute switch L3 failed:");
          Serial.println(Firebase.error());
          return;
      }

      if (hour() >= scheduler_hour && minute() >= scheduler_minute) {

          Serial.println("switch l3 on|of");

        uint8_t status = Firebase.getInt(scheduler_status_channel_three_path);
        // handle error
        if (Firebase.failed()) {
            Serial.print("get status switch L3 failed:");
            Serial.println(Firebase.error());
            return;
        }

        digitalWrite(D3,status);  // send signal to delay one
        scheduler_l3_isactived = true; //is actived

        Firebase.setInt(status_channel_three_path,status); //state status to Firebase

        // handle error
        if (Firebase.failed()) {
            Serial.print("set status switch L3 failed:");
            Serial.println(Firebase.error());
            return;
        }

      } //check if scheduler is match


    } else{

        Serial.println("scheduler l3 is actived");

      if (day() != init_day_for_l3 && month() != init_month_for_l3 && year() != init_yare_for_l3) {
          scheduler_l3_isactived = false;
          initCurentDateTimeForSwitchthree();
      }

    }

  }  //check state


} //scheduler ligth three




void scheduler_switch_L4() {

  Serial.println("int l4");

  static bool scheduler_l4_isactived = false;

  bool l4_state = Firebase.getBool(scheduler_state_channel_four_path); // get sate one
  // handle error
  if (Firebase.failed()) {
      Serial.print("get state switch L4 failed:");
      Serial.println(Firebase.error());
      return;
  }

  if (l4_state) {

    if (!scheduler_l4_isactived) { //check scheduler is actived

          Serial.println("int scheduler l4 check actived");

      int scheduler_hour = Firebase.getInt(scheduler_time_hour_channel_four_path);
      // handle error
      if (Firebase.failed()) {
          Serial.print("get hour switch L4 failed:");
          Serial.println(Firebase.error());
          return;
      }

      int scheduler_minute = Firebase.getInt(scheduler_time_minute_channel_four_path);
      // handle error
      if (Firebase.failed()) {
          Serial.print("get minute switch L4 failed:");
          Serial.println(Firebase.error());
          return;
      }

      if (hour() >= scheduler_hour && minute() >= scheduler_minute) {

          Serial.println("switch l4 on|of");

        uint8_t status = Firebase.getInt(scheduler_status_channel_four_path);
        // handle error
        if (Firebase.failed()) {
            Serial.print("get status switch L4 failed:");
            Serial.println(Firebase.error());
            return;
        }

        digitalWrite(D4,status);  // send signal to delay one
        scheduler_l4_isactived = true; //is actived

        Firebase.setInt(status_channel_four_path,status); //state status to Firebase

        // handle error
        if (Firebase.failed()) {
            Serial.print("set status switch L4 failed:");
            Serial.println(Firebase.error());
            return;
        }

      } //check if scheduler is match


    } else{

        Serial.println("scheduler l4 is actived");

      if (day() != init_day_for_l4 && month() != init_month_for_l4 && year() != init_yare_for_l4) {
          scheduler_l4_isactived = false;
          initCurentDateTimeForSwitchfour();
      }

    }

  }  //check state


} //scheduler ligth four

/*----------------------------------------------------------------------------------------*/

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

long getTimeNow(){

  long ntp_time = 0;

   while (ntp_time < 1000000) {
     timeClient.update();
     ntp_time = timeClient.getEpochTime();
     delay(1000);
   }

return ntp_time;

} // get timestramp

