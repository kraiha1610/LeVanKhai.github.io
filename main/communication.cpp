#include "communication.h"
#include "lcd_ui.h"
#include "pinout.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ===== WiFi =====
const char* ssid = "GT2";
const char* wifiPassword = "111111111";

// ===== HiveMQ Cloud =====
const char* mqtt_server = "427891f686ab499fa2fef547e99b1e33.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "khaiutc2";
const char* mqtt_pass = "Khai1610";

// ===== MQTT client =====
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ===== MQTT callback =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  Serial.print("MQTT msg ["); Serial.print(topic); Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == "web/data") {
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, msg);
    if (!err) {
      // Nhận setpoint từ web
      if (doc.containsKey("setTemp")) user_temp = doc["setTemp"];
      if (doc.containsKey("setHumi")) user_humi = doc["setHumi"];
      if (doc.containsKey("turn")) {
        int minutes = doc["turn"];
        turn_hour = minutes / 60;
        turn_min  = minutes % 60;
        user_hour = turn_hour;   // đồng bộ với user_hour
        user_min  = turn_min;    // đồng bộ với user_min
        turnInterval = (unsigned long)(turn_hour*3600 + turn_min*60)*1000;
        lastTurn = millis();
      }

      // Nhận mode (số 0–4) và gán vào loaiTrung
      if (doc.containsKey("mode")) {
        loaiTrung = doc["mode"]; // số 0–4
      }

      // Nếu có reset thì khởi động lại thời gian
      if (doc.containsKey("reset") && doc["reset"] == true) {
        startTime = millis();
      }

      // Debug ra Serial để kiểm tra
      Serial.printf("Web gửi: Temp=%.1f, Humi=%.0f, Turn=%d, Mode=%d\n",
                    user_temp, user_humi, doc["turn"].as<int>(), loaiTrung);

      // Cập nhật thông số và LCD
      thietLapThongSo();
      drawMainFrame();
    } else {
      Serial.println("Lỗi parse JSON từ web/data");
    }
  }
}


// ===== Khởi tạo WiFi + MQTT =====
void initCommunication() {
  WiFi.begin(ssid, wifiPassword);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("SSID: "); Serial.println(WiFi.SSID());
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not available, will retry...");
  }

  // Luôn cấu hình MQTT client, không phụ thuộc vào WiFi
  espClient.setInsecure(); // hoặc setCACert nếu có CA
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  client.setKeepAlive(30);
}

// ===== Xử lý WiFi + MQTT trong loop =====
void handleCommunication() {
  static bool wifiWasConnected = false;
  static unsigned long lastWifiAttempt = 0;

  // Kiểm tra WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      Serial.println("WiFi lost, reconnecting...");
      wifiWasConnected = false;
    }
    if (millis() - lastWifiAttempt > 2000) { // thử lại mỗi 2 giây
      WiFi.disconnect();
      WiFi.begin(ssid, wifiPassword);
      lastWifiAttempt = millis();
      Serial.println("Trying to reconnect WiFi...");
    }
    return; // chưa có WiFi thì không xử lý MQTT
  }

  // Nếu vừa mới có WiFi (trước đó chưa có)
  if (!wifiWasConnected) {
    Serial.println("WiFi connected, initializing MQTT...");
    Serial.print("SSID: "); Serial.println(WiFi.SSID());
    Serial.print("IP: "); Serial.println(WiFi.localIP());

    // Re-init MQTT client
    client.disconnect();
    espClient.stop();
    espClient.setInsecure();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);
    client.setKeepAlive(30);

    wifiWasConnected = true;
  }

  // Kiểm tra MQTT
  if (!client.connected()) {
    if (client.connect("IncubatorClient", mqtt_user, mqtt_pass)) {
      Serial.println("MQTT connected");
      client.subscribe("web/data");
    } else {
      Serial.print("MQTT connect failed, rc=");
      Serial.println(client.state());
    }
  }

  client.loop(); // duy trì MQTT
}

// ===== Publish trạng thái ESP32 =====
void publishEsp32Data(float temp, float hum, int mode, String label,
                      float setTemp, float setHumi, int turn, int ngayAp) {
  if (!client.connected()) {
    Serial.println("MQTT not connected, trying to reconnect...");
    if (client.connect("IncubatorClient", mqtt_user, mqtt_pass)) {
      Serial.println("MQTT reconnected before publish");
      client.subscribe("web/data");
    } else {
      Serial.print("MQTT reconnect failed, rc=");
      Serial.println(client.state());
      return;
    }
  }

  DynamicJsonDocument doc(256);
  doc["mode"] = mode;
  doc["temp"] = temp;
  doc["humi"] = hum;
  doc["label"] = label;
  doc["setTemp"] = setTemp;
  doc["setHumi"] = setHumi;
  doc["turn"] = turn;
  doc["ngayAp"] = ngayAp;

  // thêm trạng thái relay
  doc["relayTurn"] = digitalRead(RELAY_TURN); 
  doc["relayFan"]  = digitalRead(RELAY_FAN);
  doc["relayHumi"] = digitalRead(RELAY_HUMI);

  //thêm trạng thái bộ điều khiển
  doc["Kp"] = current_Kp;
  doc["Ki"] = current_Ki;
  doc["Kd"] = current_Kd;
  doc["Output"] = Output;

  String payload;
  serializeJson(doc, payload);
  client.publish("esp32/data", payload.c_str());
}
