# 150 m development: standalone sound start (WIFI4)

## Current behavior

Car 82 is selected explicitly, regardless of which UNO R4 WiFi board is installed. Motor and steering PWM are enabled. After boot, the board initializes Wi-Fi (attempt after 10 seconds), requests ESC neutral, waits three seconds, and requires three valid non-target audio windows before arming. Two consecutive valid whistle-match windows start forward driving. A phone is not required. Similar sustained high-frequency sounds can also trigger this detector.

Automatic driving continues if the phone disconnects, the page is hidden, or the control session expires. Manual driving retains the 700 ms command timeout and stops on page hiding. STOP is latched until physical reset; resetting repeats automatic sound-start preparation. Neutral is a requested pulse, not verified braking.

## Trial defaults

| Setting | Value |
|---|---|
| Vehicle | 82 |
| ESC neutral | 1530 us, provisional legacy bench value |
| Custom / Slow / Normal | 1200 / 1410 / 1380 us |
| Reverse test endpoint | 1580 us |
| Steering left / center / right | 1150 / 1550 / 1950 us |
| Front obstacle stop | Valid reading at or below 80 cm |
| Side reference | Left, 60 cm target |
| Side correction | 1 us/cm, limited to +/-50 us |
| Maximum run time | 180 seconds |
| Maximum measurement age | 1000 ms |

Only 1430 us is the recorded forward starting threshold. Other speed presets, stopping distance, and side-control gains are trial settings, not measured performance. The timeout does not measure 150 m. There is no encoder, IMU, GNSS, or vehicle-speed feedback. These defaults enable development; they do not certify compliance with course rules.

Fresh no-echo measurements are permitted on an open road. The side controller returns to center when there is no valid side echo. A valid front reading within the stop distance requests neutral and latches STOP. Measurements older than 1000 ms stop the car. No echo is unknown: this sensor cannot distinguish open space from a disconnected transducer or missed obstacle. Check the front sensor using a real cardboard target before ground testing. Side correction requires a suitable reflecting reference and is not heading control.

## Phone connection and controls

1. Upload `race_150m.ino` with all its headers. Lift the drive wheels for the first powered test. USB powers the board; the ESC battery powers the motor.
2. Join `MEDA-Race82-<MAC suffix>` with password `MedaRace2026`. Keep the connection when iOS reports no Internet.
3. Open **http://192.168.4.1** in Safari. Choose **English** or **Chinese** using the language selector. The preference is stored in the browser when available.
4. Telemetry and STOP work without claiming a control session. **Connect Wi-Fi** claims control without disarming an existing autonomous run. Opening the page also does not disarm.
5. **STOP** requests neutral/center from any client on the password-protected AP, even if another client owns the controls. Wait for acknowledgement. If the radio connection is lost, remote STOP cannot reach the car.
6. **Idle / Disarm** cancels automatic preparation or driving. Use Idle before changing presets, parameters, or modes.
7. **Arm Sound Start** restarts neutral settling and the quiet-then-whistle sequence. It does not require a continuous phone heartbeat.
8. **Test Sound Detection — No Motion** is an Idle-only test. It requires three clear windows and two matches, shows DETECTED, and never drives the motor. Repeat the button to retest.
9. **Manual Mode** enables sliders after neutral settling. Hold **HOLD TO DRIVE** to apply commands; release requests neutral/center. The default 25% throttle scale can remain in the deadband. With Custom, +100% throttle at 100% scale requests 1200 us, not full throttle. Manual mode does not use obstacle stopping.
10. Parameter changes last until reset. Actuator calibration locks after PWM initialization; change startup calibration in `race_config.h`. Change preset pulses and automatic control parameters in Idle.

A phone may load the page and claim control during an autonomous run. Page transfers are chunked, but wireless latency means this is not an independent hardware emergency-stop circuit. Connect before the run when possible.

## Wiring

| Connection | UNO R4 WiFi |
|---|---|
| Front HC-SR04, J7 | TRIG D8 / ECHO D12 |
| Left HC-SR04, J4 | TRIG D9 / ECHO D11 |
| KY-037 | AO A0 / DO D7; AO drives recognition |
| Steering | D5, 50 Hz |
| ESC | D6, 100 Hz |
| TRU-08 inputs | D2/D3 reserved, not implemented |

Sensors use 5V and common GND. Follow the project power circuit for battery operation. ESC BEC 6V must not be connected directly to Arduino 5V.

## Board identity

| Original board car | Station MAC |
|---|---|
| 81 | F4:12:FA:70:5D:80 |
| 82 | D8:85:AC:A8:E9:6C |
| 83 | 34:B7:DA:63:8C:7C |

The station MAC is read after Wi-Fi station initialization. The AP MAC is displayed separately. Board identity does not change the active vehicle calibration.

## Sound and diagnostics

Serial baud: 115200. Firmware marker: `MEDA150 WIFI4`. States: 0 configuration, 1 settling, 2 waiting clear, 3 armed, 4 running, 5 stopped. Serial logging is suppressed during motion; the web page provides telemetry. Serial x or n requests latched STOP.

Audio: 10 kHz, 2560 samples per 256 ms window. A/B share a carrier near 3.2 kHz. Match requires RMS >= 4 ADC counts, Tone >= 0.65, no clipping, and valid sample timing. A/B classification is diagnostic only. This is two matching windows, not a two-second continuous-tone requirement; exact course timing must be verified separately. Wi-Fi and sonar add gaps between windows.

## Sources, protocol, and validation

`wifi_panel.html`, `wifi_panel.js`, and `wifi_i18n.js` are editable browser sources. `wifi_panel_page.h` embeds all three for the Arduino. Rebuild the header and upload after editing the page; refresh Safari after upload. No external scripts or Internet service are required.

GET / serves the page; GET /status returns 20-byte telemetry; GET /info returns identity and sound-test information. POST /stop always requests STOP without a session token. POST /claim obtains a session for other commands. POST /cmd requires the token and a strictly increasing sequence. The local Wi-Fi password is the access boundary; this is not an Internet-facing service.

Host controller tests cover sound gating, open-road no-echo behavior, obstacle/stale-range/time-limit stopping, manual deadman, control-session ownership, autonomous disconnect, and no-session STOP. Browser tests cover controls, language switching, pulse scaling, and STOP using mocked HTTP. UNO R4 WiFi compilation is checked with Renesas UNO 1.5.3. Real sound detection, braking distance, side control, and wireless range still require hardware testing.

The default Custom preset is 1200 us, selected by the user. Slow and Normal remain 1410 and 1380 us. The Custom preset is stronger than both named presets for this car; these labels do not measure speed. Automatic start applies 1200 us directly, without the tug sketch acceleration ramp.
