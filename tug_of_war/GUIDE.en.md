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

1. Face a reflector toward the front sensor. For the first bench test, place cardboard about 50–100 cm in front. The sensor needs a valid echo at startup and throughout the pull.
2. Power on in quiet surroundings. The ESC holds neutral for three seconds. Three consecutive valid windows without the target tone arm the detector. **Wait for the onboard LED to light before sounding the whistle.**
3. The detector samples at 10 kHz in 256 ms windows. Two consecutive matches to the roughly 3.2 kHz carrier shared by RA0020 tones A and B trigger a front-range check. If it passes, the ESC starts the acceleration ramp described below. Similar high-frequency sounds can also trigger the detector.

## Stopping and resetting

The ESC receives neutral **10 seconds** after the first forward command. Independently, the controller checks the front range about every 60 ms. Pulling is permitted only for **20 cm < range ≤ 400 cm**. A close target, an out-of-range reading, or a missing echo latches the stop. The LED goes out. Removing the cardboard leaves the car stopped. Press RESET to prepare for the next bout. For a bench stop, send `x` or `n` in Serial Monitor at 115200 baud.

## Optional manual backup

Whistle start is the default. To enable the backup option, set `ENABLE_RESET_BACKUP` to `true` in the correct sketch and upload again. Leave D4 open during power-up. For a manual bout, hold RESET, fit a jumper from D4 to GND while holding RESET, and release RESET only after the whistle. The firmware waits at least three seconds after initialization and checks the front range before driving. Both stop conditions apply. Powering up with the jumper fitted also selects timed start, so remove it before cycling power.

## Match checks

Record three tests for each chassis. (1) With the rope under load, measure the time from each whistle tone to first wheel motion. Both must be within one second. (2) Maintain a valid echo throughout a pull and record the neutral command at ten seconds and actual wheel stop. (3) In a separate pull, bring the cardboard close before ten seconds and measure the remaining clearance at rest. The 20 cm threshold is a trial setting. Adjust it using measured stopping distance. Both sketches compiled and passed recorded-audio and simulated stop tests. Record the physical results separately for each car.

Arrive at least five minutes before the match. Check that the carabiner attaches, and bring the design, stopping-distance calculation and pre-test video for the TA/SA inspection. [Course rules](https://sites.google.com/edu.teu.ac.jp/applied-creative-mecha2026/競技/綱引き競技ルール力比べ#h.sl9okkvpmokx)

## Acceleration update

Car 81 launches at 1240 us; car 82 at 1330 us. Both ramp to 1100 us over 300 ms, then hold. Smaller pulses request more forward throttle. START_US, PULL_US and RAMP_US configure the ramp. These are trial settings, not calibrated full throttle. Table values remain measured onset records. All stops remain active during acceleration; the 10-second deadline starts with the first drive command.
