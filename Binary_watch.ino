#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>


#define BAT_VD 0
#define LED_ROW_TOP 1
#define LED_ROW_BOTTOM 2
#define LED_A 3
#define LED_B 5
#define LED_C 6
#define LED_D 7
#define LED_E 10
#define SW1 4
#define SW2 20
#define SW3 21

uint16_t TIME_UNTIL_HELD = 500;   // ms
uint16_t TIME_UNTIL_PRESSED = 50;   // ms
uint16_t SLEEP_AFTER = 10e3;   // ms
uint16_t DISPLAY_INTERVAL = 10;   // ms
uint16_t DISPLAY_FADEOUT_DELAY = 1000;   // us
float BATTERY_VOLTAGE_DIVIDER_CONST = 819.0;
uint16_t TIME_OFFSET = 7200;   // s

const char* SSID = "SSID";
const char* PASSWORD = "PASSWORD";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


uint8_t brightness = 255;
bool sleeping = false;
uint32_t last_interaction_millis = 0;



struct TimeDate {
  bool show = true;
  uint8_t index = 0;   // 0-hour 1-minute 2-day 3-month 
};


struct StopWatch {
  bool show = false;
  bool active = false;
  uint32_t value = 0;
  uint32_t startMillis = 0;
};


struct TimeUpdating {
  bool active = false;
  uint32_t activeMillis = 0;
};


struct Button {
  bool down = false;
  bool isHeld = false;
  bool wasPressed = false;
  bool wasHeld = false;
  uint32_t downMillis = 0;
};


struct TimeDate timeDate;
struct StopWatch stopWatch;
struct TimeUpdating timeUpdating;

struct Button button_1;
struct Button button_2;
struct Button button_3;



float getBatteryVoltage() {
  return analogRead(BAT_VD) / BATTERY_VOLTAGE_DIVIDER_CONST;
}



void updateButtons() {
  uint32_t current_millis = millis();
  uint8_t sw1_state = digitalRead(SW1);
  uint8_t sw2_state = digitalRead(SW2);
  uint8_t sw3_state = digitalRead(SW3);
  
  if (sw1_state == 0 && !button_1.down) {
    button_1.down = true;
    button_1.downMillis = current_millis;
  }

  if (sw2_state == 0 && !button_2.down) {
    button_2.down = true;
    button_2.downMillis = current_millis;
  }

  if (sw3_state == 0 && !button_3.down) {
    button_3.down = true;
    button_3.downMillis = current_millis;
  }
  
  button_1.wasHeld = false;
  button_1.wasPressed = false;
  button_2.wasHeld = false;
  button_2.wasPressed = false;
  button_3.wasHeld = false;
  button_3.wasPressed = false;

  if(sw1_state == 1 && button_1.down) {
    button_1.down = false;
    button_1.isHeld = false;
    if (current_millis - button_1.downMillis > TIME_UNTIL_HELD) {
      button_1.wasHeld = true;
      last_interaction_millis = current_millis;
    }
    else if (current_millis - button_1.downMillis > TIME_UNTIL_PRESSED) {
      button_1.wasPressed = true;
      last_interaction_millis = current_millis;
    }
  }

  if(sw2_state == 1 && button_2.down) {
    button_2.down = false;
    button_2.isHeld = false;
    if (current_millis - button_2.downMillis > TIME_UNTIL_HELD) {
      button_2.wasHeld = true;
      last_interaction_millis = current_millis;
    }
    else if (current_millis - button_2.downMillis > TIME_UNTIL_PRESSED) {
      button_2.wasPressed = true;
      last_interaction_millis = current_millis;
    }
  }
  
  if(sw3_state == 1 && button_3.down) {
    button_3.down = false;
    button_3.isHeld = false;
    if (current_millis - button_3.downMillis > TIME_UNTIL_HELD) {
      button_3.wasHeld = true;
      last_interaction_millis = current_millis;
    }
    else if (current_millis - button_3.downMillis > TIME_UNTIL_PRESSED) {
      button_3.wasPressed = true;
      last_interaction_millis = current_millis;
    }
  }
  

  if (current_millis - button_1.downMillis > TIME_UNTIL_HELD && button_1.down) {
    button_1.isHeld = true;
  }

  if (current_millis - button_2.downMillis > TIME_UNTIL_HELD && button_2.down) {
    button_2.isHeld = true;
  }

  if (current_millis - button_3.downMillis > TIME_UNTIL_HELD && button_3.down) {
    button_3.isHeld = true;
  }
}



