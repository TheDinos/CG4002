#include "I2Cdev.h"

#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

#include <WiFi.h>
#include <ArduinoJson.h>
#include <mqtt_client.h>
MPU6050 mpu;
#define ACCEL_MAG_THRESHOLD 5000
#define RESET_TO_IDLE_MS 500
#define RELOAD_ROLL_MAX_THRESHOLD -20 //roll has to be less than this value for the hand to be behind the back
#define RELOAD_ROLL_MAX_THRESHOLD_RAD (RELOAD_ROLL_MAX_THRESHOLD * M_PI / 180)
#define RELOAD_ROLL_MIN_THRESHOLD -45
#define RELOAD_ROLL_MIN_THRESHOLD_RAD (RELOAD_ROLL_MIN_THRESHOLD * M_PI / 180)
#define PUSH_ROLL_MIN_THRESHOLD 60 //pitch has to be greater than this value for the hand to be facing forward
#define PUSH_ROLL_MIN_THRESHOLD_RAD (PUSH_ROLL_MIN_THRESHOLD * M_PI / 180)
#define PUSH_ROLL_MAX_THRESHOLD 100
#define PUSH_ROLL_MAX_THRESHOLD_RAD (PUSH_ROLL_MAX_THRESHOLD * M_PI / 180)
#define RELOAD_POINTER_MAX_THRESHOLD 1350 //voltage from flex sensor has to be less than this
#define RELOAD_MIDDLE_MAX_THRESHOLD 1250
#define RELOAD_RING_MAX_THRESHOLD 1200
#define PUSH_POINTER_MIN_THRESHOLD 2000
#define PUSH_MIDDLE_MIN_THRESHOLD 1900
#define PUSH_RING_MIN_THRESHOLD 2000


// MPU control/status vars

bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer
uint16_t accelMag;
Quaternion q;
VectorInt16 aa;
VectorInt16 aaReal;
VectorFloat gravity;
float ypr[3];
enum motionState {
  IDLE, MOVEMENT, RELOAD,  PUSH
};
motionState state;
long int currentMillis = 0, buzzMillis = 0, movementMillis = 0,
         lastPrintMillis = 0, batteryMillis = 0, blinkMillis = 0, pushMillis = 0;
int buzzed = 0, debounced = 0, lowBat = 0, blinking = 0, previousStatePush = 0; 
JsonDocument gloveDoc;
char gloveBuffer[32];
void printAPR() {
  Serial.print("accel: ");
  Serial.print(accelMag);
  Serial.print(" pitch: ");
  Serial.print(ypr[1] * 180 / M_PI);
  Serial.print(" roll: ");
  Serial.print(ypr[2] * 180 / M_PI);
  Serial.print(" flex: ");
  Serial.print(analogRead(A3)); Serial.print(" ");
  Serial.print(analogRead(A2)); Serial.print(" ");
  Serial.println(analogRead(A1));
}
// --- WIFI Details ---
//const char* ssid = "Samspot7";
//const char* password = "spotsam7";
const char* ssid = "Galaxy"; //marcus details
const char* password = "wumx7371";
// --- MQTT Server (Laptop) ---
//const char* mqtt_server = "10.34.31.47"; //samuel details
//const char* mqtt_server = "10.140.212.47";
const char* mqtt_server = "10.34.141.48"; //marcus details

int mqtt_port = 8883;

// --- Board Details ---
const char* client_id = "BEETLE01"; // REMEMBER TO CHANGE FOR THE SPECIFIC BOARD

