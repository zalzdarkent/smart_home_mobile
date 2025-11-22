import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:flutter_dotenv/flutter_dotenv.dart';

class ApiService {
  String get _baseUrl => dotenv.env['BASE_URL'] ?? '';

  // Fetch sensor status
  Future<Map<String, dynamic>?> fetchStatus() async {
    try {
      final response = await http.get(Uri.parse('$_baseUrl/status'));
      
      if (response.statusCode == 200) {
        final jsonData = json.decode(response.body);
        return jsonData['data'];
      }
      return null;
    } catch (e) {
      throw Exception('Failed to connect to server');
    }
  }

  // Fetch device info
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

  // Control LED
  Future<bool> controlLED(bool value) async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/set?led=${value ? 1 : 0}')
      );
      return response.statusCode == 200;
    } catch (e) {
      return false;
    }
  }

  // Control Door
  Future<bool> controlDoor(bool value) async {
    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/set?door=${value ? 1 : 0}')
      );
      return response.statusCode == 200;
    } catch (e) {
      return false;
    }
  }

  // Control Mode
  Future<bool> controlMode(bool value) async {
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