uint8_t getTimeData(uint8_t index) {
  time_t unixTime = timeClient.getEpochTime() + TIME_OFFSET;
  struct tm* timeInfo = localtime(&unixTime);

  switch (index) {
    case 0:
      return timeInfo->tm_hour;
    
    case 1:
      return timeInfo->tm_min;
    
    case 2:
      return timeInfo->tm_mday;
    
    case 3:
      return timeInfo->tm_mon + 1;
  }
  return 159;
}



bool updateTime() {
  return timeClient.update();
}



void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SSID, PASSWORD);
  }
}



void disconnectFromWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}



void displayValue(uint8_t value = 0, bool mode_led_top = false, bool mode_led_bottom = false, bool update = false) {
  static uint8_t tens = 0;
  static uint8_t remainder = 0;
  static bool led_top = false;
  static bool led_bottom = false;
  

  static bool displaying_tens = true;
  static uint32_t last_update = 0;

  if (update) {
    tens = value / 10;
    remainder = value % 10;
    led_top = mode_led_top;
    led_bottom = mode_led_bottom;
  }

  if (millis() - last_update > DISPLAY_INTERVAL) {
    last_update = millis();
    displaying_tens = !displaying_tens;
    uint8_t num;

    if (displaying_tens) {
      analogWrite(LED_ROW_TOP, brightness);
      analogWrite(LED_ROW_BOTTOM, 0);
      num = tens;
    }
    else {
      analogWrite(LED_ROW_TOP, 0);
      analogWrite(LED_ROW_BOTTOM, brightness);
      num = remainder;
    }
    
    digitalWrite(LED_A, LOW);
    digitalWrite(LED_B, LOW);
    digitalWrite(LED_C, LOW);
    digitalWrite(LED_D, LOW);
    digitalWrite(LED_E, LOW);

    delayMicroseconds(DISPLAY_FADEOUT_DELAY);

    if (led_top && displaying_tens) {
      digitalWrite(LED_E, HIGH);
    }
    if (led_bottom && !displaying_tens) {
      digitalWrite(LED_E, HIGH);
    }

    if (num >= 8) {
      digitalWrite(LED_A, HIGH);
      num -= 8;
    }
    if (num >= 4) {
    digitalWrite(LED_B, HIGH);
      num -= 4;
    }
    if (num >= 2) {
      digitalWrite(LED_C, HIGH);
      num -= 2;
    }
    if (num >= 1) {
      digitalWrite(LED_D, HIGH);
        num -= 1;
    }
  }
}



void displayWipe() {
  analogWrite(LED_ROW_TOP, 0);
  analogWrite(LED_ROW_BOTTOM, 0);
  digitalWrite(LED_A, LOW);
  digitalWrite(LED_B, LOW);
  digitalWrite(LED_C, LOW);
  digitalWrite(LED_D, LOW);
  digitalWrite(LED_E, LOW);
}



void loadingAnimation(bool reset = false, uint16_t interval = 100) {
  static uint32_t last_update = 0;
  static uint8_t step = 0;

  if (reset) {
    step = 0;
    displayWipe();
  }

  if (millis() - last_update > interval) {
    last_update = millis();
    if (step < 7) {
      step++;
    }
    else {
      step = 0;
    }

    switch (step) {
      case 0:
        analogWrite(LED_ROW_TOP, brightness);
        analogWrite(LED_ROW_BOTTOM, 0);
        break;
      
      case 1:
        digitalWrite(LED_A, LOW);
        digitalWrite(LED_B, HIGH);
        break;
      
      case 2:
        digitalWrite(LED_B, LOW);
        digitalWrite(LED_C, HIGH);
        break;
      
      case 3:
        digitalWrite(LED_C, LOW);
        digitalWrite(LED_D, HIGH);
        break;
      
      case 4:
        analogWrite(LED_ROW_TOP, 0);
        analogWrite(LED_ROW_BOTTOM, brightness);
        break;
      
      case 5:
        digitalWrite(LED_D, LOW);
        digitalWrite(LED_C, HIGH);
        break;
      
      case 6:
        digitalWrite(LED_C, LOW);
        digitalWrite(LED_B, HIGH);
        break;
      
      case 7:
        digitalWrite(LED_B, LOW);
        digitalWrite(LED_A, HIGH);
        break;
    }
  }
}



