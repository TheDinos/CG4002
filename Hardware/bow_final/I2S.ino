void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 16,
    .dma_buf_len = 256,
    .use_apll = 1
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void example_disp_buff(uint8_t* buf, int length)
{
  printf("======\n");
  for (int i = 0; i < length; i++) {
    printf("%02x ", buf[i]);
    if ((i + 1) % 8 == 0) {
      printf("\n");
    }
  }
  printf("======\n");
}


void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
{
  uint32_t j = 0;
  uint32_t dac_value = 0;
  for (int i = 0; i < len; i += 2) {

    dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
    d_buff[j++] = dac_value * 256 / 2048;
  }
}

int16_t calculateRMS(const int16_t* buff, int len) {
  int64_t sum = 0;
  for (int i = 0; i < len; i++) {
    int16_t sample = buff[i];
    sum += (int64_t)sample * sample;
  }
  return (int16_t)sqrt(sum / len);
}

void i2s_adc(void *arg)
{
  static JsonDocument bowBatteryDoc;
  static char bowBatBuffer[16];
  //    int flash_wr_size = 0;
  size_t bytes_read;
  int silence_count = 0;
  bool is_recording = false;
  //  uint8_t payload[i2s_read_len + 4];
  uint32_t audio_seq = 0;
  char* i2s_read_buff = (char*) calloc(I2S_READ_LEN, sizeof(char));
  //  uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  static long beforeProcessing = 0;
  int dummyValue;
  // Discard first two reads to let the mic stabilise
  i2s_read(I2S_PORT, (void*) i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);
  i2s_read(I2S_PORT, (void*) i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);

  Serial.println(" *** VAD Listening *** ");

  while (true) {
    if (uxQueueSpacesAvailable(audioQueue) == 0) {
      Serial.println("audio queue full");
      xQueueReceive(audioQueue, &dummyValue, 0);
    }
    AudioFrame* frame = &audioBuffer[audioBufferHead];


    i2s_read(I2S_PORT, frame->pcm, I2S_READ_LEN, &bytes_read, portMAX_DELAY);
    int16_t rms = calculateRMS(frame->pcm, bytes_read / sizeof(int16_t));
    for (int i = 0; i < bytes_read / sizeof(int16_t); i++) {
      int32_t amplified = (int32_t)frame->pcm[i] * 4;
      // Clamp to int16 range to avoid wrap-around distortion
      if (amplified > 32767)       amplified = 32767;
      else if (amplified < -32768) amplified = -32768;
      frame->pcm[i] = (int16_t)amplified;
    }

    //    printf("RMS: %d\n", rms);

    if (rms > VAD_THRESHOLD) {
      if (!lowBat) {
        digitalWrite(LED_BUILTIN, HIGH);
      }
      if (!is_recording) {
        Serial.println("Voice detected - recording");
        is_recording = true;
      }
      frame->sequence = audio_seq;
      xQueueSend(audioQueue, &audioBufferHead, 0);
      audioBufferHead = (audioBufferHead + 1) % BUFFER_LEN;
      audio_seq++;
      Serial.print(".");

      silence_count = 0;
    }
    else if (is_recording) {
      silence_count++;
      if (silence_count >= VAD_SILENCE_LIMIT) {
        Serial.println(" *** Silence detected - pausing *** ");
        is_recording = false;
        silence_count = 0;
        memcpy(frame->pcm, emptyPacket, I2S_READ_LEN);
      }

      frame->sequence = audio_seq;
      xQueueSend(audioQueue, &audioBufferHead, 0);
      audioBufferHead = ( audioBufferHead + 1 ) % BUFFER_LEN;
      audio_seq++;
      Serial.print(".");
    }
    else if (!lowBat) {
      digitalWrite(LED_BUILTIN, LOW);
    }
    currentMillis = millis();
    if (currentMillis - batteryMillis > 20000) {
      Serial.println(adc1_get_raw(ADC1_CHANNEL_0));
      if (adc1_get_raw(ADC1_CHANNEL_0) < 2100) {
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


    //    nowI2S = millis();
    //    if (nowI2S - lastBatTime > 5000) {
    //      //      Serial.println(adc1_get_raw(ADC1_CHANNEL_0));
    //      bowBatteryDoc.clear();
    //      bowBatteryDoc["value"] = adc1_get_raw(ADC1_CHANNEL_0);
    //
    //      serializeJson(bowBatteryDoc, bowBatBuffer);
    //      xQueueSend(batteryQueue, bowBatBuffer, 0);
    //      //      esp_mqtt_client_enqueue(mqtt_client, "sensor/bowBattery", bowBatBuffer, 0, 0, 0, true);
    //      lastBatTime = nowI2S;
    //    }
  }
}
