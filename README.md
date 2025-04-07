# ESP32-C3 Sony Camera Remote

A simple, reliable Bluetooth remote control for Sony cameras using an ESP32-C3 microcontroller.

## Overview

This project turns an ESP32-C3 into a wireless remote shutter for Sony cameras that support Bluetooth control. It uses the built-in button and LED of the ESP32-C3 to provide a compact, easy-to-use remote trigger solution.

## Features

- **One-Button Operation**: Press the built-in BOOT button to take a photo
- **Auto-Reconnection**: Automatically reconnects to your camera when powered on
- **Visual Feedback**: LED indicates connection status and operation
- **Camera Pairing**: Long-press to enter pairing mode for connecting to a different camera
- **Persistent Storage**: Remembers your camera between power cycles

## Hardware Requirements

- ESP32-C3 development board (any variant with a BOOT button and LED)
- Sony camera with Bluetooth remote capability (tested with Sony A7CR)
- USB cable for programming
- Optional: Battery power source for portable use

## Setup Instructions

### 1. Installation

1. Clone this repository
2. Open the project in Arduino IDE
3. Install required libraries:
   - ESP32 BLE Arduino
   - Preferences

### 2. Configuration

Before uploading, modify the default camera MAC address if needed:

```cpp
#define DEFAULT_CAMERA_ADDRESS "XX:XX:XX:XX:XX:XX"  // Replace with your camera's address
```

You can find your camera's Bluetooth MAC address in the camera's Bluetooth settings or by using a Bluetooth scanner app.

### 3. Upload

1. Connect your ESP32-C3 to your computer
2. Select the appropriate board in Arduino IDE (e.g., "ESP32C3 Dev Module")
3. Upload the sketch

### 4. Pairing

1. Enable Bluetooth on your Sony camera
2. Set your camera to accept Bluetooth remote connections (usually in the network or remote settings)
3. Power on your ESP32-C3 remote
4. Wait for connection (LED will stop blinking and remain on when connected)
5. Accept pairing on your camera if prompted

## Usage

- **Take Photo**: Short press the BOOT button
- **Pair New Camera**: Hold the BOOT button for 3+ seconds, then follow prompts in the serial monitor
- **Connection Status**:
  - Solid LED: Connected to camera
  - Blinking LED: Not connected
  - Rapid blink pattern: Taking photo or entering pairing mode

## Customization

The code can be easily modified to:
- Change the button press timing
- Adjust LED behavior
- Modify shutter sequence timing
- Add additional camera control functions

## Troubleshooting

- **Can't Connect**: Verify your camera's Bluetooth is enabled and in pairing mode
- **LED Keeps Blinking**: Make sure the camera address is correct
- **Shutter Not Triggering**: Check that your camera model supports the BLE shutter protocol
