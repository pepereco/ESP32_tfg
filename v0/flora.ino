
// device count
static int deviceCount = sizeof FLORA_DEVICES / sizeof FLORA_DEVICES[0];

//This uuid are ids that the BLE service and characteristics that publish the Flora mi is publishing, more info:https://github-wiki-see.page/m/ChrisScheffler/miflora/wiki/The-Basics#services--characteristics and https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/
// the remote service we wish to connect to
static BLEUUID serviceUUID("00001204-0000-1000-8000-00805f9b34fb");

// the characteristic of the remote service we are interested in
static BLEUUID uuid_version_battery("00001a02-0000-1000-8000-00805f9b34fb");
static BLEUUID uuid_sensor_data("00001a01-0000-1000-8000-00805f9b34fb");
static BLEUUID uuid_write_mode("00001a00-0000-1000-8000-00805f9b34fb");

TaskHandle_t hibernateTaskHandle = NULL;
TaskHandle_t forceWriteHandle= NULL;
TaskHandle_t forceControlHandle= NULL;


WiFiClient espClient;
PubSubClient client(espClient);
BLERemoteCharacteristic* floraCharacteristicWrite;
volatile int done_force_write_task;
static RTC_NOINIT_ATTR  int pos_before_reset;

void TaskWriteForce(void * pvParameters) {

  uint8_t buf[2] = {0xA0, 0x1F};
  done_force_write_task =0;
  Serial.println("in force write before");
  floraCharacteristicWrite->writeValue(buf, 2, true);
  Serial.println("in force write after");
  done_force_write_task =1;
  vTaskDelete(forceWriteHandle);
  vTaskDelete(forceControlHandle);

}
void TaskControlForce(void * pvParameters) {
  unsigned long start = millis();
  while (millis()-start <3000){
    if (done_force_write_task==1){
      Serial.println("in force control before 1");
      //vTaskDelete(forceWriteHandle);
      vTaskDelete(forceControlHandle);
      //break;
    }
    Serial.println("in force control before 0");
  }
  Serial.println("in force control after 3 sec");
  vTaskDelete(forceWriteHandle);
  vTaskDelete(forceControlHandle);
  xTaskCreate(TaskWriteForce,"TaskWriteForce",5000,NULL,1,&forceWriteHandle );  
  xTaskCreate(TaskControlForce,"TaskControlForce",5000,NULL,1,&forceControlHandle);

}

void connectWifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
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
  Serial.println("Connecting to MQTT...");
  //espClient.setCACert(test_root_ca);
  client.setServer(MQTT_HOST, MQTT_PORT);

  while (!client.connected()) {
    if (!client.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.print("MQTT connection failed:");
      Serial.print(client.state());
      Serial.println("Retrying...");
      delay(MQTT_RETRY_WAIT);
    }
  }

  Serial.println("MQTT connected");
  Serial.println("");
}

void disconnectMqtt() {
  client.disconnect();
  Serial.println("MQTT disconnected");
}

BLEClient* getFloraClient(BLEAddress floraAddress) {
  Serial.println("start client get");
  BLEClient* floraClient = BLEDevice::createClient();
  Serial.println("start client get 2");

  esp_ble_addr_type_t type = BLE_ADDR_TYPE_PUBLIC;
  bool ret_conn = floraClient->connect(floraAddress, type);
  
  if (!ret_conn) {
    Serial.println("- Connection failed, skipping");
    return nullptr;
  }

  Serial.println("- Connection successful");
  return floraClient;
}

BLERemoteService* getFloraService(BLEClient* floraClient) {
  //WE get the service from the flora BLE server that contains the characteristics that we want such as the sensors
  BLERemoteService* floraService = nullptr;

  try {
    floraService = floraClient->getService(serviceUUID);
  }
  catch (...) {
    // something went wrong
  }
  if (floraService == nullptr) {
    Serial.println("- Failed to find data service");
  }
  else {
    Serial.println("- Found data service");
  }

  return floraService;
}

bool forceFloraServiceDataMode(BLERemoteService* floraService) {
  //We force the flora mi to publish the sensors characteristic by writing some bytes to the write characteristic.
  
  // get device mode characteristic, needs to be changed to read data
  Serial.println("- Force device in data mode");
  floraCharacteristicWrite = nullptr;
  Serial.println("in force 1");
  try {
    floraCharacteristicWrite = floraService->getCharacteristic(uuid_write_mode);
  }
  catch (...) {
    Serial.println("somethings is wrong");
    // something went wrong
  }
  if (floraCharacteristicWrite == nullptr) {
    Serial.println("-- Failed, skipping device");
    return false;
  }
  Serial.println("in force 2");

  uint8_t buf[2] = {0xA0, 0x1F};
  floraCharacteristicWrite->writeValue(buf, 2, true);

  /* 
  done_force_write_task=0;
  // write the magic data bytes that force the flora to transmit the sensor data
  BaseType_t ret_task1 = xTaskCreate(TaskWriteForce,"TaskWriteForce",5000,NULL,2,&forceWriteHandle);  
  Serial.println(ret_task1);

  BaseType_t ret_task2 = xTaskCreate(TaskControlForce,"TaskControlForce",5000,NULL,2,&forceControlHandle);
  Serial.println(ret_task2);
  //Wait
  while (done_force_write_task==0){}
  Serial.println("in force 3");
  */
  delay(500);
  return true;
}

