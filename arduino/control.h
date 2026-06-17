#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

// ===== Biến toàn cục PID-Fuzzy =====
extern double Input, Output;
extern unsigned long windowSize;
extern unsigned long windowStartTime;

// ===== Các hệ số PID hiện tại =====
extern float current_Kp, current_Ki, current_Kd;

// ===== Prototype =====
void controlSystem();

#endif
