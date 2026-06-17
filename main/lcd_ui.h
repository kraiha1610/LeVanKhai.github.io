#ifndef LCD_UI_H
#define LCD_UI_H

#include <LiquidCrystal_I2C.h>
#include <SHT3x.h>

// ===== LCD & Sensor =====
extern LiquidCrystal_I2C lcd;
extern SHT3x Sensor;

// ===== Biến đo đạc =====
extern float temp, hum;

// ===== Biến cài đặt gốc =====
extern float user_temp, user_humi;
extern int user_hour, user_min;
extern float setTemp, setHumi;
extern int turn_hour, turn_min;
extern int temp_hour, temp_min;

// ===== Biến chế độ & thời gian =====
extern int screen, loaiTrung;
extern String tenTrung[];
extern unsigned long startTime, lastTurn, turnInterval, turnDuration, turnStart, lastUpdateLCD;
extern bool turning;

// ===== Struct Button =====
struct Button {
  int pin;
  int lastReading;
  int state;
  unsigned long lastDebounceTime;
};

// ===== Các nút (extern để main.ino dùng được) =====
extern Button btnSet;
extern Button btnUp;
extern Button btnDown;
extern Button btnMode;

// ===== Prototype =====
void initLCD();
bool press(int pin);
bool readButton(Button &btn);
void thietLapThongSo();
void drawMainFrame();
void mainScreen(unsigned long now);
void setTempScreen();
void setHumiScreen();
void setTurnScreen();

#endif
