#include "PowerButton.h"

int PowerButton::_instances = 0;
TaskHandle_t PowerButton::_queueHandlerTaskHandle = NULL;
QueueHandle_t PowerButton::_queueHandle = NULL;
PowerButton *PowerButton::_powerButtonInstances[POWER_BUTTON_MAX_CLASS_INSTANCES] = {NULL};

PowerButton::PowerButton(int pinNumber, PowerButtonPull_t pull, bool inverted, uint16_t longPress, uint16_t timeout, uint16_t debounceTimeout) {

  if (pull == NO_PULL) {
      pinMode(pinNumber, INPUT);
  } else if (pull = PULL_DOWN) {
      pinMode(pinNumber, INPUT_PULLDOWN);
  } else if (pull == PULL_UP) {
      pinMode(pinNumber, INPUT_PULLUP);
  }

  _pinNumber = pinNumber;
  _instances++; 

  for (uint8_t i = 0; i < POWER_BUTTON_MAX_CLASS_INSTANCES; i++) {
    if (_powerButtonInstances[i] == NULL) {
      _powerButtonInstances[i] = this;
      _classInstanceId = i;
      break;
    }
  }

  if (_instances == 1) {
    _queueHandle = xQueueCreate(POWER_BUTTON_QUEUE_SIZE, sizeof(PowerButton *));
    xTaskCreatePinnedToCore((TaskFunction_t) _queueHandlerTask, "_queueHandlerTask", POWER_BUTTON_QUEUE_TASK_STACK, NULL, POWER_BUTTON_QUEUE_TASK_PRIORITY, &_queueHandlerTaskHandle, POWER_BUTTON_QUEUE_TASK_CORE);
  }

  _releasedState = inverted ? 1 : 0;
  _timeout = timeout;
  _longPressTime = longPress * 1000;
  _debounceTimeout = debounceTimeout * 1000;
}

void PowerButton::_queueHandlerTask(void *params) {
  PowerButton *aPowerButton;

  for(;;) {
    xQueueReceive(_queueHandle, &aPowerButton, portMAX_DELAY);

    for (uint8_t i = 0; i < POWER_BUTTON_MAX_ACTIONS; i++) {
      if ((aPowerButton->_actionsArray[i].inUse == true) && (aPowerButton->_actionsArray[i].dataArraySize == aPowerButton->_currentEventIndexCpy)) {

        if (memcmp(&aPowerButton->_actionsArray[i].dataArray, &aPowerButton->_currenEventCpy, aPowerButton->_currentEventIndexCpy * sizeof(PowerButtonEvent_t)) == 0) {
          aPowerButton->_actionsArray[i].callback(aPowerButton->_actionsArray[i].params);
        }

      }      
    }
  }
}

void PowerButton::timerRun(uint8_t timeIncreaseMs) {
  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

  for (uint8_t i = 0; i < _instances; i++) {

    if (_powerButtonInstances[i] != NULL) {
      if (_powerButtonInstances[i]->_timerAlreadyRunning) {
        bool sendQueue = false;

        if (_powerButtonInstances[i]->_actionTimerValue >= _powerButtonInstances[i]->_timeout) {
          //TODO: this should probably be shorter
          portENTER_CRITICAL_ISR(&_powerButtonInstances[i]->_buttonMux);
          _powerButtonInstances[i]->_actionTimerValue = 0;
          _powerButtonInstances[i]->_timerAlreadyRunning = false;
          _powerButtonInstances[i]->_currentEventIndexCpy = _powerButtonInstances[i]->_currentEventIndex;
          mempcpy(_powerButtonInstances[i]->_currenEventCpy, _powerButtonInstances[i]->_currenEvent, _powerButtonInstances[i]->_currentEventIndex * sizeof(PowerButtonEvent_t));
          _powerButtonInstances[i]->_currentEventIndex = 0;
          portEXIT_CRITICAL_ISR(&_powerButtonInstances[i]->_buttonMux);
          xQueueSendFromISR(_queueHandle, &_powerButtonInstances[i], &pxHigherPriorityTaskWoken);
          if (pxHigherPriorityTaskWoken) {
              portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
          } 
        } 
        else {
          portENTER_CRITICAL_ISR(&_powerButtonInstances[i]->_buttonMux);
          _powerButtonInstances[i]->_actionTimerValue += timeIncreaseMs;
          portEXIT_CRITICAL_ISR(&_powerButtonInstances[i]->_buttonMux);
        }
      }
    }
  }
}

