# MeshSOS

A Particle IOT project aimed at creating a robust SOS emergency management system for senior citizens.

## The Idea
The project uses mesh networking (Only available till Device OS version 1.6.x) to increase the reliability of the system.
Use of a mesh network allows the system to register a distress call with the central servers even if the caller device is not connected to the internet directly.
If even one device in the mesh has internet access, the system ensures that the message is received by the concerned authorities.

## Devices
This project was made using the Argon (Bluetooth + WiFi) and Boron (Bluetooth + LTE) IOT boards from Particle (https://www.particle.io/).
These devices have in-built mesh networking capabilities (currently being depricated; unavailable after Jan 2021).