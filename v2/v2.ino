
#include <Servo.h>
#include "BLEDevice.h"
#include <WiFi.h>
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
int vertical_velocity = 5000;
int current_step_horizontal = 0;
int once=1;
int counter=1;
static RTC_NOINIT_ATTR  int pos_reset; //Position before the soft reset during an action
static RTC_NOINIT_ATTR  int action_reset; // 0-No action, 1-Flora, 2-...

Servo myservo;  // create servo object to control a servo


//timer interrupts
hw_timer_t * correction_vertical_int = NULL;
hw_timer_t * correction_servo_int = NULL;
hw_timer_t * watchdog_flora = NULL;
hw_timer_t * correction_vertical_jam_int = NULL;

void setup()
{
  Serial.begin(115200);

  autoconnectAP();
  stepper_vertical_setup();
  stepper_horizontal_setup();
  
  flora_setup();
  water_setup();
  light_setup();

  servo_setup();
  
}

void loop(){
  if (once==1){
    flora_rutine();
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


  

  Serial.println("change servo state");
  Serial.println(pos->angular);
  //enable stepper vertical before servo move securing the position and not fall.
  enable_stepper_vertical();
  servo_to_pos(pos->angular);
  

  /*
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
  */
}

void move_to_id(int id){
  struct position_plant pos;
  get_position(id, &pos);
  move_to_pos(&pos);
}


void autoconnectAP(){
  
  //CONNECT TO INTERNET
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  //wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("hort_vertical_AP","password"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      // ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
  }

}
