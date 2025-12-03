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
  html += "<title>Smart Home Control</title>";
  html += "<style>body{font-family:Arial;max-width:800px;margin:50px auto;padding:20px;}";
  html += ".card{background:#f0f0f0;padding:20px;margin:10px 0;border-radius:10px;}";
  html += "button{padding:10px 20px;margin:5px;font-size:16px;cursor:pointer;border-radius:5px;}";
  html += ".on{background:#4CAF50;color:white;border:none;}";
  html += ".off{background:#f44336;color:white;border:none;}</style></head><body>";
  html += "<h1>🏠 Smart Home Dashboard</h1>";
  
  html += "<div class='card'><h2>📊 Status Sensor</h2>";
  html += "<p>🌡️ Suhu: " + String(temperature) + " °C</p>";
  html += "<p>💧 Kelembaban: " + String(humidity) + " %</p>";
  html += "<p>📏 Jarak: " + String(distance) + " cm</p>";
  html += "<p>🚶 Gerakan: " + String(motionState == HIGH ? "Terdeteksi" : "Tidak Ada") + "</p></div>";
  
  html += "<div class='card'><h2>🎛️ Kontrol</h2>";
  html += "<p>Mode: <strong>" + String(manualMode ? "MANUAL" : "OTOMATIS") + "</strong></p>";
  html += "<button class='on' onclick=\"fetch('/api/mode?val=1')\">Mode Manual</button>";
  html += "<button class='off' onclick=\"fetch('/api/mode?val=0')\">Mode Otomatis</button><br><br>";
  html += "<button class='on' onclick=\"fetch('/api/led?val=1')\">💡 Lampu ON</button>";
  html += "<button class='off' onclick=\"fetch('/api/led?val=0')\">💡 Lampu OFF</button><br><br>";
  html += "<button class='on' onclick=\"fetch('/api/door?val=1')\">🚪 Buka Pintu</button>";
  html += "<button class='off' onclick=\"fetch('/api/door?val=0')\">🚪 Tutup Pintu</button></div>";
  
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
    
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
      if (!isnan(tempLocal)) temperature = tempLocal;
      if (!isnan(humLocal)) humidity = humLocal;
      motionState = motionLocal;
      distance = distLocal;
      xSemaphoreGive(sensorDataMutex);
    }
    
    // Log pembacaan sensor (setiap 10 detik sekali)
    static int readCount = 0;
    if (readCount % 5 == 0) {
      Serial.println("📊 Sensor Reading:");
      Serial.printf("   Temp: %.2f°C | Hum: %.2f%% | Dist: %.2fcm | Motion: %s\n", 
                    temperature, humidity, distance, motionState ? "YES" : "NO");
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
    
    // Kirim ke MQTT
    if (client.connected()) {
      String payload = "{\"temp\":" + String(tempLocal, 2) + 
                      ",\"hum\":" + String(humLocal, 2) +
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
        Serial.printf("   - Temperature: %.2f°C\n", tempLocal);
        Serial.printf("   - Humidity: %.2f%%\n", humLocal);
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
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);
  
  // Baca sensor pertama kali (biar ga NAN)
  delay(2000); // DHT22 butuh waktu startup
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (isnan(temperature)) temperature = 0.0;
  if (isnan(humidity)) humidity = 0.0;
  
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