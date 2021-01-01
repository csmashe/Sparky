# sparkle 
Bluetooth "Universal Remote" Foot pedal for Digital Guitar Amps, on ESP32. I originally built Justin Nelsons tinderbox pedal for my spark amp. I wanted to take it to the next level and allow more than just flipping through 4 presets. This is a merge of Justin's idea with the Spark Amp Foot Pedal V1. Build the pedal exactly how Justin has it and then use this code to expand on his idea.


https://github.com/jrnelson90/tinderboxpedal

https://blog.studioblip.com/guitar/amps/spark/footpedalV1/


## Environment Setup Notes:
* So far only tested with the Positive Grid Spark 40 Amp
* **ESP32 Setup:** https://github.com/jrnelson90/tinderboxpedal/wiki/ESP32-Arduino-Core-Setup-for-TinderBox-ESP-v0.3.1
* **Please check your specific ESP32 dev board pinouts if attempting to use this version.**


## Basic Schematics:

### ESP32 Schematic
**In this code pins 4, 5, 18, and 19 are used for button input. Your ESP32 dev board's GPIO layout may be different, so please double-check after wiring and before running!**
![](src/tinderbox_ESP32.png)



## Other great projects used in the Sparkel Pedal include:

### ESP32
* ESP32 Arduino-Core:  https://github.com/espressif/arduino-esp32
* Thingpulse SSD1306/SH1106 ESP Driver: https://github.com/ThingPulse/esp8266-oled-ssd1306
* ButtonFever: https://github.com/mickey9801/ButtonFever

### Wireless Sniffing
* Wireshark: https://github.com/wireshark/wireshark

## License

Copyright 2020 Sparkel Pedal Project and Christopher Smashe

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
