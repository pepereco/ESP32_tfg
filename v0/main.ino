#include <Servo.h>
#include "BLEDevice.h"
#include <WiFi.h>
#include <PubSubClient.h>

#include "config_flora.h"

#define plants_per_floor 14
#define vertical_velocity 2000

typedef struct position_plant{
  int vertical;
  int horizontal;
  int angular;
}position_plant;


// defines pins numbers
const int stepPin = 14;
const int dirPin = 26; 
const int enablePin = 32; 
const int final_carrera_p1=33;
const int final_carrera_p2=25;
const int final_carrera_p3=12;
const int final_carrera_p4=27;
const int motorPin1 = 15;    // 28BYJ48 In1
const int motorPin2 = 2;    // 28BYJ48 In2
const int motorPin3 = 0;   // 28BYJ48 In3
const int motorPin4 = 4;   // 28BYJ48 In4
const int  servo_control =13;
const int waterPin = 5;
const int lightPin = 17;

const position_plant array_positions_floor[plants_per_floor+1] ={
  {0,0,120},{0,29347,11},{0,20380,20},{0,12228,30},{0,6114,42},{0,4891,62},{0,8152,80},{0,16304,90},{0,24456,95},{0,33830,100},{0,32608,40},{0,29347,45},{0,29347,55},{0,30570,65},{0,32608,73},
};

//Global vars
int vertical_pos_state;
int current_step_horizontal = 0;
int once=1;
int counter =0;

Servo myservo;  // create servo object to control a servo


//timer interrupt
hw_timer_t * correction_vertical_int = NULL;
hw_timer_t * correction_servo_int = NULL;
hw_timer_t * timer_force_write = NULL;

void setup()
{
  Serial.begin(115200);
  stepper_vertical_setup();
  stepper_horizontal_setup();
  servo_setup();
  flora_setup();
  pinMode(waterPin, OUTPUT);
  digitalWrite(waterPin,LOW);
  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin,LOW);
}

void loop(){
  counter+=1;
  flora_read_send_data(counter);
  
  //myservo.write(40);  
  //delay(100);    
  //myservo.write(120); 
  //delay(100);     
  //servo_to_pos(40);
  //servo_to_pos(120);
  //anticlockwise_servo_full_test();
  delay(6000);
  if (once ==1){
    once=0;
    water(200);
    delay(3000);
    lightON();
    
  }
  
  /*
  if (once ==1){
    once=0;    
    for (int i=10; i<=14;i++){
      Serial.println(i);
      move_to_id(i);
      delay(3000);
      Serial.println(i+15);
      move_to_id(i+15);
      delay(3000);
    } 
   }
    
  /*
  //SIMPLE TEST VERTICAL
  if (once==1){
    once =0;
    vertical_pos(1);
    delay(5000);
    
    vertical_pos(2);
    delay(5000);

    vertical_pos(3);
    delay(5000);

    vertical_pos(4);
    delay(5000);  
  }*/
  
}

void get_position(int id, struct position_plant *pos){
  int plane_id;
  pos->vertical =(id/(plants_per_floor+1))+1;
  plane_id = id%(plants_per_floor+1);
  pos->horizontal = array_positions_floor[plane_id].horizontal;
  pos->angular = array_positions_floor[plane_id].angular;
}

void move_to_pos(struct position_plant *pos){
  
  if ((vertical_pos_state==2 && pos->vertical==3 )|| (vertical_pos_state== 3 && pos->vertical==2 ) ){
    timerAlarmEnable(correction_servo_int);
    Serial.println("change vertical state 1");
    Serial.println(pos->vertical);
    vertical_pos(pos->vertical);
    timerAlarmDisable(correction_servo_int);
  }
  else{
    Serial.println("change vertical state 2");
    Serial.println(pos->vertical);
    vertical_pos(pos->vertical);
  }

  Serial.println("change servo state");
  Serial.println(pos->angular);
  
  servo_to_pos(pos->angular);
  
  
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

void move_to_id(int id){
  struct position_plant pos;
  get_position(id, &pos);
  move_to_pos(&pos);
}

void water( int ml){
  //Starting point until the water is ready
  digitalWrite(waterPin,HIGH);
  delay(2000);
  
  //EVery 3.5 sec -> 100ml
  int time_water;
  time_water = ml * 35;
  delay(time_water);
  digitalWrite(waterPin,LOW);
  
}

void lightON(){
  digitalWrite(lightPin,HIGH);
  
}
