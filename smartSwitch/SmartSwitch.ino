#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include "FirebasePath.h"
// Set firebae information
#define FIREBASE_HOST "laothing-d014b.firebaseio.com"
#define FIREBASE_AUTH "0twt7Oi5P0bJ55QSVZenjAgJuRXrXNbwc8YuR5ZT"
// set AP WIFI
#define AP_SSID "smartswitch10004"
#define AP_PASSWORD "12345678"
#define SETTING_MODE  D8
#define STATE_CONNECTION  D5
#define CHANNEL_ONE D1
#define CHANNEL_TWO D2
#define CHANNEL_THREE D6
#define CHANNEL_FOUR D7


String WIFI_SSID = "HUAWEI P9";
String WIFI_PASSWORD = "98866777";

static bool manual_connect = false; // is manual connection
static bool isCloseServer = false; // count lose connection
static bool isInitFirebase = false;

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

static int count_connection_lose = 0; // count lose connection

IPAddress ip(192,168,4,1);
ESP8266WebServer server(ip,80);
bool looping = true;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.asia.pool.ntp.org", 7*3600, 60000);

void setup() {
Serial.begin(250000);
//set pin mode
pinMode(CHANNEL_ONE,OUTPUT);
pinMode(CHANNEL_TWO,OUTPUT);
pinMode(CHANNEL_THREE,OUTPUT);
pinMode(CHANNEL_FOUR,OUTPUT);
pinMode(SETTING_MODE,INPUT);
pinMode(STATE_CONNECTION,OUTPUT);
WiFi.softAPdisconnect(true);

digitalWrite(CHANNEL_ONE, HIGH);
digitalWrite(CHANNEL_TWO, HIGH);
digitalWrite(CHANNEL_THREE, HIGH);
digitalWrite(CHANNEL_FOUR, HIGH);

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

    digitalWrite(CHANNEL_ONE,HIGH); //trun off L1
    digitalWrite(CHANNEL_TWO,HIGH);//trun off L2
    digitalWrite(CHANNEL_THREE,HIGH); //trun off L1
    digitalWrite(CHANNEL_FOUR,HIGH);//trun off L2

    digitalWrite(STATE_CONNECTION,HIGH);//trun on state LED
    delay(100);
    digitalWrite(STATE_CONNECTION,LOW);//trun off state LED
    delay(100);
    digitalWrite(STATE_CONNECTION,HIGH);//trun on state LED
    delay(100);
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

  WiFi.softAPdisconnect(true);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.reconnect();
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
          server.on("/controller", HTTP_POST, handleSwitchController);
          server.on("/close",HTTP_GET,hanndelCloseServer);
          
          server.onNotFound([](){
            server.send(200,"text/html","<h2>Not Found</h2>");
            });

           server.on("/",[](){
             server.send(200,"text/html","<h2>Your Connected ...</h2>");
            });

          server.begin();

            while(looping) {
            server.handleClient();
           }

       if (isCloseServer) {
           return 1;
         }
         return 2;
} //create server api for mode setting wifi

