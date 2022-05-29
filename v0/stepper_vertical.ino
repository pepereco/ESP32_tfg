
volatile int reajuste_pos=0;
//params 
const int steps_ref = 200;


void IRAM_ATTR vertical_pos1_interruption()
{

  Serial.println("reajuste int pos1 out");

  if (vertical_pos_state==1){
    Serial.println("reajuste int pos1 in");
    reajuste_pos=1;
    timerAlarmEnable(correction_vertical_int);
    //vertical_pos_1();
  }
  else{
    detachInterrupt(final_carrera_p1);
  }
  
}

void IRAM_ATTR vertical_pos2_interruption()
{

  Serial.println("reajuste int pos2 out");
  if (vertical_pos_state==2){
    Serial.println("reajuste int pos2 in");
    reajuste_pos=1;
    timerAlarmEnable(correction_vertical_int);
    //vertical_pos_2();
  }
  else{
    detachInterrupt(final_carrera_p2);
  }
  

}

void IRAM_ATTR vertical_pos3_interruption()
{

  if (vertical_pos_state==3){
    reajuste_pos=1;
    timerAlarmEnable(correction_vertical_int);
    //vertical_pos_3();
  }
  else{
    detachInterrupt(final_carrera_p3);
  }

}

void IRAM_ATTR vertical_pos4_interruption()
{

  if (vertical_pos_state==4){
    reajuste_pos=1;
    timerAlarmEnable(correction_vertical_int);
    //vertical_pos_4();
  }
  else{
    detachInterrupt(final_carrera_p4);
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
  //vertical_pos_state=1;
  
  correction_vertical_int = timerBegin(1, 80, true);
  timerAttachInterrupt(correction_vertical_int, &time_cb, true);
  timerAlarmWrite(correction_vertical_int, 200000, true);
  timerAlarmDisable(correction_vertical_int);

  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enablePin,OUTPUT);
  pinMode(final_carrera_p1,INPUT_PULLUP);
  pinMode(final_carrera_p2,INPUT_PULLUP);
  pinMode(final_carrera_p3,INPUT_PULLUP);
  pinMode(final_carrera_p4,INPUT_PULLUP);
  //attachInterrupt(final_carrera_p1, vertical_pos1_interruption, RISING);
  //attachInterrupt(final_carrera_p2, vertical_pos2_interruption, RISING);
  //attachInterrupt(final_carrera_p3, vertical_pos3_interruption, RISING);
  //attachInterrupt(final_carrera_p4, vertical_pos4_interruption, RISING);

   digitalWrite(enablePin,HIGH);
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


  digitalWrite(enablePin,LOW);
  digitalWrite(dirPin,LOW); //Changes the rotations direction

  vertical_pos_state=1;

  if (reajuste_pos==0){
    while(digitalRead(final_carrera_p1)==1){  
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
    digitalWrite(dirPin,LOW);
    for(int x = 0; x < steps_ref*3; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    Serial.println("end pos1");
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
        digitalWrite(enablePin,HIGH);
    }
    else{
      reajuste_pos=0;
      timerAlarmDisable(correction_vertical_int);
      Serial.println("end pos1");
      digitalWrite(enablePin,HIGH);
    }
  }
}

void vertical_pos_2(){
  
  digitalWrite(enablePin,LOW);
  if (vertical_pos_state==1){
    Serial.println("in low");
    digitalWrite(dirPin,HIGH);
  }
  else{
    digitalWrite(dirPin,LOW);
  }
  vertical_pos_state=2;

  if (reajuste_pos==0){
    Serial.println("in not reajuste pos 2");
    while(digitalRead(final_carrera_p2)==1){  
      
      // Makes 400 pulses for making two full cycle rotation
      for(int x = 0; x < steps_ref/4; x++) {
        digitalWrite(stepPin,HIGH);
        delayMicroseconds(vertical_velocity);
        digitalWrite(stepPin,LOW);
        delayMicroseconds(vertical_velocity);
      }
    }
    //Makes 400 pulses for center position
    for(int x = 0; x < steps_ref; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    //makes 400*3 pulses for being on top of button
    digitalWrite(dirPin,LOW);
    for(int x = 0; x < steps_ref*3; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    Serial.println("end pos2");
    attachInterrupt(final_carrera_p2, vertical_pos2_interruption, FALLING);
    digitalWrite(enablePin,HIGH);
  }
  else if (reajuste_pos==1){
    Serial.println("in reajuste pos 2");
    if (digitalRead(final_carrera_p2)==0){
      for(int x = 0; x < 30; x++) {
          digitalWrite(stepPin,HIGH);
          delayMicroseconds(vertical_velocity);
          digitalWrite(stepPin,LOW);
          delayMicroseconds(vertical_velocity);
        }
        digitalWrite(enablePin,HIGH);
    }
    else{
      reajuste_pos=0;
      timerAlarmDisable(correction_vertical_int);
      Serial.println("end pos2");
      digitalWrite(enablePin,HIGH);
    }
  }
}


void vertical_pos_3(){
  
  digitalWrite(enablePin,LOW);
  if (vertical_pos_state==1 || vertical_pos_state ==2 ){
    digitalWrite(dirPin,HIGH);
  }
  else{
    digitalWrite(dirPin,LOW);
  }
  vertical_pos_state=3;

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
    
    for(int x = 0; x < steps_ref; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    //makes 400*3 pulses for being on top of button
    digitalWrite(dirPin,LOW);
    for(int x = 0; x < steps_ref*3; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    
    attachInterrupt(final_carrera_p3, vertical_pos3_interruption, FALLING);
    Serial.println("end pos3");
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
        digitalWrite(enablePin,HIGH);
    }
    else{
      reajuste_pos=0;
      timerAlarmDisable(correction_vertical_int);
      Serial.println("end pos3");
      digitalWrite(enablePin,HIGH);
    }
  }
}

void vertical_pos_4(){
  
  digitalWrite(enablePin,LOW);

  if (vertical_pos_state==1 || vertical_pos_state ==2 || vertical_pos_state ==3 ){
    Serial.println("in high pos3");
    digitalWrite(dirPin,HIGH);
  }
  else{
    Serial.println("in low pos3");
    digitalWrite(dirPin,LOW);
  }
  
  vertical_pos_state=4;
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
    for(int x = 0; x < steps_ref; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    //makes 400*3 pulses for being on top of button
    digitalWrite(dirPin,LOW);
    for(int x = 0; x < steps_ref*3; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
    Serial.println("end pos4");
    attachInterrupt(final_carrera_p4, vertical_pos4_interruption, FALLING);
    digitalWrite(enablePin,HIGH);
  }
  else if (reajuste_pos==1){
    if (digitalRead(final_carrera_p4)==0){
      for(int x = 0; x < 30; x++) {
          digitalWrite(stepPin,HIGH);
          delayMicroseconds(vertical_velocity);
          digitalWrite(stepPin,LOW);
          delayMicroseconds(vertical_velocity);
        }
        digitalWrite(enablePin,HIGH);
    }
    else{
      reajuste_pos=0;
      timerAlarmDisable(correction_vertical_int);
      Serial.println("end pos4");
      digitalWrite(enablePin,HIGH);
    }
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
  
  digitalWrite(dirPin,HIGH);
  for(int x = 0; x < steps_ref*10; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
  digitalWrite(enablePin,HIGH);
}

void go_up(){
  digitalWrite(enablePin,LOW);
  digitalWrite(dirPin,LOW);
  for(int x = 0; x < steps_ref*10; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(vertical_velocity);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(vertical_velocity);
    }
  digitalWrite(enablePin,HIGH);
  if (vertical_pos_state==4){
    attachInterrupt(final_carrera_p4, vertical_pos4_interruption, FALLING);
  }
  else if (vertical_pos_state==2){
    attachInterrupt(final_carrera_p2, vertical_pos4_interruption, FALLING);
  }
}
