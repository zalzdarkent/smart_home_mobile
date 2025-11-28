# Smart Home Mobile - Migration to MQTT

## Perubahan yang Dilakukan

Aplikasi Flutter smart home telah berhasil diubah dari menggunakan HTTP requests ke MQTT protocol untuk komunikasi real-time dengan ESP32.

### 1. Dependencies yang Ditambahkan
- **mqtt_client: ^10.4.0** - Library untuk MQTT client di Flutter

### 2. File Baru yang Dibuat
- **lib/services/mqtt_service.dart** - Service utama untuk handling MQTT connections dan communications

### 3. File yang Dimodifikasi

#### a. pubspec.yaml
- Menambahkan dependency `mqtt_client: ^10.4.0`

#### b. .env
- Menambahkan konfigurasi MQTT:
  ```
  MQTT_SERVER=broker.emqx.io
  MQTT_PORT=1883
  ```

#### c. lib/controllers/dashboard_controller.dart
- Mengganti `ApiService` dengan `MqttService`
- Mengganti timer polling dengan real-time stream listener
- Implementasi real-time data updates melalui MQTT

#### d. lib/services/api_service.dart
- Mengubah control methods untuk menggunakan MQTT
- Mempertahankan HTTP methods sebagai fallback
- Integrasi dengan `MqttService` untuk kontrol perangkat

### 4. MQTT Topics yang Digunakan

Sesuai dengan konfigurasi ESP32:
- **smart_home/status** - Untuk menerima data sensor real-time
- **smart_home/mode/set** - Untuk mengubah mode manual/auto
- **smart_home/led/set** - Untuk kontrol LED (1=ON, 0=OFF)
- **smart_home/door/set** - Untuk kontrol door/servo (1=OPEN, 0=CLOSE)

### 5. Keuntungan Perubahan ke MQTT

1. **Real-time Updates**: Data sensor diupdate secara real-time tanpa polling
2. **Efisiensi**: Mengurangi network traffic dibanding HTTP polling
3. **Reliability**: MQTT memiliki built-in QoS (Quality of Service)
4. **Scalability**: Mudah untuk menambahkan device atau sensor baru
5. **Battery Efficiency**: Untuk mobile device, mengurangi konsumsi battery

### 6. Cara Penggunaan

1. Pastikan ESP32 terhubung dan running (kode Arduino sudah mendukung MQTT)
2. Install dependencies: `flutter pub get`
3. Jalankan aplikasi: `flutter run`
4. Aplikasi akan otomatis terhubung ke MQTT broker dan menerima data real-time

### 7. Troubleshooting

- **Tidak ada data yang muncul**: Cek koneksi MQTT broker di log
- **Kontrol tidak berfungsi**: Pastikan ESP32 terhubung ke MQTT broker yang sama
- **Connection timeout**: Cek network connectivity dan MQTT broker status

### 8. Struktur Data MQTT

Data yang dikirim dari ESP32 melalui topic `smart_home/status`:
```json
{
  "temperature": 25.5,
  "humidity": 60.2,
  "distance": 15.3,
  "motion": 0,
  "led": 1,
  "door": 0,
  "mode": "manual"
}
```

Command yang dikirim ke ESP32:
- Mode: "1" (manual) atau "0" (auto)
- LED: "1" (on) atau "0" (off)  
- Door: "1" (open) atau "0" (close)