void handleSwitchController() {

  String switchname = server.arg("swn");
  uint8_t cmd = server.arg("cmd").toInt();

  if (switchname == "L1") {
    digitalWrite(CHANNEL_ONE,!cmd);
  }else if (switchname == "L2") {
    digitalWrite(CHANNEL_TWO,!cmd);
  }else if (switchname == "L3") {
    digitalWrite(CHANNEL_THREE,!cmd);
  }else if (switchname == "L4") {
    digitalWrite(CHANNEL_FOUR,!cmd);
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
  isCloseServer = false;
  WiFi.softAPdisconnect(true);
  server.close();

} // handle setting mode

void hanndelCloseServer() {
 server.send(200,"application/json","{ \"stat\" : \"ok\" }");
  delay(500);
  looping = false;
  isCloseServer = true;
  WiFi.softAPdisconnect(true);
  server.close();
} //handle close server


uint8_t ConnectwifiAndFirebase() {

     const char* ssid ;
     const char* passwd ;

  ssid = WIFI_SSID.c_str();
  passwd = WIFI_PASSWORD.c_str();
  //disconnect wifi
  ESP.eraseConfig();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  // connect to wifi.
  WiFi.begin(ssid, passwd);

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

  timeClient.begin(); //time server
  setSyncProvider(getTimeNow);
  setSyncInterval(1000 * 60 * 3600);

  initCurentDateTimeForSwitchOne(); //init date time for l1
  initCurentDateTimeForSwitchtow(); //init date time for l2
  initCurentDateTimeForSwitchthree(); //init date time for l3
  initCurentDateTimeForSwitchfour(); //init date time for l4

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
  
  handleSwitchChannelOne(); // handle switch L1
  handleSwitchChannelTwo(); //handle switch L2
  handleSwitchChannelThree(); //handel switch L3
  handleSwitchChannelFour(); //handel switch L4

  scheduler_switch_L1(); //scheduler l1
  scheduler_switch_L2(); //scheduler l2
  scheduler_switch_L3(); //scheduler l3
  scheduler_switch_L4(); //scheduler l4

  checkConnection(); //check connection

  delay(100);

}

} // handle command switchLigth in Firebase

void AnswerUplink() {

static uint8_t uplink = 0;
int state_uplink= Firebase.getInt(uplink_path);

// handle error
if (Firebase.failed()) {
    count_connection_lose ++;
    return;
}

count_connection_lose = 0;

if (state_uplink != uplink) {
int ack = state_uplink;
Firebase.setInt(ack_path,ack);
// handle error
if (Firebase.failed()) {
    count_connection_lose ++;
    return;
}

count_connection_lose =0;
uplink = state_uplink;

}

} // check if have uplink signal


void checkConnection() {

  if (count_connection_lose > 10) {
    digitalWrite(STATE_CONNECTION, LOW);
  }else{
    digitalWrite(STATE_CONNECTION, HIGH);
  }

} //check connection to internet


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

// -------------------------- scheduler -------------------------//

void scheduler_switch_L1() {
  static bool scheduler_l1_isactived = false;
  bool l1_state = Firebase.getBool(scheduler_state_channel_one_path); // get sate one
  // handle error
  if (Firebase.failed()) {
      count_connection_lose ++ ;
      return;
  }

  count_connection_lose = 0;

  if (l1_state) {

    if (!scheduler_l1_isactived) { //check scheduler is actived
      int scheduler_hour = Firebase.getInt(scheduler_time_hour_channel_one_path);
      // handle error
      if (Firebase.failed()) {
            count_connection_lose ++ ;
          return;
      }

      count_connection_lose = 0;

      int scheduler_minute = Firebase.getInt(scheduler_time_minute_channel_one_path);
      // handle error
      if (Firebase.failed()) {
            count_connection_lose ++ ;
          return;
      }

      count_connection_lose = 0;

      if (hour() >= scheduler_hour && minute() >= scheduler_minute) {
        uint8_t status = Firebase.getInt(scheduler_status_channel_one_path);
        // handle error
        if (Firebase.failed()) {
              count_connection_lose ++ ;
            return;
        }

        count_connection_lose = 0;

        digitalWrite(CHANNEL_ONE,!status);  // send signal to delay one
        scheduler_l1_isactived = true; //is actived

        Firebase.setInt(status_channel_one_path+"/status",status); //state status to Firebase

        // handle error
        if (Firebase.failed()) {
              count_connection_lose ++ ;
            return;
        }

        count_connection_lose = 0;

      } //check if scheduler is match


    } else{
      if (day() != init_day_for_l1 && month() != init_month_for_l1 && year() != init_yare_for_l1) {
          scheduler_l1_isactived = false;
          initCurentDateTimeForSwitchOne();
      }

    }

  }  //check state


} //scheduler ligth one


