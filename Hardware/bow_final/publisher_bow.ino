void publisher_bow(void *arg) {
//  char audio_buf[I2S_READ_LEN + 4];
  char bow_buf[64];
  char battery_buf[16];
  while (true) {
    // Blocks here waiting for data, not inside MQTT internals

    if (xQueueReceive(bowQueue, bow_buf, (5000/portTICK_PERIOD_MS)) == pdTRUE) {
      esp_mqtt_client_publish(mqtt_client, "sensor/bow", bow_buf, 0, 0, 0);
//            Serial.println("publishing bow");

    }
    if (xQueueReceive(batteryQueue, battery_buf, 0) == pdTRUE) {
      esp_mqtt_client_publish(mqtt_client, "sensor/battery", battery_buf, 0, 0, 0);
            Serial.println("publishing battery");

    }
  }
}
