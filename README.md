# ESPMiser

ESPMiser is an ESP-IDF project for an ESP32-S3-based terminal that will display data from the LEOP server and a local environmental sensor.

## Prerequisites

- ESP-IDF v6.1 (tested)
- Git
- An ESP32-S3 development board
- A USB data cable

## Configuration

Run all commands from an activated ESP-IDF terminal in the project's root directory.

Select the ESP32-S3 project target:

```bash
idf.py set-target esp32s3
```

Open the project configuration menu:

```bash
idf.py menuconfig
```

Saved configuration is stored locally in `sdkconfig`. Shared default settings are provided by `sdkconfig.defaults`.

## Build

Build the project from the project's root directory:

```bash
idf.py build
```

A sucessful build creates the firmware and other generated files in the `build` directory.

## Flash

Connect your ESP32-S3 to your computer using a USB data cable and identify its serial port. On Windows, the port will usually have a name such as `COM5`.

Flash the firmware by replacing `<PORT>` with the boards serial port:


```bash
idf.py -p <PORT> flash
```

For example, if the board is connected through `COM5`, run:

```bash
idf.py -p COM5 flash
```

The `-p` option tells ESP-IDF which serial port to use.

> **Note:** These flashing instructions have not yet been verified as the ESP32-S3 hardware is not currently available at the time of writing.







six seven