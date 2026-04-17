void publisher(void *arg) {
  uint8_t audioIndex = 0;
  while (true) {
    // Blocks here waiting for data, not inside MQTT internals

    if (xQueueReceive(audioQueue, &audioIndex, (5000/portTICK_PERIOD_MS)) == pdTRUE) {
      AudioFrame *publishFrame = &audioBuffer[audioIndex];
      esp_mqtt_client_publish(mqtt_client, "sensor/audio", (char *)publishFrame, sizeof(AudioFrame), 0, 0);
    }
 vTaskDelay(pdMS_TO_TICKS(2));
  }
}
