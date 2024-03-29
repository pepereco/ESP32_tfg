
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

WiFiClient espClient;
PubSubClient client(espClient);


void IRAM_ATTR watchdog_flora_cb() {
  esp_restart();
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
  BLEClient* floraClient = BLEDevice::createClient();

  if (!floraClient->connect(floraAddress)) {
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
  BLERemoteCharacteristic* floraCharacteristic;
  
  // get device mode characteristic, needs to be changed to read data
  Serial.println("- Force device in data mode");
  floraCharacteristic = nullptr;
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_write_mode);
  }
  catch (...) {
    Serial.println("somethings is wrog");
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping device");
    return false;
  }

  // write the magic data bytes that force the flora to transmit the sensor data
  uint8_t buf[2] = {0xA0, 0x1F};
  floraCharacteristic->writeValue(buf, 2, true);

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

  //create watchdog for reading from flora actions
  watchdog_flora = timerBegin(0, 80, true);//configurem timer amb preescaler a 80
  timerAttachInterrupt(watchdog_flora, &watchdog_flora_cb, true);
  timerAlarmWrite(watchdog_flora, 60000000, true); //definim temps en microsegons 1000000microsegons =1 segon, aixo es degut al preescaler de 80.
  timerAlarmDisable(watchdog_flora);

  esp_reset_reason_t reason_reset;
  reason_reset = esp_reset_reason();

  if (reason_reset == ESP_RST_POWERON || reason_reset == ESP_RST_UNKNOWN  ){
    pos_reset=16;
    action_reset =0; 
  }


  Serial.println("pos after reset");
  Serial.println(pos_reset);
}

void disconnect_wifi_mqtt(){
  // disconnect wifi and mqtt
  disconnectWifi();
  disconnectMqtt();
}


void flora_read_send_data(int pos_id) {

  Serial.println("pos before reset");
  Serial.println(pos_id);

  //Enable flora action reset 
  action_reset=1;

  //Start watchdog
  timerAlarmEnable(watchdog_flora);

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
  action_reset = 0;
  timerAlarmDisable(watchdog_flora);
}
