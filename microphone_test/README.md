# Autonomous A/B Start-Signal Detector

Upload `microphone_test.ino` with `ra0020_classifier.h` beside it to UNO R4 WiFi. Open the local `sensor_monitor.html` in desktop Chrome or Edge to observe it. Serial speed: **115200 baud**. Close the Arduino IDE Serial Monitor before connecting the browser.

## Operation

1. After reset, keep the environment quiet for about one second. Three valid non-target blocks arm the detector (`State:ARMED`). A target sound already present at boot does not start the detector.
2. Play either RA0020 reference signal for about one second. Two consecutive matching blocks set **`Start:1`**, `State:STARTED`, and turn on the onboard LED.
3. START remains latched until reset. Reset clears START and repeats initialization; reset itself is not a start signal.

The decision and latch run entirely on Arduino, including without a USB host or webpage. `startDetected` is the integration point for a future vehicle state machine. This bench sketch does not command the ESC or steering servo.

## Wiring

Microphone AO → A0, DO → D7, GND → GND, and a 5V-compatible microphone module VCC → 5V. HC-SR04 Trig → D9, Echo → D11, VCC → 5V, GND → GND. Disconnect power before wiring.

The detector uses AO. DO and the module comparator potentiometer do not identify the sound. The onboard LED now represents the latched START result, not DO.

## Detection and limitations

Both supplied recordings have a carrier near 3.2 kHz. The detector checks energy around this shared carrier, sufficient signal amplitude, no significant clipping, and valid sampling timing. It no longer requires the recordings' 26 Hz / 61 Hz modulation patterns to match. `Class:A/B/Unknown` is retained only as a diagnostic; it does not control START.

A 256 ms block contains 2560 samples at a target rate of 10 kHz. The shared carrier test is a coarse spectral match (10-sample quadrature windows), not exact sound identification. Similar sustained tones can trigger it, including a continuous 3.2 kHz tone. Motor sounds, alarms, echoes, and the actual start location must be tested before connecting this to vehicle motion. Removing A/B discrimination trades specificity for tolerance; it does not establish a hardware fix for previous missed detections.

Processing, serial output, and ultrasonic measurements cause gaps between sample blocks. Recognition needs two usable blocks, typically around 0.6–1 second after arming; real hardware timing is unverified. Ultrasonic triggers are at least 250 ms apart. A distance of -1 means no valid echo, not a clear path.

## Diagnostics

- `Reason:WEAK`: RMS is below the initial 4 ADC-count threshold; also normal in silence. Compare silence and playback values before changing this threshold.
- `Reason:NOT_TARGET`: insufficient energy in the shared carrier band (initial `MIN_TONE_RATIO=0.65`).
- `Reason:TIMING`: sample timing is unsuitable. `Sample_Hz` should be about 10000. Blocks reject more than two delays over 25 microseconds, any delay reaching 100 microseconds, or a sample rate outside 9900–10100 Hz.
- `Reason:CLIPPING`: more than 1% of samples are near the ADC rails; reduce input volume or increase distance.
- `Reason:MATCH`: current block matches; the state still governs arming and confirmation.
- `Tone` is a band-energy metric, not a confidence probability. `RMS` is in 12-bit ADC counts. `Mod_Hz` and `Purity` are optional A/B diagnostics.

Do not replace a microphone solely because the detector rejects a signal. First compare serial output in silence and during playback. A microphone module that does not deliver a usable analog audio waveform cannot be fixed by changing classification thresholds alone.

## Validation

The original A/B feature extraction was tested offline against the supplied MP3 recordings. This unified detector has different acceptance criteria: those earlier negative steady-tone tests do not describe its specificity. Browser state display checks passed. Actual microphone signal quality, detection range, false triggers, and real-time sampling still require hardware testing.

The unified detector compiled for UNO R4 WiFi with core 1.6.0: 59,808 bytes flash and 14,364 bytes global RAM. Offline shared-carrier checks passed for three windows from each source recording, and a state-model check covered initial arming, interrupted matches, confirmation, and latching. These do not replace hardware testing.
