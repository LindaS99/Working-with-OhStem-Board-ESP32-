/*
Disclaimer:
The code has not been fully tested, as the necessary USB cable was not available at the time of development. Please verify functionality in your environment.
*/
#include <Arduino.h>
#include "scheduler.h"

// Pins for 7-Segment-Anzeige (0–6)
const int SEG[] = {0, 1, 2, 3, 4, 5, 6};

// Ampel 1 (D8, D7)
#define L1_PIN_A D8
#define L1_PIN_B D7

// Ampel 2 (D10, D9)
#define L2_PIN_A D10
#define L2_PIN_B D9

// Times in ms
const int RED_TIME = 5000;
const int GREEN_TIME = 3000;
const int YELLOW_TIME = 2000;
const int DISPLAY_INTERVAL = 1000;

volatile int countdown = 0;

// Define states for the FSM
typedef enum {
  RED,
  GREEN,
  YELLOW
} TrafficLightState;

// Traffic light FSM structure
typedef struct {
  TrafficLightState currentState;
  uint8_t pinA;
  uint8_t pinB;
  uint32_t timeInState;
} TrafficLightFSM;

// Ampel 1 and Ampel 2 FSM instances
TrafficLightFSM ampel1FSM = {RED, L1_PIN_A, L1_PIN_B, 0};
TrafficLightFSM ampel2FSM = {GREEN, L2_PIN_A, L2_PIN_B, 0};

// Function to set traffic light state
void setAmpel(uint8_t pinA, uint8_t pinB, TrafficLightState state) {
  uint8_t stateBits = 0;
  switch (state) {
    case RED:
      stateBits = 0b11; // Red
      break;
    case GREEN:
      stateBits = 0b10; // Green
      break;
    case YELLOW:
      stateBits = 0b01; // Yellow
      break;
  }
  digitalWrite(pinA, bitRead(stateBits, 1));  // higher bit
  digitalWrite(pinB, bitRead(stateBits, 0)); // lower bit
}

// FSM Task for Ampel 1
void ampel1Task() {
  switch (ampel1FSM.currentState) {
    case RED:
      setAmpel(ampel1FSM.pinA, ampel1FSM.pinB, RED);
      countdown = RED_TIME / 1000;
      ampel1FSM.currentState = GREEN;
      SCH_Add_Task(ampel1Task, RED_TIME, 0); // Schedule next state
      break;

    case GREEN:
      setAmpel(ampel1FSM.pinA, ampel1FSM.pinB, GREEN);
      countdown = GREEN_TIME / 1000;
      ampel1FSM.currentState = YELLOW;
      SCH_Add_Task(ampel1Task, GREEN_TIME, 0); // Schedule next state
      break;

    case YELLOW:
      setAmpel(ampel1FSM.pinA, ampel1FSM.pinB, YELLOW);
      countdown = YELLOW_TIME / 1000;
      ampel1FSM.currentState = RED;
      SCH_Add_Task(ampel1Task, YELLOW_TIME, 0); // Schedule next state
      break;
  }
}

// FSM Task for Ampel 2
void ampel2Task() {
  switch (ampel2FSM.currentState) {
    case RED:
      setAmpel(ampel2FSM.pinA, ampel2FSM.pinB, RED);
      ampel2FSM.currentState = GREEN;
      SCH_Add_Task(ampel2Task, RED_TIME, 0); // Schedule next state
      break;

    case GREEN:
      setAmpel(ampel2FSM.pinA, ampel2FSM.pinB, GREEN);
      ampel2FSM.currentState = YELLOW;
      SCH_Add_Task(ampel2Task, GREEN_TIME, 0); // Schedule next state
      break;

    case YELLOW:
      setAmpel(ampel2FSM.pinA, ampel2FSM.pinB, YELLOW);
      ampel2FSM.currentState = RED;
      SCH_Add_Task(ampel2Task, YELLOW_TIME, 0); // Schedule next state
      break;
  }
}

// Task for countdown display
void displayTask() {
  if (countdown >= 0) {
    Serial.print("Countdown: ");
    Serial.println(countdown);
    countdown--;
  } else {
    Serial.println("Ampel aus");
  }
  SCH_Add_Task(displayTask, DISPLAY_INTERVAL, 0); // Schedule itself
}

void setup() {
  // Serial Communication
  Serial.begin(115200);

  // Pins
  for (int i = 0; i < 7; i++) pinMode(SEG[i], OUTPUT);
  pinMode(L1_PIN_A, OUTPUT); pinMode(L1_PIN_B, OUTPUT);
  pinMode(L2_PIN_A, OUTPUT); pinMode(L2_PIN_B, OUTPUT);

  // Scheduler
  SCH_Init();

  // Add tasks to scheduler
  SCH_Add_Task(ampel1Task, 0, 0);              // Start Ampel 1
  SCH_Add_Task(ampel2Task, RED_TIME, 0);       // Start Ampel 2 with initial delay
  SCH_Add_Task(displayTask, DISPLAY_INTERVAL, 0); // Start display task
}

void loop() {
  SCH_Dispatch_Tasks();
}
