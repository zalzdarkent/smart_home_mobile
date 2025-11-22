# 🏠 Smart Home IoT Dashboard

Aplikasi Flutter untuk monitoring dan kontrol perangkat IoT Smart Home berbasis ESP32 dengan real-time sensor data dan remote control.

<p align="center">
  <img src="public/evidence/image.png" alt="Smart Home Dashboard" width="300">
</p>

## ✨ Features

### 📊 Real-time Monitoring
- **Temperature Sensor** - Monitor suhu ruangan (°C)
- **Humidity Sensor** - Monitor kelembaban udara (%)
- **Distance Sensor** - Deteksi jarak dengan ultrasonik (cm)
- **Motion Sensor** - Deteksi pergerakan PIR sensor

### 🎛️ Remote Control
- **LED Control** - Nyalakan/matikan lampu dari jarak jauh
- **Smart Door** - Buka/tutup pintu otomatis
- **Mode Switching** - Toggle antara AUTO dan MANUAL mode

### 📱 Device Information
- IP Address & MAC Address
- WiFi SSID & Signal Strength (RSSI)
- Device Uptime & Version Info

### 🔄 Auto-Refresh
- Data sensor update otomatis setiap 2 detik
- Pull-to-refresh untuk manual update
- Real-time status indicator

## 🏗️ Architecture

Project ini menggunakan **Clean Architecture** dengan separation of concerns:

```
lib/
├── controllers/          # Business Logic & State Management
│   └── dashboard_controller.dart
├── services/            # API Layer & Network Calls
│   └── api_service.dart
├── components/          # Reusable UI Components
│   ├── status_header.dart
│   ├── section_title.dart
│   ├── sensor_card.dart
│   ├── mode_control.dart
│   ├── control_card.dart
│   └── device_info_card.dart
├── widgets/            # Complex Widget Compositions
│   ├── error_widget.dart
│   ├── sensor_grid.dart
│   └── control_grid.dart
└── pages/             # Screen Pages (Pure UI)
    └── dashboard.dart
```

### Design Patterns
- ✅ **MVC Pattern** - Model-View-Controller separation
- ✅ **Observer Pattern** - ChangeNotifier untuk state management
- ✅ **Repository Pattern** - ApiService sebagai data layer
- ✅ **Component-Based** - Reusable UI components

## 🚀 Getting Started

### Prerequisites
- Flutter SDK (3.8.1 atau lebih baru)
- Dart SDK
- Node.js (untuk mock server)
- Android Studio / VS Code
- Android device atau emulator

### Installation

1. **Clone repository**
   ```bash
   git clone https://github.com/zalzdarkent/smart_home_mobile.git
   cd smart_home_mobile
   ```

2. **Install dependencies**
   ```bash
   flutter pub get
   npm install  # untuk mock server
   ```

3. **Setup environment variables**
   ```bash
   cp .env.example .env
   ```
   Edit `.env` sesuai konfigurasi API kamu:
   ```env
   BASE_URL=http://localhost:3000
   ```

4. **Run mock server** (di terminal terpisah)
   ```bash
   node server.js
   ```
   Server akan berjalan di `http://localhost:3000`

5. **Run aplikasi Flutter**
   ```bash
   flutter run
   ```

## 🔧 Configuration

### Environment Variables
File `.env` digunakan untuk konfigurasi sensitive data:
- `BASE_URL` - URL endpoint API server

> ⚠️ **Note**: File `.env` tidak di-commit ke Git. Gunakan `.env.example` sebagai template.

### API Endpoints

Mock server menyediakan endpoint:
- `GET /status` - Get sensor data real-time
- `GET /api/info` - Get device information
- `GET /set?led=1` - Control LED (0=OFF, 1=ON)
- `GET /set?door=1` - Control Door (0=CLOSE, 1=OPEN)
- `GET /set?mode=1` - Control Mode (0=AUTO, 1=MANUAL)

## 📦 Dependencies

```yaml
dependencies:
  flutter_dotenv: ^5.1.0     # Environment variable management
  http: ^1.6.0               # HTTP client untuk API calls
  cupertino_icons: ^1.0.8    # iOS style icons
```

## 🎨 UI/UX Features

- **Modern Design** - Clean dan minimalist interface
- **Gradient Cards** - Beautiful card design dengan shadow
- **Color-Coded Status** - Visual indicator untuk setiap sensor
- **Responsive Layout** - Grid layout yang adaptive
- **Smooth Animations** - Transition & interaction yang smooth
- **Error Handling** - User-friendly error messages

## 📱 Screenshots

### Dashboard View

<p align="center">
  <img src="public/evidence/image.png" alt="Dashboard" width="300">
</p>

*Real-time monitoring dengan kontrol panel yang intuitif*

## 🔐 Security

- ✅ Environment variables untuk sensitive data
- ✅ `.env` file excluded dari Git
- ✅ Private getter untuk base URL
- ✅ Error handling yang aman

## 🛠️ Development

### Project Structure
```
smart_home/
├── lib/                    # Source code Flutter
├── server.js              # Mock API server (Node.js)
├── public/evidence/       # Screenshots & assets
├── .env                   # Environment config (gitignored)
├── .env.example          # Template environment
└── README.md             # This file
```

### Best Practices Applied
- Clean Architecture
- Separation of Concerns
- DRY (Don't Repeat Yourself)
- SOLID Principles
- Component-based development
- Environment configuration
- Error boundary handling

## 📝 API Mock Server

Server simulasi untuk testing tanpa hardware ESP32:
- Auto-update sensor data setiap 2 detik
- Simulasi temperature: 25-30°C
- Simulasi humidity: 60-80%
- Simulasi distance: 10-50cm
- Random motion detection
- CORS enabled

## 🤝 Contributing

Contributions are welcome! Silakan buat Pull Request atau Issue.

## 👨‍💻 Author

**Zalz Darkent**
- GitHub: [@zalzdarkent](https://github.com/zalzdarkent)
- Repository: [smart_home_mobile](https://github.com/zalzdarkent/smart_home_mobile)

## 📄 License

This project is for educational purposes (UTS IoT Project).

## 🙏 Acknowledgments

- Flutter Team untuk framework yang amazing
- Material Design untuk design guidelines
- Node.js & Express untuk mock server

---

**Built with ❤️ using Flutter**