void scheduler_switch_L2() {
  static bool scheduler_l2_isactived = false;
  bool l2_state = Firebase.getBool(scheduler_state_channel_tow_path); // get sate one
  // handle error
  if (Firebase.failed()) {
        count_connection_lose ++ ;
      return;
  }

count_connection_lose = 0;

  if (l2_state) {

    if (!scheduler_l2_isactived) { //check scheduler is actived
      int scheduler_hour = Firebase.getInt(scheduler_time_hour_channel_tow_path);
      // handle error
      if (Firebase.failed()) {
            count_connection_lose ++ ;
          return;
      }

      count_connection_lose = 0;

      int scheduler_minute = Firebase.getInt(scheduler_time_minute_channel_tow_path);
      // handle error
      if (Firebase.failed()) {
            count_connection_lose ++ ;
          return;
      }

      count_connection_lose = 0;

      if (hour() >= scheduler_hour && minute() >= scheduler_minute) {
        uint8_t status = Firebase.getInt(scheduler_status_channel_tow_path);
        // handle error
        if (Firebase.failed()) {
              count_connection_lose ++ ;
            return;
        }

        count_connection_lose = 0;

        digitalWrite(CHANNEL_TWO,!status);  // send signal to delay one
        scheduler_l2_isactived = true; //is actived

        Firebase.setInt(status_channel_two_path+"/status",status); //state status to Firebase

        // handle error
        if (Firebase.failed()) {
              count_connection_lose ++ ;
            return;
        }

        count_connection_lose = 0;

      } //check if scheduler is match


    } else{
      if (day() != init_day_for_l2 && month() != init_month_for_l2 && year() != init_yare_for_l2) {
          scheduler_l2_isactived = false;
          initCurentDateTimeForSwitchtow();
      }
    }

  }  //check state


} //scheduler ligth tow




void scheduler_switch_L3() {

  static bool scheduler_l3_isactived = false;

  bool l3_state = Firebase.getBool(scheduler_state_channel_three_path); // get sate one
  // handle error
  if (Firebase.failed()) {
        count_connection_lose ++ ;
      return;
  }
  count_connection_lose = 0;
  if (l3_state) {

    if (!scheduler_l3_isactived) { //check scheduler is actived
      int scheduler_hour = Firebase.getInt(scheduler_time_hour_channel_three_path);
      // handle error
      if (Firebase.failed()) {
            count_connection_lose ++ ;
          return;
      }

      count_connection_lose = 0;

      int scheduler_minute = Firebase.getInt(scheduler_time_minute_channel_three_path);
      // handle error
      if (Firebase.failed()) {
            count_connection_lose ++ ;
          return;
      }

      count_connection_lose = 0;

      if (hour() >= scheduler_hour && minute() >= scheduler_minute) {
        uint8_t status = Firebase.getInt(scheduler_status_channel_three_path);
        // handle error
        if (Firebase.failed()) {
              count_connection_lose ++ ;
            return;
        }
        count_connection_lose = 0;
        digitalWrite(CHANNEL_THREE,!status);  // send signal to delay one
        scheduler_l3_isactived = true; //is actived

        Firebase.setInt(status_channel_three_path+"/status",status); //state status to Firebase

        // handle error
        if (Firebase.failed()) {
              count_connection_lose ++ ;
            return;
        }
        count_connection_lose = 0;
      } //check if scheduler is match


    } else{

      if (day() != init_day_for_l3 && month() != init_month_for_l3 && year() != init_yare_for_l3) {
          scheduler_l3_isactived = false;
          initCurentDateTimeForSwitchthree();
      }

    }

  }  //check state


} //scheduler ligth three




