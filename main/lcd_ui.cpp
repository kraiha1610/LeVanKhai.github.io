#include "lcd_ui.h"
#include "pinout.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SHT3x.h>

// ===== LCD & Sensor =====
LiquidCrystal_I2C lcd(0x27, 20, 4);
SHT3x Sensor;

// ===== SYMBOL & BIẾN ĐO ĐẠC =====
byte degreeSymbol[8] = { 0b00111,0b00101,0b00111,0,0,0,0,0 };
float temp = 0, hum = 0;

// ===== BIẾN CÀI ĐẶT GỐC =====
float user_temp = 32, user_humi = 60.0;
int user_hour = 2, user_min = 0;
float setTemp = 32, setHumi = 60.0;
int turn_hour = 2, turn_min = 0;
int temp_hour = 2, temp_min = 0;

// ===== BIẾN CHẾ ĐỘ & THỜI GIAN =====
int screen = 0;
int loaiTrung = 0;
String tenTrung[] = {"CHINH", "GA", "VIT", "CUT", "NGONG"};
unsigned long startTime = 0, lastTurn = 0, turnInterval = 2 * 3600 * 1000;
unsigned long turnDuration = 15000, turnStart = 0, lastUpdateLCD = 0;
bool turning = false;

// ===== DEBOUNCE NÚT =====
const int DEBOUNCE_DELAY = 50; // ms
Button btnSet = {BTN_SET, HIGH, HIGH, 0};
Button btnUp   = {BTN_UP,   HIGH, HIGH, 0};
Button btnDown = {BTN_DOWN, HIGH, HIGH, 0};
Button btnMode   = {BTN_MODE,   HIGH, HIGH, 0};

bool readButton(Button &btn) {
  int reading = digitalRead(btn.pin);
  if (reading != btn.lastReading) {
    btn.lastDebounceTime = millis();
  }
  if ((millis() - btn.lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != btn.state) {
      btn.state = reading;
      if (btn.state == LOW) {
        btn.lastReading = reading;
        return true;
      }
    }
  }
  btn.lastReading = reading;
  return false;
}

// ===== HÀM KHỞI TẠO LCD =====
void initLCD() {
  Wire.begin(21, 22);
  lcd.init(); lcd.backlight();
  lcd.createChar(0, degreeSymbol);

  pinMode(BTN_SET, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(RELAY_TURN, OUTPUT); digitalWrite(RELAY_TURN, LOW);

  lcd.setCursor(6, 0); lcd.print("HE THONG");
  lcd.setCursor(4, 1); lcd.print("MAY AP TRUNG");
  lcd.setCursor(5, 2); lcd.print("PID - FUZZY");
  delay(3000);

  Sensor.Begin();
  startTime = millis();
  lastTurn = millis();
  drawMainFrame();
}

// ===== HÀM HỖ TRỢ =====
void thietLapThongSo() {
  if (loaiTrung == 0) { 
    setTemp = user_temp; 
    setHumi = user_humi;
    turn_hour = user_hour; 
    turn_min  = user_min;
  }
  else if (loaiTrung == 1) { setTemp = 37.5; setHumi = 60; turn_hour = 2; turn_min = 0; }
  else if (loaiTrung == 2) { setTemp = 37.8; setHumi = 65; turn_hour = 3; turn_min = 0; }
  else if (loaiTrung == 3) { setTemp = 37.2; setHumi = 55; turn_hour = 1; turn_min = 0; }
  else if (loaiTrung == 4) { setTemp = 37.9; setHumi = 70; turn_hour = 4; turn_min = 0; }
  
  turnInterval = (unsigned long)(turn_hour * 3600 + turn_min * 60) * 1000;
  lastTurn = millis(); 
}

void drawMainFrame() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("T:        "); 
  lcd.setCursor(10, 0); lcd.print(" H:       ");
  lcd.setCursor(0, 1); lcd.print("MODE: [        ]");
  lcd.setCursor(0, 2); lcd.print("NGAY AP:        ");
  lcd.setCursor(0, 3); lcd.print("DAO SAU:        ");
}

// ===== GIAO DIỆN CHÍNH =====
void mainScreen(unsigned long now) {
  lcd.setCursor(2, 0); lcd.print(temp, 2); lcd.write(0); lcd.print("C");
  lcd.setCursor(13, 0); lcd.print(hum, 2); lcd.print("%");

  lcd.setCursor(7, 1);
  if (loaiTrung == 0) lcd.print("TU CHINH");
  else { lcd.print("AP "); lcd.print(tenTrung[loaiTrung]); }

  int dayIncubate = (now - startTime) / 86400000;
  lcd.setCursor(9, 2); lcd.print(dayIncubate); lcd.print(" ngay   ");

  lcd.setCursor(9, 3);
  if (turning) {
    lcd.print("DANG DAO... ");
  } else {
    long remain = turnInterval - (now - lastTurn);
    if (remain < 0) remain = 0;
    unsigned long totalSec = remain / 1000;
    int h = totalSec / 3600;
    int m = (totalSec % 3600) / 60;
    int s = totalSec % 60;
    if(h<10) lcd.print("0"); lcd.print(h); lcd.print(":");
    if(m<10) lcd.print("0"); lcd.print(m); lcd.print(":");
    if(s<10) lcd.print("0"); lcd.print(s);
  }
}

// ===== MÀN HÌNH CÀI ĐẶT =====
void setTempScreen() {
  lcd.setCursor(0, 0); lcd.print("Cai dat nhiet do");
  lcd.setCursor(0, 2); lcd.print("Set: "); lcd.print(user_temp, 1); lcd.write(0); lcd.print("C  ");
  if (readButton(btnUp)) { 
    user_temp += 0.5; 
    thietLapThongSo(); 
    lastUpdateLCD = 0; 
  }
  if (readButton(btnDown)) { 
    user_temp -= 0.5; 
    thietLapThongSo(); 
    lastUpdateLCD = 0; 
  }
}

void setHumiScreen() {
  lcd.setCursor(0, 0); lcd.print("Cai dat do am");
  lcd.setCursor(0, 2); lcd.print("Set: "); lcd.print(user_humi, 1); lcd.print("%   ");
  if (readButton(btnUp)) { 
    user_humi += 0.5; 
    thietLapThongSo(); 
    lastUpdateLCD = 0; 
  }
  if (readButton(btnDown)) { 
    user_humi -= 0.5; 
    thietLapThongSo(); 
    lastUpdateLCD = 0; 
  }
}

void setTurnScreen() {
  lcd.setCursor(0, 0); lcd.print("Chu ky dao");
  lcd.setCursor(0, 1); lcd.print("Gio : "); lcd.print(temp_hour); lcd.print("   ");
  lcd.setCursor(0, 2); lcd.print("Phut: "); lcd.print(temp_min); lcd.print("   ");
  lcd.setCursor(0, 3); lcd.print("Up/Dn de thay doi");

  if (readButton(btnUp)) {
    temp_min++; 
    if (temp_min >= 60) { temp_min = 0; temp_hour++; }
    user_hour = temp_hour; user_min = temp_min;
    thietLapThongSo(); 
    lastUpdateLCD = 0; 
  }
  if (readButton(btnDown)) {
    temp_min--; 
    if (temp_min < 0) { temp_min = 59; if (temp_hour > 0) temp_hour--; }
    user_hour = temp_hour; user_min = temp_min;
    thietLapThongSo(); 
    lastUpdateLCD = 0; 
  }
}
