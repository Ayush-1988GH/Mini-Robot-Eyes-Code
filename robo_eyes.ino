#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUZZER_PIN  8
#define TRIG_PIN    9
#define ECHO_PIN    10

// ─── Notes ────────────────────────────────────────────────────
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_E5 659
#define NOTE_G5 784
#define NOTE_A3 220
#define NOTE_F3 175

// ─── Globals ──────────────────────────────────────────────────
bool wasClose            = false;
int  currentEmotion      = -1;
int  interactionStep     = 0;
bool helloShown          = false;
bool isSleeping          = false;
bool byeShown            = false;

// Goodbye sequence state
// 0 = normal, 1 = angry, 2 = annoyed, 3 = done
int  goodbyeStage        = 0;
unsigned long goodbyeStageStart = 0;

unsigned long lastDistCheck    = 0;
unsigned long lastBlinkTime    = 0;
unsigned long lastActivityTime = 0;
unsigned long lastSeenTime     = 0;

// ─── Buzzer ───────────────────────────────────────────────────
void playNote(int freq, int dur) {
  tone(BUZZER_PIN, freq, dur);
  delay(dur * 1.2);
  noTone(BUZZER_PIN);
}
void softBeep(int freq, int dur) {
  for (int i = 0; i < dur; i += 20) {
    tone(BUZZER_PIN, freq, 10);
    delay(20);
  }
  noTone(BUZZER_PIN);
}

// ─── Melodies ─────────────────────────────────────────────────
void melodyHello() {
  playNote(NOTE_C4, 80); playNote(NOTE_E4, 80);
  playNote(NOTE_G4, 80); playNote(NOTE_C5, 80);
  playNote(NOTE_G4, 80); playNote(NOTE_C5, 200);
}
void melodyBye() {
  playNote(NOTE_C5,150); playNote(NOTE_G4,150);
  playNote(NOTE_E4,150); playNote(NOTE_C4,400);
}
void melodyAngry() {
  playNote(NOTE_F3, 80); playNote(NOTE_F3, 80);
  playNote(NOTE_F3, 80); playNote(NOTE_A3, 200);
}
void melodyAnnoyed() {
  // Long slow groan
  for (int f = NOTE_A4; f >= NOTE_F3; f -= 6) {
    tone(BUZZER_PIN, f, 25); delay(22);
  }
  noTone(BUZZER_PIN);
}
void melodySleepImportant() {
  // Sassy descending tune
  playNote(NOTE_E5, 100); playNote(NOTE_C5, 100);
  playNote(NOTE_G4, 100); playNote(NOTE_E4, 150);
  playNote(NOTE_C4, 300);
}
void melodyHappy()     { playNote(NOTE_C4,100); playNote(NOTE_E4,100); playNote(NOTE_G4,100); playNote(NOTE_C5,200); }
void melodySad()       { playNote(NOTE_E4,200); playNote(NOTE_D4,200); playNote(NOTE_C4,300); playNote(NOTE_A3,500); }
void melodySurprised() { playNote(NOTE_C4, 60); playNote(NOTE_E4, 60); playNote(NOTE_G4, 60); playNote(NOTE_E5,220); }
void melodyPetted()    { softBeep(NOTE_E4,150); softBeep(NOTE_G4,150); softBeep(NOTE_B4,200); }
void melodyCamera()    { playNote(NOTE_G5, 40); playNote(NOTE_E5, 40); playNote(NOTE_C5, 60); playNote(NOTE_G4,120); }
void melodySmile()     { playNote(NOTE_C4, 80); playNote(NOTE_E4, 80); playNote(NOTE_G4, 80); playNote(NOTE_E4, 80); playNote(NOTE_G4, 80); playNote(NOTE_C5,180); }
void melodyWink()      { playNote(NOTE_G4, 60); playNote(NOTE_C5,100); }
void melodySleepy() {
  tone(BUZZER_PIN, NOTE_A4, 100); delay(100);
  for (int f = NOTE_A4; f >= NOTE_C4; f -= 8) {
    tone(BUZZER_PIN, f, 30); delay(28);
  }
  softBeep(NOTE_C4, 400);
  noTone(BUZZER_PIN);
}