void scheduler_switch_L4() {

  static bool scheduler_l4_isactived = false;

  bool l4_state = Firebase.getBool(scheduler_state_channel_four_path); // get sate one
  // handle error
  if (Firebase.failed()) {
        count_connection_lose ++ ;
      return;
  }
count_connection_lose = 0;
  if (l4_state) {

    if (!scheduler_l4_isactived) { //check scheduler is actived
      int scheduler_hour = Firebase.getInt(scheduler_time_hour_channel_four_path);
      // handle error
      if (Firebase.failed()) {
            count_connection_lose ++ ;
          return;
      }
count_connection_lose = 0;
      int scheduler_minute = Firebase.getInt(scheduler_time_minute_channel_four_path);
      // handle error
      if (Firebase.failed()) {
            count_connection_lose ++ ;
          return;
      }
count_connection_lose = 0;
      if (hour() >= scheduler_hour && minute() >= scheduler_minute) {
        uint8_t status = Firebase.getInt(scheduler_status_channel_four_path);
        // handle error
        if (Firebase.failed()) {
              count_connection_lose ++ ;
            return;
        }
        count_connection_lose = 0;
        digitalWrite(CHANNEL_FOUR,!status);  // send signal to delay one
        scheduler_l4_isactived = true; //is actived

        Firebase.setInt(status_channel_four_path+"/status",status); //state status to Firebase

        // handle error
        if (Firebase.failed()) {
              count_connection_lose ++ ;
            return;
        }
      count_connection_lose = 0;
      } //check if scheduler is match


    } else{
      if (day() != init_day_for_l4 && month() != init_month_for_l4 && year() != init_yare_for_l4) {
          scheduler_l4_isactived = false;
          initCurentDateTimeForSwitchfour();
      }

    }

  }  //check state


} //scheduler ligth four

/*----------------------------------------------------------------------------------------*/

/*----------------------------- handle switch --------------------------------------------*/
void handleSwitchChannelOne() {

    static uint8_t isactive_L1 = 0;
  //get status swwitch L1
  uint8_t state_L1 = Firebase.getInt(status_channel_one_path+"/status");

  // handle error
  if (Firebase.failed()) {
        count_connection_lose ++ ;
      return;
  }
count_connection_lose = 0;
  if (isactive_L1 != state_L1) {
    digitalWrite(CHANNEL_ONE,!state_L1);
    isactive_L1 = state_L1;
    Firebase.setInt(status_channel_one_path+"/ack", state_L1);
    if (Firebase.failed()) {
          count_connection_lose ++ ;
        return;
    }

  }

} // handle switch L1


void handleSwitchChannelTwo() {

    static uint8_t isactive_L2 = 0;
  //get status swwitch L2
  uint8_t state_L2 = Firebase.getInt(status_channel_two_path+"/status");

  // handle error
  if (Firebase.failed()) {
        count_connection_lose ++ ;
      return;
  }
count_connection_lose = 0;
  if (isactive_L2 != state_L2) {
    digitalWrite(CHANNEL_TWO,!state_L2);
    isactive_L2 = state_L2;
    Firebase.setInt(status_channel_two_path+"/ack", state_L2);
    if (Firebase.failed()) {
          count_connection_lose ++ ;
        return;
    }
  }

} // handle switch L2


void handleSwitchChannelThree() {

    static uint8_t isactive_L3 = 0;
  //get status swwitch L1
  uint8_t state_L3 = Firebase.getInt(status_channel_three_path+"/status");

  // handle error
  if (Firebase.failed()) {
        count_connection_lose ++ ;
      return;
  }
count_connection_lose = 0;
  if (isactive_L3 != state_L3) {
    digitalWrite(CHANNEL_THREE,!state_L3);
    isactive_L3 = state_L3;
    Firebase.setInt(status_channel_three_path+"/ack", state_L3);
    if (Firebase.failed()) {
          count_connection_lose ++ ;
        return;
    }
  }

} // handle switch L1


void handleSwitchChannelFour() {

    static uint8_t isactive_L4 = 0;
  //get status swwitch L1
  uint8_t state_L4 = Firebase.getInt(status_channel_four_path+"/status");

  // handle error
  if (Firebase.failed()) {
        count_connection_lose ++ ;
      return;
  }
count_connection_lose = 0;
  if (isactive_L4 != state_L4) {
    digitalWrite(CHANNEL_FOUR,!state_L4);
    isactive_L4 = state_L4;

    Firebase.setInt(status_channel_four_path+"/ack", state_L4);
    if (Firebase.failed()) {
          count_connection_lose ++ ;
        return;
    }

  }

count_connection_lose = 0;

} // handle switch L1

/*----------------------------------------------------------------------------------------*/

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
