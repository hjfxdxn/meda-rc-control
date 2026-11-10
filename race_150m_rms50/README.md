# 150 m: RMS 50 sound start and LED status

Open `race_150m_rms50.ino` in Arduino IDE, select Arduino UNO R4 WiFi and upload. Keep all headers in this folder. This separate variant preserves the original `race_150m` folder.

## Automatic operation

The selected chassis is **82**, regardless of board MAC. Wi-Fi initialization is attempted about ten seconds after boot; actuator PWM is initialized afterward. Hold neutral for three seconds, then require three consecutive quiet windows (RMS < 50). Two consecutive matching windows (RMS >= 50) request automatic start, subject to front-range and data-age checks. Each audio window samples 500 points at nominal 10 kHz, approximately 50 ms. Sonar and network processing add gaps, so two windows do not guarantee a 100 ms end-to-end start delay.

Only AC RMS is used. There is no frequency, A/B, clipping or sampling-jitter rejection. Loud claps and impacts may start the car. ADC resolution is 12 bits. RMS is in ADC counts, not volts or dB. `sound_rms.h` contains the threshold. The legacy Tone=0 and Class=? telemetry fields are unused.

Default forward output is **1200 us**, applied directly without a ramp. Neutral is 1530 us; steering center 1550 us. Forward is a trial command, not calibrated speed. The original Custom / Slow / Normal settings remain 1200 / 1410 / 1380 us.

## Wiring and stopping

- ESC D6 at 100 Hz; steering D5 at 50 Hz; microphone AO A0.
- Front HC-SR04: Trig D8, Echo D12. Left HC-SR04: Trig D9, Echo D11.
- Automatic front stop: valid distance <= 80 cm.
- Left distance target: 60 cm, correction 1 us/cm, limited to +/-50 us.
- Fresh missing echoes are permitted; missing side echo returns steering to center. Either sensor record older than 1000 ms stops automatic operation.
- Automatic timeout: 180 seconds. The program does not measure 150 m of travel.
- Wi-Fi STOP or Serial x/n at 115200 baud latches stop. RESET is required afterward.
- Manual mode retains its 700 ms command timeout and has no automatic obstacle stop. Phone disconnection does not stop autonomous driving.

## LED matrix

The left side displays **82**, identifying the selected vehicle rather than the board. The matrix starts after the deferred Wi-Fi/output initialization, so it may be blank for the first ten seconds or longer during Wi-Fi setup.

| Symbol | Meaning |
|---|---|
| - | Preparing or waiting for quiet |
| Check mark | Armed for sound; in sound-test mode this does not enable driving |
| > | Automatic driving command active |
| X | Stop latched |
| M | Manual mode (not necessarily moving) |
| I | Idle / disarmed |
| S | Sound detected in no-motion sound-test mode |

The matrix shows software state, not measured wheel motion. A matrix initialization failure is reported over Serial and does not disable the other functions.

## Wi-Fi

Connect to the `MEDA-Race82-...` AP with password `MedaRace2026`, then open http://192.168.4.1. Existing English/Chinese controls are retained. Use Idle before changing presets or mode. Test direction and stopping with wheels raised before road tests; recheck the 80 cm stopping margin at the selected forward output.
