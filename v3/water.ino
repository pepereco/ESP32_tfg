
void water_setup(){
  pinMode(waterPin, OUTPUT);
  digitalWrite(waterPin,LOW);
}

void water( int ml){
  if (ml > 0){
    //Starting point until the water is ready
    Serial.println("in water");
    Serial.println(ml);
    digitalWrite(waterPin,HIGH);
    delay(2000);
    
    //EVery 3.5 sec -> 100ml
    int time_water;
    time_water = ml * 35;
    delay(time_water);
    digitalWrite(waterPin,LOW);
  }
}
