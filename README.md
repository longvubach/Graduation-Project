# Graduation-Project: Remote-Controlled Robotic Arm Using EMG Sensors

An end-to-end, low-latency Embedded IoT system designed to control a 5-DOF robotic arm remotely using Electromyography (EMG) muscle sensors. This project implements a dual-microcontroller architecture leveraging high-performance wireless protocols to ensure real-time, high-precision human-machine collaboration.

## 📌 Features
* **Dual-MCU Architecture:** Separates data acquisition (Transmitter) and kinematic actuation (Receiver) to prevent processing bottlenecks.
* **Real-Time Control:** Implements the lightweight MQTT protocol for ultra-low latency telemetry and remote web override capabilities.
* **Cloud Analytics:** Utilizes HTTP POST requests to log historical physiological data directly to ThingSpeak for monitoring and analytics.
* **High-Precision Actuation:** Controls 5 servo motors simultaneously with 12-bit PWM resolution via I2C protocol.
  
## 🏗️ System Architecture & Data Flow
1. **Data Acquisition (Transmitter Node):** Two EMG Sensors (EMG1 & EMG2) capture analog muscle signals from the user. An ESP32-C6 processes and filters these physiological signals.
2. **Connectivity & Telemetry:**
   * The ESP32-C6 broadcasts real-time control payloads to an MQTT Broker, which synchronizes data bi-directionally with a Web Dashboard.
   * Concurrently, the ESP32-C6 uploads data blocks via HTTP to the ThingSpeak cloud platform for long-term storage.
3. **Motion Control (Receiver Node):** An ESP8266 microcontroller subscribes to the MQTT command topics. It translates the incoming coordinates and sends them to the PCA-9685 Servo Driver via I2C to orchestrate 5 independent Servo Motors.

## 🛠️ Hardware Stack
* **Microcontrollers:**
  * ESP32-C6 (RISC-V architecture, Wi-Fi 6, BLE, and Zigbee/Thread support)
  * ESP8266 (Highly cost-effective Tensilica L106 32-bit MCU with integrated Wi-Fi)
* **Sensors:** 2x Analog Electromyography (EMG) Muscle Sensors
* **Actuators & Drivers:**
  * PCA-9685 16-Channel 12-bit PWM I2C Servo Driver
  * 5x High-Torque Servo Motors (representing 5 Degrees of Freedom)

## 💻 Software & Protocols
* **Languages:** C/C++ (Embedded Hardware), Python (Data validation & scripting)
* **IoT Protocols:** MQTT (Real-time telemetry), HTTP (Cloud logging)
* **Hardware Interface:** I2C, Analog-to-Digital Conversion (ADC), PWM
* **Cloud/Web Platforms:** ThingSpeak, Web Dashboard (HTML5/CSS3/JavaScript)

## 👥 Contributors
This project was developed as a graduation thesis by:
* **Nguyen Ngoc Tu** ([Tu IOT](https://github.com/nguyenngoctu30)) - *Lead Embedded & IoT Engineer* (System Architecture, ESP32-C6 firmware, MQTT/HTTP integration, Node-RED)
* **Vu Bach Long** ([Vu Bach Long](https://github.com/longvubach)) - *Hardware Actuation & Web Developer* (ESP8266 firmware, PCA-9685 driver integration, Frontend development)
