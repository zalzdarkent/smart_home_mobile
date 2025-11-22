import 'package:flutter/material.dart';
import 'dart:async';
import '../services/api_service.dart';

class DashboardController extends ChangeNotifier {
  final ApiService _apiService = ApiService();
  Timer? _refreshTimer;

  Map<String, dynamic>? deviceInfo;
  bool isLoading = true;
  String? errorMessage;
  
  bool manualMode = false;
  bool ledStatus = false;
  bool doorStatus = false;
  double temperature = 0.0;
  double humidity = 0.0;
  int distance = 0;
  int motion = 0;

  void initialize() {
    loadData();
    _refreshTimer = Timer.periodic(
      const Duration(seconds: 2),
      (_) => fetchStatus(),
    );
  }

  void dispose() {
    _refreshTimer?.cancel();
    super.dispose();
  }

  Future<void> loadData() async {
    await fetchStatus();
    await fetchDeviceInfo();
  }

  Future<void> fetchStatus() async {
    try {
      final data = await _apiService.fetchStatus();
      if (data != null) {
        temperature = (data['temperature'] as num).toDouble();
        humidity = (data['humidity'] as num).toDouble();
        distance = data['distance'] as int;
        motion = data['motion'] as int;
        ledStatus = data['led'] == 1;
        doorStatus = data['door'] == 1;
        manualMode = data['manualMode'] as bool;
        isLoading = false;
        errorMessage = null;
        notifyListeners();
      }
    } catch (e) {
      errorMessage = e.toString();
      isLoading = false;
      notifyListeners();
    }
  }

  Future<void> fetchDeviceInfo() async {
    final data = await _apiService.fetchDeviceInfo();
    if (data != null) {
      deviceInfo = data;
      notifyListeners();
    }
  }

  Future<String?> handleLED(bool value) async {
    if (await _apiService.controlLED(value)) {
      await fetchStatus();
      return 'LED ${value ? 'ON' : 'OFF'}';
    }
    return null;
  }

  Future<String?> handleDoor(bool value) async {
    if (await _apiService.controlDoor(value)) {
      await fetchStatus();
      return 'Door ${value ? 'OPENED' : 'CLOSED'}';
    }
    return null;
  }

  Future<String?> handleMode(bool value) async {
    if (await _apiService.controlMode(value)) {
      await fetchStatus();
      return 'Mode: ${value ? 'MANUAL' : 'AUTO'}';
    }
    return null;
  }

  void retry() {
    isLoading = true;
    errorMessage = null;
    notifyListeners();
    fetchStatus();
  }
}