void ledTest1(uint16_t interval = 300) {
  analogWrite(LED_ROW_TOP, 255);
  analogWrite(LED_ROW_BOTTOM, 0);

  digitalWrite(LED_A, HIGH);
  delay(interval);
  digitalWrite(LED_A, LOW);

  digitalWrite(LED_B, HIGH);
  delay(interval);
  digitalWrite(LED_B, LOW);

  digitalWrite(LED_C, HIGH);
  delay(interval);
  digitalWrite(LED_C, LOW);

  digitalWrite(LED_D, HIGH);
  delay(interval);
  digitalWrite(LED_D, LOW);

  digitalWrite(LED_E, HIGH);
  delay(interval);
  digitalWrite(LED_E, LOW);

  analogWrite(LED_ROW_TOP, 0);
  analogWrite(LED_ROW_BOTTOM, 255);

  digitalWrite(LED_A, HIGH);
  delay(interval);
  digitalWrite(LED_A, LOW);

  digitalWrite(LED_B, HIGH);
  delay(interval);
  digitalWrite(LED_B, LOW);

  digitalWrite(LED_C, HIGH);
  delay(interval);
  digitalWrite(LED_C, LOW);

  digitalWrite(LED_D, HIGH);
  delay(interval);
  digitalWrite(LED_D, LOW);

  digitalWrite(LED_E, HIGH);
  delay(interval);
  digitalWrite(LED_E, LOW);

  analogWrite(LED_ROW_BOTTOM, 0);
}



void ledTest2(uint8_t interval_1 = 2, uint8_t interval_2 = 5, bool top_row = true) {  
  digitalWrite(LED_A, HIGH);
  digitalWrite(LED_B, HIGH);
  digitalWrite(LED_C, HIGH);
  digitalWrite(LED_D, HIGH);
  digitalWrite(LED_E, HIGH);
  if (top_row) {
    analogWrite(LED_ROW_BOTTOM, 0);
    for (uint8_t i = 0; i < 255; i++) {
      analogWrite(LED_ROW_TOP, i);
      delay(interval_1);
    }
    for (uint8_t i = 255; i > 0; i--) {
      analogWrite(LED_ROW_TOP, i);
      delay(interval_2);
    }
  }
  else {
    analogWrite(LED_ROW_TOP, 0);
    for (uint8_t i = 0; i < 255; i++) {
      analogWrite(LED_ROW_BOTTOM, i);
      delay(interval_1);
    }
    for (uint8_t i = 255; i > 0; i--) {
      analogWrite(LED_ROW_BOTTOM, i);
      delay(interval_2);
    }
  }
  displayWipe();
}



void failureAnimation(uint8_t interval_1 = 2, uint8_t interval_2 = 8) {
  displayWipe();
  digitalWrite(LED_E, HIGH);
  for (uint8_t i = 0; i < 255; i++) {
      analogWrite(LED_ROW_TOP, i);
      analogWrite(LED_ROW_BOTTOM, i);
      delay(interval_1);
  }
  for (uint8_t i = 255; i > 0; i--) {
    analogWrite(LED_ROW_TOP, i);
    analogWrite(LED_ROW_BOTTOM, i);
    delay(interval_2);
  }
  displayWipe();
}



void successAnimation(uint8_t interval_1 = 2, uint8_t interval_2 = 8) {
  digitalWrite(LED_A, HIGH);
  digitalWrite(LED_B, HIGH);
  digitalWrite(LED_C, HIGH);
  digitalWrite(LED_D, HIGH);
  digitalWrite(LED_E, LOW);
  analogWrite(LED_ROW_BOTTOM, 0);
  for (uint8_t i = 0; i < 255; i++) {
    analogWrite(LED_ROW_TOP, i);
    delay(interval_1);
  }
  for (uint8_t i = 255; i > 0; i--) {
    analogWrite(LED_ROW_TOP, i);
    delay(interval_2);
  }
  displayWipe();
}



void timeClientStart() {
  connectToWiFi();
  while (millis() < 5e3) {
    loadingAnimation();
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.begin();
      timeClient.setUpdateInterval(0);
      timeClient.update();
      break;
    }
  }
  disconnectFromWiFi();
}