bool readFloraDataCharacteristic(BLERemoteService* floraService, String baseTopic) {
  //Read flora sensors characteristic and format the data
  BLERemoteCharacteristic* floraCharacteristic = nullptr;

  // get the main device data characteristic
  Serial.println("- Access characteristic from device");
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_sensor_data);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping device");
    return false;
  }

  // read characteristic value
  Serial.println("- Read value from characteristic");
  std::string value;
  try{
    value = floraCharacteristic->readValue();
  }
  catch (...) {
    // something went wrong
    Serial.println("-- Failed, skipping device");
    return false;
  }
  const char *val = value.c_str();

  Serial.print("Hex: ");
  for (int i = 0; i < 16; i++) {
    Serial.print((int)val[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");

  int16_t* temp_raw = (int16_t*)val;
  float temperature = (*temp_raw) / ((float)10.0);
  Serial.print("-- Temperature: ");
  Serial.println(temperature);

  int moisture = val[7];
  Serial.print("-- Moisture: ");
  Serial.println(moisture);

  int light = val[3] + val[4] * 256;
  Serial.print("-- Light: ");
  Serial.println(light);
 
  int conductivity = val[8] + val[9] * 256;
  Serial.print("-- Conductivity: ");
  Serial.println(conductivity);

  if (temperature > 200) {
    Serial.println("-- Unreasonable values received, skip publish");
    return false;
  }

  //Send data with MQTT packed in buffers of 64 bytes

  char buffer[64];

  snprintf(buffer, 64, "%f", temperature);
  client.publish((baseTopic + "temperature").c_str(), buffer); 
  snprintf(buffer, 64, "%d", moisture); 
  client.publish((baseTopic + "moisture").c_str(), buffer);
  snprintf(buffer, 64, "%d", light);
  client.publish((baseTopic + "light").c_str(), buffer);
  snprintf(buffer, 64, "%d", conductivity);
  client.publish((baseTopic + "conductivity").c_str(), buffer);

  return true;
}


bool processFloraService(BLERemoteService* floraService, char* deviceMacAddress, int pos_id) {
  // set device in data mode
  if (!forceFloraServiceDataMode(floraService)) {
    return false;
  }

  String baseTopic = MQTT_BASE_TOPIC + "/" + deviceMacAddress + "/"+ pos_id + "/";
  bool dataSuccess = readFloraDataCharacteristic(floraService, baseTopic);

  return dataSuccess;
}

bool processFloraDevice(BLEAddress floraAddress, char* deviceMacAddress, int tryCount, int pos_id) {
  Serial.print("Processing Flora device at ");
  Serial.print(floraAddress.toString().c_str());
  Serial.print(" (try ");
  Serial.print(tryCount);
  Serial.println(")");

  // connect to flora ble server
  BLEClient* floraClient = getFloraClient(floraAddress);
  if (floraClient == nullptr) {
    return false;
  }

  // connect data service
  BLERemoteService* floraService = getFloraService(floraClient);
  if (floraService == nullptr) {
    floraClient->disconnect();
    return false;
  }

  // process devices data
  bool success = processFloraService(floraService, deviceMacAddress, pos_id);

  // disconnect from device
  floraClient->disconnect();

  return success;
}

/*
void hibernate() {
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION * 1000000ll);
  Serial.println("Going to sleep now.");
  delay(100);
  esp_deep_sleep_start();
}

void delayedHibernate(void *parameter) {
  delay(EMERGENCY_HIBERNATE*1000); // delay for five minutes
  Serial.println("Something got stuck, entering emergency hibernate...");
  hibernate();
}
*/
void flora_setup() {
  Serial.println("Initialize BLE client...");
  BLEDevice::init("");
  BLEDevice::setPower(ESP_PWR_LVL_P7);

  // connecting wifi and mqtt server
  connectWifi();
  connectMqtt();
  Serial.println("var after reset");
  Serial.println(pos_before_reset);
}

void disconnect_wifi_mqtt(){
  // disconnect wifi and mqtt
  disconnectWifi();
  disconnectMqtt();
}


void flora_read_send_data(int pos_id) {
  Serial.println("var before reset");
  pos_before_reset = pos_id;
  Serial.println(pos_before_reset);
  
  // process devices
  for (int i=0; i<deviceCount; i++) {
    int tryCount = 0;
    char* deviceMacAddress = FLORA_DEVICES[i];
    BLEAddress floraAddress(deviceMacAddress);

    while (tryCount < RETRY) {
      tryCount++;
      if (processFloraDevice(floraAddress, deviceMacAddress, tryCount, pos_id)) {
        break;
      }
      delay(1000);
    }
    delay(1500);
  }
}
