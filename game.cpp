#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TM1637Display.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Constants and Pins
const byte WHITEBUTTON = 2;
const byte REDBUTTON = 3;
const byte GREENBUTTON = 4;
const byte BLUEBUTTON = 5;

const byte REDPIN = 9;
const byte GREENPIN = 10;
const byte BLUEPIN = 11;
const byte BUZZER = 6;

const byte CLK_PIN = 8;
const byte DIO_PIN = 7;

TM1637Display counter = TM1637Display(CLK_PIN, DIO_PIN);

enum GameState { INIT, COUNTDOWN, PLAY, END, DISPLAY_SCORE };
GameState gameState = INIT;

int points = 0;
int highestScore = 0;
int startTime;
int elapsedTime;

int currentLight = -1;
int lastLightChangeTime = 0;
const int lightChangeDelay = 200;

byte whitePrevious, redPrevious, greenPrevious, bluePrevious;

// Task Scheduler
typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

const unsigned short tasksNum = 4;
task tasks[tasksNum];

enum LEDStates { LED_INIT, LED_PLAY };
int TickFct_LED(int state);

enum ButtonStates { BUTTON_INIT, BUTTON_PLAY };
int TickFct_Button(int state);

enum DisplayStates { DISPLAY_INIT, DISPLAY_SHOW };
int TickFct_Display(int state);

enum BuzzerStates { BUZZER_INIT, BUZZER_PLAY };
int TickFct_Buzzer(int state);

void setup() {
  pinMode(WHITEBUTTON, INPUT_PULLUP);
  pinMode(REDBUTTON, INPUT_PULLUP);
  pinMode(GREENBUTTON, INPUT_PULLUP);
  pinMode(BLUEBUTTON, INPUT_PULLUP);

  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  counter.setBrightness(5);
  counter.clear();

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ;
  }
  display.clearDisplay();
  display.display();

  // Task Scheduler Setup
  unsigned long GCD = 50; // Greatest Common Divisor for task periods
  tasks[0] = { LED_INIT, 50, 0, &TickFct_LED };
  tasks[1] = { BUTTON_INIT, 50, 0, &TickFct_Button };
  tasks[2] = { DISPLAY_INIT, 200, 0, &TickFct_Display };
  tasks[3] = { BUZZER_INIT, 50, 0, &TickFct_Buzzer };

  gameState = COUNTDOWN;
  startTime = millis();
}

void loop() {
  elapsedTime = millis() - startTime;

  for (unsigned short i = 0; i < tasksNum; i++) {
    if (millis() - tasks[i].elapsedTime >= tasks[i].period) {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis();
    }
  }
}

int TickFct_LED(int state) {
  switch (state) {
    case LED_INIT:
      if (gameState == PLAY) state = LED_PLAY;
      break;

    case LED_PLAY:
      if (gameState == END) state = LED_INIT;
      if (millis() - lastLightChangeTime >= lightChangeDelay) {
        switch (currentLight) {
          case 0: analogWrite(REDPIN, 100); analogWrite(GREENPIN, 100); analogWrite(BLUEPIN, 100); break; // White
          case 1: analogWrite(REDPIN, 100); analogWrite(GREENPIN, 0); analogWrite(BLUEPIN, 0); break; // Red
          case 2: analogWrite(REDPIN, 0); analogWrite(GREENPIN, 100); analogWrite(BLUEPIN, 0); break; // Green
          case 3: analogWrite(REDPIN, 100); analogWrite(GREENPIN, 0); analogWrite(BLUEPIN, 100); break; // Pink
        }
        lastLightChangeTime = millis();
      }
      break;

    default: state = LED_INIT; break;
  }
  return state;
}

int TickFct_Button(int state) {
  switch (state) {
    case BUTTON_INIT:
      if (gameState == PLAY) state = BUTTON_PLAY;
      break;

    case BUTTON_PLAY:
      if (gameState == END) state = BUTTON_INIT;
      bool whiteCurrent = digitalRead(WHITEBUTTON);
      bool redCurrent = digitalRead(REDBUTTON);
      bool greenCurrent = digitalRead(GREENBUTTON);
      bool blueCurrent = digitalRead(BLUEBUTTON);

      if (whiteCurrent == LOW && whitePrevious == HIGH && currentLight == 0) {
        points++;
        currentLight = random(4);
      } else if (redCurrent == LOW && redPrevious == HIGH && currentLight == 1) {
        points++;
        currentLight = random(4);
      } else if (greenCurrent == LOW && greenPrevious == HIGH && currentLight == 2) {
        points++;
        currentLight = random(4);
      } else if (blueCurrent == LOW && bluePrevious == HIGH && currentLight == 3) {
        points++;
        currentLight = random(4);
      }

      whitePrevious = whiteCurrent;
      redPrevious = redCurrent;
      greenPrevious = greenCurrent;
      bluePrevious = blueCurrent;

      if (elapsedTime > 30000) {
        gameState = END;
        if (points > highestScore) highestScore = points;
      }
      break;

    default: state = BUTTON_INIT; break;
  }
  return state;
}

int TickFct_Display(int state) {
  switch (state) {
    case DISPLAY_INIT:
      if (gameState == DISPLAY_SCORE) state = DISPLAY_SHOW;
      break;

    case DISPLAY_SHOW:
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("Score: ");
      display.print(points);
      display.setCursor(0, 16);
      display.print("High: ");
      display.print(highestScore);
      display.display();
      break;

    default: state = DISPLAY_INIT; break;
  }
  return state;
}

int TickFct_Buzzer(int state) {
  switch (state) {
    case BUZZER_INIT:
      if (gameState == PLAY) state = BUZZER_PLAY;
      break;

    case BUZZER_PLAY:
      if (gameState == END) state = BUZZER_INIT;
      if (points > 0 && points % 5 == 0) {
        tone(BUZZER, 500, 200);
      }
      break;

    default: state = BUZZER_INIT; break;
  }
  return state;
}