// --- Certificate Authority Details (For TLS) ---
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDmzCCAoOgAwIBAgIUKOhWS7PuBNxVJeUwb33rTJR/qUowDQYJKoZIhvcNAQEL
BQAwXTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDEWMBQGA1UEAwwNMTAuMjIzLjIwMi40
ODAeFw0yNjAyMjYwODU0NDBaFw0zNjAyMjQwODU0NDBaMF0xCzAJBgNVBAYTAkFV
MRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRz
IFB0eSBMdGQxFjAUBgNVBAMMDTEwLjIyMy4yMDIuNDgwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQCdnB8EXBsKt7sNa+3uCfdNm7l3EHpZFDkod6gtaAuY
kVBwbf2rsTgBC/6V+poZq70k38Z2aTIp7hiQFZdNRNV8bVW3hjTzwSwMTv/Q81jC
U+3I7EljoIebfDp7Oz2DH9oqifR5fPvdzEtHFO8FkXO7SYXD6kPLxni4haDrA9kU
cleUPbJN86O1jRvvbN5t7MK4dv0tzygQTbEZ402/HXPGElviAoQWrFVQSAoTbX/s
sIgZALUUXmQ9968ke+aXbeW8aDohrc13ajb5PEbRfspWKwmX/M5cP7W2ZGbTYdzj
88+XyLN+Ax/rsPfP5CRS/rHv0wDVkMnYYPuAzMRcjim7AgMBAAGjUzBRMB0GA1Ud
DgQWBBRXCfkxK39AsSKRTqu8zi58LZDPSTAfBgNVHSMEGDAWgBRXCfkxK39AsSKR
Tqu8zi58LZDPSTAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQA+
UjqkUhotCKvLvmeAB2aoovezlNqDvEI/aUbdl4Uy7KSi2E3wN3WRCgccJDbxqw0W
rcs9VLGbT/UjFQyJOXnHW+6vq/BZhi8+jszELIcQum16Za3A5l7YnlcqioKia9wy
sGH9CcA6rbfhH9/0Sc303OL6JWm1MqB54g94idLkjaGsn47qgdSnsdWK403HG+n+
3YzRDBapCTZkT2C0lxdWv0EPzq3GBrBBbvVUjtcQfqRUoAQKOuSVW1x2J8q/6foS
cjdwJK9H/yqdgdA1nY3hnb8VM5uGYbWtl2NgD87g5beElZc2uPQMKAP1PbndZc7E
g60VbbnzZLo6eO278wDW
-----END CERTIFICATE-----)EOF";

// --- Connection Client ---
esp_mqtt_client_handle_t mqtt_client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
      Serial.println("Connected to MQTT Server");
      // Subscribe to topics (If needed)
      break;

    case MQTT_EVENT_DISCONNECTED:
      Serial.println("Disconnected from MQTT Server. Will auto-reconnect.");
      break;

    case MQTT_EVENT_DATA:
      break;
    case MQTT_EVENT_ERROR:
      Serial.println("MQTT Error encountered");
      break;

    default:
      break;
  }
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Connected to WiFi");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Disconnected from WiFi");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);  
}

