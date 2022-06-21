
#include <Servo.h>
#include "BLEDevice.h"
#include <WiFi.h>
#include <string.h>
#include <PubSubClient.h>
#include <WiFiManager.h> 

#include "config_flora.h"

#define positions_per_floor 15
#define VELOCITY_UP 100;
#define VELOCITY_DOWN 100;

typedef struct position_plant{
  int vertical;
  int horizontal;
  int angular;
}position_plant;


// defines pins numbers
const int stepPin = 14;
const int dirPin = 26; 
const int enablePin = 32; 
const int final_carrera_p1=25;
const int final_carrera_p2=21;
const int final_carrera_p3=35;
const int final_carrera_p4=27;
const int stepper_5v_pin1  = 15;    // 28BYJ48 In1
const int stepper_5v_pin2 = 2;    // 28BYJ48 In2
const int stepper_5v_pin3 = 0;   // 28BYJ48 In3
const int stepper_5v_pin4 = 4;   // 28BYJ48 In4
const int  servo_control =13;
const int waterPin = 5;
const int lightPin = 17;

const position_plant array_positions_floor[positions_per_floor+1] ={
  {0,0,120},{0,29347,11},{0,20380,20},{0,12228,30},{0,6114,42},{0,4891,62},{0,8152,80},{0,16304,90},{0,24456,95},{0,33830,100},{0,32608,40},{0,29347,45},{0,29347,55},{0,30570,65},{0,32608,73},
};

//Global vars
int vertical_pos_state;
int angular_pos_state=60;    // variable to store the servo position
int current_step_horizontal = 0;
int ml = 0;
int mins =0;
bool sub_mqtt_flag = false;


int once=1;
int counter=1;
int start_pos=0;

static RTC_NOINIT_ATTR  int pos_reset; //Position before the soft reset during an action
static RTC_NOINIT_ATTR  int action_reset; // 0-No action, 1-Flora, 2-...
static RTC_NOINIT_ATTR char ssid_wifi[100];
static RTC_NOINIT_ATTR char pwd_wifi[100];

Servo myservo;  // create servo object to control a servo


//timer interrupts
hw_timer_t * correction_vertical_int = NULL;
hw_timer_t * correction_servo_int = NULL;
hw_timer_t * watchdog_flora = NULL;
hw_timer_t * correction_vertical_jam_int = NULL;

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  Serial.begin(115200);

  //First water setup or everything gets wet
  water_setup();

  esp_reset_reason_t reason_reset;
  reason_reset = esp_reset_reason();

  if (reason_reset == ESP_RST_POWERON || reason_reset == ESP_RST_UNKNOWN  ){
    autoconnectAP(1);
  }
  else{
    autoconnectAP(0);
  }
  
  stepper_vertical_setup();
  stepper_horizontal_setup();
  
  flora_setup();
  light_setup();

  servo_setup();
  
}

void loop(){

  if (once==1){
    /*
    
    if (start_pos ==1){
      timerAlarmEnable(correction_servo_int);
      move_to_id(0);
      start_pos=0;
      timerAlarmDisable(correction_servo_int);
    }*/
    light_rutine();
    once =0;
  }
  
}

void get_position(int id, struct position_plant *pos){
  int plane_id;
  pos->vertical =(id/(positions_per_floor))+1;
  plane_id = id%(positions_per_floor);
  pos->horizontal = array_positions_floor[plane_id].horizontal;
  pos->angular = array_positions_floor[plane_id].angular;
}

void move_to_pos(struct position_plant *pos){
  if (vertical_pos_state!=pos->vertical){
    if (((vertical_pos_state==2 || vertical_pos_state==1 )&& (pos->vertical==3 || pos->vertical==4) )|| ((vertical_pos_state== 3 || vertical_pos_state== 4) && (pos->vertical==2 || pos->vertical==1) ) ){
      timerAlarmEnable(correction_servo_int);
    }
    if (vertical_pos_state>(pos->vertical)){
      Serial.println("change vertical state 2");
      timerAlarmEnable(correction_vertical_jam_int);
      Serial.println(pos->vertical);
    }
    vertical_pos(pos->vertical);
    timerAlarmDisable(correction_servo_int);
    timerAlarmDisable(correction_vertical_jam_int);
  }

  Serial.println("change servo state");
  Serial.println(pos->angular);
  //enable stepper vertical before servo move securing the position and not fall.
  enable_servo();
  servo_to_pos(pos->angular);
  

  
  if ((pos->horizontal)==0){
    start_horiz();
  }
  else{
    Serial.println("in horiz stepper");
    Serial.println(pos->horizontal);
    Serial.println(current_step_horizontal);
    if (current_step_horizontal<=pos->horizontal){
      front_horiz((pos->horizontal)- current_step_horizontal);
    }
    else{
      back_horiz(current_step_horizontal-(pos->horizontal));
    }
  } 
  
}

void move_to_id(int id){
  struct position_plant pos;
  get_position(id, &pos);
  move_to_pos(&pos);
}


void autoconnectAP(int ap){

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  
  if (ap ==1){

    //CONNECT TO INTERNET
  
    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result
  
    bool res;
    res = wm.autoConnect("hort_vertical_AP","password"); // password protected ap
  
    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        strncpy(ssid_wifi, wm.getWiFiSSID().c_str(), 99);
        strncpy(pwd_wifi, wm.getWiFiPass().c_str(), 99);
        //ssid_wifi = wm.getWiFiSSID().c_str();
        //pwd_wifi = wm.getWiFiPass().c_str();
        Serial.println("credentials wifi");
        Serial.println(ssid_wifi);
        Serial.println(pwd_wifi);
        
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }

  }
  else {
 
    Serial.println("credentials wifi");
    Serial.println(ssid_wifi);
    Serial.println(pwd_wifi);
    connectWifi(ssid_wifi, pwd_wifi);
  
  }
}


void callback_mqtt(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  char *ret;

  ret = strstr(topic, "water");
  if (ret){
    ml = atoi(messageTemp.c_str());
  }
  else{
    ret = strstr(topic, "light_resp");
    if (ret)
        mins = atoi(messageTemp.c_str());
        Serial.println("in mins response");
        Serial.println(mins);
  }

  sub_mqtt_flag = true;
}

void connectWifi(const char* wifi_ssid, const char* wifi_password) {
  int start = millis();
  Serial.println("Connecting to WiFi...");
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis()-start>10000){
      connectWifi(wifi_ssid, wifi_password);
      break;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("");
}


void disconnectWifi() {
  WiFi.disconnect(true);
  Serial.println("WiFi disonnected");
}

void connectMqtt() {
  static int retry_counter2=0;
  Serial.println("Connecting to MQTT...");
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback_mqtt);
  int retry_counter=0;
  while (!client.connected()) {
    if (!client.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.print("MQTT connection failed:");
      Serial.print(client.state());
      Serial.println("Retrying...");
      delay(MQTT_RETRY_WAIT);
      retry_counter +=1;
      if (retry_counter > 3){
        retry_counter2 +=1;
        connectWifi(ssid_wifi, pwd_wifi);
        connectMqtt();  
        break;
      }
      
    }
  }

  Serial.println("MQTT connected");
  Serial.println("");
}

void disconnectMqtt() {
  client.disconnect();
  Serial.println("MQTT disconnected");
}
