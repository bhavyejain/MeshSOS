# Argon Firmware

This directory contains the source code of the firmware for Particle Argon boards.

## Directories
* **firmware** : Contains the final binary for the argon board. This firmware can be flashed onto an Argon via USB.
* **lib** : Contains the source code for the libraries used in the project
* **src** : Contains the source code for the firmware of the project

## Device OS and Libraries

The target Device OS version for this project is deviceOS@1.4.4 for Argon.

The libraries used in the project are:
* google-maps-device-locator
* JsonParserGeneratorRK
* DiagnosticsHelperRK

## Source Files

The header file publish-utilities.h contains helper functions to publish messages in the mesh or to the console.

The INO file sos-firmware.ino contains the main firmware code for Argon boards.