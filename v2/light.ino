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
