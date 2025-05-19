#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs for BLE service and characteristic
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Global variables
bool deviceConnected = false;       // Flag to track BLE connection status
BLECharacteristic *pCharacteristic; // Pointer to the BLE characteristic
String dataBuffer = "";             // Buffer to store received data
unsigned long lastReceiveTime = 0;  // Timestamp of the last received data
const unsigned long DATA_TIMEOUT = 500; // Timeout duration (500ms)
const unsigned int MAX_BUFFER_SIZE = 100;  // Maximum buffer size

// BLE Server Callback Class - Handles connection and disconnection events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true; // Set connection status to true
    Serial.println("Device connected.");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false; // Set connection status to false
    Serial.println("Device disconnected.");

    // Restart advertising for reconnection
    pServer->startAdvertising();
    Serial.println("Advertising restarted.");
  }
};

// BLE Characteristic Callback Class - Handles data received from the client
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    // Retrieve the received data
    String newData = pCharacteristic->getValue();
    
    if (newData.length() > 0) {
      // If the buffer size exceeds the maximum limit, clear the buffer
      if (dataBuffer.length() + newData.length() > MAX_BUFFER_SIZE) {
        dataBuffer = "";
        Serial.println("Buffer overflow. Clearing buffer.");
      }

      // Append the new data to the buffer
      dataBuffer += newData;
      lastReceiveTime = millis(); // Update the last receive timestamp
    }
  }
};

void setup() {
  // Initialize the serial monitor
  Serial.begin(115200);
  Serial.println("Starting BLE Server...");

  // Initialize the BLE device with a device name
  BLEDevice::init("Dayoung_ESP32-BLE");

  // Create a BLE server and set its callback functions
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create a BLE service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE characteristic with read, write, notify, and indicate properties
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Set the callback function for the characteristic
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Set an initial value for the characteristic
  pCharacteristic->setValue("Hello iPhone!");

  // Start the BLE service
  pService->start();

  // Start advertising the BLE service
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("BLE Server Started. Ready to send and receive data.");
}

void loop() {
  // If data is received and 500ms have passed, print the data buffer
  if (dataBuffer.length() > 0 && millis() - lastReceiveTime >= DATA_TIMEOUT) {
    Serial.print("Received from iPhone: ");
    Serial.println(dataBuffer);

    // Clear the buffer after processing
    dataBuffer = "";
  }

  // If a client is connected and data is available in the serial monitor
  if (deviceConnected && Serial.available()) {
    // Read data from the serial monitor
    String input = Serial.readStringUntil('\n');
    input.trim();  // Remove whitespace characters
    
    // If the input is not empty, send it to the connected BLE client
    if (input.length() > 0) {
      Serial.print("Sending to iPhone: ");
      Serial.println(input);

      // Send the data via the BLE characteristic
      pCharacteristic->setValue(input.c_str());
      pCharacteristic->notify(); // Notify the connected client
    }
  }

  // Short delay to avoid rapid processing
  delay(50);
}
