// --------------------------------------------------------------------
// Proyek UTS Internet of Things: Smart Home Terintegrasi
// VERSI LENGKAP: RTOS + Blynk + Local Web Server + MQTT
// --------------------------------------------------------------------

// 1. KONFIGURASI BLYNK
#define BLYNK_TEMPLATE_ID "TMPL6UAC-zx7N"
#define BLYNK_TEMPLATE_NAME "Smart Home"
#define BLYNK_AUTH_TOKEN "-nviSBxe4-ZNiRebwh7ejj5fMxX722PL"
#define BLYNK_PRINT Serial

// 2. LIBRARY
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <BlynkSimpleEsp32.h>
#include <WebServer.h>
#include <PubSubClient.h>

// 3. KREDENSIAL WIFI & BLYNK
const char* ssid = "Asisten Laboratorium";
const char* password = "2025Labkomp:3";
char auth[] = BLYNK_AUTH_TOKEN;

// MQTT CONFIG
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic_status = "smart_home/status";
const char* mqtt_topic_mode_set = "smart_home/mode/set";
const char* mqtt_topic_led_set = "smart_home/led/set";
const char* mqtt_topic_door_set = "smart_home/door/set";

// 4. DEFINISI PIN
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define PIR_PIN 18
#define LED_PIN 5
#define TRIG_PIN 19
#define ECHO_PIN 23
#define SERVO_PIN 2

// 5. INISIALISASI OBJEK
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);
Servo doorServo;
WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

// 6. VARIABEL GLOBAL (Shared Resources)
float temperature = 0.0, humidity = 0.0, distance = 0.0;
int motionState = LOW;
bool manualMode = false;
int manualLedState = LOW;
int manualServoPos = 0;
unsigned long startTime = 0;

// 7. RTOS HANDLES
SemaphoreHandle_t sensorDataMutex;
TaskHandle_t broadcastTaskHandle;
TaskHandle_t blynkTaskHandle;

// 8. DEKLARASI FUNGSI TASK
void taskBacaSensor(void *pvParameters);
void taskKontrolLampu(void *pvParameters);
void taskKontrolPintu(void *pvParameters);
void taskUpdateLCD(void *pvParameters);
void taskBroadcastData(void *pvParameters);
void taskBlynkRun(void *pvParameters);
void taskWebServer(void *pvParameters);
void taskMqtt(void *pvParameters);

// 9. FUNGSI HELPER
void sendCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.sendHeader("Access-Control-Max-Age", "86400");
}

float bacaJarakUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

// 10. MQTT CALLBACK & RECONNECT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println("========================================");
  Serial.println("📥 MQTT MESSAGE RECEIVED:");
  Serial.printf("   Topic: %s\n", topic);
  Serial.printf("   Payload: %s\n", msg.c_str());
  Serial.println("========================================");

  if (String(topic) == mqtt_topic_mode_set) {
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      manualMode = (msg == "1");
      xSemaphoreGive(sensorDataMutex);
    }
    Serial.printf("✅ Mode changed to: %s\n", manualMode ? "MANUAL" : "AUTO");
    xTaskNotifyGive(broadcastTaskHandle);
  } 
  else if (String(topic) == mqtt_topic_led_set) {
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      manualMode = true;
      manualLedState = (msg == "1") ? HIGH : LOW;
      xSemaphoreGive(sensorDataMutex);
    }
    Serial.printf("✅ LED changed to: %s\n", msg == "1" ? "ON" : "OFF");
    xTaskNotifyGive(broadcastTaskHandle);
  } 
  else if (String(topic) == mqtt_topic_door_set) {
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      manualMode = true;
      manualServoPos = (msg == "1") ? 90 : 0;
      xSemaphoreGive(sensorDataMutex);
    }
    Serial.printf("✅ Door changed to: %s\n", msg == "1" ? "OPEN" : "CLOSE");
    xTaskNotifyGive(broadcastTaskHandle);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("🔄 Menghubungkan ke MQTT broker...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("========================================");
      Serial.println("✅ MQTT CONNECTED!");
      Serial.printf("   Broker: %s:%d\n", mqtt_server, mqtt_port);
      Serial.printf("   Client ID: %s\n", clientId.c_str());
      Serial.println("   Subscribed topics:");
      
      client.subscribe(mqtt_topic_mode_set);
      Serial.printf("   - %s\n", mqtt_topic_mode_set);
      
      client.subscribe(mqtt_topic_led_set);
      Serial.printf("   - %s\n", mqtt_topic_led_set);
      
      client.subscribe(mqtt_topic_door_set);
      Serial.printf("   - %s\n", mqtt_topic_door_set);
      
      Serial.println("========================================");
    } else {
      Serial.print("❌ MQTT connection failed, rc=");
      Serial.print(client.state());
      Serial.println(" | Retry in 5 seconds...");
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
  }
}

