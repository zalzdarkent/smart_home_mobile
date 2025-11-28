import 'package:flutter/material.dart';
import 'dart:async';
import '../services/mqtt_service.dart';

class DashboardController extends ChangeNotifier {
  final MqttService _mqttService = MqttService();
  StreamSubscription? _statusSubscription;

  Map<String, dynamic>? deviceInfo;
  bool isLoading = true;
  String? errorMessage;
  
  bool manualMode = false;
  bool ledStatus = false;
  bool doorStatus = false;
  double temperature = 0.0;
  double humidity = 0.0;
  double distance = 0.0;
  int motion = 0;

  void initialize() async {
    await connectToMqtt();
    setupStatusListener();
  }

  @override
  void dispose() {
    _statusSubscription?.cancel();
    _mqttService.dispose();
    super.dispose();
  }

  Future<void> connectToMqtt() async {
    isLoading = true;
    errorMessage = null;
    notifyListeners();

    try {
      final connected = await _mqttService.connect();
      if (connected) {
        isLoading = false;
        errorMessage = null;
      } else {
        errorMessage = 'Failed to connect to MQTT broker';
        isLoading = false;
      }
    } catch (e) {
      errorMessage = 'MQTT connection error: $e';
      isLoading = false;
    }
    notifyListeners();
  }

  void setupStatusListener() {
    print('🎯 Setting up MQTT status listener...');
    _statusSubscription = _mqttService.statusStream.listen(
      (data) {
        print('🎯 Received data from MQTT stream: $data');
        updateFromMqttData(data);
      },
      onError: (error) {
        print('❌ MQTT stream error: $error');
        errorMessage = 'MQTT stream error: $error';
        notifyListeners();
      },
      onDone: () {
        print('⚠️ MQTT stream closed');
      },
    );
  }

  void updateFromMqttData(Map<String, dynamic> data) {
    try {
      print('🔄 Updating from MQTT data: $data');
      
      // Check both field name formats (temp/temperature, hum/humidity, etc.)
      temperature = (data['temperature'] as num?)?.toDouble() ?? 
                   (data['temp'] as num?)?.toDouble() ?? 0.0;
      humidity = (data['humidity'] as num?)?.toDouble() ?? 
                (data['hum'] as num?)?.toDouble() ?? 0.0;
      distance = (data['distance'] as num?)?.toDouble() ?? 
                (data['dist'] as num?)?.toDouble() ?? 0.0;
      motion = (data['motion'] as num?)?.toInt() ?? 0;
      ledStatus = (data['led'] as num?) == 1;
      
      // For door, check if it's a servo position (0-180) or binary (0/1)
      final doorValue = data['door'] as num?;
      if (doorValue != null) {
        // If door value is > 45 degrees, consider it open
        doorStatus = doorValue > 45;
      }
      
      print('📊 Parsed values:');
      print('   Temperature: $temperature°C');
      print('   Humidity: $humidity%');
      print('   Distance: $distance cm');
      print('   Motion: $motion');
      print('   LED: $ledStatus');
      print('   Door: $doorStatus (raw: $doorValue)');
      
      // Parse mode from string or boolean
      final modeValue = data['mode'];
      if (modeValue is String) {
        manualMode = modeValue.toLowerCase() == 'manual';
      } else if (modeValue is bool) {
        manualMode = modeValue;
      }
      print('   Mode: $manualMode (${manualMode ? 'MANUAL' : 'AUTO'})');

      isLoading = false;
      errorMessage = null;
      notifyListeners();
    } catch (e) {
      print('❌ Error parsing MQTT data: $e');
      errorMessage = 'Error parsing MQTT data: $e';
      notifyListeners();
    }
  }

  // Keep this for fallback or initial load if needed
  Future<void> loadData() async {
    // For MQTT, we rely on real-time stream data
    // This method can be used for manual refresh
    if (!_mqttService.isConnected) {
      await connectToMqtt();
    }
  }

  Future<String?> handleLED(bool value) async {
    try {
      final success = await _mqttService.controlLED(value);
      if (success) {
        return 'LED ${value ? 'ON' : 'OFF'}';
      }
      return 'Failed to control LED';
    } catch (e) {
      return 'Error: $e';
    }
  }

  Future<String?> handleDoor(bool value) async {
    try {
      final success = await _mqttService.controlDoor(value);
      if (success) {
        return 'Door ${value ? 'OPENED' : 'CLOSED'}';
      }
      return 'Failed to control Door';
    } catch (e) {
      return 'Error: $e';
    }
  }

  Future<String?> handleMode(bool value) async {
    try {
      final success = await _mqttService.controlMode(value);
      if (success) {
        return 'Mode: ${value ? 'MANUAL' : 'AUTO'}';
      }
      return 'Failed to change mode';
    } catch (e) {
      return 'Error: $e';
    }
  }

  void retry() {
    isLoading = true;
    errorMessage = null;
    notifyListeners();
    connectToMqtt();
  }
}
