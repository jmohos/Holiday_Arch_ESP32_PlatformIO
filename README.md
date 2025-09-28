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

## 🚀 Build

From the PlatformIO toolbar in VS Code, or from a terminal:

```bash
# Build
pio run
```

## 🚀 Upload - Direct via USB

From the PlatformIO toolbar in VS Code, or from a terminal:

### Upload firmware to device
```bash
# Build
pio run -t upload
```

### Monitor serial output
pio device monitor

## 🚀 Upload - Over the Air

You need to know the IP address of the specific target boards.
Each board should have a static DHCP allocation based on its
MAC address in the home router.

| Device | ID # | Router Name     | Mac Address          | IP Address     | SSID     | Wifi Pass       |
|--------|------|------------------|-----------------------|----------------|----------|------------------|
| Box 1  | 1    | esp32c6-CC8254   | 8C:BF:EA:CC:82:54     | 192.168.50.30  | showiot  | IOTNetwork4me!  |
| Box 2  | 2    | esp32c6-CC8DF8   | 8C:BF:EA:CC:8D:F8     | 192.168.50.48  | showiot  | IOTNetwork4me!  |
| Box 3  | 3    | esp32c6-CC7350   | 8C:BF:EA:CC:73:50     | 192.168.50.50  | showiot  | IOTNetwork4me!  |
| Box 4  | 4    | esp32c6-CC75E8   | 8C:BF:EA:CC:75:E8     | 192.168.50.51  | showiot  | IOTNetwork4me!  |
| Box 5  | 5    | esp32c6-7F8D28   | 98:88:E0:7F:8D:28     | 192.168.50.52  | showiot  | IOTNetwork4me!  |


When you know the target up, open a PIO terminal and type:

```bash
# Build
pio run -t upload -e xiao_esp32c6 --upload-port 192.168.50.30
```
