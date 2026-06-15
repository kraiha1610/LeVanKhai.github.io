#include "pinout.h"
#include "control.h"
#include "lcd_ui.h"

// ===== Định nghĩa biến PID-Fuzzy =====
double Input = 0, Output = 0;
unsigned long windowSize = 1000;
unsigned long windowStartTime = 0;

float base_Kp = 0.38, base_Ki = 0.0006, base_Kd = 5.0;
float current_Kp = 0, current_Ki = 0, current_Kd = 0;
float e_old = 0, I_sum = 0, de_filtered_old = 0;

// ===== Bộ mờ =====
#define min_f(a, b) ((a) < (b) ? (a) : (b))

const float peak_dkp[5] = {-0.10, -0.05, 0.0, 0.05, 0.15}; 
const float peak_dki[5] = {-0.0002, -0.0001, 0.0, 0.0001, 0.0002}; 
const float peak_dkd[5] = {-2.0, -1.0, 0.0, 1.0, 2.0}; 

const int rule_dkp[5][5] = { 
  {0,0,0,1,2}, {0,0,1,2,3}, {1,1,2,3,3}, {2,3,3,4,4}, {2,3,4,4,4}
};
const int rule_dki[5][5] = { 
  {0,0,0,1,2}, {0,0,1,2,3}, {1,1,2,3,3}, {1,2,3,4,4}, {2,3,4,4,4}
};
const int rule_dkd[5][5] = { 
  {4,4,4,3,2}, {4,4,3,2,1}, {3,3,2,1,1}, {2,1,1,0,0}, {2,1,0,0,0}
};

float trimf(float x, float a, float b, float c) {
  if (x == b) return 1.0; 
  if (x <= a || x >= c) return 0.0;
  if (x < b) return (x - a) / (b - a);
  return (c - x) / (c - b);
}

// ===== Fuzzy-PID =====
float compute_Fuzzy_PID(float sp, float pv, float dt) {
  float e = sp - pv;
  float de_raw = (e - e_old) / dt;
  e_old = e;

  float de_filtered = 0.85 * de_filtered_old + 0.15 * de_raw;
  de_filtered_old = de_filtered;

  float e_in = constrain(e, -1.0, 3.0);      
  float de_in = constrain(de_filtered, -0.1, 0.1); 

  float mu_e[5], mu_de[5];
  mu_e[0] = trimf(e_in, -1.0, -0.5, -0.1);
  mu_e[1] = trimf(e_in, -0.3, -0.1, 0.0);
  mu_e[2] = trimf(e_in, -0.1, 0.0, 0.1);
  mu_e[3] = trimf(e_in, 0.0, 0.4, 1.0);
  mu_e[4] = trimf(e_in, 0.6, 1.5, 3.0);

  mu_de[0] = trimf(de_in, -0.1, -0.06, -0.01);
  mu_de[1] = trimf(de_in, -0.03, -0.01, 0.0);
  mu_de[2] = trimf(de_in, -0.01, 0.0, 0.01);
  mu_de[3] = trimf(de_in, 0.0, 0.01, 0.03);
  mu_de[4] = trimf(de_in, 0.02, 0.05, 0.1);

  float dkp=0,dki=0,dkd=0,den=0;
  for(int i=0;i<5;i++){
    for(int j=0;j<5;j++){
      float w = min_f(mu_e[i], mu_de[j]);
      if(w>0){
        dkp += w * peak_dkp[rule_dkp[i][j]];
        dki += w * peak_dki[rule_dki[i][j]];
        dkd += w * peak_dkd[rule_dkd[i][j]];
        den += w;
      }
    }
  }

  current_Kp = base_Kp + (den>0 ? dkp/den : 0);
  current_Ki = base_Ki + (den>0 ? dki/den : 0);
  current_Kd = base_Kd + (den>0 ? dkd/den : 0);

  I_sum += e * dt;
  float out_temp = current_Kp*e + current_Ki*I_sum + current_Kd*de_filtered;
  if((out_temp>1000.0 && e>0)||(out_temp<0.0 && e<0)) I_sum -= e*dt;
  I_sum = constrain(I_sum,-20000,20000);

  float out = current_Kp*e + current_Ki*I_sum + current_Kd*de_filtered;
  return constrain(out*1000.0,0.0,1000.0);
}

// ===== Hàm điều khiển =====
void controlSystem() {
  Sensor.UpdateData();
  Input = Sensor.GetTemperature();
  hum = Sensor.GetRelHumidity();
  if(isnan(Input)) return;

  Output = compute_Fuzzy_PID(setTemp, Input, 1.0);

  Serial.print("T:"); Serial.print(Input);
  Serial.print(" | Set_T:"); Serial.print(setTemp);
  Serial.print(" | H:"); Serial.print(hum);
  Serial.print(" | Set_H:"); Serial.print(setHumi);
  Serial.print(" | Out:"); Serial.print(Output);
  Serial.print(" | Kp:"); Serial.print(current_Kp,3);
  Serial.print(" | Ki:"); Serial.print(current_Ki,4);
  Serial.print(" | Kd:"); Serial.println(current_Kd,2);

  /*lcd.setCursor(0,0);
  lcd.print("T:"); lcd.print(Input,2); lcd.print("C H:"); lcd.print(hum,1); lcd.print("%   ");
  lcd.setCursor(0,1);
  lcd.print("Sp:"); lcd.print(setTemp,2); lcd.print("C Kp:"); lcd.print(current_Kp,2);
  lcd.setCursor(0,2);
  lcd.print("Ki:"); lcd.print(current_Ki,4); lcd.print(" Kd:"); lcd.print(current_Kd,1);
  lcd.setCursor(0,3);
  int power_percent = (int)(Output/10);
  lcd.print("Power: "); lcd.print(power_percent); lcd.print("% ("); lcd.print((int)Output); lcd.print("ms)");*/
}
