import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:flutter_dotenv/flutter_dotenv.dart';
import './mqtt_service.dart';

class ApiService {
  String get _baseUrl => dotenv.env['BASE_URL'] ?? '';
  final MqttService _mqttService = MqttService();

  // Fetch sensor status - Now uses MQTT but keeps HTTP as fallback
  Future<Map<String, dynamic>?> fetchStatus() async {
    // Primary method: Use MQTT real-time data
    // This method is kept for compatibility but MQTT stream is preferred
    try {
      final response = await http.get(Uri.parse('$_baseUrl/status'));
      
      if (response.statusCode == 200) {
        final jsonData = json.decode(response.body);
        return jsonData['data'];
      }
      return null;
    } catch (e) {
      // If HTTP fails, we rely on MQTT stream data
      print('HTTP fallback failed, using MQTT: $e');
      return null;
    }
  }

  // Fetch device info - Keep HTTP for device information
  Future<Map<String, dynamic>?> fetchDeviceInfo() async {
    try {
      final response = await http.get(Uri.parse('$_baseUrl/api/info'));
      
      if (response.statusCode == 200) {
        final jsonData = json.decode(response.body);
        return jsonData['device'];
      }
      return null;
    } catch (e) {
      print('Failed to fetch device info: $e');
      return null;
    }
  }

  // Control LED - Now uses MQTT
  Future<bool> controlLED(bool value) async {
    return await _mqttService.controlLED(value);
  }

  // Control Door - Now uses MQTT
  Future<bool> controlDoor(bool value) async {
    return await _mqttService.controlDoor(value);
  }

  // Control Mode - Now uses MQTT
  Future<bool> controlMode(bool value) async {
    return await _mqttService.controlMode(value);
  }

  // HTTP fallback methods (optional)
  Future<bool> controlLEDHttp(bool value) async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/set?led=${value ? 1 : 0}')
      );
      return response.statusCode == 200;
    } catch (e) {
      return false;
    }
  }

  Future<bool> controlDoorHttp(bool value) async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/set?door=${value ? 1 : 0}')
      );
      return response.statusCode == 200;
    } catch (e) {
      return false;
    }
  }

  Future<bool> controlModeHttp(bool value) async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/set?mode=${value ? 1 : 0}')
      );
      return response.statusCode == 200;
    } catch (e) {
      return false;
    }
  }
}
