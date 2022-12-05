#include <Arduino.h>
#include "PowerButton.h"

PowerButton myNewPowerButton27 = PowerButton(27, NO_PULL, false, 200, 100, 50);
PowerButton myNewPowerButton0 = PowerButton(0, NO_PULL, true, 200, 300, 50);

hw_timer_t *aTimer; 

void IRAM_ATTR gpioIsr27() {
  myNewPowerButton27.pinRun();
}

void IRAM_ATTR gpioIsr0() {
  myNewPowerButton0.pinRun();
}

void ARDUINO_ISR_ATTR timer_isr(void) {
  PowerButton::timerRun(1); //pass the time in milliseconds elapsed since last time
}

void function(void *params) {
  uint8_t *parameter = (uint8_t*) params;
  Serial.printf("Pin %u callback has been fired %u\n", parameter[1], parameter[0]);
  parameter[0] = parameter[0] + 1;
}

void setup() {
  Serial.begin(115200);

  uint8_t *paramSlot27 = (uint8_t *) malloc(4);
  paramSlot27[0] = 0;
  paramSlot27[1] = 27;
  uint8_t *paramSlot0 = (uint8_t *) malloc(4);
  paramSlot0[0] = 0;
  paramSlot0[1] = 0;

  //configure gpio interrupt
  attachInterrupt(27, gpioIsr27, CHANGE);
  attachInterrupt(0, gpioIsr0, CHANGE);

  //configure timer interrupt
  aTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(aTimer, timer_isr, true);
  timerAlarmWrite(aTimer, 1000, true);
  timerAlarmEnable(aTimer);

  //Attach a callback 
  PowerButtonEvent_t myPowerButtonEvent[3];
  myPowerButtonEvent[0] = PUSH;
  myPowerButtonEvent[1] = PUSH;
  myPowerButtonEvent[2] = PUSH;
  myNewPowerButton27.attachCallback(myPowerButtonEvent, 2, function, paramSlot27);
  myNewPowerButton0.attachCallback(myPowerButtonEvent, 3, function, paramSlot0);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}