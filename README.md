# ESP32-FreeRTOS-TMP36-IoT
An IoT-based temperature monitoring system using ESP32, FreeRTOS, and TMP36 sensor
This project is a real-time, multi-tasking IoT temperature monitoring system built on the ESP32 using FreeRTOS. It reads analog voltage from a TMP36 sensor, applies digital filtering, updates a local OLED display, and publishes the data to an MQTT broker concurrently without blocking the system.
## Features & Architecture
Instead of the standard sequential Arduino architecture, this project utilizes **FreeRTOS** to handle multiple operations.
**TaskSensorRead (Priority 2):** Reads raw analog data from the TMP36 sensor (Pin 34) every 1000ms and sends it to the temperature queue.
* **TaskDataProcess (Priority 2):** Receives raw ADC data, converts it to voltage, and calculates the temperature in Celsius. It applies an **Exponential Moving Average (EMA) Filter** before distributing the clean data.
* **TaskDisplay (Priority 1):** Monitors the display queue and updates the 0.96" I2C OLED screen in real-time.
* **TaskNetwork (Priority 1):** Manages Wi-Fi and MQTT connections. It streams data to the `emre/temperature` topic on the HiveMQ broker. If connection drops, it handles reconnection automatically without freezing the sensor reading operations.
