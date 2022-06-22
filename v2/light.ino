const unsigned long SECOND = 1000;
const unsigned long MIN = 60*SECOND;

void light_setup(){
  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin,LOW);
}


void lightON(){
  Serial.println("in light");
  digitalWrite(lightPin,HIGH);
  
}
void lightOFF(){
  digitalWrite(lightPin,LOW);
  
}

void lights(){
  disable_stepper_vertical();
  disable_servo();
  lightON();
  //delay(MIN * mins);
  delay(mins);
  lightOFF();
}

void light_rutine(){
  Serial.println("IN LIGHT RUTINE");
  enable_stepper_vertical();
  enable_servo();
  //for (int i = 15; i<=60;i++){
  for (int i = 15; i<=17;i++){
    Serial.println("this is the new pos");
    Serial.println(i);
    //canvi de pis
    if (i==30){
        move_to_id(0);
        i+=15;
      }
    //Acabem
    else if (i==60){
        move_to_id(30); 
        move_to_id(45); 
        disable_stepper_vertical();
        disable_servo();
        break;
      }
    else{
      if (i==15){
        move_to_id(0);  
      }
      else if (i==45){
        move_to_id(30); 
        
      }
      else{
        Serial.println(i-15);
        move_to_id(i-15);
        disable_stepper_vertical();
        disable_servo();
        delay(3000);
        Serial.println("get light");
        get_server_light(i);
        lights();
      }     
    } 
  }
}

void get_server_light(int pos){
  connectWifi(ssid_wifi, pwd_wifi);
  connectMqtt();
  client.subscribe((MQTT_BASE_TOPIC + "/" + "light_resp" + "/" + pos).c_str());
  connectMqtt();
  char buffer[64];
  snprintf(buffer, 64, "%d", 1);
  client.publish((MQTT_BASE_TOPIC + "/" + "light_req" + "/" + pos).c_str(), buffer); 
  //Wait to receive mqtt response message from server
  int start = millis();
  while ((millis() - start < 10000) && (sub_mqtt_flag == false)){
    client.loop();
  }
  client.unsubscribe((MQTT_BASE_TOPIC + "/" + "light_resp" + "/" + pos).c_str());
  client.disconnect();
  //if connection fails we assign mins=0
  if (sub_mqtt_flag == false){
    mins = 0;
  }
  sub_mqtt_flag=false;
}

bool request_start_lights(){
  bool ret_value;
  connectWifi(ssid_wifi, pwd_wifi);
  connectMqtt();
  client.subscribe((MQTT_BASE_TOPIC + "/" + "light_start_resp").c_str());
  connectMqtt();
  char buffer[64];
  snprintf(buffer, 64, "%d", 1);
  client.publish((MQTT_BASE_TOPIC + "/" + "light_start_req").c_str(), buffer); 
  //Wait to receive mqtt response message from server
  int start = millis();
  while ((millis() - start < 10000) && (sub_mqtt_flag == false)){
    client.loop();
  }
  client.unsubscribe((MQTT_BASE_TOPIC + "/" + "light_start_resp").c_str());
  client.disconnect();
  //if connection fails we assign mins=0
  if (sub_mqtt_flag == false){
    ret_value=false;
  }
  else{
    if (light_start_resp ==1){
      ret_value=true;
    }
    else{
      ret_value=false;
    }
  }
  sub_mqtt_flag=false;
  return ret_value;
}

