#include <Arduino.h>

#define POWER_BUTTON_QUEUE_TASK_PRIORITY 5
#define POWER_BUTTON_QUEUE_TASK_STACK 1024 * 4
#define POWER_BUTTON_QUEUE_TASK_CORE 0
#define POWER_BUTTON_QUEUE_SIZE 5
#define POWER_BUTTON_EVENT_MAX_SIZE 5
#define POWER_BUTTON_MAX_ACTIONS 5
#define POWER_BUTTON_MAX_CLASS_INSTANCES 10 

typedef enum  {
	NO_PULL,
	PULL_UP,
	PULL_DOWN,
} __attribute__ ((__packed__)) PowerButtonPull_t;

typedef enum  {
	PUSH,
	RELSEASE,
	LONG_PUSH,
} __attribute__ ((__packed__)) PowerButtonEvent_t;

typedef enum  {
  PB_ERROR_OK,
  PB_ERROR_MAX_INSTANCES,
  PB_ERROR_MAX_ACTIONS,
  PB_ERROR_EVENT_TOO_LONG,
  PB_ERROR_INVALID_CALLBACK_ID
} __attribute__ ((__packed__)) PowerButtonError_t;

class PowerButton {
  public:
    PowerButton(int pinNumber, PowerButtonPull_t pull = NO_PULL, bool inverted = false, uint16_t longPress = 200, uint16_t timeout = 200, uint16_t debounceTimer = 50);
    ~PowerButton(void);

    int getPinNumber(void);
		uint8_t attachCallback(PowerButtonEvent_t* event, int event_size, void (*callback)(void *), void *params); 
		void removeCallback(int callbackId); 
    void pinRun(void);
    static void timerRun(uint8_t timeIncreaseMs = 1);

    void attachOnPressImmediate(void (*callback)(void));
    void attachOnReleaseImmediate(void (*callback)(void));
    void removeOnPressImmediate(void);
    void removeOnReleaseImmediate(void);
    bool pressed(void);
    void attachOnPressInmediate(void);
    typedef struct {
      PowerButtonEvent_t dataArray[POWER_BUTTON_EVENT_MAX_SIZE];
      uint8_t dataArraySize = 0;
      void (*callback)(void *);
      void *params;
      bool inUse = false;
    } __attribute__ ((__packed__)) PowerButtonAction_t;

  private:
    static int _instances;
    static PowerButton *_powerButtonInstances[POWER_BUTTON_MAX_CLASS_INSTANCES];
    static void _queueHandlerTask(void *params);
    static TaskHandle_t _queueHandlerTaskHandle;
    static QueueHandle_t _queueHandle;

    uint8_t _pinNumber = 0;
		uint8_t _currentEventIndex = 0;
		PowerButtonEvent_t _currenEvent[POWER_BUTTON_EVENT_MAX_SIZE];	
    uint8_t _buttonPrevState = 2;
    uint32_t _buttonPrevTime = 0;
    bool _timerAlreadyRunning = false;
    uint16_t _actionTimerValue = 0;
    PowerButtonAction_t _actionsArray[POWER_BUTTON_MAX_ACTIONS];
    uint8_t _actionsArrayCount = 0;
    uint8_t _classInstanceId;
		uint8_t _currentEventIndexCpy = 0;
		PowerButtonEvent_t _currenEventCpy[POWER_BUTTON_EVENT_MAX_SIZE];	
    portMUX_TYPE _buttonMux = portMUX_INITIALIZER_UNLOCKED;
    uint8_t _releasedState;
    uint16_t _timeout;
    uint32_t _longPressTime;
    bool _debounceRunning = false;
    uint32_t _debounceTimeout;

    void (*_onPressImmediateCb)(void) = NULL;
    void (*_onReleaseImmediateCb)(void) = NULL;
    bool _pressed = false;
};