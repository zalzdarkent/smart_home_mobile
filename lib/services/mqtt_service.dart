import 'dart:convert';
import 'dart:async';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:flutter_dotenv/flutter_dotenv.dart';

class MqttService {
  static final MqttService _instance = MqttService._internal();
  factory MqttService() => _instance;
  MqttService._internal();

  MqttServerClient? _client;
  bool _isConnected = false;
  
  // Stream controllers untuk data
  final StreamController<Map<String, dynamic>> _statusController = 
      StreamController<Map<String, dynamic>>.broadcast();
  
  // Topics
  static const String statusTopic = 'smart_home/status';
  static const String modeSetTopic = 'smart_home/mode/set';
  static const String ledSetTopic = 'smart_home/led/set';
  static const String doorSetTopic = 'smart_home/door/set';

  // Getters
  Stream<Map<String, dynamic>> get statusStream => _statusController.stream;
  bool get isConnected => _isConnected;

  // Initialize MQTT connection
  Future<bool> connect() async {
    try {
      final mqttServer = dotenv.env['MQTT_SERVER'] ?? 'broker.hivemq.com';
      final mqttPort = int.tryParse(dotenv.env['MQTT_PORT'] ?? '1883') ?? 1883;
      
      // Disconnect existing client if any
      if (_client != null) {
        _client!.disconnect();
      }
      
      // Create client with unique ID
      final clientId = 'flutter_smart_home_${DateTime.now().millisecondsSinceEpoch}';
      _client = MqttServerClient(mqttServer, clientId);
      _client!.port = mqttPort;
      _client!.keepAlivePeriod = 60;
      _client!.connectTimeoutPeriod = 10000; // 10 seconds
      _client!.autoReconnect = false;
      _client!.onDisconnected = _onDisconnected;
      _client!.onConnected = _onConnected;
      _client!.onSubscribed = _onSubscribed;
      _client!.logging(on: true);

      // Set connection message
      final connMessage = MqttConnectMessage()
          .withClientIdentifier(clientId)
          .withWillTopic('will') // Will topic
          .withWillMessage('client disconnected')
          .startClean() // Non persistent session
          .withWillQos(MqttQos.atMostOnce);
      _client!.connectionMessage = connMessage;

      print('🔄 Connecting to MQTT broker: $mqttServer:$mqttPort with client ID: $clientId');
      
      // Connect with timeout
      final connectionResult = await _client!.connect().timeout(
        const Duration(seconds: 10),
        onTimeout: () {
          print('❌ MQTT connection timeout');
          return null;
        },
      );
      
      if (connectionResult != null && _client!.connectionStatus!.state == MqttConnectionState.connected) {
        print('✅ MQTT Connected successfully');
        _isConnected = true;
        
        // Subscribe to status topic
        print('📡 Subscribing to topic: $statusTopic');
        _client!.subscribe(statusTopic, MqttQos.atMostOnce);
        
        // Listen to messages
        print('👂 Setting up message listener...');
        _client!.updates!.listen(_onMessage);
        
        return true;
      } else {
        print('❌ MQTT Connection failed. Status: ${_client!.connectionStatus!.state}');
        print('❌ Return code: ${_client!.connectionStatus!.returnCode}');
        return false;
      }
    } catch (e) {
      print('❌ MQTT Connection error: $e');
      _isConnected = false;
      return false;
    }
  }

  // Disconnect from MQTT
  void disconnect() {
    _client?.disconnect();
  }

  // Send control command
  Future<bool> sendCommand(String topic, String payload) async {
    if (!_isConnected || _client == null) {
      print('❌ MQTT not connected');
      return false;
    }

    try {
      final builder = MqttClientPayloadBuilder();
      builder.addString(payload);
      
      _client!.publishMessage(topic, MqttQos.atLeastOnce, builder.payload!);
      print('📤 MQTT Message sent - Topic: $topic, Payload: $payload');
      return true;
    } catch (e) {
      print('❌ Error sending MQTT message: $e');
      return false;
    }
  }

  // Control methods
  Future<bool> controlMode(bool manualMode) async {
    return await sendCommand(modeSetTopic, manualMode ? '1' : '0');
  }

  Future<bool> controlLED(bool ledOn) async {
    return await sendCommand(ledSetTopic, ledOn ? '1' : '0');
  }

  Future<bool> controlDoor(bool doorOpen) async {
    return await sendCommand(doorSetTopic, doorOpen ? '1' : '0');
  }

  // Event handlers
  void _onConnected() {
    print('✅ MQTT client connected');
    _isConnected = true;
  }

  void _onDisconnected() {
    print('❌ MQTT client disconnected');
    _isConnected = false;
  }

  void _onSubscribed(String topic) {
    print('📥 Subscribed to topic: $topic');
  }

  void _onMessage(List<MqttReceivedMessage<MqttMessage>> messages) {
    for (final message in messages) {
      final topic = message.topic;
      final payload = MqttPublishPayload.bytesToStringAsString(
          (message.payload as MqttPublishMessage).payload.message);

      print('📨 Received MQTT message - Topic: $topic, Payload: $payload');

      if (topic == statusTopic) {
        try {
          final data = json.decode(payload) as Map<String, dynamic>;
          print('✅ Successfully parsed MQTT data: $data');
          _statusController.add(data);
        } catch (e) {
          print('❌ Error parsing status JSON: $e');
          print('❌ Raw payload: "$payload"');
          // Try to handle the data differently if it's not JSON
          _handleNonJsonPayload(payload);
        }
      }
    }
  }

  void _handleNonJsonPayload(String payload) {
    try {
      // If the payload is not JSON, try to parse it as a simple format
      // Based on the Arduino output, it might be sending individual values
      print('🔧 Attempting to parse non-JSON payload: $payload');
      
      // Try to extract data if it's in a different format
      // For now, just log it for debugging
    } catch (e) {
      print('❌ Failed to handle non-JSON payload: $e');
    }
  }

  // Get latest status (fallback method)
  Future<Map<String, dynamic>?> getStatus() async {
    // For MQTT, we rely on the stream
    // This is just a fallback that returns null since we use real-time data
    return null;
  }

  // Clean up
  void dispose() {
    _statusController.close();
    disconnect();
  }
}