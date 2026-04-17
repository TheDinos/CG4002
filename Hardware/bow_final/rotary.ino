void IRAM_ATTR read_encoder() {
  // Encoder interrupt routine for both pins. Updates counter
  // if they are valid and have rotated a full indent

  static uint8_t old_AB = 4;  // Lookup table index
  static int8_t encval = 0;   // Encoder value
  static const int8_t enc_states[]  = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0}; // Lookup table

  old_AB <<= 2; // Remember previous state

  if (digitalRead(ENC_A)) old_AB |= 0x02; // Add current state of pin A
  if (digitalRead(ENC_B)) old_AB |= 0x01; // Add current state of pin B

  encval += enc_states[( old_AB & 0x0f )];

  // Update counter if encoder has rotated a full indent, that is at least 4 steps
  if ( encval > 3 ) {       // Four steps forward
    counter = counter + 1;            // Update counter
    encval = 0;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(rotaryQueue, &counter, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
  else if ( encval < -3 ) {       // Four steps backward

    //    _lastDecReadTime = micros();
    counter = counter - 1;            // Update counter
    encval = 0;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(rotaryQueue, &counter, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void rotary(void *arg) {
  int current_counter;
  while (true) {
    if (xQueueReceive(rotaryQueue, &current_counter, portMAX_DELAY) == pdTRUE) {
      now = millis();
      Serial.print("bow charge: ");
      Serial.print(current_counter);
      Serial.print(", delta: ");
      Serial.println(now - lastTickTime);
      bowDoc.clear();
      
      if (current_counter < lastCounter && now - lastTickTime < 100) {
        Serial.println("Bow Fired!");
        bowDoc["draw"] = current_counter;
        bowDoc["fired"] = 1;
      } else {
        bowDoc["draw"] = current_counter;
        bowDoc["fired"] = 0;
      }

      serializeJson(bowDoc, bowBuffer);
      xQueueSend(bowQueue, bowBuffer, 0);

      lastCounter = current_counter;
      lastTickTime = now;

      digitalWrite(25, current_counter >= 1 ? HIGH : LOW);
      digitalWrite(26, current_counter >= 2 ? HIGH : LOW);
      digitalWrite(27, current_counter >= 3 ? HIGH : LOW);
    }
  }
}
