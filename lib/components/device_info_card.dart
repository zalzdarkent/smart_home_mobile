import 'package:flutter/material.dart';

class DeviceInfoCard extends StatelessWidget {
  final Map<String, dynamic> deviceInfo;

  const DeviceInfoCard({
    super.key,
    required this.deviceInfo,
  });

  @override
  Widget build(BuildContext context) {
    final uptime = deviceInfo['uptime'] as int;
    final hours = uptime ~/ 3600;
    final minutes = (uptime % 3600) ~/ 60;
    final seconds = uptime % 60;

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: Column(
        children: [
          _buildInfoRow('Device', deviceInfo['name']),
          const Divider(),
          _buildInfoRow('IP Address', deviceInfo['ip']),
          const Divider(),
          _buildInfoRow('MAC Address', deviceInfo['mac']),
          const Divider(),
          _buildInfoRow('WiFi SSID', deviceInfo['ssid']),
          const Divider(),
          _buildInfoRow('Signal Strength', '${deviceInfo['rssi']} dBm'),
          const Divider(),
          _buildInfoRow('Uptime', '${hours}h ${minutes}m ${seconds}s'),
          const Divider(),
          _buildInfoRow('Version', deviceInfo['version']),
        ],
      ),
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Expanded(
            flex: 2,
            child: Text(
              label,
              style: TextStyle(
                color: Colors.grey[600],
                fontSize: 13,
              ),
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
          ),
          const SizedBox(width: 8),
          Expanded(
            flex: 3,
            child: Text(
              value,
              style: const TextStyle(
                fontWeight: FontWeight.bold,
                fontSize: 13,
              ),
              textAlign: TextAlign.right,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
          ),
        ],
      ),
    );
  }
}
