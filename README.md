# NAVIC-GSM-with-ESP

## Overview
This project integrates a NAVIC GPS receiver with the Vajravegha 4G LTE ESP32 and pushes the received data to ThingSpeak and Firebase. The same process is repeated with the Bharat Pi 4G board. The project utilizes the TinyGPS and TinyGSM libraries.

## Features
- Integration of NAVIC GPS receiver with Vajravegha 4G LTE ESP32 and Bharat Pi 4G board
- Data pushed to ThingSpeak and Firebase
- Firebase works only when WiFi is connected
- Data storage in SPIFFS of ESP32 when internet connectivity is lost
- Bulk data push to ThingSpeak and Firebase when internet connection is regained
- LED indicators for various statuses

## Libraries Used
- TinyGPS
- TinyGSM

## Setup
1. Connect the NAVIC GPS receiver to the ESP32 or Bharat Pi 4G board.
2. Connect the status LED to the ESP32 or Bharat Pi 4G board. 
    - Use PIN 16 (RX) and PIN 17 (TX) for Vajravegha
    - Use PIN 27 (RX) and PIN 26 (TX) for Bharat Pi

## License
This project is licensed under the MIT License - see the LICENSE file for details.
