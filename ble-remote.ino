#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEAddress.h>
#include <Preferences.h>

// ESP32-C3 settings
#define BUTTON_PIN 9           // BOOT button on ESP32-C3
#define LED_PIN 8              // Built-in LED on ESP32-C3
#define DEFAULT_CAMERA_ADDRESS "XX:XX:XX:XX:XX:XX"  // Default camera address

// Button timing
#define LONG_PRESS_TIME 3000   // 3 seconds for long press
#define DEBOUNCE_TIME 50       // Debounce time in ms

// BLE UUIDs for Sony camera shutter control
static BLEUUID SHUTTER_SERVICE_UUID("8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF");
static BLEUUID WRITE_CHARACTERISTIC_UUID("0000FF01-0000-1000-8000-00805f9b34fb");
static BLEUUID NOTIFY_CHARACTERISTIC_UUID("0000FF02-0000-1000-8000-00805f9b34fb");

// Global variables
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pWriteCharacteristic = nullptr;
BLERemoteCharacteristic* pNotifyCharacteristic = nullptr;
bool deviceConnected = false;
String cameraAddress;
Preferences preferences;

// LED control
void setLed(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

// LED blink pattern
void blinkLed(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    setLed(true);
    delay(delayMs);
    setLed(false);
    delay(delayMs);
  }
}

// BLE client callbacks
class ClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connected to camera");
    deviceConnected = true;
    setLed(true);
  }
  
  void onDisconnect(BLEClient* pclient) {
    Serial.println("Disconnected from camera");
    deviceConnected = false;
    setLed(false);
  }
};

// Callback for notifications from the camera
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Camera Notification: ");
    for (int i = 0; i < length; i++) {
      Serial.printf("%02X ", pData[i]);
    }
    Serial.println();
}

// Connect to camera
bool connectToCamera(const char* address) {
  BLEAddress cameraAddress(address);
  
  // Create BLE client
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new ClientCallbacks());
  
  // Connection feedback
  Serial.printf("Connecting to camera at %s...\n", address);
  
  if (!pClient->connect(cameraAddress)) {
    Serial.println("Connection failed");
    return false;
  }
  
  // Save successful connection address
  preferences.putString("cameraAddr", address);
  Serial.println("Camera address saved");
  
  // Get the service
  BLERemoteService* pService = pClient->getService(SHUTTER_SERVICE_UUID);
  if (!pService) {
    Serial.println("Failed to find shutter service");
    pClient->disconnect();
    return false;
  }
  
  // Get the write characteristic
  pWriteCharacteristic = pService->getCharacteristic(WRITE_CHARACTERISTIC_UUID);
  if (!pWriteCharacteristic) {
    Serial.println("Failed to find write characteristic");
    pClient->disconnect();
    return false;
  }
  Serial.println("Write characteristic found");
  
  // Get the notify characteristic (optional)
  pNotifyCharacteristic = pService->getCharacteristic(NOTIFY_CHARACTERISTIC_UUID);
  if (pNotifyCharacteristic && pNotifyCharacteristic->canNotify()) {
    pNotifyCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Notification callback registered");
  }
  
  return true;
}

// Send the shutter command sequence to the camera
void sendShutterSequence() {
  if (!deviceConnected || !pWriteCharacteristic) {
    Serial.println("Not connected or write characteristic not available");
    return;
  }
  
  // Visual feedback during operation
  setLed(false);
  
  // Commands from the XML macro
  uint8_t commands[][2] = {
    {0x01, 0x07},  // Focus
    {0x01, 0x09},  // Trigger
    {0x01, 0x08},  // Hold
    {0x01, 0x06}   // Release
  };
  
  int delays[] = {1000, 500, 500};
  
  for (int i = 0; i < 4; i++) {
    Serial.printf("Writing 0x%02X%02X with WRITE_REQUEST...\n", commands[i][0], commands[i][1]);
    
    // Use WRITE_REQUEST type (true for response)
    pWriteCharacteristic->writeValue(commands[i], 2, true);
    
    if (i < 3) {
      delay(delays[i]);
    }
  }
  
  Serial.println("Shutter sequence complete");
  
  // Success indicator
  setLed(true);
}

