// Lab 10 – Event-Driven Programming with Push Buttons and a Touch Sensor
// STARTER FILE – follow the lab procedure and add your code under each comment.
#include <Arduino.h>

// -------------------- Pin numbers --------------------
#define BUTTON_A  32 // polled button
#define BUTTON_B  33 // interrupt button
#define TOUCH_PIN 27 // touch sensor wire
#define GREEN_LED 25
#define RED_LED   26
#define BUZZER    13

// -------------------- Global variables --------------------
// The state of button A on the last pass of loop(). It starts released (HIGH).
int lastStateA = HIGH;
int pressCountA = 0;                          // How many presses have been counted.
unsigned long lastChangeA = 0;                // The time (in ms) of the last change that was accepted.
// Changes closer together than 50 ms are bounce, so they are ignored.
const unsigned long DEBOUNCE_TIME = 50; 

// These two variables are changed inside the ISR, so they need the keyword
// for a shared ISR variable (Common Commands: "Shared ISR Variable").
// buttonBFlag: raised (true) by the ISR when button B is pressed.
volatile bool buttonBFlag = false;
// lastInterruptB: the time of the last accepted press (used to debounce).
volatile unsigned long lastInterruptB = 0;
// pressCountB is only used in loop(), so it needs no keyword.
int pressCountB = 0;

// Your threshold from the calibration table: (not touched + touched) ÷ 2. 
const int TOUCH_THRESHOLD = 730; 

// A Callback is a variable that holds a function (no inputs, returns nothing).
typedef void (*Callback)();
Callback buttonACallback;   // what button A does
Callback buttonBCallback;   // what button B does
Callback touchCallback;     // what a touch does

// Changed inside touch_ISR(), so they are volatile.
volatile bool touchFlag = false;        // raised by the ISR on a touch
volatile unsigned long lastTouch = 0;   // time of the last accepted touch

// The state of the alarm (only used outside the ISRs).
bool armed = false;                     // true = system armed (green LED on)
bool alarmOn = false;                   // true = red LED and buzzer on
unsigned long alarmStart = 0;           // the time the alarm started
const unsigned long ALARM_TIME = 3000;  // how long the alarm lasts: 3000 ms

// -------------------- Event handlers and callbacks --------------------
// Event handler for button A – runs once for each real press.
void onButtonAPressed()
{
  pressCountA++;                                // add 1 to the count
  Serial.print("Button A pressed. Count = ");   // print the text...
  Serial.println(pressCountA);                  // ...then the number and a new line
} 

// Event handler for button B – called from loop(), never from the ISR,
// so Serial.print() is allowed here.
void onButtonBPressed()
{
  pressCountB++;                                            // add 1 to the count
  Serial.print("Button B pressed (interrupt). Count = ");   // print the text...
  Serial.println(pressCountB);                              // ...then the number
} 

// alarmOff() – turns the alarm OFF.
// It is used when the alarm times out, when button B silences it,
// and when the system is disarmed.
void alarmOff()
{
  // 1. Remember that the alarm is no longer on:
  //    set the variable alarmOn to false.
  alarmOn = false;

  // 2. Turn the red LED off.
  //    (Common Commands: "Turn an Output On / Off" – use RED_LED and LOW)
  digitalWrite(RED_LED, LOW);

  // 3. Turn the buzzer off in the same way (use BUZZER and LOW).
  digitalWrite(BUZZER, LOW);
} 

// startAlarm() – turns the alarm ON and starts the 3-second timer.
// It is called by onTouchDetected() when the wire is touched while armed.
void startAlarm()
{
  // 1. Remember that the alarm is on: set alarmOn to true.
  alarmOn = true;
  
  // 2. Start the timer: save the time now in the variable alarmStart.
  //    (Common Commands: "Save the Time Now")
  //    The // Timer code in loop() compares millis() with alarmStart
  //    and turns the alarm off after ALARM_TIME (3000 ms).
  alarmStart = millis();

  // 3. Turn the red LED on (use RED_LED and HIGH).
  digitalWrite(RED_LED, HIGH);

  // 4. Turn the buzzer on (use BUZZER and HIGH).
  digitalWrite(BUZZER, HIGH);
} 

// toggleArmed() – the callback for button A.
// Each press switches the system between ARMED and DISARMED.
void toggleArmed()
{
  // 1. Flip the variable armed: true becomes false, false becomes true.
  //    (Common Commands: "Flip a bool")
  armed = !armed;

  // 2. Write an if / else that checks whether armed is true.
  //    If armed is true (the system was just armed):
  //      - turn the green LED on (GREEN_LED, HIGH)
  //      - print  System ARMED  on the Serial Monitor
  if (armed == true)
  {
    digitalWrite(GREEN_LED, HIGH);
    Serial.print("System ARMED");
    Serial.println();
  }
  //    Else (the system was just disarmed):
  //      - turn the green LED off (GREEN_LED, LOW)
  //      - call alarmOff(), so an alarm that is ringing stops
  //      - print  System DISARMED  on the Serial Monitor
  else
  {
    digitalWrite(GREEN_LED, LOW);
    alarmOff();
    Serial.print("System DISARMED");
    Serial.println();
  }
} 

