#include "blynk.h"
#include "pinout.h"
#include "lcd_ui.h"
#include "control.h"

// ================== BLYNK CREDENTIALS ==================
#define BLYNK_TEMPLATE_ID   "TMPL6pbDI56zw"
#define BLYNK_TEMPLATE_NAME "app"
#define BLYNK_AUTH_TOKEN    "r2Xju09fRcbcOBXmXseuSoQvg-W6hqWJ"

#include <BlynkSimpleEsp32.h>

void initBlynk() {
  Blynk.config(BLYNK_AUTH_TOKEN); // chỉ config, không gọi WiFi
  Serial.println("Blynk Configured");
}

void handleBlynk() {
  Blynk.run(); // luôn gọi mỗi vòng lặp

  // chỉ gửi dữ liệu lên app theo chu kỳ
  static unsigned long lastSendBlynk = 0;
  if (millis() - lastSendBlynk > 2000) { // mỗi 2 giây
    Blynk.virtualWrite(V0, temp);
    Blynk.virtualWrite(V1, hum);
    Blynk.virtualWrite(V3, (millis() - startTime) / 86400000);
    Blynk.virtualWrite(V4, setTemp);
    Blynk.virtualWrite(V5, setHumi);
    Blynk.virtualWrite(V6, turnInterval / 60000);

    Blynk.virtualWrite(V20, digitalRead(RELAY_HEATER));
    Blynk.virtualWrite(V21, digitalRead(RELAY_FAN));
    Blynk.virtualWrite(V22, digitalRead(RELAY_HUMI));
    Blynk.virtualWrite(V23, digitalRead(RELAY_TURN));

    String modeText = (loaiTrung == 0) ? "TU CHINH" : ("AP " + tenTrung[loaiTrung]);
    Blynk.virtualWrite(V2, modeText);

    lastSendBlynk = millis();
  }
}



// ================== NHẬN DỮ LIỆU TỪ BLYNK APP ==================

BLYNK_WRITE(V4) {  // Set Nhiệt độ
  user_temp = param.asFloat();
  thietLapThongSo();
  drawMainFrame();
}

BLYNK_WRITE(V5) {  // Set Độ ẩm
  user_humi = param.asFloat();
  thietLapThongSo();
  drawMainFrame();
}

BLYNK_WRITE(V6) {  // Chu kỳ đảo trứng (phút)
  int minutes = param.asInt();
  turn_hour = minutes / 60;
  turn_min  = minutes % 60;
  user_hour = turn_hour;
  user_min  = turn_min;
  turnInterval = (unsigned long)minutes * 60000;
  lastTurn = millis();
  thietLapThongSo();
  drawMainFrame();
}

// ================== 5 NÚT CHẾ ĐỘ ==================
BLYNK_WRITE(V10) { loaiTrung = 0; thietLapThongSo(); drawMainFrame(); syncModeButtons(); }
BLYNK_WRITE(V11) { loaiTrung = 1; thietLapThongSo(); drawMainFrame(); syncModeButtons(); }
BLYNK_WRITE(V12) { loaiTrung = 2; thietLapThongSo(); drawMainFrame(); syncModeButtons(); }
BLYNK_WRITE(V13) { loaiTrung = 3; thietLapThongSo(); drawMainFrame(); syncModeButtons(); }
BLYNK_WRITE(V14) { loaiTrung = 4; thietLapThongSo(); drawMainFrame(); syncModeButtons(); }

// ================== ĐỒNG BỘ TRẠNG THÁI NÚT ==================
void syncModeButtons() {
  Blynk.virtualWrite(V10, (loaiTrung == 0) ? 1 : 0);
  Blynk.virtualWrite(V11, (loaiTrung == 1) ? 1 : 0);
  Blynk.virtualWrite(V12, (loaiTrung == 2) ? 1 : 0);
  Blynk.virtualWrite(V13, (loaiTrung == 3) ? 1 : 0);
  Blynk.virtualWrite(V14, (loaiTrung == 4) ? 1 : 0);

  String modeText = (loaiTrung == 0) ? "TU CHINH" : ("AP " + tenTrung[loaiTrung]);
  Blynk.virtualWrite(V2, modeText); // hiển thị tên chế độ trên app
}