uint8_t PowerButton::attachCallback(PowerButtonEvent_t* event, int eventSize, void (*callback)(void *), void *params) {
  for (int i = 0; i < POWER_BUTTON_MAX_ACTIONS; i++) {
    if (_actionsArray[i].inUse == false) {
      _actionsArray[i].inUse = true;
      _actionsArray[i].callback = callback;
      _actionsArray[i].params = params;
      _actionsArray[i].dataArraySize = eventSize;
      memcpy(_actionsArray[i].dataArray, event, eventSize * sizeof(PowerButtonEvent_t));
      _actionsArrayCount++;
      return i;
    }
  }
  return 0xff;
}

void PowerButton::removeCallback(int callbackId) {
  _actionsArray[callbackId].inUse = false;
  _actionsArray[callbackId].callback = NULL;
  _actionsArray[callbackId].params = NULL;
  _actionsArray[callbackId].dataArraySize = 0;
  memset(_actionsArray[callbackId].dataArray, 0, POWER_BUTTON_EVENT_MAX_SIZE);
  _actionsArrayCount--;
}

PowerButton::~PowerButton(void) {
  //teardown queue and task if no other instances of the powerbutton are running

  _powerButtonInstances[_classInstanceId] = NULL;

  if (_instances) {
    _instances--; 
    if (_instances == 0) {
      vTaskDelete(_queueHandlerTaskHandle);
      vQueueDelete(_queueHandle);
      _queueHandlerTaskHandle = NULL;
      _queueHandle = NULL;
    }
  }

}

int PowerButton::getPinNumber(void) {
  return _pinNumber;
}

void PowerButton::pinRun(void) {
  uint8_t pinValue = digitalRead(_pinNumber);
  uint32_t currentTime = micros();

  //handle overflows
  if (_buttonPrevTime > currentTime) {
    uint32_t timeDiff = 0xFFFFFFFF - _buttonPrevTime; 
    _buttonPrevTime = 0; 
    currentTime = currentTime + timeDiff;
  }

  if (_buttonPrevState == 2 && pinValue == _releasedState) {
    _pressed = true;
    return; //do nothing if first time we get a released state 
  }

  uint32_t timeDifference = currentTime - _buttonPrevTime;

  //handle debounce
  if (_debounceRunning) {
    if (timeDifference < _debounceTimeout) {
      return;
    }
    _debounceRunning = false;
  }

  if (pinValue != _buttonPrevState) {

    _debounceRunning = true;
    if (pinValue == _releasedState) { 
      _pressed = false;
      if (_currentEventIndex < POWER_BUTTON_EVENT_MAX_SIZE) {
        portENTER_CRITICAL_ISR(&_buttonMux);
        if (timeDifference >= _longPressTime) {
          _currenEvent[_currentEventIndex] = LONG_PUSH;
        } else {
          _currenEvent[_currentEventIndex] = PUSH;
        }
        _currentEventIndex++;
        portEXIT_CRITICAL_ISR(&_buttonMux);
      } 

      portENTER_CRITICAL_ISR(&_buttonMux);
      _actionTimerValue = 0;
      _timerAlreadyRunning = true;
      portEXIT_CRITICAL_ISR(&_buttonMux);
      if (_onReleaseImmediateCb != NULL) {
        _onReleaseImmediateCb();
      }
    }    
    else {
      _pressed = true;
      portENTER_CRITICAL_ISR(&_buttonMux);
      _timerAlreadyRunning = false;
      portEXIT_CRITICAL_ISR(&_buttonMux);
      if (_onPressImmediateCb != NULL) {
        _onPressImmediateCb();
      }
    }
  }
  _buttonPrevState = pinValue;
  _buttonPrevTime = currentTime;
}

void PowerButton::attachOnPressImmediate(void (*callback)(void)) {
  _onPressImmediateCb = callback;
}

void PowerButton::removeOnReleaseImmediate(void) {
  _onPressImmediateCb = NULL;
}

void PowerButton::attachOnReleaseImmediate(void (*callback)(void)) {
  _onReleaseImmediateCb = callback;
}

void PowerButton::removeOnPressImmediate(void) {
  _onReleaseImmediateCb = NULL;
}

bool PowerButton::pressed(void) {
  return _pressed;
}