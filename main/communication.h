#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include "control.h"
// Khởi tạo WiFi + MQTT
void initCommunication();

// Vòng lặp xử lý MQTT
void handleCommunication();

// Publish trạng thái ESP32 lên MQTT
void publishEsp32Data(float temp, float hum, int mode, String label,
                      float setTemp, float setHumi, int timeDao, int ngayAp);

#endif