// silenceAlarm() – the callback for button B.
// It stops the alarm but keeps the system armed.
void silenceAlarm()
{
  // 1. Call alarmOff() to turn the red LED and the buzzer off.
  alarmOff();

  // 2. Print  Alarm silenced  on the Serial Monitor.
  Serial.print("Alarm silenced");
  Serial.println();
} 


// onTouchDetected() – the callback for the touch sensor.
// loop() calls it every time touch_ISR() raises touchFlag.
void onTouchDetected()
{
  // 1. Write an if that checks whether armed is true.
  //    The alarm must only start when the system is armed.

  // 2. Inside the if:
  //      - print  TOUCH! Alarm ON  on the Serial Monitor
  //      - call startAlarm()
  //    If the system is not armed, the function does nothing.
  if (armed == true)
  {
    Serial.print("TOUCH! Alarm ON");
    Serial.println();
    startAlarm();
  }
  else
  {
    //Do nothing
  }
} 

// ---------- Interrupt Service Routines (ISRs) ----------
// ISR for button B – runs the instant the pin falls, even if loop() is busy.
// Blank before the name: the keyword that keeps an ISR in fast memory
// (Common Commands: "ISR Function"). Keep an ISR short: no Serial, no delay().
void IRAM_ATTR buttonB_ISR()
{
  // Ignore presses less than 200 ms apart (debounce inside the ISR).
  if (millis() - lastInterruptB > 200)
  {
    lastInterruptB = millis();   // remember when this press happened
    buttonBFlag = true;        // raise the flag for loop(): true or false?
  } 
} 

// ISR for the touch wire. The blank is the same keyword as in buttonB_ISR().
// While a finger stays on the wire this ISR keeps running, so only one
// touch per second is accepted.
void IRAM_ATTR touch_ISR()
{
  if (millis() - lastTouch > 1000)
  {
    lastTouch = millis();   // remember when this touch happened
    touchFlag = true;       // raise the flag for loop()
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Lab 10: Event-Driven Programming");

  // Inputs and outputs
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Interrupts
  // Run buttonB_ISR() when the pin of button B changes (Common Commands:
  // "Attach an Interrupt").
  // 1st blank: the command that turns a pin number into an interrupt number.
  // 2nd blank: the mode. Button B goes HIGH → LOW when pressed:
  //            RISING, FALLING or CHANGE?
  attachInterrupt(digitalPinToInterrupt(BUTTON_B), buttonB_ISR, FALLING);

  // Under // Interrupts: attach touch_ISR() to the touch pin. It runs when the
  // touch value drops below TOUCH_THRESHOLD (Common Commands: "Touch Interrupt").
  touchAttachInterrupt(TOUCH_PIN, touch_ISR, TOUCH_THRESHOLD);

  // Callbacks
  // Under // Callbacks: choose which function each event calls.
  // No brackets here – you store the function, you do not run it.
  buttonACallback = toggleArmed;     // button A arms and disarms
  buttonBCallback = silenceAlarm;    // button B must silence the alarm
  touchCallback = onTouchDetected;   // a touch starts the alarm (if armed)
}

void loop()
{
  // Button A (polling)
  // 1. Read button A now (Common Commands: "Read a Pin").
  int stateA = digitalRead(BUTTON_A);

  // 2. Did the state change, AND has more than DEBOUNCE_TIME passed
  //    since the last change? (Common Commands: "Time Since Start")
  if (stateA != lastStateA && millis() - lastChangeA > DEBOUNCE_TIME)
  {
    // 3. Accept the change: save the time now, and remember the new state.
    lastChangeA = millis();
    lastStateA = stateA;

    // 4. Only a change to "pressed" is a press event.
    //    Is a pressed button HIGH or LOW?
    if (stateA == LOW)
    {
      buttonACallback();   // call the event handler
    }
  }  

  // Button B (interrupt)
  // Flag pattern: did the ISR raise the flag?
  if (buttonBFlag == true)
  {
    buttonBFlag = false;   // lower the flag first, so the press is handled once
    buttonBCallback();    // then call the handler
  } 

  //Touch sensor (interrupt)
  // Touch sensor (interrupt) – the flag pattern again, like button B.
  if (touchFlag == true)
  {
    touchFlag = false;     // lower the flag
    touchCallback();       // call the touch callback ("Call a Callback")
  } 

  // Timer
  // Timer – turn the alarm off when ALARM_TIME has passed since alarmStart.
  // Use the time since start (Common Commands: "Time Passed").
  if (alarmOn == true && millis() - alarmStart >= ALARM_TIME)
  {
    alarmOff();
    Serial.println("Alarm timed out");
  }
}