// --------------------------------------------------------------------
// WEB SERVER HANDLERS
// --------------------------------------------------------------------
void handleRoot() {
  sendCORSHeaders();
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Smart Home Dashboard</title>";
  html += "<style>";
  html += "*{margin:0;padding:0;box-sizing:border-box;}";
  html += "body{font-family:'Segoe UI',Tahoma,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px;}";
  html += ".container{max-width:500px;margin:0 auto;}";
  html += "h1{color:#fff;text-align:center;font-size:28px;margin-bottom:30px;font-weight:600;}";
  html += ".card{background:#fff;border-radius:20px;padding:25px;margin-bottom:20px;box-shadow:0 10px 30px rgba(0,0,0,0.2);}";
  html += ".card-title{color:#4a5568;font-size:18px;font-weight:600;margin-bottom:20px;text-align:center;}";
  html += ".sensor-grid{display:grid;grid-template-columns:1fr 1fr;gap:15px;}";
  html += ".sensor-item{text-align:center;padding:15px;background:#f7fafc;border-radius:12px;}";
  html += ".sensor-icon{font-size:24px;margin-bottom:8px;}";
  html += ".sensor-label{color:#718096;font-size:12px;margin-bottom:5px;}";
  html += ".sensor-value{color:#2d3748;font-size:20px;font-weight:700;}";
  html += ".mode-btn{width:100%;padding:15px;border:none;border-radius:12px;font-size:16px;font-weight:600;cursor:pointer;margin-bottom:15px;transition:all 0.3s;}";
  html += ".mode-manual{background:#3182ce;color:#fff;}";
  html += ".mode-auto{background:#48bb78;color:#fff;}";
  html += ".mode-auto.active{background:#2f855a;}";
  html += ".mode-status{text-align:center;color:#718096;font-size:14px;margin-bottom:15px;}";
  html += ".control-title{color:#4a5568;font-size:16px;font-weight:600;margin:20px 0 15px 0;}";
  html += ".control-row{display:flex;align-items:center;justify-content:space-between;margin-bottom:15px;}";
  html += ".control-label{color:#4a5568;font-size:15px;font-weight:500;}";
  html += ".toggle-group{display:flex;gap:10px;}";
  html += ".toggle-btn{padding:8px 20px;border:none;border-radius:8px;font-size:14px;font-weight:600;cursor:pointer;transition:all 0.3s;}";
  html += ".btn-open{background:#3182ce;color:#fff;}";
  html += ".btn-close{background:#e53e3e;color:#fff;}";
  html += ".btn-on{background:#48bb78;color:#fff;}";
  html += ".btn-off{background:#718096;color:#fff;}";
  html += ".toggle-btn:hover{opacity:0.8;transform:translateY(-2px);}";
  html += "@media(max-width:600px){.container{padding:10px;}h1{font-size:24px;}}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Smart Home Dashboard</h1>";
  
  // Status Ruangan Card
  html += "<div class='card'>";
  html += "<div class='card-title'>Status Ruangan</div>";
  html += "<div class='sensor-grid'>";
  html += "<div class='sensor-item'><div class='sensor-icon'>🌡️</div><div class='sensor-label'>Suhu</div><div class='sensor-value'>" + String(temperature, 1) + " °C</div></div>";
  html += "<div class='sensor-item'><div class='sensor-icon'>💧</div><div class='sensor-label'>Kelembaban</div><div class='sensor-value'>" + String(humidity, 0) + " %</div></div>";
  html += "<div class='sensor-item'><div class='sensor-icon'>📏</div><div class='sensor-label'>Jarak</div><div class='sensor-value'>" + String(distance, 0) + " cm</div></div>";
  html += "<div class='sensor-item'><div class='sensor-icon'>🔥</div><div class='sensor-label'>Gerakan</div><div class='sensor-value'>" + String(motionState == HIGH ? "⚠️" : "--") + "</div></div>";
  html += "</div></div>";
  
  // Kontrol Sistem Card
  html += "<div class='card'>";
  html += "<div class='card-title'>Kontrol Sistem</div>";
  html += "<div class='mode-section'><div class='mode-status'><strong>Mode Sistem</strong></div>";
  html += "<button class='mode-btn " + String(manualMode ? "mode-manual" : "mode-auto active") + "' onclick=\"fetch('/api/mode?val=" + String(manualMode ? "0" : "1") + "').then(()=>setTimeout(()=>location.reload(),500))\">";
  html += manualMode ? "Beralih ke Mode MANUAL" : "Beralih ke Mode OTOMATIS Aktif";
  html += "</button></div>";
  
  html += "<div style='text-align:center;background:#edf2f7;padding:10px;border-radius:8px;margin:15px 0;'>";
  html += "<span style='color:#4a5568;font-weight:600;'>Mode OTOMATIS Aktif</span></div>";
  
  html += "<div class='control-title'>Kontrol Manual</div>";
  
  // Lampu LED Control
  html += "<div class='control-row'>";
  html += "<span class='control-label'>Lampu (LED)</span>";
  html += "<div class='toggle-group'>";
  int ledState = digitalRead(LED_PIN);
  html += "<button class='toggle-btn " + String(ledState ? "btn-on" : "btn-off") + "' style='min-width:80px;' onclick=\"fetch('/api/led?val=" + String(ledState ? "0" : "1") + "').then(()=>setTimeout(()=>location.reload(),500))\">";
  html += ledState ? "ON" : "OFF";
  html += "</button></div></div>";
  
  // Pintu Servo Control
  html += "<div class='control-row'>";
  html += "<span class='control-label'>Pintu (Servo)</span>";
  html += "<div class='toggle-group'>";
  int doorState = doorServo.read();
  html += "<button class='toggle-btn btn-open' onclick=\"fetch('/api/door?val=1').then(()=>setTimeout(()=>location.reload(),500))\">Buka</button>";
  html += "<button class='toggle-btn btn-close' onclick=\"fetch('/api/door?val=0').then(()=>setTimeout(()=>location.reload(),500))\">Tutup</button>";
  html += "</div></div>";
  
  html += "</div></div>";
  html += "<script>setInterval(()=>location.reload(),5000);</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  sendCORSHeaders();
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"motion\":" + String(motionState) + ",";
  json += "\"led\":" + String(digitalRead(LED_PIN)) + ",";
  json += "\"door\":" + String(doorServo.read()) + ",";
  json += "\"mode\":\"" + String(manualMode ? "manual" : "auto") + "\",";
  json += "\"uptime\":" + String((millis() - startTime) / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void handleMode() {
  sendCORSHeaders();
  if (server.hasArg("val")) {
    int val = server.arg("val").toInt();
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      manualMode = (val == 1);
      xSemaphoreGive(sensorDataMutex);
    }
    xTaskNotifyGive(broadcastTaskHandle);
    server.send(200, "text/plain", "Mode changed");
  } else {
    server.send(400, "text/plain", "Missing parameter");
  }
}

void handleLED() {
  sendCORSHeaders();
  if (server.hasArg("val")) {
    int val = server.arg("val").toInt();
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      manualMode = true;
      manualLedState = (val == 1) ? HIGH : LOW;
      xSemaphoreGive(sensorDataMutex);
    }
    xTaskNotifyGive(broadcastTaskHandle);
    server.send(200, "text/plain", "LED changed");
  } else {
    server.send(400, "text/plain", "Missing parameter");
  }
}

