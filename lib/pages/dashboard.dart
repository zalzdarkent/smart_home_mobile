import 'package:flutter/material.dart';
import '../components/status_header.dart';
import '../components/section_title.dart';
import '../components/mode_control.dart';
import '../components/device_info_card.dart';
import '../widgets/error_widget.dart';
import '../widgets/sensor_grid.dart';
import '../widgets/control_grid.dart';
import '../controllers/dashboard_controller.dart';

class Dashboard extends StatefulWidget {
  const Dashboard({super.key});

  @override
  State<Dashboard> createState() => _DashboardState();
}

class _DashboardState extends State<Dashboard> {
  late final DashboardController _controller;

  @override
  void initState() {
    super.initState();
    _controller = DashboardController();
    _controller.addListener(_onControllerUpdate);
    _controller.initialize();
  }

  @override
  void dispose() {
    _controller.removeListener(_onControllerUpdate);
    _controller.dispose();
    super.dispose();
  }

  void _onControllerUpdate() {
    if (mounted) setState(() {});
  }

  void _showSnackBar(String? message, {bool isError = false}) {
    if (message == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Operation failed'),
          backgroundColor: Colors.red,
          duration: const Duration(seconds: 2),
        ),
      );
      return;
    }

    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: isError ? Colors.red : Colors.green,
        duration: const Duration(seconds: 2),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF5F5F5),
      appBar: AppBar(
        elevation: 0,
        backgroundColor: const Color(0xFF1A237E),
        title: const Text(
          '🏠 Smart Home Dashboard',
          style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh, color: Colors.white),
            onPressed: _controller.loadData,
          ),
        ],
      ),
      body: _controller.isLoading
          ? const Center(child: CircularProgressIndicator())
          : _controller.errorMessage != null
          ? ErrorDisplay(
              errorMessage: _controller.errorMessage!,
              onRetry: _controller.retry,
            )
          : RefreshIndicator(
              onRefresh: _controller.loadData,
              child: SingleChildScrollView(
                physics: const AlwaysScrollableScrollPhysics(),
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    StatusHeader(manualMode: _controller.manualMode),
                    const SizedBox(height: 20),
                    const SectionTitle(title: '📊 Sensor Monitoring'),
                    const SizedBox(height: 12),
                    SensorGrid(
                      temperature: _controller.temperature,
                      humidity: _controller.humidity,
                      distance: _controller.distance,
                      motion: _controller.motion,
                    ),
                    const SizedBox(height: 24),
                    const SectionTitle(title: '🎛️ Control Panel'),
                    const SizedBox(height: 12),
                    ModeControl(
                      manualMode: _controller.manualMode,
                      onChanged: (value) async {
                        final msg = await _controller.handleMode(value);
                        _showSnackBar(msg);
                      },
                    ),
                    const SizedBox(height: 12),
                    ControlGrid(
                      ledStatus: _controller.ledStatus,
                      doorStatus: _controller.doorStatus,
                      onLEDTap: () async {
                        final msg = await _controller.handleLED(
                          !_controller.ledStatus,
                        );
                        _showSnackBar(msg);
                      },
                      onDoorTap: () async {
                        final msg = await _controller.handleDoor(
                          !_controller.doorStatus,
                        );
                        _showSnackBar(msg);
                      },
                    ),
                    const SizedBox(height: 24),
                    if (_controller.deviceInfo != null) ...[
                      const SectionTitle(title: 'ℹ️ Device Information'),
                      const SizedBox(height: 12),
                      DeviceInfoCard(deviceInfo: _controller.deviceInfo!),
                    ],
                  ],
                ),
              ),
            ),
    );
  }
}
