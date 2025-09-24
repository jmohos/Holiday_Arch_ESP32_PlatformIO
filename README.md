# Joe's Holiday Arch PlatformIO Project

This project uses PlatformIO with an ESP32 microcontroller. It provides a starting point
for embedded development, including code, configuration, and documentation.

## 📂 Project Structure

- **src/** – main source files (`main.cpp`)
- **include/** – header files
- **lib/** – project-specific libraries
- **test/** – unit tests
- **platformio.ini** – PlatformIO build configuration
- **README.md** – project documentation

## 🛠 Requirements

- [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) extension  
- ESP32 development board (e.g., ESP32-DevKitC, ESP32-C3, ESP32-S3, etc.)

## 🚀 Build & Upload

From the PlatformIO toolbar in VS Code, or from a terminal:

```bash
# Build
pio run

# Upload firmware to device
pio run -t upload

# Monitor serial output
pio device monitor
