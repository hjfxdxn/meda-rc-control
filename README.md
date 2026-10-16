# MEDA RC Control — UNO R4 WiFi Test Programs

Three independent test sketches. The sensor and PWM tests have browser interfaces; the sound-triggered ESC test runs autonomously. Upload one `.ino` sketch at a time to the Arduino UNO R4 WiFi. Uploading another sketch replaces the program on the board.

| Test | Arduino sketch | Browser interface | Serial baud rate |
|---|---|---|---|
| Microphone + HC-SR04 | [microphone_test.ino](microphone_test/microphone_test.ino) | [sensor_monitor.html](microphone_test/sensor_monitor.html) | 115200 |
| Sound-triggered ESC reverse test | [start_test.ino](start_test/start_test.ino) | Serial Monitor | 115200 |
| ESC + steering servo PWM | [pwm_bench_test.ino](pwm_bench_test/pwm_bench_test.ino) | [control_panel.html](pwm_bench_test/control_panel.html) | 115200 |

## Getting started

1. Download and extract this repository using **Code → Download ZIP**, or clone it with Git.
2. Install **Arduino UNO R4 Boards** in Arduino IDE. Select **UNO R4 WiFi** and the appropriate serial port, then open and upload the desired `.ino` sketch.
3. Close the Arduino IDE Serial Monitor and Serial Plotter to release the serial port.
4. Open the corresponding local HTML file in desktop Chrome or Edge. Click the connection button and select the board. Download the HTML file first; the GitHub file preview does not run the interface.
5. Disconnect the serial port in the browser before uploading a different sketch.

## Microphone and ultrasonic sensor wiring

Disconnect power before wiring. Follow the labels printed on each module and connect both modules to the Arduino ground.

| Module | Module pin | UNO R4 WiFi |
|---|---|---|
| Microphone (5V-compatible module with AO/DO outputs) | VCC / + | 5V |
| Microphone | GND / G | GND |
| Microphone | AO | A0 |
| Microphone | DO | D7 |
| HC-SR04 | VCC | 5V |
| HC-SR04 | GND | GND |
| HC-SR04 | Trig | D9 |
| HC-SR04 | Echo | D11 |

A/B recognition of the supplied RA0020 reference sounds is available; see [recognition setup and limitations](microphone_test/README.md). Keep `ra0020_classifier.h` beside the sketch when uploading.

The onboard LED indicates the latched START result. The microphone DO value is diagnostic only.

The ultrasonic sensor uses its default GPIO mode and takes measurements at least 250 ms apart. `Distance_cm:-1` means no measurement is available or the echo timed out; it must not be interpreted as a clear path. Microphone peak-to-peak voltage is not a decibel measurement. Audio is sampled in 256 ms blocks, with gaps for processing, serial output, and ultrasonic measurements (up to 40 ms for an echo). The displayed DO value is the state at the instant of the serial report, so brief sounds may be missed. The distance alert in the browser does not control the vehicle.

Reference: [HC-SR04 V2020 manual](https://akizukidenshi.com/goodsaffix/hc-sr04_v20.pdf).

## PWM bench test

Connect the steering signal to D5 and the ESC signal to D6, with a common ground. Power the servo and ESC according to their specifications; do not power them from Arduino GPIO pins. Lift the driven wheels before applying power.

The pulse widths in the sketch come from the course example and are not measured calibration values for your vehicle. Neutral is not a verified brake command. The program requests neutral at the end of a test. Disconnect ESC power when necessary to stop the test.

Existing calibration records are available in [STEERING_CALIBRATION.md](STEERING_CALIBRATION.md).

## Validation status

The A/B sensor sketch compiled successfully for UNO R4 WiFi (Renesas UNO core 1.6.0). The browser interface passed JavaScript syntax, parsing, and A/B/Unknown display checks. Offline audio feature checks are documented in [the recognition guide](microphone_test/README.md). Microphone playback, sampling timing, and hardware integration remain unverified.

## Sound-triggered ESC test

See [start_test instructions](start_test/README.md). Keep `ra0020_classifier.h` beside the sketch. This version adds experimental A/B modulation diagnostics while either matching start tone still triggers the motor. The pulse remains **1580 us (reverse)** for **2000 ms**, followed by a **1530 us neutral request**. Each reset permits only one start. Send `x` or `n` to cancel. Lift the driven wheels during testing.

This sketch controls only the ESC on D6. It does not initialize the steering servo, and ultrasonic distance does not stop the motor. The latest A/B diagnostic version compiled successfully for UNO R4 WiFi: 63,304 bytes flash and 14,452 bytes global RAM. The latest classification and reverse sequence still require hardware verification.
