#include <Servo.h>
#include <WiFi.h>
#include <string.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "config_flora.h"
#include "BLEDevice.h"

//CONFIG PARAMS
// Temp de sleep entre tasques
#define SLEEP_DURATION  4 * 3600
//Posicions a cada nivell
#define positions_per_floor 15

//Estructura per definir la posició en les 3 dimensions
typedef struct position_plant{
  int vertical;
  int horizontal;
  int angular;
}position_plant;

//configuracio de les posicions de les plantes en un nivell
const position_plant array_positions_floor[positions_per_floor+1] ={
  {0,0,120},{0,29347,11},{0,20380,20},{0,12228,30},{0,6114,42},{0,4891,62},{0,8152,80},{0,16304,90},{0,24456,95},{0,33830,100},{0,32608,40},{0,29347,45},{0,29347,55},{0,30570,65},{0,32608,73},
};

// PINS ESP32
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

//Global vars
//Estats motors
int vertical_pos_state;   // nema17 pos
int angular_pos_state=60;    // servo pos
int current_step_horizontal = 0;  // 28BYJ motor pos
//Variables del callback mqtt
volatile int ml = 0;
volatile int mins =0;
volatile int light_start_resp=0;
volatile int sleep_time_resp=0;
volatile bool sub_mqtt_flag = false;
//flag per marcar inici
int start_pos=0;
//variables guardades en memoria RTC que sobreviuen als resets per sofware
static RTC_NOINIT_ATTR  int pos_reset; //Posicio abans de un reset per software
static RTC_NOINIT_ATTR  int action_reset; // Causant del reset-> 0-No action, 1-Reset from Flora
//credencials WIFI
static RTC_NOINIT_ATTR char ssid_wifi[100];//
static RTC_NOINIT_ATTR char pwd_wifi[100];

//timer interrupts
hw_timer_t * correction_vertical_int = NULL;
hw_timer_t * correction_servo_int = NULL;
hw_timer_t * watchdog_flora = NULL;
hw_timer_t * correction_vertical_jam_int = NULL;

//Objectes clients wifi i mqtt
WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  Serial.begin(115200);

  //First water setup or things can get wet
  water_setup();
  //Mirem si tenim si tenim un reset per sofware
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
  //Si no hi ha software reset
  if (action_reset == 0){
    if (!request_sleep_time()){
      if (request_start_lights()){
        light_rutine();
      }
      else{
        flora_rutine();
      }
    }
  }
  //Si no hi ha software reset provocat pel sensor flora
  else if (action_reset == 1){
    flora_rutine();
  }
  hibernate();

}

//Funcio que retorna per parametre la posicio pos a partir del id
void get_position(int id, struct position_plant *pos){
  int plane_id;
  pos->vertical =(id/(positions_per_floor))+1;
  plane_id = id%(positions_per_floor);
  pos->horizontal = array_positions_floor[plane_id].horizontal;
  pos->angular = array_positions_floor[plane_id].angular;
}

//Funcio encarregada de moure la base a la posicio indicada pel parametre pos
void move_to_pos(struct position_plant *pos){
  //Primer movem el braç a la posicio vertical indicada
  if (vertical_pos_state!=pos->vertical){
    //Mirem si estem passant per un canvi de nivell
    if (((vertical_pos_state==2 || vertical_pos_state==1 )&& (pos->vertical==3 || pos->vertical==4) )|| ((vertical_pos_state== 3 || vertical_pos_state== 4) && (pos->vertical==2 || pos->vertical==1) ) ){
      timerAlarmEnable(correction_servo_int);
    }
    if (vertical_pos_state>(pos->vertical)){
      timerAlarmEnable(correction_vertical_jam_int);
    }
    vertical_pos(pos->vertical);
    timerAlarmDisable(correction_servo_int);
    timerAlarmDisable(correction_vertical_jam_int);
  }
  //disable stepper vertical abans de que el servo comenci
  disable_stepper_vertical();

  //Movem el braç a la posicio angular indicada
  enable_servo();
  servo_to_pos(pos->angular);

  //FInalment movem la base a la posicio horitzontal indicada
  if ((pos->horizontal)==0){
    start_horiz();
  }
  else{
    if (current_step_horizontal<=pos->horizontal){
      front_horiz((pos->horizontal)- current_step_horizontal);
    }
    else{
      back_horiz(current_step_horizontal-(pos->horizontal));
    }
  }

}

//Funcio que mou la base al id
void move_to_id(int id){
  struct position_plant pos;
  get_position(id, &pos);
  move_to_pos(&pos);
}

//Funcio encarrega de connectar la ESP32 al WIFI, creant un punt d'acces per entrar les credencials si no les te, o bé connectantse directament si les te.
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
        ESP.restart();
    }
    else {
        strncpy(ssid_wifi, wm.getWiFiSSID().c_str(), 99);
        strncpy(pwd_wifi, wm.getWiFiPass().c_str(), 99);

        //if you get here you have connected to the WiFi
        Serial.println("connected to wifi");
    }

  }
  else {
    //Connect with saved credentials in RTC memory
    Serial.println("credentials wifi");
    Serial.println(ssid_wifi);
    Serial.println(pwd_wifi);
    connectWifi(ssid_wifi, pwd_wifi);

  }
}

//Calback que es crida quan arriba un missatge a una subscripcio mqtt
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
    if (ret){
        mins = atoi(messageTemp.c_str());
        Serial.println("In mins response:");
        Serial.println(mins);
      }
    else{
      ret = strstr(topic, "light_start_resp");
      if (ret){
          light_start_resp = atoi(messageTemp.c_str());
          Serial.println("Start lights response:");
          Serial.println(light_start_resp);
      }
      else{
      ret = strstr(topic, "sleep_time_resp");
      if (ret){
          sleep_time_resp = atoi(messageTemp.c_str());
          Serial.println("start sleep_time_resp response");
          Serial.println(sleep_time_resp);
        }
      }
    }
  }

  sub_mqtt_flag = true;
}
//Funcio que connecta el wifi mitjançant les credencials passades
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

void hibernate() {
  disable_stepper_vertical();
  disable_servo();
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION * 1000000ll);
  Serial.println("Going to sleep now.");
  delay(100);
  esp_deep_sleep_start();
}

bool request_sleep_time(){
  bool ret_value;
  connectWifi(ssid_wifi, pwd_wifi);
  connectMqtt();
  client.subscribe((MQTT_BASE_TOPIC + "/" + "sleep_time_resp").c_str());
  connectMqtt();
  char buffer[64];
  snprintf(buffer, 64, "%d", 1);
  client.publish((MQTT_BASE_TOPIC + "/" + "sleep_time_req").c_str(), buffer);
  //Wait to receive mqtt response message from server
  int start = millis();
  while ((millis() - start < 10000) && (sub_mqtt_flag == false)){
    client.loop();
  }
  client.unsubscribe((MQTT_BASE_TOPIC + "/" + "sleep_time_resp").c_str());
  client.disconnect();
  if (sub_mqtt_flag == false){
    ret_value=false;
  }
  else{
    if (sleep_time_resp ==1){
      ret_value=true;
    }
    else{
      ret_value=false;
    }
  }
  sub_mqtt_flag=false;
  return ret_value;
}