// ─── Hello Screen ─────────────────────────────────────────────
void showHello() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(3);
  display.setCursor(14, 8);
  display.print("HELLO");
  display.setTextSize(1);
  display.setCursor(22, 50);
  display.print("Nice to meet you!");
  display.display();
  melodyHello();
  delay(3000);
  display.clearDisplay();
  display.display();
}

// ─── "Sleep is more important" screen ─────────────────────────
void showSleepImportant() {
  melodySleepImportant();
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10, 10);
  display.print("Hmph! Sleep is");
  display.setCursor(10, 26);
  display.print("more important");
  display.setCursor(10, 42);
  display.print("than you! >:(");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
}

// ─── BYE Screen ───────────────────────────────────────────────
void showByeScreen() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(3);
  display.setCursor(22, 8);
  display.print("BYE!");
  display.setTextSize(1);
  display.setCursor(16, 50);
  display.print("Don't wake me up!");
  display.display();
  melodyBye();
  delay(2500);
  display.clearDisplay();
  display.display();
}

// ─── Sleeping (closed eyes + ZZZ) ─────────────────────────────
void showSleeping() {
  isSleeping     = true;
  currentEmotion = 5;
  display.clearDisplay();
  display.fillRoundRect(14, 29, 36, 7, 3, WHITE);
  display.fillRoundRect(78, 29, 36, 7, 3, WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(60, 18); display.print("z");
  display.setCursor(68, 11); display.print("z");
  display.setCursor(76,  4); display.print("Z");
  display.display();
}

// ─── Draw Eyes ────────────────────────────────────────────────
void drawEmotion(int state) {
  display.clearDisplay();
  switch (state) {
    case 1: // Happy
      display.fillCircle(32, 36, 18, WHITE);
      display.fillCircle(96, 36, 18, WHITE);
      display.fillRect(0, 36, 128, 30, BLACK);
      break;
    case 2: // Sad
      display.fillCircle(32, 34, 16, WHITE);
      display.fillCircle(96, 34, 16, WHITE);
      display.fillCircle(34, 36,  6, BLACK);
      display.fillCircle(98, 36,  6, BLACK);
      display.fillTriangle(16,20,48,20,32,28, BLACK);
      display.fillTriangle(80,20,112,20,96,28, BLACK);
      break;
    case 3: // Angry
      display.fillCircle(32, 32, 18, WHITE);
      display.fillCircle(96, 32, 18, WHITE);
      display.fillCircle(35, 34,  7, BLACK);
      display.fillCircle(99, 34,  7, BLACK);
      display.fillTriangle(14,14,44,22,14,22, BLACK);
      display.fillTriangle(84,22,114,14,114,22, BLACK);
      break;
    case 4: // Surprised
      display.fillCircle(32, 32, 22, WHITE);
      display.fillCircle(96, 32, 22, WHITE);
      display.fillCircle(32, 32, 10, BLACK);
      display.fillCircle(96, 32, 10, BLACK);
      display.fillCircle(36, 27,  3, WHITE);
      display.fillCircle(100,27,  3, WHITE);
      break;
    case 5: // Sleepy idle (half closed)
      display.fillCircle(32, 32, 18, WHITE);
      display.fillCircle(96, 32, 18, WHITE);
      display.fillCircle(35, 36,  6, BLACK);
      display.fillCircle(99, 36,  6, BLACK);
      display.fillRect(12, 14, 42, 20, BLACK);
      display.fillRect(76, 14, 42, 20, BLACK);
      break;
    case 7: // Annoyed — half closed + furrowed brows
      display.fillCircle(32, 32, 18, WHITE);
      display.fillCircle(96, 32, 18, WHITE);
      display.fillCircle(35, 34,  6, BLACK);
      display.fillCircle(99, 34,  6, BLACK);
      // Heavy eyelids (more closed than sleepy)
      display.fillRect(12, 14, 42, 22, BLACK);
      display.fillRect(76, 14, 42, 22, BLACK);
      // Annoyed inner brows slanting
      display.fillTriangle(14,16,34,22,14,22, BLACK);
      display.fillTriangle(84,22,114,16,114,22, BLACK);
      break;
    case 6: // Big smile / petted
      display.fillCircle(32, 32, 20, WHITE);
      display.fillCircle(96, 32, 20, WHITE);
      display.fillRect(12, 10, 42, 24, BLACK);
      display.fillRect(76, 10, 42, 24, BLACK);
      display.fillCircle(28, 36,  3, WHITE);
      display.fillCircle(92, 36,  3, WHITE);
      break;
  }
  display.display();
}

// ─── Show Emotion ─────────────────────────────────────────────
void showEmotion(int state) {
  if (state == currentEmotion) return;
  currentEmotion = state;
  drawEmotion(state);
  switch (state) {
    case 1: melodyHappy();     break;
    case 2: melodySad();       break;
    case 3: melodyAngry();     break;
    case 4: melodySurprised(); break;
    case 5: melodySleepy();    break;
    case 6: melodyPetted();    break;
    case 7: melodyAnnoyed();   break;
  }
}

// ─── Blink (skipped while sleeping) ───────────────────────────
void doBlink() {
  if (isSleeping) return;
  display.clearDisplay();
  display.fillRoundRect(14, 29, 36, 7, 3, WHITE);
  display.fillRoundRect(78, 29, 36, 7, 3, WHITE);
  display.display();
  delay(130);
  drawEmotion(currentEmotion);
}

// ─── Gestures ─────────────────────────────────────────────────
void gestureLook() {
  for (int x = 36; x >= 22; x -= 3) {
    display.clearDisplay();
    display.fillCircle(32,32,18,WHITE); display.fillCircle(96,32,18,WHITE);
    display.fillCircle(x,    32,7,BLACK); display.fillCircle(x+64,32,7,BLACK);
    display.display(); delay(40);
  }
  delay(350);
  for (int x = 22; x <= 42; x += 3) {
    display.clearDisplay();
    display.fillCircle(32,32,18,WHITE); display.fillCircle(96,32,18,WHITE);
    display.fillCircle(x,    32,7,BLACK); display.fillCircle(x+64,32,7,BLACK);
    display.display(); delay(40);
  }
  delay(350);
  for (int x = 42; x >= 36; x--) {
    display.clearDisplay();
    display.fillCircle(32,32,18,WHITE); display.fillCircle(96,32,18,WHITE);
    display.fillCircle(x,    32,7,BLACK); display.fillCircle(x+64,32,7,BLACK);
    display.display(); delay(40);
  }
}

void gestureWink() {
  melodyWink();
  for (int r = 18; r >= 2; r -= 2) {
    display.clearDisplay();
    display.fillCircle(32,32,r,WHITE);
    display.fillCircle(96,32,18,WHITE); display.fillCircle(100,32,7,BLACK);
    display.display(); delay(30);
  }
  display.clearDisplay();
  display.fillRoundRect(14,29,36,6,3,WHITE);
  display.fillCircle(96,32,18,WHITE); display.fillCircle(100,32,7,BLACK);
  display.display(); delay(400);
  for (int r = 2; r <= 18; r += 2) {
    display.clearDisplay();
    display.fillCircle(32,32,r,WHITE);
    display.fillCircle(96,32,18,WHITE); display.fillCircle(100,32,7,BLACK);
    display.display(); delay(30);
  }
  drawEmotion(currentEmotion);
}

void gestureHeart() {
  display.clearDisplay();
  display.fillCircle(27,30,7,WHITE); display.fillCircle(37,30,7,WHITE);
  display.fillTriangle(20,33,44,33,32,46,WHITE);
  display.fillCircle(91,30,7,WHITE); display.fillCircle(101,30,7,WHITE);
  display.fillTriangle(84,33,108,33,96,46,WHITE);
  display.display(); delay(900);
  display.clearDisplay();
  display.fillCircle(27,30,9,WHITE); display.fillCircle(37,30,9,WHITE);
  display.fillTriangle(18,34,46,34,32,50,WHITE);
  display.fillCircle(91,30,9,WHITE); display.fillCircle(101,30,9,WHITE);
  display.fillTriangle(82,34,110,34,96,50,WHITE);
  display.display(); delay(300);
  drawEmotion(currentEmotion);
}

void gestureSmile() {
  melodySmile();
  for (int b = 0; b < 3; b++) {
    display.clearDisplay();
    display.fillCircle(32,34,18,WHITE); display.fillCircle(96,34,18,WHITE);
    display.fillRect(0,34,128,32,BLACK);
    display.display(); delay(180);
    display.clearDisplay();
    display.fillCircle(32,36,18,WHITE); display.fillCircle(96,36,18,WHITE);
    display.fillRect(0,36,128,30,BLACK);
    display.display(); delay(180);
  }
  drawEmotion(currentEmotion);
}

void gestureCamera() {
  display.clearDisplay();
  display.drawRect(10,8,108,52,WHITE); display.drawRect(12,10,104,48,WHITE);
  display.drawLine(10,8,22,8,WHITE);   display.drawLine(10,8,10,20,WHITE);
  display.drawLine(106,8,118,8,WHITE); display.drawLine(118,8,118,20,WHITE);
  display.drawLine(10,60,22,60,WHITE); display.drawLine(10,48,10,60,WHITE);
  display.drawLine(106,60,118,60,WHITE);display.drawLine(118,48,118,60,WHITE);
  display.fillCircle(42,34,12,WHITE);  display.fillCircle(86,34,12,WHITE);
  display.fillCircle(45,34,5,BLACK);   display.fillCircle(89,34,5,BLACK);
  display.display(); delay(700);
  display.fillRect(0,0,128,64,WHITE);
  display.display();
  playNote(NOTE_G5,40); delay(100);
  for (int i = 0; i <= 32; i += 4) {
    display.clearDisplay();
    display.fillRect(0,    0,128,    i,WHITE);
    display.fillRect(0,64-i,128,    i,WHITE);
    display.display(); delay(18);
  }
  melodyCamera(); delay(150);
  for (int i = 32; i >= 0; i -= 4) {
    display.clearDisplay();
    display.fillRect(0,    0,128,i,WHITE);
    display.fillRect(0,64-i,128,i,WHITE);
    display.display(); delay(18);
  }
  delay(200);
  drawEmotion(currentEmotion);
}

void gesturePetted() {
  for (int r = 18; r >= 2; r -= 2) {
    display.clearDisplay();
    display.fillCircle(32,32,r,WHITE);
    display.fillCircle(96,32,r,WHITE);
    display.display(); delay(40);
  }
  delay(300);
  melodyPetted();
  for (int r = 2; r <= 20; r += 2) {
    display.clearDisplay();
    display.fillCircle(32,32,r,WHITE);
    display.fillCircle(96,32,r,WHITE);
    display.display(); delay(40);
  }
  currentEmotion = -1;
  showEmotion(6);
}

// ─── Distance (averaged + median) ────────────────────────────
long getDistance() {
  long readings[5];
  int  validCount = 0;
  for (int i = 0; i < 5; i++) {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(4);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long dur = pulseIn(ECHO_PIN, HIGH, 38000);
    if (dur > 0) {
      long cm = dur * 0.034 / 2;
      if (cm > 2 && cm < 400) readings[validCount++] = cm;
    }
    delay(10);
  }
  if (validCount == 0) return 999;
  for (int i = 0; i < validCount - 1; i++)
    for (int j = i + 1; j < validCount; j++)
      if (readings[j] < readings[i]) {
        long tmp = readings[i]; readings[i] = readings[j]; readings[j] = tmp;
      }
  return readings[validCount / 2];
}

// ─── Next Activity ────────────────────────────────────────────
void runNextActivity() {
  switch (interactionStep % 6) {
    case 0: currentEmotion=-1; showEmotion(1); gestureSmile();  break;
    case 1: gesturePetted(); delay(800);                        break;
    case 2: currentEmotion=-1; showEmotion(1); gestureWink();   break;
    case 3: gestureCamera(); currentEmotion=-1; showEmotion(1); gestureHeart(); break;
    case 4: currentEmotion=-1; showEmotion(1); gestureLook();   break;
    case 5: currentEmotion=-1; showEmotion(6); gestureHeart();
            delay(500); currentEmotion=-1; showEmotion(1);      break;
  }
  interactionStep++;
  lastActivityTime = millis();
}

// ─── Run Goodbye Sequence (non-blocking stages) ───────────────
void handleGoodbyeSequence(unsigned long now) {
  switch (goodbyeStage) {

    case 1: // Just started — show angry
      currentEmotion = -1;
      showEmotion(3); // Angry face + melody
      goodbyeStageStart = now;
      goodbyeStage = 2;
      break;

    case 2: // Wait 5 sec angry, then go annoyed
      if (now - goodbyeStageStart > 5000) {
        currentEmotion = -1;
        showEmotion(7); // Annoyed face + melody
        goodbyeStageStart = now;
        goodbyeStage = 3;
      }
      break;

    case 3: // Annoyed for 5 sec with blinking, then sleep text
      // Allow blinking during annoyed stage
      if (now - lastBlinkTime > 3000) {
        lastBlinkTime = now;
        doBlink();
      }
      if (now - goodbyeStageStart > 5000) {
        goodbyeStage = 4;
        goodbyeStageStart = now;
        // "Sleep is more important" screen
        showSleepImportant();
      }
      break;

    case 4: // BYE screen then sleep
      showByeScreen();
      showSleeping();
      byeShown     = true;
      goodbyeStage = 0; // done
      break;
  }
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("OLED not found!"));
    while (true);
  }
  display.clearDisplay();
  display.display();
  delay(300);

  // HELLO once on power on
  showHello();
  helloShown = true;

  // Boot eyes
  currentEmotion = -1; showEmotion(4); delay(600);
  doBlink();            delay(200);
  currentEmotion = -1; showEmotion(1); delay(800);
  currentEmotion = -1; showEmotion(5);

  lastSeenTime     = millis();
  isSleeping       = false;
  byeShown         = false;
  goodbyeStage     = 0;
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── Distance check every 300ms ────────────────────────────
  if (now - lastDistCheck > 300) {
    lastDistCheck = now;
    long dist = getDistance();
    Serial.print("Distance: "); Serial.println(dist);

    if (dist < 20) {
      // ── Someone detected — cancel goodbye, wake up ─────
      lastSeenTime  = now;
      byeShown      = false;
      isSleeping    = false;
      goodbyeStage  = 0; // cancel any goodbye in progress

      if (!wasClose) {
        wasClose         = true;
        interactionStep  = 0;
        lastActivityTime = now;

        currentEmotion = -1; showEmotion(4); delay(700);
        doBlink(); delay(200);
        currentEmotion = -1; showEmotion(1); delay(500);
        gestureLook();
        lastActivityTime = millis();

      } else {
        if (now - lastActivityTime > 5000) {
          runNextActivity();
        }
      }

    } else {
      // ── No one detected ───────────────────────────────
      if (wasClose) {
        wasClose     = false;
        interactionStep = 0;
        lastSeenTime = millis();
        // Start goodbye sequence
        goodbyeStage = 1;
      }
    }
  }

  // ── Run goodbye stages if active ──────────────────────────
  if (goodbyeStage > 0 && !isSleeping) {
    handleGoodbyeSequence(now);
  }

  // ── Auto blink every 5 sec — only when active + not angry ─
  if (!isSleeping && goodbyeStage != 2 && now - lastBlinkTime > 5000) {
    lastBlinkTime = now;
    doBlink();
  }
}
