#include <Arduino.h>

// Pins für 7-Segment-Anzeige (0–6)
const int SEG[] = {0, 1, 2, 3, 4, 5, 6};

// Ampel 1 auf Binärcode (D8, D7)
#define L1_PIN_A D8
#define L1_PIN_B D7

// Ampel 2 auf Binärcode (D10, D9)
#define L2_PIN_A D10
#define L2_PIN_B D9

// Zeiten in ms
const int RED_TIME    = 5000;
const int GREEN_TIME  = 3000;
const int YELLOW_TIME = 2000;
const int DISPLAY_INTERVAL = 1000;

volatile int countdown = 0;

void setAmpel(uint8_t pinA, uint8_t pinB, uint8_t state) {
  digitalWrite(pinA, bitRead(state, 1));  // höheres Bit
  digitalWrite(pinB, bitRead(state, 0));  // niederes Bit
}

// ---------- Display-Logik ----------
void displayNumber(int number) {
  bool seg[7];
  switch (number) {
    case 0: seg[0]=1; seg[1]=0; seg[2]=0; seg[3]=0; seg[4]=0; seg[5]=0; seg[6]=0; break;
    case 1: seg[0]=1; seg[1]=1; seg[2]=1; seg[3]=0; seg[4]=1; seg[5]=1; seg[6]=0; break;
    case 2: seg[0]=0; seg[1]=1; seg[2]=0; seg[3]=0; seg[4]=0; seg[5]=0; seg[6]=1; break;
    case 3: seg[0]=0; seg[1]=1; seg[2]=0; seg[3]=0; seg[4]=1; seg[5]=0; seg[6]=0; break;
    case 4: seg[0]=0; seg[1]=0; seg[2]=1; seg[3]=0; seg[4]=1; seg[5]=1; seg[6]=0; break;
    default: for (int i = 0; i < 7; i++) seg[i] = 1; break;
  }
  for (int i = 0; i < 7; i++) digitalWrite(SEG[i], seg[i]);
}

// ---------- Task: Ampel 1 ----------
void light1Task(void *pvParameters) {
  while (true) {
    setAmpel(L1_PIN_A, L1_PIN_B, 0b11); // Rot
    countdown = RED_TIME / 1000;
    vTaskDelay(pdMS_TO_TICKS(RED_TIME));

    setAmpel(L1_PIN_A, L1_PIN_B, 0b10); // Grün
    countdown = GREEN_TIME / 1000;
    vTaskDelay(pdMS_TO_TICKS(GREEN_TIME));

    setAmpel(L1_PIN_A, L1_PIN_B, 0b01); // Gelb
    countdown = YELLOW_TIME / 1000;
    vTaskDelay(pdMS_TO_TICKS(YELLOW_TIME));

    setAmpel(L1_PIN_A, L1_PIN_B, 0b00); // AUS (nur kurz)
  }
}

// ---------- Task: Ampel 2 (gegenläufig) ----------
void light2Task(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(RED_TIME));  // Startversatz

  while (true) {
    setAmpel(L2_PIN_A, L2_PIN_B, 0b10); // Grün
    vTaskDelay(pdMS_TO_TICKS(GREEN_TIME));

    setAmpel(L2_PIN_A, L2_PIN_B, 0b01); // Gelb
    vTaskDelay(pdMS_TO_TICKS(YELLOW_TIME));

    setAmpel(L2_PIN_A, L2_PIN_B, 0b11); // Rot
    vTaskDelay(pdMS_TO_TICKS(RED_TIME));
  }
}

// ---------- Task: Anzeige ----------
void displayTask(void *pvParameters) {
  while (true) {
    if (countdown >= 0) {
      Serial.print("Countdown: ");
      Serial.println(countdown);
      countdown--;
    } else {
      Serial.println("Ampel aus");
    }
    vTaskDelay(pdMS_TO_TICKS(DISPLAY_INTERVAL));
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 7; i++) pinMode(SEG[i], OUTPUT);
  pinMode(L1_PIN_A, OUTPUT); pinMode(L1_PIN_B, OUTPUT);
  pinMode(L2_PIN_A, OUTPUT); pinMode(L2_PIN_B, OUTPUT);

  setAmpel(L2_PIN_A, L2_PIN_B, 0b11);  // Ampel 2 startet mit Rot

  xTaskCreatePinnedToCore(light1Task, "Light1", 1000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(light2Task, "Light2", 1000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(displayTask, "Display", 1000, NULL, 1, NULL, 1);
}

void loop() {
  // leer, da alles über Tasks läuft
}


/* Test LED

#define PIN_A D8  // D8
#define PIN_B D7  // D7

void setAmpel(uint8_t state) {
  digitalWrite(PIN_A, bitRead(state, 1));  // höherwertiges Bit
  digitalWrite(PIN_B, bitRead(state, 0));  // niederwertiges Bit
}

void setup() {
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
}

void loop() {
  setAmpel(0b00); // OFF
  delay(1000);

  setAmpel(0b01); // Yellow
  delay(1000);

  setAmpel(0b10); // Green
  delay(1000);

  setAmpel(0b11); // Red
  delay(1000);
}
*/
/* Pretty Display
void printAmpel(const char* label, bool red, bool yellow, bool green) {
  Serial.println();
  Serial.println(label);
  Serial.println("┌─────┐");
  Serial.print  ("│ ");
  Serial.print(red    ? "🔴" : "⚫");
  Serial.println(" │");

  Serial.print  ("│ ");
  Serial.print(yellow ? "🟡" : "⚫");
  Serial.println(" │");

  Serial.print  ("│ ");
  Serial.print(green  ? "🟢" : "⚫");
  Serial.println(" │");
  Serial.println("└─────┘");
}

void displayTask(void *pvParameters) {
  while (true) {
    // Ampelzustände lesen (nur Anzeige, keine Steuerung)
    bool l1_red    = digitalRead(L1_RED);
    bool l1_yellow = digitalRead(L1_YELLOW);
    bool l1_green  = digitalRead(L1_GREEN);

    bool l2_red    = digitalRead(L2_RED);
    bool l2_yellow = digitalRead(L2_YELLOW);
    bool l2_green  = digitalRead(L2_GREEN);

    Serial.println("\n========== Anzeige ==========");

    printAmpel("Ampel 1", l1_red, l1_yellow, l1_green);
    printAmpel("Ampel 2", l2_red, l2_yellow, l2_green);

    if (countdown >= 0) {
      Serial.print("Countdown: ");
      Serial.println(countdown);
      countdown--;
    } else {
      Serial.println("Countdown: ---");
    }

    Serial.println("=============================");
    vTaskDelay(pdMS_TO_TICKS(DISPLAY_INTERVAL));
  }
}
*/