void updateUserInterface() {
  uint32_t current_millis = millis();

  if (sleeping) {
    if (current_millis - last_interaction_millis < SLEEP_AFTER) {
      sleeping = false;
    }
  }
  else {
    if (current_millis - last_interaction_millis > SLEEP_AFTER) {
      sleeping = true;
      disconnectFromWiFi();
      displayWipe();
    }
    
    if (button_1.wasPressed) {
      if (button_2.isHeld) {
        if (brightness > 51) {
          brightness -= 51;
        }
        else if (brightness > 10) {
          brightness -= 10;
        }
      }
      else {
        if (timeUpdating.active) {
          timeUpdating.active = false;
          timeDate.show = true;
          disconnectFromWiFi();
        }
        else {
          if (timeDate.show) {
            if (timeDate.index == 0) {
              timeDate.index = 3;
            }
            else {
              timeDate.index--;
            }
          }
          else {
            stopWatch.show = false;
            timeDate.show = true;
            timeUpdating.active = false;
          }
        }
      }
    }

    if (timeUpdating.active) {
      loadingAnimation();
      if (WiFi.status() == WL_CONNECTED || current_millis - timeUpdating.activeMillis > 5e3) {
        if (WiFi.status() == WL_CONNECTED) {
          if (updateTime()) {
            successAnimation();
          }
          else {
            failureAnimation();
          }
        }
        else {
          failureAnimation();
        }
        timeUpdating.active = false;
        timeDate.show = true;
        disconnectFromWiFi();
      }
    }

    if (button_1.wasHeld && !timeUpdating.active && !sleeping) {
      stopWatch.show = false;
      timeDate.show = false;
      timeUpdating.active = true;
      timeUpdating.activeMillis = current_millis;
      connectToWiFi();
      loadingAnimation(true, 100);
    }


    if (button_2.wasPressed) {
      if (timeDate.show) {
        if (timeDate.index == 3) {
          timeDate.index = 0;
        }
        else {
          timeDate.index++;
        }
      }
      else {
        stopWatch.show = false;
        timeDate.show = true;
        timeUpdating.active = false;
      }
    }

    if (timeDate.show && !sleeping) {
      uint8_t timeValue = getTimeData(timeDate.index);
      switch (timeDate.index) {
        case 0:
          displayValue(timeValue, true, false, true);
          break;
        case 1:
          displayValue(timeValue, false, true, true);
          break;
        case 2:
          displayValue(timeValue, true, true, true);
          break;
        case 3:
          displayValue(timeValue, false, false, true);
          break;
      }
    }

    
    if (button_3.wasPressed) {
      if (button_2.isHeld) {
        if (brightness < 51) {
          brightness += 10;
        }
        else if (brightness < 255) {
          brightness += 51;
        }
      }
      else {
        if (stopWatch.show) {
          if (stopWatch.active) {
            stopWatch.active = false;
          }
          else {
            stopWatch.active = true;
            if (stopWatch.value == 0) {
              stopWatch.startMillis = current_millis;
            }
          }
        }
        else {
          stopWatch.show = true;
          timeDate.show = false;
          timeUpdating.active = false;
          if (stopWatch.value == 0) {
            displayValue(0, true, true, true);
          }
        }
      }
    }

    if (button_3.wasHeld) {
      stopWatch.show = true;
      timeDate.show = false;
      timeUpdating.active = false;
      stopWatch.active = false;
      stopWatch.value = 0;
      displayValue(0, true, true, true);
    }

    if (stopWatch.active) {
      stopWatch.value = current_millis - stopWatch.startMillis;
    }

    if (stopWatch.show && !sleeping) {
      if (stopWatch.active) {
        displayValue((stopWatch.value / 1000), false, false, true);
      }
      else {
        displayValue();
      }
    }
  }
}




void setup() {
  Serial.begin(115200);

  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(SW3, INPUT);
  pinMode(BAT_VD, INPUT);
  pinMode(LED_ROW_TOP, OUTPUT);
  pinMode(LED_ROW_BOTTOM, OUTPUT);
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(LED_C, OUTPUT);
  pinMode(LED_D, OUTPUT);
  pinMode(LED_E, OUTPUT);

  timeClientStart();
}



void loop() {
  updateButtons();
  updateUserInterface();
}