# Tug-of-war operating guide for cars 81 and 82

[中文](GUIDE.zh-CN.md) · [日本語](GUIDE.ja.md) · [English](GUIDE.en.md)

Both cars pull in forward gear. Face the rear of each car toward the center line and attach the rope to the rear carabiner mount. Each car drives nose first toward its own side when the whistle sounds.

## Select and upload the sketch

| Chassis (open sketch) | ESC | ESC neutral | Forward onset | Steering center |
|---|---|---:|---:|---:|
| [81](../tug_car_81/tug_car_81.ino) | Tamiya TEU-105BK | 1510 µs | 1340 µs | 1449 µs |
| [82](../tug_car_82/tug_car_82.ino) | Hobbywing QuicRun WP 1060 Brushed | 1530 µs | 1430 µs | 1550 µs |

Select the physical chassis even if the Arduino boards have been swapped. Keep `ra0020_classifier.h` in the sketch folder. In Arduino IDE, select **Arduino UNO R4 WiFi** and the connected port, then upload. The forward values are measured wheel-start thresholds. The neutral pulse is a candidate value for car 81 and a provisional setting for car 82. Lift the drive wheels to verify neutral and direction, then test starting with the rope under load.

## Onboard matrix

The left side shows the chassis number **81 / 82** and the right side shows its state. In automatic mode, **sound the whistle when the check mark appears**.

| Example | Meaning |
|---|---|
| 81 − | Preparing. Wait for neutral settling and quiet audio windows. |
| 81 ✓ | Armed for whistle A or B. Front range is checked at launch. |
| 81 > | Forward commanded and the ten-second timer is running. |
| 81 × | Stop latched. Read the serial stop reason and reset for the next bout. |

Car 82 uses the same symbols. Manual backup goes directly from preparing to driving or stopped.

## Wiring and automatic start

Connect the ESC signal to **D6** (100 Hz), steering to **D5** (50 Hz), and the microphone analog output to **A0**. On the front HC-SR04, connect Trig to **D8** and Echo to **D12**. Use a common ground. The course specifies the GY-MAX4466 microphone and lists the KY-037 separately as a previous class stock module.

1. A continuous reflector is not required at startup. To test stopping, face a cardboard target toward the sensor and move it below 20 cm, measured from the transducers.
2. Power on in quiet surroundings. The ESC holds neutral for three seconds. Three consecutive valid windows without the target tone arm the detector. **Wait for the onboard LED to light before sounding the whistle.**
3. Current detection uses AC RMS only: RMS >= 50 (car 81) / 10 (car 82) at 12-bit ADC resolution for two consecutive approximately 50 ms windows. Tone, A/B classification, clipping and sampling-timing gates are not used. After three seconds at neutral, three consecutive windows below the selected threshold arm the detector. Claps and impacts can also trigger it. Serial reports RMS and state about every 250 ms while waiting, and START when forward is commanded. The retained ra0020_classifier.h is reference material and is no longer used by the tug sketches.

## Stopping and resetting

The ten-second timer starts at the first forward command. Ultrasound is checked about every 60 ms: only a received echo measuring **less than 20 cm** latches neutral. Exactly 20 cm, greater distances, out-of-range readings and missing echoes do not stop or block startup. A disconnected sensor cannot be distinguished from open space by missing echoes; verify cardboard stopping before each match. Stop remains latched until RESET. Serial `x` or `n` at 115200 baud also stops.

## Optional manual backup

Whistle start is the default. To enable the backup option, set `ENABLE_RESET_BACKUP` to `true` in the correct sketch and upload again. Leave D4 open during power-up. For a manual bout, hold RESET, fit a jumper from D4 to GND while holding RESET, and release RESET only after the whistle. The firmware waits at least three seconds after initialization and checks the front range before driving. Both stop conditions apply. Powering up with the jumper fitted also selects timed start, so remove it before cycling power.

## Match checks

Record three tests for each chassis. (1) With the rope under load, measure the time from each whistle tone to first wheel motion. Both must be within one second. (2) Maintain a valid echo throughout a pull and record the neutral command at ten seconds and actual wheel stop. (3) In a separate pull, bring the cardboard close before ten seconds and measure the remaining clearance at rest. The 20 cm threshold is a trial setting. Adjust it using measured stopping distance. Both sketches compiled and passed recorded-audio and simulated stop tests. Record the physical results separately for each car.

Arrive at least five minutes before the match. Check that the carabiner attaches, and bring the design, stopping-distance calculation and pre-test video for the TA/SA inspection. [Course rules](https://sites.google.com/edu.teu.ac.jp/applied-creative-mecha2026/競技/綱引き競技ルール力比べ#h.sl9okkvpmokx)

## Acceleration update

Car 81 launches at 1240 us; car 82 at 1330 us. Over 300 ms, car 81 ramps to 1200 us and car 82 to 1100 us, then holds. Smaller pulses request more forward throttle. START_US, PULL_US and RAMP_US configure the ramp. These are trial settings, not calibrated full throttle. Table values remain measured onset records. All stops remain active during acceleration; the 10-second deadline starts with the first drive command.

Current detection uses AC RMS only: RMS >= 50 (car 81) / 10 (car 82) at 12-bit ADC resolution for two consecutive approximately 50 ms windows. Tone, A/B classification, clipping and sampling-timing gates are not used. After three seconds at neutral, three consecutive windows below the selected threshold arm the detector. Claps and impacts can also trigger it. Serial reports RMS and state about every 250 ms while waiting, and START when forward is commanded. The retained ra0020_classifier.h is reference material and is no longer used by the tug sketches.

Car 81 uses RMS >= 50; car 82 retains RMS >= 10. Arming requires three consecutive windows below the selected threshold; starting requires two consecutive windows at or above it.