void handleDoor() {
  sendCORSHeaders();
  if (server.hasArg("val")) {
    int val = server.arg("val").toInt();
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      manualMode = true;
      manualServoPos = (val == 1) ? 90 : 0;
      xSemaphoreGive(sensorDataMutex);
    }
    xTaskNotifyGive(broadcastTaskHandle);
    server.send(200, "text/plain", "Door changed");
  } else {
    server.send(400, "text/plain", "Missing parameter");
  }
}

void handleOptions() {
  sendCORSHeaders();
  server.send(204);
}

// --------------------------------------------------------------------
// TASK 1: BACA SENSOR
// --------------------------------------------------------------------
void taskBacaSensor(void *pvParameters) {
  (void) pvParameters;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  for(;;) {
    float tempLocal = dht.readTemperature();
    float humLocal = dht.readHumidity();
    int motionLocal = digitalRead(PIR_PIN);
    float distLocal = bacaJarakUltrasonic();
    
    // Error handling untuk DHT22
    if (isnan(tempLocal) || isnan(humLocal)) {
      Serial.println("⚠️  DHT22 Error: NaN detected, keeping previous values");
      // Jangan update nilai, pakai nilai sebelumnya
    } else {
      if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        temperature = tempLocal;
        humidity = humLocal;
        xSemaphoreGive(sensorDataMutex);
      }
    }
    
    // Update sensor lain (PIR dan Ultrasonic)
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      motionState = motionLocal;
      if (distLocal > 0) distance = distLocal; // Only update if valid
      xSemaphoreGive(sensorDataMutex);
    }
    
    // Log pembacaan sensor (setiap 10 detik sekali)
    static int readCount = 0;
    if (readCount % 5 == 0) {
      Serial.println("📊 Sensor Reading:");
      Serial.printf("   Temp: %.2f°C (raw: %.2f) | Hum: %.2f%% (raw: %.2f)\n", 
                    temperature, tempLocal, humidity, humLocal);
      Serial.printf("   Dist: %.2fcm | Motion: %s\n", 
                    distance, motionState ? "YES" : "NO");
      if (isnan(tempLocal) || isnan(humLocal)) {
        Serial.println("   ⚠️  DHT22 mengembalikan NaN - cek koneksi sensor!");
      }
    }
    readCount++;
    
    vTaskDelayUntil(&xLastWakeTime, 2000 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------------------
// TASK 2: KONTROL LAMPU
// --------------------------------------------------------------------
void taskKontrolLampu(void *pvParameters) {
  (void) pvParameters;
  
  for(;;) {
    int ledState = LOW;
    
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      if (manualMode) {
        ledState = manualLedState;
      } else {
        // Mode otomatis: nyala jika ada gerakan DAN suhu > 30°C
        ledState = (motionState == HIGH && temperature > 30.0) ? HIGH : LOW;
      }
      xSemaphoreGive(sensorDataMutex);
    }
    
    digitalWrite(LED_PIN, ledState);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------------------
// TASK 3: KONTROL PINTU
// --------------------------------------------------------------------
void taskKontrolPintu(void *pvParameters) {
  (void) pvParameters;
  
  for(;;) {
    int targetPos = 0;
    
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      if (manualMode) {
        targetPos = manualServoPos;
      } else {
        // Mode otomatis: buka jika ada gerakan DAN objek < 20cm
        targetPos = (motionState == HIGH && distance > 0 && distance < 20) ? 90 : 0;
      }
      xSemaphoreGive(sensorDataMutex);
    }
    
    doorServo.write(targetPos);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------------------
// TASK 4: UPDATE LCD
// --------------------------------------------------------------------
void taskUpdateLCD(void *pvParameters) {
  (void) pvParameters;
  
  for(;;) {
    float tempLocal, humLocal;
    bool modeLocal;
    
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      tempLocal = temperature;
      humLocal = humidity;
      modeLocal = manualMode;
      xSemaphoreGive(sensorDataMutex);
    }
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(tempLocal, 1);
    lcd.print("C H:");
    lcd.print(humLocal, 0);
    lcd.print("%");
    
    lcd.setCursor(0, 1);
    lcd.print(modeLocal ? "Mode: MANUAL" : "Mode: AUTO");
    
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------------------
// TASK 5: BROADCAST DATA (ke Blynk & MQTT)
// --------------------------------------------------------------------
void taskBroadcastData(void *pvParameters) {
  (void) pvParameters;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  // Tunggu sensor baca data dulu (5 detik pertama)
  Serial.println("Broadcast task: Menunggu sensor membaca data...");
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  
  for(;;) {
    // Tunggu notifikasi ATAU timeout 5 detik (kirim periodik)
    ulTaskNotifyTake(pdTRUE, 5000 / portTICK_PERIOD_MS);
    
    float tempLocal, humLocal, distLocal;
    int motionLocal, ledState, doorPos;
    bool modeLocal;
    
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      tempLocal = temperature;
      humLocal = humidity;
      distLocal = distance;
      motionLocal = motionState;
      modeLocal = manualMode;
      xSemaphoreGive(sensorDataMutex);
    }
    
    ledState = digitalRead(LED_PIN);
    doorPos = doorServo.read();
    
    // Kirim ke Blynk
    if (Blynk.connected()) {
      Blynk.virtualWrite(V0, modeLocal ? 1 : 0);
      Blynk.virtualWrite(V1, ledState);
      Blynk.virtualWrite(V2, tempLocal);
      Blynk.virtualWrite(V3, humLocal);
      Blynk.virtualWrite(V4, doorPos > 45 ? 1 : 0);
      Blynk.virtualWrite(V5, distLocal);
      Blynk.virtualWrite(V6, motionLocal);
    }
    
    // Kirim ke MQTT (kirim walaupun temp/hum NaN, yang penting sensor lain tetap terkirim)
    if (client.connected()) {
      // Handle NaN values - ganti dengan 0 atau nilai default untuk JSON
      float tempToSend = isnan(tempLocal) ? 0.0 : tempLocal;
      float humToSend = isnan(humLocal) ? 0.0 : humLocal;
      
      String payload = "{\"temp\":" + String(tempToSend, 2) + 
                      ",\"hum\":" + String(humToSend, 2) +
                      ",\"dist\":" + String(distLocal, 2) +
                      ",\"motion\":" + String(motionLocal) +
                      ",\"led\":" + String(ledState) +
                      ",\"door\":" + String(doorPos) +
                      ",\"mode\":\"" + String(modeLocal ? "manual" : "auto") + "\"}";
      
      // Publish dengan QoS 1 untuk reliability
      bool published = client.publish(mqtt_topic_status, payload.c_str(), false);
      
      if (published) {
        Serial.println("========================================");
        Serial.println("📤 MQTT DATA PUBLISHED:");
        Serial.printf("   Topic: %s\n", mqtt_topic_status);
        Serial.printf("   Payload: %s\n", payload.c_str());
        Serial.println("   Data breakdown:");
        Serial.printf("   - Temperature: %.2f°C %s\n", tempToSend, isnan(tempLocal) ? "(NaN - sent as 0)" : "");
        Serial.printf("   - Humidity: %.2f%% %s\n", humToSend, isnan(humLocal) ? "(NaN - sent as 0)" : "");
        Serial.printf("   - Distance: %.2f cm\n", distLocal);
        Serial.printf("   - Motion: %s\n", motionLocal ? "DETECTED" : "None");
        Serial.printf("   - LED: %s\n", ledState ? "ON" : "OFF");
        Serial.printf("   - Door: %d° (%s)\n", doorPos, doorPos > 45 ? "OPEN" : "CLOSED");
        Serial.printf("   - Mode: %s\n", modeLocal ? "MANUAL" : "AUTO");
        Serial.println("========================================");
      } else {
        Serial.println("❌ MQTT Publish FAILED!");
      }
    } else {
      Serial.println("⚠️  MQTT not connected, skipping publish...");
    }
    
    vTaskDelayUntil(&xLastWakeTime, 5000 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------------------
// TASK 6: BLYNK RUN
// --------------------------------------------------------------------
void taskBlynkRun(void *pvParameters) {
  (void) pvParameters;
  
  for(;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!Blynk.connected()) {
        Serial.println("Blynk terputus, reconnecting...");
        Blynk.connect();
      } else {
        Blynk.run();
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------------------
// TASK 7: WEB SERVER
// --------------------------------------------------------------------
void taskWebServer(void *pvParameters) {
  (void) pvParameters;
  
  for(;;) {
    server.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------------------
// TASK 8: MQTT
// --------------------------------------------------------------------
void taskMqtt(void *pvParameters) {
  (void) pvParameters;
  
  for(;;) {
    if (!client.connected()) {
      reconnectMQTT();
    }
    client.loop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------------------
// BLYNK EVENT HANDLERS
// --------------------------------------------------------------------
BLYNK_WRITE(V0) {
  int state = param.asInt();
  if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
    manualMode = (state == 1);
    xSemaphoreGive(sensorDataMutex);
  }
  xTaskNotifyGive(broadcastTaskHandle);
  Serial.printf("Blynk V0: Mode = %s\n", manualMode ? "MANUAL" : "AUTO");
}

BLYNK_WRITE(V1) {
  int state = param.asInt();
  if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
    manualMode = true;
    manualLedState = (state == 1) ? HIGH : LOW;
    xSemaphoreGive(sensorDataMutex);
  }
  xTaskNotifyGive(broadcastTaskHandle);
  Serial.printf("Blynk V1: LED = %s\n", state ? "ON" : "OFF");
}

BLYNK_WRITE(V4) {
  int state = param.asInt();
  if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
    manualMode = true;
    manualServoPos = (state == 1) ? 90 : 0;
    xSemaphoreGive(sensorDataMutex);
  }
  xTaskNotifyGive(broadcastTaskHandle);
  Serial.printf("Blynk V4: Door = %s\n", state ? "OPEN" : "CLOSE");
}

// --------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  // Init LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home RTOS");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi...");
  
  // Init Hardware
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);
  
  // Init DHT22 dengan delay lebih lama
  dht.begin();
  Serial.println("Menunggu DHT22 siap...");
  delay(3000); // DHT22 butuh waktu startup minimal 2-3 detik
  
  // Baca sensor pertama kali dengan retry (biar ga NAN)
  Serial.println("Membaca sensor DHT22 pertama kali...");
  for (int i = 0; i < 5; i++) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    
    if (!isnan(temperature) && !isnan(humidity)) {
      Serial.printf("✅ DHT22 OK: T=%.2f°C, H=%.2f%%\n", temperature, humidity);
      break;
    }
    
    Serial.printf("Retry %d/5...\n", i+1);
    delay(2000);
  }
  
  if (isnan(temperature)) temperature = 25.0; // Default value
  if (isnan(humidity)) humidity = 50.0; // Default value
  
  // Connect WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  startTime = millis();
  
  // Update LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  
  // Init Blynk
  Blynk.begin(auth, ssid, password);
  Serial.println("Blynk initialized");
  
  // Init MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  
  // Init Web Server
  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/mode", handleMode);
  server.on("/api/led", handleLED);
  server.on("/api/door", handleDoor);
  server.on("/api/mode", HTTP_OPTIONS, handleOptions);
  server.on("/api/led", HTTP_OPTIONS, handleOptions);
  server.on("/api/door", HTTP_OPTIONS, handleOptions);
  server.begin();
  Serial.println("Web Server started");
  
  // Create Mutex
  sensorDataMutex = xSemaphoreCreateMutex();
  
  // Create RTOS Tasks
  xTaskCreatePinnedToCore(taskBacaSensor, "BacaSensor", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskKontrolLampu, "KontrolLampu", 2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskKontrolPintu, "KontrolPintu", 2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskUpdateLCD, "UpdateLCD", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskBroadcastData, "Broadcast", 4096, NULL, 1, &broadcastTaskHandle, 1);
  xTaskCreatePinnedToCore(taskBlynkRun, "BlynkRun", 4096, NULL, 2, &blynkTaskHandle, 1);
  xTaskCreatePinnedToCore(taskWebServer, "WebServer", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskMqtt, "MQTT", 4096, NULL, 2, NULL, 1);
  
  Serial.println("All tasks created!");
  
  // Trigger broadcast pertama kali
  xTaskNotifyGive(broadcastTaskHandle);
}

// --------------------------------------------------------------------
// LOOP (kosong karena pakai RTOS)
// --------------------------------------------------------------------
void loop() {
  // Kosong, semua dikerjakan oleh tasks
}