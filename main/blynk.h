#ifndef BLYNK_H
#define BLYNK_H

#include <Arduino.h>

// Khởi tạo và xử lý Blynk
void initBlynk();   // không cần tham số vì WiFi đã quản lý riêng
void handleBlynk();

// Đồng bộ trạng thái 5 nút chế độ trên App
void syncModeButtons();

#endif
