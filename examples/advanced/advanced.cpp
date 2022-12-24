#include <Arduino.h>
#include "PowerButton.h"

PowerButton myNewPowerButton0 = PowerButton(0, NO_PULL, true, 300, 0, 50);
hw_timer_t *aTimer; 

char characterArray[6];
uint8_t characterIndex = 0;
uint16_t timerCntr = 0;
bool timerEnabled = false;
QueueHandle_t printingHandle = NULL;
portMUX_TYPE _morseMux = portMUX_INITIALIZER_UNLOCKED;

char *morseMap[36] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--", "--..", "-----", ".----", "..---",
    "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

char letterMap[36] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

void printingTask(void *param) {
  while (1) {
    char dataArray[6];
    bool found = false;
    xQueueReceive(printingHandle, dataArray, portMAX_DELAY);
    for (int i = 0; i < 36; i++) { //turn codes to letters
      if (strcmp(morseMap[i], dataArray) == 0) {
        Serial.printf("%c", letterMap[i]);
        found = true;
        break;
      }
    }
    if (!found) {
      Serial.printf("pattern not recognized\n");
    }
  }
} 

void IRAM_ATTR gpioIsr0() {
  myNewPowerButton0.pinRun();
}

void ARDUINO_ISR_ATTR timer_isr(void) {
  PowerButton::timerRun(1); //pass the time in milliseconds elapsed since last time
  if (timerEnabled) {
    portENTER_CRITICAL_ISR(&_morseMux);
    if (!myNewPowerButton0.pressed()) { //don't count if the button is currently pressed
      timerCntr++;
    }
    if (timerCntr == 500) { //500 ms has passed without a button being pressed, send code to printing task
      timerCntr = 0;
      timerEnabled = false;
      BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
      characterArray[characterIndex] = 0x00;
      xQueueSendFromISR(printingHandle, characterArray, &pxHigherPriorityTaskWoken);
      characterIndex = 0;
      if (pxHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
      } 
    }
    portEXIT_CRITICAL_ISR(&_morseMux);
  }
}

void pushFunction(void *params) {
  //dot callback
  if (characterIndex < 5) {
    portENTER_CRITICAL(&_morseMux);
    characterArray[characterIndex] = '.';
    characterIndex++;
    timerCntr = 0;
    timerEnabled = true;
    portEXIT_CRITICAL(&_morseMux);
  }
}

void holdFunction(void *params) {
  //dash callback
  if (characterIndex < 5) {
    portENTER_CRITICAL(&_morseMux);
    characterArray[characterIndex] = '-';
    characterIndex++;
    timerCntr = 0;
    timerEnabled = true;
    portEXIT_CRITICAL(&_morseMux);
  }
}

void setup() {
  Serial.begin(115200);

  //configure gpio interrupt
  attachInterrupt(0, gpioIsr0, CHANGE);

  //configure timer interrupt
  aTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(aTimer, timer_isr, true);
  timerAlarmWrite(aTimer, 1000, true);
  timerAlarmEnable(aTimer);

  //Attach a callback 
  PowerButtonEvent_t myPowerButtonEventPush[1];
  PowerButtonEvent_t myPowerButtonEventHold[1];
  myPowerButtonEventPush[0] = PUSH;
  myPowerButtonEventHold[0] = LONG_PUSH;

  //attach callbacks for dots and dashes
  myNewPowerButton0.attachCallback(myPowerButtonEventPush, 1, pushFunction, NULL);
  myNewPowerButton0.attachCallback(myPowerButtonEventHold, 1, holdFunction, NULL);
  printingHandle = xQueueCreate(5, sizeof(char) * 5);
  xTaskCreate(printingTask, "printing task", 4 * 1024, NULL, 5, NULL);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}