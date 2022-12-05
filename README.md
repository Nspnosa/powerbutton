# POWERBUTTON ⚡
> *A button handling library*

This library is an attempt to make handling buttons a bit easier. It integrates button debounce, "button gestures" and callback registration. 

<br>

![](powerbutton.gif)

<br>

## USAGE

There are a few things the user needs to provide for this library to work properly, a pin interrupt callback (one per button used), and a timer interrupt (one for the entire class), 1ms is expected but the user can privide the interrupt time in milliseconds and the library will adjust. The pin configuration is done automatically by the class constructor.

*e.g.*:
    
    PowerButton myNewPowerButton = PowerButton(0, PULL_DOWN);

    void IRAM_ATTR gpioIsrPin0() {
        myNewPowerButton.pinRun();
    }

    void ARDUINO_ISR_ATTR timerIsr(void) {
        PowerButton::timerRun(); //1 ms interrupts
    }

A more descriptive example can be found in `src/main.cpp`.

After that, the user can register the gestures, which can consists of a combination of multiple long and short presses, and desired callbacks to call when the gesture matches and a parameter for the callback if needed.

*e.g.*:

    PowerButtonEvent_t myPowerButtonEvent[2];
    myPowerButtonEvent[0] = PUSH;
    myPowerButtonEvent[1] = LONG_PUSH;
    myNewPowerButton27.attachCallback(myPowerButtonEvent, 2, callback, NULL);

<br>

## Limitations

This library uses a single queue and a task to defer to the callbacks registered by the user. Also, there are some defines that the user might need also configure to his needs like the maximum amount of instances this class allows (in order to use a single timer and a single task we need to keep track of the objects), the size of the queue, task stack depth, among others found in `lib/PowerButton/src/PowerButton.h`.

    #define POWER_BUTTON_QUEUE_TASK_PRIORITY 5
    #define POWER_BUTTON_QUEUE_TASK_STACK 1024 * 4
    #define POWER_BUTTON_QUEUE_TASK_CORE 0
    #define POWER_BUTTON_QUEUE_SIZE 5
    #define POWER_BUTTON_EVENT_MAX_SIZE 5
    #define POWER_BUTTON_MAX_ACTIONS 5
    #define POWER_BUTTON_MAX_CLASS_INSTANCES 10 

## To do:

This library has not been tested in any production environment and still needs a lot of testing. It is provided as is without any guarantees.

The things I will be working on next consists of:

- Adding unit tests.
- Improving documentation.
- Improviing the usage of critical sections.
- Removing buffer copy if possible.
- Adding errors.