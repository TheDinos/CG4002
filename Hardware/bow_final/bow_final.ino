/* Based on Oleg Mazurov's code for rotary encoder interrupt service routines for AVR micros
   here https://chome.nerpa.tech/mcu/reading-rotary-encoder-on-arduino/
   and using interrupts https://chome.nerpa.tech/mcu/rotary-encoder-interrupt-service-routine-for-avr-micros/

   This example does not use the port read method. Tested with Nano and ESP32
   both encoder A and B pins must be connected to interrupt enabled pins, see here for more info:
   https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
*/
#include <esp_task_wdt.h>
#include <driver/i2s.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <mqtt_client.h>
#include <driver/adc.h>

// Define rotary encoder pins
#define ENC_A 13
#define ENC_B 5

// Mic variables
#define I2S_WS 15
#define I2S_SD 35
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (512) // Bytes
#define COMPRESSED_AUDIO_SIZE (I2S_READ_LEN/2)
#define RECORD_TIME       (20) //Seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)
#define VAD_THRESHOLD     100    // Adjust this value based on your mic sensitivity
#define VAD_SILENCE_LIMIT 14

#define BUFFER_LEN 64
#define AUDIO_HEADER_SIZE 4
#define AUDIO_PCM_SAMPLES (I2S_READ_LEN / sizeof(int16_t))

#define AUDIO_BUFFER_SIZE (I2S_READ_LEN + AUDIO_HEADER_SIZE)

typedef struct {
  uint32_t sequence;
  int16_t pcm[AUDIO_PCM_SAMPLES];
} AudioFrame;


int counter = 0;
portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR read_encoder();
static long nowI2S, lastBatTime = 0;
static int lastCounter = 0;
static long lastTickTime = 0;
unsigned long _lastDecReadTime = micros();
int _pauseLength = 25000;
int16_t emptyPacket[AUDIO_PCM_SAMPLES] = {0};
static long now = 0;
static JsonDocument bowDoc;
static char bowBuffer[64];
unsigned long int currentMillis = 0, batteryMillis = 0, blinkMillis = 0;
int lowBat = 0, blinking = 0;
QueueHandle_t bowQueue = xQueueCreate(BUFFER_LEN, sizeof(char) * 64);
QueueHandle_t rotaryQueue = xQueueCreate(BUFFER_LEN, sizeof(int));
QueueHandle_t audioQueue = xQueueCreate(BUFFER_LEN, sizeof(int8_t));//queue that hold the index of the buffer that have data inside to be transmitted
QueueHandle_t batteryQueue = xQueueCreate(BUFFER_LEN, 16);
//int16_t audioBuffer[BUFFER_LEN * AUDIO_BUFFER_SIZE];
AudioFrame audioBuffer[BUFFER_LEN];

uint8_t audioBufferHead = 0;
// --- WIFI Details ---
//const char* ssid = "Samspot7"; // samuels details
//const char* password = "spotsam7";
const char* ssid = "Galaxy"; //marcus details
const char* password = "wumx7371";
//const char* ssid = "Vimalapugazhan's S25";
//const char* password = "62353535";
// --- MQTT Server (Laptop) ---
//const char* mqtt_server = "10.34.31.47"; //samuels details
const char* mqtt_server = "10.34.141.48"; //marcus details
//const char* mqtt_serve r = "10.235.117.163";
//const char* mqtt_server ="10.235.117.47";
int mqtt_port = 8883;

// --- Board Details ---
const char* client_id = "BEETLE02"; // REMEMBER TO CHANGE FOR THE SPECIFIC BOARD

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
-----END CERTIFICATE-----
)EOF";

// --- Connection Client ---
esp_mqtt_client_handle_t mqtt_client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
      Serial.println("Connected to MQTT Server");
      // Subscribe to topics (If needed)
      // esp_mqtt_client_subscribe(mqtt_client, "laptop/beetle", 0);
      break;

    case MQTT_EVENT_DISCONNECTED:
      Serial.println("Disconnected from MQTT Server. Will auto-reconnect.");
      break;

    case MQTT_EVENT_DATA:
      Serial.print("Message from the following topic arrived: ");
      printf("%.*s\r\n", event->topic_len, event->topic);

      // Process information coming from subscribed topics, should be empty here
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

  // Start the serial monitor to show output
  Serial.begin(115200);
  initWifi();

  // --- Initialize MQTT settings ---
  String uri = String("mqtts://") + mqtt_server + ":" + mqtt_port; // MQTTS to incdicate secure communications
  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.broker.address.uri = uri.c_str(); // Pass full URI
  mqtt_cfg.broker.verification.certificate = root_ca; // Set CA Certificate
  mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
  mqtt_cfg.credentials.client_id = client_id;
  mqtt_cfg.buffer.out_size = 4096+32; // Information published from the ESP32 will be stored in the buffer if the processor publishes faster than the WiFi transmits
  mqtt_cfg.outbox.limit = (I2S_READ_LEN+4)*32;
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

  // Register the callback event handlers
  esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);

  // Start the background MQTT task (handles keep alive pings and reconnection)
  esp_mqtt_client_start(mqtt_client);
  // Set encoder pins and attach interrupts
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), read_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), read_encoder, CHANGE);

  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
  
  i2sInit();
  xTaskCreatePinnedToCore(i2s_adc, "i2s_adc", 1024 * 12, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(publisher, "publisher", 1024*4, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(publisher_bow, "publisher_bow", 1024*4, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(rotary, "rotary", 1024 *4, NULL, 1, NULL, 1);

}

void loop() {

}
