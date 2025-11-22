import 'package:flutter/material.dart';
import '../components/control_card.dart';

class ControlGrid extends StatelessWidget {
  final bool ledStatus;
  final bool doorStatus;
  final VoidCallback onLEDTap;
  final VoidCallback onDoorTap;

  const ControlGrid({
    super.key,
    required this.ledStatus,
    required this.doorStatus,
    required this.onLEDTap,
    required this.onDoorTap,
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
        ControlCard(
          icon: ledStatus ? Icons.lightbulb : Icons.lightbulb_outline,
          label: 'LED Light',
          isActive: ledStatus,
          onTap: onLEDTap,
          color: Colors.amber,
        ),
        ControlCard(
          icon: doorStatus ? Icons.door_front_door : Icons.door_back_door,
          label: 'Smart Door',
          isActive: doorStatus,
          onTap: onDoorTap,
          color: Colors.teal,
        ),
      ],
    );
  }
}
