import 'package:flutter/material.dart';
import '../components/sensor_card.dart';

class SensorGrid extends StatelessWidget {
  final double temperature;
  final double humidity;
  final double distance;
  final int motion;

  const SensorGrid({
    super.key,
    required this.temperature,
    required this.humidity,
    required this.distance,
    required this.motion,
  });

  @override
  Widget build(BuildContext context) {
    return GridView.count(
      crossAxisCount: 2,
      shrinkWrap: true,
      physics: const NeverScrollableScrollPhysics(),
      crossAxisSpacing: 12,
      mainAxisSpacing: 12,
      childAspectRatio: 1.3,
      children: [
        SensorCard(
          icon: Icons.thermostat,
          label: 'Temperature',
          value: '${temperature.toStringAsFixed(1)}°C',
          color: Colors.orange,
          iconColor: Colors.orangeAccent,
        ),
        SensorCard(
          icon: Icons.water_drop,
          label: 'Humidity',
          value: '${humidity.toStringAsFixed(1)}%',
          color: Colors.blue,
          iconColor: Colors.blueAccent,
        ),
        SensorCard(
          icon: Icons.straighten,
          label: 'Distance',
          value: '${distance.toStringAsFixed(1)} cm',
          color: Colors.purple,
          iconColor: Colors.purpleAccent,
        ),
        SensorCard(
          icon: motion == 1 ? Icons.sensors : Icons.sensors_off,
          label: 'Motion',
          value: motion == 1 ? 'DETECTED' : 'No Motion',
          color: motion == 1 ? Colors.red : Colors.grey,
          iconColor: motion == 1 ? Colors.redAccent : Colors.grey,
        ),
      ],
    );
  }
}
