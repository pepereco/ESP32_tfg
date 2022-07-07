const int vertical_velocity = 100;
volatile int reajuste_pos=0;
//params 
const int steps_ref = 1000;

void IRAM_ATTR vertical_correction_cb() {
  int static counter=0;
  Serial.println("in correction  cb");
  static bool enter=true;
  if (enter == true){
    enter=false;
    digitalWrite(dirPin,HIGH);
    timerAlarmWrite(correction_vertical_jam_int, 2000000, true); 
  }
  else{
    counter+=1;
    enter = true;
    digitalWrite(dirPin,LOW);
    if (counter==5){
      counter=0;
      timerAlarmWrite(correction_vertical_jam_int, 1200000, true);
    }
    else{
      timerAlarmWrite(correction_vertical_jam_int, 500000, true);
    }
  }
}

void IRAM_ATTR vertical_pos1_interruption()
{
  enable_stepper_vertical();
  Serial.println("reajuste int pos1 out");

  if (vertical_pos_state==1){
    Serial.println("reajuste int pos1 in");
    reajuste_pos=1;
    timerAlarmEnable(correction_vertical_int);
  }
  else{
    detachInterrupt(final_carrera_p1);
  }
  
}

void IRAM_ATTR vertical_pos3_interruption()
{
  enable_stepper_vertical();
  if (vertical_pos_state==3){
    reajuste_pos=1;
    timerAlarmEnable(correction_vertical_int);
    //vertical_pos_3();
  }
  else{
    detachInterrupt(final_carrera_p3);
  }

}

void IRAM_ATTR time_cb() {

  Serial.println("reajuste timer");
  if (reajuste_pos==1){
    
    if (vertical_pos_state==1){
      Serial.println("reajuste timer pos1");
      vertical_pos_1();
    }
    else if (vertical_pos_state==2){
      Serial.println("reajuste timer pos2");
      vertical_pos_2();
    }
    else if (vertical_pos_state==3){
      vertical_pos_3();
    }
    else if (vertical_pos_state==4){
      vertical_pos_4();
    }
  }

}

 
void stepper_vertical_setup()
{
  
  correction_vertical_int = timerBegin(1, 80, true);
  timerAttachInterrupt(correction_vertical_int, &time_cb, true);
  timerAlarmWrite(correction_vertical_int, 200000, true);
  timerAlarmDisable(correction_vertical_int);

  correction_vertical_jam_int = timerBegin(3, 80, true);//configurem timer amb preescaler a 80
  timerAttachInterrupt(correction_vertical_jam_int, &vertical_correction_cb, true);
  timerAlarmWrite(correction_vertical_jam_int, 2000000, true); //definim temps en microsegons 1000000microsegons =1 segon, aixo es degut al preescaler de 80.
  timerAlarmDisable(correction_vertical_jam_int);

  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enablePin,OUTPUT);
  pinMode(final_carrera_p1,INPUT);
  pinMode(final_carrera_p2,INPUT);
  pinMode(final_carrera_p3,INPUT);
  pinMode(final_carrera_p4,INPUT);
  

   digitalWrite(enablePin,HIGH);
   digitalWrite(dirPin,LOW);

   esp_reset_reason_t reason_reset;
  reason_reset = esp_reset_reason();

  if (reason_reset == ESP_RST_POWERON || reason_reset == ESP_RST_UNKNOWN  ){
    vertical_pos_state=4;
    start_pos =1;
  }
}
 


void vertical_pos(int pos){
  reajuste_pos=0;
  timerAlarmDisable(correction_vertical_int);
  detachInterrupt(final_carrera_p1);
  detachInterrupt(final_carrera_p2);
  detachInterrupt(final_carrera_p3);
  detachInterrupt(final_carrera_p4);
  if (pos==1){
    vertical_pos_1();
  }
  else if (pos==2){
    vertical_pos_2();
  }
  else if (pos==3){
    vertical_pos_3();
  }
  else if (pos==4){
    vertical_pos_4();
  }
}

