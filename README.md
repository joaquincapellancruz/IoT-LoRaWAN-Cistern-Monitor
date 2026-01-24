# IoT-LoRaWAN-Cistern-Monitor
Autonomous system for water level estimation using ESP32, LoRaWAN, and Ultrasonic sensors with a Flask-based web dashboard.

# CIME: Floating Capsule for Cistern Volume Estimation

An end-to-end IoT solution to monitor residential water cistern levels in real-time. This system uses an ultrasonic sensor to measure water levels, processes the data on an ESP32, and transmits it via LoRa to a local Raspberry Pi gateway hosting a web dashboard.

---

## Key Features

* **Long Range Communication:** Uses LoRaWAN protocol (SX1262) for reliable transmission from underground cisterns.
* **Real-time Dashboard:** A Python Flask web interface using Chart.js and SocketIO for latency under 150ms.
* **Data Logging:** SQLite database implementation for historical consumption analysis.
* **Robust Hardware:** IP65-rated enclosure with AJ-SR04M waterproof ultrasonic sensor.

---

## System Architecture

### Hardware Components
* **Node:** Heltec WiFi LoRa 32 V3 + Ultrasonic Sensor (AJ-SR04M).
* **Gateway:** Raspberry Pi Zero 2W + SX1302 Concentrator.

### Software Stack
* **Backend:** Python (Flask, SQLite).
* **Frontend:** HTML/JS (Chart.js, SocketIO).

---

## Volume Calculation

The system calculates the remaining volume ($V$) based on the sensor measurement ($h_{measured}$) and the cistern's total depth ($H$):

$$V = \text{Area} \times (H - h_{measured})$$

> **Note:** Validated with an absolute error average of only 0.20 gallons.