void setup() {
  Serial.begin(115200);
  delay(500); // Allow serial to settle
  
  // Configure pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  setLed(false);
  
  Serial.println("\nESP32-C3 Sony Camera Remote");
  Serial.println("---------------------------");
  Serial.println("Short press: Take photo");
  Serial.println("Long press: Enter pairing mode for new camera");
  
  // Initialize preferences for persistent storage
  preferences.begin("camera", false);
  
  // Initialize BLE with security
  BLEDevice::init("ESP32-C3-CamRemote");
  
  // Enable security and bonding
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_BOND;
  esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
  uint8_t key_size = 16;
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
  
  // Get saved camera address or use default
  cameraAddress = preferences.getString("cameraAddr", DEFAULT_CAMERA_ADDRESS);
  Serial.printf("Using camera address: %s\n", cameraAddress.c_str());
  
  // Blink quickly while trying to connect
  for (int i = 0; i < 5 && !deviceConnected; i++) {
    blinkLed(2, 100);
    
    if (connectToCamera(cameraAddress.c_str())) {
      Serial.println("Camera connected successfully!");
      break;
    }
    
    delay(500); // Wait before retrying
  }
  
  if (!deviceConnected) {
    Serial.println("Failed to connect to camera");
    // Blink pattern to indicate no connection
    blinkLed(5, 200);
  }
}

void loop() {
  static bool buttonPressed = false;
  static unsigned long pressStartTime = 0;
  static bool longPressHandled = false;
  
  // Button state detection with debounce
  bool currentState = digitalRead(BUTTON_PIN) == LOW;
  
  // Button press started
  if (currentState && !buttonPressed) {
    buttonPressed = true;
    pressStartTime = millis();
    longPressHandled = false;
    delay(DEBOUNCE_TIME);
  }
  
  // Check for long press while button is held
  if (buttonPressed && currentState && !longPressHandled) {
    if (millis() - pressStartTime > LONG_PRESS_TIME) {
      Serial.println("Long press detected!");
      longPressHandled = true;
      
      // Disconnect if connected
      if (deviceConnected && pClient) {
        pClient->disconnect();
        delay(500);
      }
      
      // Show we're in pairing mode
      blinkLed(10, 100);
      
      // Prompt for new camera address
      Serial.println("Enter new camera address in format XX:XX:XX:XX:XX:XX");
      
      // Wait for serial input
      while (!Serial.available()) {
        blinkLed(1, 500);
      }
      
      // Read new address
      String newAddress = Serial.readStringUntil('\n');
      newAddress.trim();
      
      if (newAddress.length() == 17) { // Valid MAC address length
        Serial.printf("Attempting to connect to new camera: %s\n", newAddress.c_str());
        cameraAddress = newAddress;
        
        if (connectToCamera(cameraAddress.c_str())) {
          Serial.println("New camera connected successfully!");
        } else {
          Serial.println("Failed to connect to new camera");
        }
      } else {
        Serial.println("Invalid address format");
      }
    }
  }
  
  // Button release - short press handling
  if (!currentState && buttonPressed) {
    unsigned long pressDuration = millis() - pressStartTime;
    buttonPressed = false;
    
    // Only handle short press if we haven't handled a long press
    if (pressDuration < LONG_PRESS_TIME && !longPressHandled) {
      Serial.println("Short press detected");
      
      // If not connected, try to connect
      if (!deviceConnected) {
        if (connectToCamera(cameraAddress.c_str())) {
          Serial.println("Connected after button press");
        } else {
          Serial.println("Failed to connect after button press");
        }
      }
      
      // If connected, send shutter command
      if (deviceConnected) {
        sendShutterSequence();
      }
    }
    
    delay(DEBOUNCE_TIME);
  }
  
  // Connection status LED indication when not taking a photo
  if (!deviceConnected) {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
      lastBlink = millis();
      blinkLed(1, 100);
    }
  }
  
  // Auto-reconnect if disconnected
  static unsigned long lastReconnectAttempt = 0;
  if (!deviceConnected && (millis() - lastReconnectAttempt > 5000)) {
    lastReconnectAttempt = millis();
    Serial.println("Attempting to reconnect...");
    connectToCamera(cameraAddress.c_str());
  }
}