void vertical_pos_1(){

  Serial.println("im in pos 1");

  digitalWrite(enablePin,LOW);
  digitalWrite(dirPin,HIGH); //Changes the rotations direction
  //delay(500);

  if (reajuste_pos==0){
    Serial.println(digitalRead(final_carrera_p1));
    while(digitalRead(final_carrera_p1)==1){  
      Serial.println("in p1 high");
      // Makes 400 pulses for making two full cycle rotation
      for(int x = 0; x < steps_ref/4; x++) {
        digitalWrite(stepPin,HIGH);
        delayMicroseconds(vertical_velocity);
        digitalWrite(stepPin,LOW);
        delayMicroseconds(vertical_velocity);
      }
    }
    // Makes last pulses for center position
    for(int x = 0; x < steps_ref; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    //makes 400*3 pulses for being on top of button
    digitalWrite(dirPin,HIGH);
    for(int x = 0; x < steps_ref*5; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    Serial.println("end pos1");
    vertical_pos_state=1;
    attachInterrupt(final_carrera_p1, vertical_pos1_interruption, FALLING);
    digitalWrite(enablePin,HIGH);
  }
  else if (reajuste_pos==1){
    if (digitalRead(final_carrera_p1)==0){
      for(int x = 0; x < 30; x++) {
          digitalWrite(stepPin,HIGH);
          delayMicroseconds(vertical_velocity);
          digitalWrite(stepPin,LOW);
          delayMicroseconds(vertical_velocity);
        }
    }
    else{
      for(int x = 0; x < 30; x++) {
          digitalWrite(stepPin,HIGH);
          delayMicroseconds(vertical_velocity);
          digitalWrite(stepPin,LOW);
          delayMicroseconds(vertical_velocity);
        }
      reajuste_pos=0;
      timerAlarmDisable(correction_vertical_int);
      Serial.println("end pos1");
      //digitalWrite(enablePin,HIGH);
    }
  }
}


void vertical_pos_2(){

  Serial.println("im in pos 2");

  digitalWrite(enablePin,LOW);

  if (vertical_pos_state==1){
    Serial.println("in low");
    digitalWrite(dirPin,LOW);
  }
  else{
    digitalWrite(dirPin,HIGH);
  }

  if (reajuste_pos==0){
    while(digitalRead(final_carrera_p2)==1){  
      Serial.println("is high pos2");
      // Makes 400 pulses for making two full cycle rotation
      for(int x = 0; x < steps_ref/4; x++) {
        digitalWrite(stepPin,HIGH);
        delayMicroseconds(vertical_velocity);
        digitalWrite(stepPin,LOW);
        delayMicroseconds(vertical_velocity);
      }
    }
    /*
    // Makes last pulses for center position
    for(int x = 0; x < steps_ref/4; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }*/
    Serial.println("end pos2");
    vertical_pos_state=2;
    //attachInterrupt(final_carrera_p2, vertical_pos2_interruption, FALLING);
    digitalWrite(enablePin,HIGH);
  }
}


void vertical_pos_3(){
  
  digitalWrite(enablePin,LOW);
  if (vertical_pos_state==1 || vertical_pos_state ==2 ){
    digitalWrite(dirPin,LOW);
  }
  else{
    digitalWrite(dirPin,HIGH);
  }

  if (reajuste_pos==0){
    while(digitalRead(final_carrera_p3)==1){  
      
      // Makes 400 pulses for making two full cycle rotation
      for(int x = 0; x < steps_ref/4; x++) {
        digitalWrite(stepPin,HIGH);
        delayMicroseconds(vertical_velocity);
        digitalWrite(stepPin,LOW);
        delayMicroseconds(vertical_velocity);
      }
    }
    //Makes 400 pulses for center position
    /*
    for(int x = 0; x < steps_ref; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    //makes 400*3 pulses for being on top of button
    digitalWrite(dirPin,HIGH);
    for(int x = 0; x < steps_ref*3; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }*/
    
    attachInterrupt(final_carrera_p3, vertical_pos3_interruption, FALLING);
    Serial.println("end pos3");
    vertical_pos_state=3;
    digitalWrite(enablePin,HIGH);
  }
  else if (reajuste_pos==1){
    if (digitalRead(final_carrera_p3)==0){
      for(int x = 0; x < 40; x++) {
          digitalWrite(stepPin,HIGH);
          delayMicroseconds(vertical_velocity);
          digitalWrite(stepPin,LOW);
          delayMicroseconds(vertical_velocity);
        }
    }
    else{
      reajuste_pos=0;
      timerAlarmDisable(correction_vertical_int);
      Serial.println("end pos3");
      //digitalWrite(enablePin,HIGH);
    }
  }
}

void vertical_pos_4(){
  
  digitalWrite(enablePin,LOW);

  if (vertical_pos_state==1 || vertical_pos_state ==2 || vertical_pos_state ==3 ){
    Serial.println("in high pos3");
    digitalWrite(dirPin,LOW);
  }
  else{
    Serial.println("in low pos3");
    digitalWrite(dirPin,HIGH);
  }
  
  if (reajuste_pos==0){
    while(digitalRead(final_carrera_p4)==1){  
      
      // Makes 400 pulses for making two full cycle rotation
      for(int x = 0; x < steps_ref/4; x++) {
        digitalWrite(stepPin,HIGH);
        delayMicroseconds(vertical_velocity);
        digitalWrite(stepPin,LOW);
        delayMicroseconds(vertical_velocity);
      }
    }
    //Makes 400 pulses for center position
    for(int x = 0; x < steps_ref*2; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    Serial.println("end pos4");
    vertical_pos_state=4;
    //attachInterrupt(final_carrera_p4, vertical_pos4_interruption, FALLING);
    digitalWrite(enablePin,HIGH);
  }
}

void go_down(){
  digitalWrite(enablePin,LOW);
  if (vertical_pos_state==4){
    detachInterrupt(final_carrera_p4);
  }
  else if (vertical_pos_state==2){
    detachInterrupt(final_carrera_p2);
  }
  
  digitalWrite(dirPin,LOW);
  for(int x = 0; x < steps_ref*5; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
  //digitalWrite(enablePin,HIGH);
}

void go_up(){
  digitalWrite(enablePin,LOW);
  digitalWrite(dirPin,HIGH);
  for(int x = 0; x < steps_ref*5; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
}

void disable_stepper_vertical(void){
  digitalWrite(enablePin,HIGH);
}

void enable_stepper_vertical(void){
  digitalWrite(enablePin,LOW);
}