void initWifi() {
  // Register WiFi event handlers
  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // Automatically reconnect
  WiFi.setAutoReconnect(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for WiFi to be connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected at IP address:");
  Serial.println(WiFi.localIP());
}
void setup() {
  pinMode(D6, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

  Serial.begin(115200);
  delay(1000); // give serial time to come up after reset

  esp_reset_reason_t reason = esp_reset_reason();
  Serial.print(">>> RESET REASON: ");
  Serial.println(reason);
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();

  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
  delay(2000); //delay two seconds so user can get ready
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXGyroOffset(126);
  mpu.setYGyroOffset(-16);
  mpu.setZGyroOffset(12);
  mpu.setXAccelOffset(976);
  mpu.setYAccelOffset(-2983);
  mpu.setZAccelOffset(1343);
  if (devStatus == 0) {
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
  else {
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }
  initWifi();
  
  // --- Initialize MQTT settings ---
  String uri = String("mqtts://") + mqtt_server + ":" + mqtt_port; // MQTTS to incdicate secure communications
  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.broker.address.uri = uri.c_str(); // Pass full URI
  mqtt_cfg.broker.verification.certificate = root_ca; // Set CA Certificate
  mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
  mqtt_cfg.credentials.client_id = client_id;
  mqtt_cfg.buffer.out_size = 2048; // Information published from the ESP32 will be stored in the buffer if the processor publishes faster than the WiFi transmits
  mqtt_cfg.task.priority = 1;
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

  // Register the callback event handlers
  esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);

  // Start the background MQTT task (handles keep alive pings and reconnection)
  esp_mqtt_client_start(mqtt_client);
}

void loop() {
  if (!dmpReady) return;
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    fifoCount = mpu.getFIFOCount();
    if (fifoCount == 1024) {
      mpu.resetFIFO();
      Serial.println("FIFO overflow! Resetting...");
      return;
    }
    currentMillis = millis();
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetAccel(&aa, fifoBuffer);

    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
    accelMag = sqrt(aaReal.x * aaReal.x
                    + aaReal.y * aaReal.y
                    + aaReal.z * aaReal.z);
    if (currentMillis - lastPrintMillis > 100) {
      printAPR();
      lastPrintMillis = currentMillis;
    }
    if (state == IDLE) {
      if (accelMag > ACCEL_MAG_THRESHOLD) {
        state = MOVEMENT;
        movementMillis = currentMillis;
      }
    }
    else if (state == MOVEMENT) {
      if (accelMag > ACCEL_MAG_THRESHOLD) {
        movementMillis = currentMillis;
      }
      if (currentMillis - movementMillis > RESET_TO_IDLE_MS) {
        state = IDLE;
      }
      if (ypr[2] < RELOAD_ROLL_MAX_THRESHOLD_RAD && ypr[2] > RELOAD_ROLL_MIN_THRESHOLD_RAD ) {
        state = RELOAD;
        Serial.print("reload! ");
        printAPR();
      }
      if (ypr[2] > PUSH_ROLL_MIN_THRESHOLD_RAD && ypr[2] < PUSH_ROLL_MAX_THRESHOLD_RAD &&
          analogRead(A3) > PUSH_POINTER_MIN_THRESHOLD &&
          analogRead(A2) > PUSH_MIDDLE_MIN_THRESHOLD &&
          analogRead(A1) > PUSH_RING_MIN_THRESHOLD) {
        state = PUSH;
        
      }
    }
    else if (state == RELOAD) {
      if (analogRead(A3) < RELOAD_POINTER_MAX_THRESHOLD &&
          analogRead(A2) < RELOAD_MIDDLE_MAX_THRESHOLD &&
          analogRead(A1) < RELOAD_RING_MAX_THRESHOLD && !debounced) {

        digitalWrite(D6, HIGH);

        esp_mqtt_client_publish(mqtt_client, "sensor/glove", "0", 0, 0, 0);
        buzzMillis = currentMillis + 100;

        buzzed = 1;
        debounced = 1;
        state = IDLE;
      }
      if (ypr[2] > RELOAD_ROLL_MAX_THRESHOLD_RAD || ypr[2] < RELOAD_ROLL_MIN_THRESHOLD_RAD ) {
        state = MOVEMENT;
        movementMillis = currentMillis;
      }
    }
    else if (state == PUSH) {

      if ( !debounced) {

        digitalWrite(D6, HIGH);
        esp_mqtt_client_publish(mqtt_client, "sensor/glove", "1", 0, 0, 0);
        Serial.print("push! ");
        printAPR();
        buzzMillis = currentMillis;
        buzzed = 3;
        debounced = 1;
        state = IDLE;
      }
      if (ypr[2] < PUSH_ROLL_MIN_THRESHOLD_RAD || ypr[2] > PUSH_ROLL_MAX_THRESHOLD_RAD) {
        state = MOVEMENT;
        movementMillis = currentMillis;
        previousStatePush = 0;
      }
    }

  }
  if (buzzed && currentMillis - buzzMillis > 200) {

    if (buzzed == 3) {
      buzzed = 2;
      digitalWrite(D6, LOW);
      buzzMillis = currentMillis - 100;
    }
    else if (buzzed == 2) {
      buzzed = 1;
      digitalWrite(D6, HIGH);
      buzzMillis = currentMillis;
    }
    else if (buzzed = 1) {
      buzzed = 0;
      digitalWrite(D6, LOW);

    }
  }
  if (debounced && currentMillis - buzzMillis > 1000) {
    debounced = 0;
  }
  if (currentMillis - batteryMillis > 20000) {
    if (analogRead(A0) < 2100) {
      lowBat = 1;
    }
    else {
      lowBat = 0;
    }
    batteryMillis = currentMillis;
  }
  if (lowBat && currentMillis - blinkMillis > 500) {
    if (blinking) {
      digitalWrite(LED_BUILTIN, LOW);
      blinking = 0;
      blinkMillis = currentMillis;
    }
    else {
      digitalWrite(LED_BUILTIN, HIGH);
      blinking = 1;
      blinkMillis = currentMillis;
    }
  }
}
