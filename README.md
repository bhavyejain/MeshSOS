# MeshSOS

A Particle IOT project aimed at creating a robust SOS emergency management system for senior citizens.

## The Idea
The project uses mesh networking (Only available till Device OS version 1.6.x) to increase the reliability of the system.
Use of a mesh network allows the system to register a distress call with the central servers even if the caller device is not connected to the internet directly.
If even one device in the mesh has internet access, the system ensures that the message is received by the concerned authorities.
Also, the system tries to provide the geolocation of the device which is sending the distress signal to aid the authorities. The location can then be used to get the optimal route to the person who sent out the distress call.
<img src="images/schematic.png">

## Devices
This project was made using the Argon (Bluetooth + WiFi) and Boron (Bluetooth + LTE) IOT boards from [Particle](https://www.particle.io/).
These devices have in-built mesh networking capabilities (currently being depricated; unavailable after Jan 2021).

## Integrations

#### Google Maps API
The geolocating feature of the api has been used to get the location of the device before sending out a distress call. The server has the option to show the location on a map, and also show the best route to the location. This integration is done by the means of a webhook which is triggered using the ```google-maps-device-locator``` Particle library.
#### Webhook To A Central Server
A central server is kept active which receives the SOS calls made from the devices. The webhook is triggered by the ```emergency``` events published on the Particle console. Once the data is sent to the server, the further handling of the situation is done by the authority-end console.

## Working
The sequential working of the project is shown in a simplified manner in the [sequence diagram](images/sequence-diagram.png).

## Device Firmware
The firmware for Particle Argon boards is contained in the argon-firmware directory. 

Please refer the readme in argon-firmware for details.

## The Server
The server for this project was created by [Kaustubh Trivedi](https://github.com/codekaust).
The django-server directory contains the code for the server.

Please refer the readme in django-server for details.