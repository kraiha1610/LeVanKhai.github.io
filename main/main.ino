#include <Arduino.h>
#include "lcd_ui.h"
#include "pinout.h"
#include "communication.h"
#include "control.h"
#include "blynk.h"
#include <WiFi.h>

// ===== Cấu hình vỉ ẩm =====
const unsigned long cycleTime = 10000; // tổng chu kỳ 10 giây
const unsigned long onTime    = 4000;  //

// ===== Hàm điều khiển vỉ ẩm =====
void controlHumidity(float temp, float setTemp, float setHumi) {
  bool tempStable = (abs(temp - setTemp) <= 0.2);
  bool humLow     = (hum < (setHumi - 3));

  if (tempStable && humLow) {
    unsigned long humCycle = millis() % cycleTime;
    if (humCycle < onTime) {
      digitalWrite(RELAY_HUMI, HIGH);  // bật vỉ ẩm
    } else {
      digitalWrite(RELAY_HUMI, LOW);   // tắt vỉ ẩm
    }
  } else {
    digitalWrite(RELAY_HUMI, LOW);     // ngoài điều kiện thì luôn tắt
  }
}

// ===== Hàm điều khiển quạt hút =====
void controlFan(float temp, float setTemp) {
  if (temp > setTemp + 0.5) {
    digitalWrite(RELAY_FAN, HIGH); // bật quạt hút
  } else if (temp < setTemp + 0.3) {
    digitalWrite(RELAY_FAN, LOW);  // tắt quạt hút
  }
}

// ===== Hàm đảo trứng =====
void controlTurnEgg(unsigned long now) {
  if ((now - lastTurn) >= turnInterval && !turning) {
    turning = true; turnStart = now; digitalWrite(RELAY_TURN, HIGH);
  }
  if (turning && (now - turnStart >= turnDuration)) {
    turning = false; lastTurn = now; digitalWrite(RELAY_TURN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  initLCD();
  initCommunication(); // MQTT

  // ===== Khởi tạo Blynk (không cần WiFi vì đã có trong communication) =====
  initBlynk(); 

  // Relay heater
  pinMode(RELAY_HEATER, OUTPUT);
  digitalWrite(RELAY_HEATER, LOW);

  // Relay vỉ ẩm
  pinMode(RELAY_HUMI, OUTPUT);
  digitalWrite(RELAY_HUMI, LOW);

  // Relay quạt hút
  pinMode(RELAY_FAN, OUTPUT);
  digitalWrite(RELAY_FAN, LOW);

  // Relay đảo trứng
  pinMode(RELAY_TURN, OUTPUT);
  digitalWrite(RELAY_TURN, LOW);

  windowStartTime = millis();
  lcd.backlight();
  Sensor.Begin();
}

void loop() {
  unsigned long now = millis();

  // ===== Blynk =====
  handleBlynk();

  //Quạt xả + vỉ ẩm + đọc cảm biến
  static unsigned long lastFast = 0;
  if (now - lastFast >= 200) {
    Sensor.UpdateData();
    temp = Sensor.GetTemperature();
    hum = Sensor.GetRelHumidity();
    
    controlFan(temp, setTemp);// quạt hút
    controlHumidity(temp, setTemp, setHumi);// vỉ ẩm
    controlTurnEgg(now);// đảo trứng

    lastFast = now;
  } 

  // ===== Nút MODE =====
  if (readButton(btnMode)) {
    if (screen == 0) {
      loaiTrung++; if (loaiTrung > 4) loaiTrung = 0;
      thietLapThongSo();
      drawMainFrame();
      syncModeButtons(); // thêm dòng này để đồng bộ lên app
    } else {
      screen = 0; 
      drawMainFrame();
      syncModeButtons(); // thêm dòng này để đồng bộ lên app
    }
  }

  // ===== Nút SET =====
  if (readButton(btnSet) && loaiTrung == 0) {
    if (screen == 3) {
      user_hour = temp_hour; user_min = temp_min;
      turn_hour = temp_hour; turn_min = temp_min;
      turnInterval = (unsigned long)(turn_hour * 3600 + turn_min * 60) * 1000;
      lastTurn = millis();
    }
    screen++; if (screen > 3) screen = 0;
    lcd.clear();
    if (screen == 0) drawMainFrame();
    if (screen == 3) { temp_hour = turn_hour; temp_min = turn_min; }
  }

  // ===== Xử lý nút Up/Down =====
  if (screen == 1) {
    if (readButton(btnUp)) { user_temp += 0.5; thietLapThongSo(); lastUpdateLCD = 0; }
    if (readButton(btnDown)) { user_temp -= 0.5; thietLapThongSo(); lastUpdateLCD = 0; }
  }
  if (screen == 2) {
    if (readButton(btnUp)) { user_humi += 0.5; thietLapThongSo(); lastUpdateLCD = 0; }
    if (readButton(btnDown)) { user_humi -= 0.5; thietLapThongSo(); lastUpdateLCD = 0; }
  }
  if (screen == 3) {
    if (readButton(btnUp)) {
      temp_min++;
      if (temp_min >= 60) { temp_min = 0; temp_hour++; }
      user_hour = temp_hour; user_min = temp_min;
      thietLapThongSo(); lastUpdateLCD = 0;
    }
    if (readButton(btnDown)) {
      temp_min--;
      if (temp_min < 0) { temp_min = 59; if (temp_hour > 0) temp_hour--; }
      user_hour = temp_hour; user_min = temp_min;
      thietLapThongSo(); lastUpdateLCD = 0;
    }
  }

  // ===== Cập nhật LCD =====
  if (now - lastUpdateLCD >= 1000) {
    if (screen == 0) mainScreen(now);
    else if (screen == 1) setTempScreen();
    else if (screen == 2) setHumiScreen();
    else if (screen == 3) setTurnScreen();
    lastUpdateLCD = now;
  }

  // ===== MQTT =====
  handleCommunication();
  static unsigned long lastSend = 0;
  if (now - lastSend > 2000) {
    int dayIncubate = (now - startTime) / 86400000;
    int timeDaoMin = turn_hour * 60 + turn_min;
    String label = (loaiTrung == 0) ? "TU CHINH" : ("AP " + tenTrung[loaiTrung]);

    publishEsp32Data(temp, hum, loaiTrung, label,
                     setTemp, setHumi, timeDaoMin, dayIncubate);
    lastSend = now;
  }

  // ===== Điều khiển =====
  static unsigned long lastControl = 0;
  if (now - lastControl >= 1000) {
    controlSystem(); // PID fuzzy cho nhiệt độ
    lastControl = now;
  }

  if (now - windowStartTime > windowSize) windowStartTime += windowSize;

  if (Output > (now - windowStartTime)) {
    digitalWrite(RELAY_HEATER, HIGH); // bật gia nhiệt
  } else {
    digitalWrite(RELAY_HEATER, LOW);  // tắt gia nhiệt
  }
}
