
int vel_clockwise=50;
int vel_anticlockwise=50;
int test;

void IRAM_ATTR servo_correction_cb() {
  Serial.println("in servo correction cb");
  if ((digitalRead(final_carrera_p2)==0) || (digitalRead(final_carrera_p3)==0) ){
    angular_pos_state=130;
    myservo.write(angular_pos_state);
    delay(100);  
  }
}


void servo_setup() {
  
  correction_servo_int = timerBegin(0, 80, true);//configurem timer amb preescaler a 80
  timerAttachInterrupt(correction_servo_int, &servo_correction_cb, true);
  timerAlarmWrite(correction_servo_int, 300000, true); //definim temps en microsegons 1000000microsegons =1 segon, aixo es degut al preescaler de 80.
  timerAlarmDisable(correction_servo_int);
  
  myservo.attach(servo_control);


}


void anticlockwise_servo(int degree){
  for (angular_pos_state ; angular_pos_state >= degree; angular_pos_state -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(angular_pos_state);              // tell servo to go to position in variable 'pos'
    delay(vel_anticlockwise);                       // waits 15ms for the servo to reach the position
  }
}

void clockwise_servo(int degree){
  for (angular_pos_state; angular_pos_state <= degree; angular_pos_state += 1) { // goes from 0 degrees to 180 degrees
    myservo.write(angular_pos_state);              // tell servo to go to position in variable 'pos'
    delay(vel_clockwise);                       // waits 15ms for the servo to reach the position
  } 
}

void servo_to_pos(int angular_pos){
  if (angular_pos < angular_pos_state){
      anticlockwise_servo(angular_pos);
    }
    else if (angular_pos > angular_pos_state){
      clockwise_servo(angular_pos);
    }
}

void servo_hard_shake(void){
  int degree=5;
  for (int i=0; i<20; i++){
    myservo.write(angular_pos_state+degree);
    delay(300);
    myservo.write(angular_pos_state-degree);
    delay(300);
  }
  vel_clockwise=50;
}

void servo_soft_shake(void){
  int degree=5;
  for (int i=0; i<5; i++){
    clockwise_servo(angular_pos_state+degree);
    delay(300);
    anticlockwise_servo (angular_pos_state-degree);
    delay(300);
  }
}

void disable_servo(void){
  myservo.detach();
}

void enable_servo(void){
  myservo.attach(servo_control);
}
