// array of different xiaomi flora MAC addresses
char* FLORA_DEVICES[] = {
    "C4:7C:8D:6E:17:07"
};
char* floraMacAddress = "C4:7C:8D:6E:17:07";

// sleep between to runs in seconds
#define SLEEP_DURATION 30 * 60
// emergency hibernate countdown in seconds
#define EMERGENCY_HIBERNATE 3 * 60
// how often should a device be retried in a run when something fails
#define RETRY 3

const char*   WIFI_SSID       = "MOVISTAR_F543";
const char*   WIFI_PASSWORD   = "3327CAC745F77E9FBB62";

//Serial number
const String     SERIAL_NUM = "111111";

// MQTT topic gets defined by "<MQTT_BASE_TOPIC>/<MAC_ADDRESS>/<property>"
// where MAC_ADDRESS is one of the values from FLORA_DEVICES array
// property is either temperature, moisture, conductivity, light 
const char*   MQTT_HOST       = "broker.hivemq.com";
const int     MQTT_PORT       = 1883;
const char*   MQTT_CLIENTID   = "miflora-client";
const char*   MQTT_USERNAME   = "tfg_user";
const char*   MQTT_PASSWORD   = "RandomPassword321";
const String  MQTT_BASE_TOPIC = SERIAL_NUM; 
const int     MQTT_RETRY_WAIT = 5000;
