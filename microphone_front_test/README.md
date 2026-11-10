# Front ultrasonic and microphone test

Open `microphone_front_test.ino` with `ra0020_classifier.h` in the same folder. Select Arduino UNO R4 WiFi, upload, and open Serial Monitor at 115200 baud.

| Device | Pin |
|---|---|
| Front HC-SR04 Trig | D8 |
| Front HC-SR04 Echo | D12 |
| Microphone AO | A0 |
| Microphone DO (optional diagnostic) | D7 |
| Sensor supply | 5V and common GND |

USB power is sufficient for the board and these sensors. Keep the ESC battery disconnected for this sensor-only test. This sketch does not initialize or drive the ESC or steering servo.

Place a flat cardboard target facing the front sensor at 50-100 cm. `Distance_cm` reports centimetres; -1 means no complete echo before the 40 ms timeout. This diagnostic sketch preserves the original timeout and does not apply the tug program's 20-400 cm interlock.

Audio fields include `RMS`, `Tone`, `Timing_OK`, `MaxLate_us`, `LateSamples`, and `Clipped`. `Start:1` means the sound detector latched a match; it does not command a motor. Reset to re-arm. `Class` and `Candidate` are diagnostic A/B estimates. A disconnected DO pin can show an unstable value without affecting analog detection.

This copy changes only the ultrasonic pin assignment and uses the original microphone classifier. It does not exercise the tug sketch's LED matrix interrupts or its complete control loop.

The RMS threshold is 10 ADC counts, matching the current tug sketches; the tone threshold remains 0.65.
