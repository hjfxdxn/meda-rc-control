# Bluetooth control on iPhone / iPad

## Setup

1. In Arduino IDE Library Manager install **ArduinoBLE**. Tested library versions: ArduinoBLE 2.1.0 and Arduino_SpiNINA 0.0.2, with UNO R4 Boards 1.6.0. The UNO R4 WiFi connectivity firmware must support BLE (Arduino specifies ESP32-S3 firmware 0.2.0 or newer). No radio firmware update was performed here.
2. Upload `race_150m.ino`. Default firmware advertises and reports sensors, but motor PWM is disabled.
3. Open `ble_panel.html` in **Bluefy** on iPhone/iPad from an **HTTPS** host, with `ble_panel.js` in the same directory. A GitHub file preview is not a running page. The page is prepared locally; GitHub Pages/other hosting has not been enabled or published. For GitHub Pages, the repository root can serve `/race_150m/ble_panel.html` after the files are pushed and Pages is configured.
4. Tap **Connect Bluetooth**, then choose `MEDA-82-XXXX`. The suffix identifies the board's BLE address; it does not choose the car profile. Grant Bluetooth access to Bluefy if requested. Pair from the page, not from iOS Bluetooth settings.
5. The panel selects **Idle** on connection. Read parameters before changing them. The browser must remain visible and awake.

Windows Chrome/Edge can also use the Web Bluetooth page from an appropriate secure origin. This is BLE GATT, not a Bluetooth serial-port adapter. It does not require new signal wiring or occupy D2/D3.

## Controls

- **Idle / Disarm** requests neutral/center and clears arming. Mode changes require Idle first.
- **Manual Mode** requires valid actuator calibration, `ENABLE_MOTOR_OUTPUT=true`, and at least three seconds at neutral after PWM initialization. On first activation the board may reply `ERR settling`; wait three seconds, then select Manual again. It does not start automatically after an error.
- Set throttle and steering sliders, then hold **HOLD TO DRIVE**. Releasing requests neutral/center. Negative throttle is reverse, positive is forward, regardless of which side of neutral the ESC uses. Scale fields are 1–100% of the configured test span. Default scales are 25% throttle and 20% steering. This is pulse interpolation, not measured speed or steering angle.
- Manual control has no obstacle stop or automatic reverse/brake sequence. Use it for attended bench/low-speed tests. The configured endpoints define the test range, not necessarily full ESC travel.
- **Arm Automatic** also requires `config.confirmed=true` in firmware plus a complete valid race configuration. It waits for a fresh quiet-then-whistle sequence and does not start motion directly from the button. It uses the forward endpoint as the fixed drive pulse; choose that value accordingly.
- **STOP** requests neutral and center, then locks all motion until a physical board reset. USB `x` / `n` has the same latch. Parameter editing cannot clear it.
- Manual command timeout: 700 ms. Phone-armed automatic heartbeat timeout: 1500 ms. These are software deadlines, plus actual loop/radio servicing latency. Late packets cannot revive an expired lease.
- Disconnecting the control link, hiding/closing the page, or losing its updates causes the active Bluetooth-controlled mode to request STOP. Do not rely on it as an independent hardware emergency stop.
- Automatic operation started by the standalone firmware does not require a phone heartbeat until the phone explicitly arms it. Connecting this panel selects Idle.

## Parameters

All values are session-only and are lost on reset. Parameters can change only in Idle. Actuator calibration (`n,f,r,c,l,u`) locks after PWM initializes; reset the board to change it. Automatic rules/hardware confirmation and the motor-enable flag cannot be changed by BLE.

| Key | Meaning | Range |
|---|---|---|
| n | ESC neutral | 1000–2000 us |
| f | Forward test endpoint and automatic drive pulse | 1000–2000 us |
| r | Reverse test endpoint | 1000–2000 us |
| c / l / u | Steering center / left / right | 900–2100 us; full ordering validated before motion |
| stop | Front stop distance | 2–399 cm |
| side | Side target distance | 2–400 cm |
| gain | Steering gain ×10 | 1–200, meaning 0.1–20 us/cm |
| limit | Maximum automatic correction | 1–400 us, must fit steering bounds |
| time | Run watchdog | 1–300 seconds |
| win | Consecutive matching audio windows | 1–20; still requires rule verification |
| sideR | Direction of J4 (Echo D11) | 0=left, 1=right |

Board selection remains car 82 regardless of MAC address. Its historical steering center/bounds are prefilled. ESC neutral/drive values are not inferred from starting-threshold averages. Keep `ENABLE_MOTOR_OUTPUT=false` until commissioning. Set `REVERSE_TEST_PULSE_US` in firmware for standalone defaults, or configure `r` in Idle before the first PWM initialization.

## GATT protocol

Service: `a1500000-4d45-4441-8000-008200000001`.

- Command: `a1500001-4d45-4441-8000-008200000001`, write with response, ASCII, maximum 20 bytes.
- Status: `a1500002-4d45-4441-8000-008200000001`, read/notify, binary 20-byte packets.
- Application reply: `a1500003-4d45-4441-8000-008200000001`, read/notify, ASCII. Wait for this reply before the next command; an ATT write response alone is not application acceptance.

Commands: `IDLE`, `MANUAL`, `AUTO`, `STOP`, `H` (automatic heartbeat), `M,t,s` (manual throttle/steering percentages), `P,key,value` (set), `G,key` (get). Replies: `OK ...`, `ERR ...`, or `V,key,value`. Refresh manual commands approximately every 100 ms even at zero. `H` does not renew the manual drive lease. STOP is allowed from every mode.

Status offsets (all 16-bit values little-endian):

| Offset | Value |
|---|---|
| 0 | Protocol version 1 |
| 1 | Mode 0 Idle / 1 Manual / 2 Automatic |
| 2 / 3 | Controller state / stop reason, see README |
| 4 | Flags: bit0 PWM ready, bit1 STOP latched, bit2 motor-enable build flag, bit3 automatic confirmation flag |
| 5 | Vehicle number |
| 6 / 8 | Front / side distance in mm; 65535 means invalid |
| 10 / 12 | Last ESC / steering pulse in us; zero means not initialized |
| 14 / 16 | RMS ×10 / tone ratio ×1000 |
| 18 / 19 | A/B diagnostic character / side direction (0 left, 1 right) |

Generic BLE tools can inspect these characteristics without hosting the panel, but a manual drive client must implement the lease refresh. There is no application authentication/pairing restriction in this development protocol; use it only in a controlled test area. Competition permission for wireless control has not been confirmed.

## Timing and validation

BLE polling avoids sonar echo transactions and the 256 ms audio sample capture. A STOP received during audio capture is handled after capture and analysis. Motor-running automatic mode and manual mode do not sample audio. Radio library calls can add latency; physical stop latency and simultaneous radio/sensor timing require bench measurement.

Host tests cover control gates, manual scaling, parameter bounds/locking, stop latch, command timeout, link loss and heartbeat timeout. Compilation and mocked browser tests do not prove iOS radio operation or physical braking. No board upload or iPhone test has been performed.

Sources: [ArduinoBLE firmware compatibility](https://github.com/arduino-libraries/ArduinoBLE), [Bluefy connection guide](https://bluefy.app/guide.html).
