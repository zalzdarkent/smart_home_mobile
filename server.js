// ============================================================
// MOCK SERVER - ESP32 Smart Home API (untuk testing Postman)
// ============================================================

const express = require('express');
const cors = require('cors');
const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(express.json());

// ============================================================
// STATE DATA (simulasi sensor IoT)
// ============================================================
let deviceState = {
  manualMode: false,
  temperature: 26.5,
  humidity: 65.0,
  distance: 15,
  motion: 0,
  led: 0,
  door: 0
};

let deviceInfo = {
  name: "Smart Home ESP32 (Mock)",
  version: "1.0.0",
  ip: "192.168.1.100",
  mac: "AA:BB:CC:DD:EE:FF",
  ssid: "Asisten Laboratorium",
  rssi: -45,
  startTime: Date.now()
};

// ============================================================
// SIMULASI SENSOR (update otomatis setiap 2 detik)
// ============================================================
setInterval(() => {
  // Simulasi perubahan suhu (25-30°C)
  deviceState.temperature = 25 + Math.random() * 5;
  
  // Simulasi kelembaban (60-80%)
  deviceState.humidity = 60 + Math.random() * 20;
  
  // Simulasi jarak ultrasonik (10-50cm)
  deviceState.distance = Math.floor(10 + Math.random() * 40);
  
  // Simulasi motion sensor (random deteksi)
  if (Math.random() > 0.7) {
    deviceState.motion = 1;
  } else {
    deviceState.motion = 0;
  }
  
  // Update RSSI (kualitas WiFi)
  deviceInfo.rssi = -40 - Math.floor(Math.random() * 20);
  
  console.log('📊 Sensor updated:', {
    temp: deviceState.temperature.toFixed(1),
    hum: deviceState.humidity.toFixed(1),
    dist: deviceState.distance,
    motion: deviceState.motion
  });
}, 2000);

// ============================================================
// API ENDPOINTS
// ============================================================

// Root endpoint
app.get('/', (req, res) => {
  res.send(`
    <html>
      <head>
        <title>Mock ESP32 API</title>
        <style>
          body { font-family: Arial; padding: 20px; }
          .endpoint { background: #f0f0f0; padding: 10px; margin: 10px 0; border-radius: 5px; }
          code { background: #e0e0e0; padding: 2px 6px; border-radius: 3px; }
        </style>
      </head>
      <body>
        <h1>🏠 Mock ESP32 Smart Home API</h1>
        <p>Server berjalan untuk simulasi testing tanpa hardware IoT</p>
        
        <h2>Available Endpoints:</h2>
        
        <div class="endpoint">
          <strong>GET /status</strong><br>
          Mendapatkan status sensor dan device realtime
        </div>
        
        <div class="endpoint">
          <strong>GET /set?mode=1</strong><br>
          Mengubah mode (0=AUTO, 1=MANUAL)
        </div>
        
        <div class="endpoint">
          <strong>GET /set?led=1</strong><br>
          Kontrol lampu (0=OFF, 1=ON)
        </div>
        
        <div class="endpoint">
          <strong>GET /set?door=1</strong><br>
          Kontrol pintu (0=TUTUP, 1=BUKA)
        </div>
        
        <div class="endpoint">
          <strong>GET /api/info</strong><br>
          Mendapatkan informasi device (IP, MAC, uptime, dll)
        </div>
        
        <h3>Test di Postman:</h3>
        <code>http://localhost:${PORT}/status</code>
      </body>
    </html>
  `);
});

// GET /status - Data sensor realtime
app.get('/status', (req, res) => {
  console.log('✅ GET /status');
  res.json({
    success: true,
    data: {
      manualMode: deviceState.manualMode,
      temperature: parseFloat(deviceState.temperature.toFixed(1)),
      humidity: parseFloat(deviceState.humidity.toFixed(1)),
      distance: deviceState.distance,
      motion: deviceState.motion,
      led: deviceState.led,
      door: deviceState.door
    },
    timestamp: Date.now()
  });
});

// OPTIONS /status - CORS preflight
app.options('/status', (req, res) => {
  res.status(204).send();
});

// GET /set - Kontrol device
app.get('/set', (req, res) => {
  const { mode, led, door } = req.query;
  let message = '';
  let changed = false;

  if (mode !== undefined) {
    deviceState.manualMode = mode === '1';
    message = `Mode changed to ${deviceState.manualMode ? 'MANUAL' : 'AUTO'}`;
    changed = true;
    console.log(`🔧 Mode: ${deviceState.manualMode ? 'MANUAL' : 'AUTO'}`);
  }

  if (led !== undefined) {
    deviceState.manualMode = true;
    deviceState.led = led === '1' ? 1 : 0;
    message = `LED turned ${deviceState.led === 1 ? 'ON' : 'OFF'}`;
    changed = true;
    console.log(`💡 LED: ${deviceState.led === 1 ? 'ON' : 'OFF'}`);
  }

  if (door !== undefined) {
    deviceState.manualMode = true;
    deviceState.door = door === '1' ? 1 : 0;
    message = `Door ${deviceState.door === 1 ? 'OPENED' : 'CLOSED'}`;
    changed = true;
    console.log(`🚪 Door: ${deviceState.door === 1 ? 'OPENED' : 'CLOSED'}`);
  }

  if (changed) {
    res.json({
      success: true,
      message: message
    });
  } else {
    res.status(400).json({
      success: false,
      message: 'No valid parameters'
    });
  }
});

// OPTIONS /set - CORS preflight
app.options('/set', (req, res) => {
  res.status(204).send();
});

// GET /api/info - Device information
app.get('/api/info', (req, res) => {
  console.log('ℹ️ GET /api/info');
  const uptime = Math.floor((Date.now() - deviceInfo.startTime) / 1000);
  
  res.json({
    success: true,
    device: {
      name: deviceInfo.name,
      version: deviceInfo.version,
      ip: deviceInfo.ip,
      mac: deviceInfo.mac,
      ssid: deviceInfo.ssid,
      rssi: deviceInfo.rssi,
      uptime: uptime,
      freeHeap: Math.floor(200000 + Math.random() * 50000) // Simulasi free heap
    }
  });
});

// OPTIONS /api/info - CORS preflight
app.options('/api/info', (req, res) => {
  res.status(204).send();
});

// 404 handler
app.use((req, res) => {
  res.status(404).json({
    success: false,
    message: 'Endpoint not found'
  });
});

// ============================================================
// START SERVER
// ============================================================
app.listen(PORT, () => {
  console.log('╔════════════════════════════════════════════╗');
  console.log('║  🏠 ESP32 Smart Home Mock Server         ║');
  console.log('╚════════════════════════════════════════════╝');
  console.log('');
  console.log(`🚀 Server running at: http://localhost:${PORT}`);
  console.log('');
  console.log('📋 Test Endpoints:');
  console.log(`   GET  http://localhost:${PORT}/status`);
  console.log(`   GET  http://localhost:${PORT}/set?led=1`);
  console.log(`   GET  http://localhost:${PORT}/api/info`);
  console.log('');
  console.log('📊 Sensor simulation active (updates every 2s)');
  console.log('🔧 CORS enabled for all origins');
  console.log('');
  console.log('Press Ctrl+C to stop');
  console.log('═══════════════════════════════════════════════');
});
