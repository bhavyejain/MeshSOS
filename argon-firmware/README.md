# Argon Firmware

This directory contains the source code of the firmware for Particle Argon boards.

## Directories
* **firmware** : Contains the final binaries for the argon board. This firmware can be flashed onto an Argon via USB.
* **lib** : Contains the source code for the libraries used in the project
* **src** : Contains the source code for the firmware of the project

## Firmware

`firmware_argon_v302.bin` is the main firmware for the Argon boards.

`firmware_telemetry` is the main firmware with telemetry data collection for network analysis.

## Device OS and Libraries

The target Device OS version for this project is deviceOS@1.4.4 for Argon.

The libraries used in the project are:
* google-maps-device-locator
* JsonParserGeneratorRK
* DiagnosticsHelperRK

## Source Files

The header file `publish-utilities.h` contains helper functions to publish messages in the mesh or to the console.

The INO file `sos-firmware.ino` contains the main firmware code for Argon boards.

The code is well-commented for being self explanatory.

## Working
The sequential working of the project is shown in a simplified manner in the [sequence diagram](../images/sequence-diagram.png).

Broadly, there are 3 basic cases that are handled during the execution of the system:

1. If the device is connected to the internet (via Wi-Fi), it fetches its location and then directly publishes an event to the Particle cloud. The integrated webhook then takes care of sending the data to our server. In this case, the mesh is not involved.
2. If the device does not have an internet connection, it publishes the message in the mesh network, so that a node with an active internet connection can publish the message to the Particle cloud. This case leads to 2 sub cases:
    * Case 1 : The device has location information stored (it had an internet connection sometime after start-up). In this case, the final mesh node simply publishes the message to the Particle cloud.
    * Case 2 : The device does not have any location information and fails to fetch it. In this case, when the message is published in the mesh the second time, a flag is attached to the message which lets the gateway node to append its own location information to the message being published to the Particle cloud.

Acknowledgements from the Particle cloud are always sent into the mesh by the gateway node, unless the acknowledgement was meant for that node itself.

The system makes maximum of 3 attempts to deliver a message to the cloud successfully, after which it declares the process as failed.

## Flashing the Argon with firmware

Connect the device via a USB cable and enter the following commands in `Particle CLI` sequentially to flash the device with the firmware.

1) Make the device enter DFU mode
```sh
particle usb dfu [device_name_if_multiple_deviced_connected]
```
2) Navigate to the directory containing the `.bin` firmware

3) Flash the device with the firmware
```sh
particle flash --usb .\firmware_argon_v302.bin
```

## Create mesh and add devices via Particle CLI (might work post deprication)
Particle documentation:

https://docs.particle.io/reference/developer-tools/cli/#particle-mesh

## Some helpful commands for particle CLI (Device connected via USB)

`particle serial wifi` : To connect to a wifi via CLI.

`particle usb listen` : To put the device into listening mode.

`particle usb stop-listening` : To exit listening mode.

`particle serial monitor` : To open the serial monitor for serial logs.

`particle flash --usb tinker` : Reset device to tinker app (original firmware of device) post development-apocalypse if the device is going crazy.

## Circuit connection schematic
<img src="../images/circuit.png">

## Collecting data with firmware_telemetry binary

1. Install a serial logging application like PuTTY (Windows).
2. Use `particle serial monitor` in the Particle CLI to get the COM serial line for your Argon.
3. Configure the logging application to listen to this COM port. For PuTTY, select `Serial` from `Connection type` and change the `Serial line` to your COM port, e.g., `COM3`.
4. Congifure the logging settings to print `all session output` and choose a log file name.
5. Start the application session to get serial output and save them as logs.

Reference: [About serial - Particle](https://docs.particle.io/tutorials/learn-more/about-serial/) , [Logging in PuTTY](https://www.eye4software.com/hydromagic/documentation/articles-and-howtos/serial-port-logging/)

### Relevant Log Labels:
* Round trip time
* Resolution time
* btn_med PRESSED
* btn_pol PRESSED
* ATTEMPTS
* ACK with ERROR
* ACK RECEIVED