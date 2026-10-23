# USB and Wi-Fi ESC / steering calibration (CAL2)

## Wireless bench control

The UNO R4 WiFi hosts the same English control panel as a local Wi-Fi access point. This is browser-based adjustment, not wireless sketch uploading, Internet remote control, or simultaneous fleet control. No router, campus account, or computer server is needed.

1. Open `pwm_bench_test.ino` in Arduino IDE and upload to **Arduino UNO R4 WiFi** using USB. Keep `wifi_bench.h` and `control_panel_page.h` in the same sketch folder.
2. Lift the driven wheels. Confirm ESC neutral and steering clearance before enabling ESC power.
3. Join the car hotspot from a phone or computer. Demonstration password: **MedaBench2026**. Change `kBenchPassword` in `wifi_bench.h` before use if others should not join.
4. Stay connected even if the phone says "No Internet". Open **http://192.168.4.1** (HTTP, not HTTPS), then press **Connect Wi-Fi**. If unavailable, check the AP address printed in Serial Monitor at 115200 baud.
5. Check the displayed car number and board MAC. Choose and apply a steering profile manually only after confirming the physical car. MAC identification does not change centers or limits automatically.
6. Adjust ESC neutral, steering center/limits and slider scales using the existing controls. Hold a slider to test; release or press **NEUTRAL** to request neutral. Click **Disconnect** before handing control to another browser.

| Car | Expected board MAC | Hotspot |
|---|---|---|
| 81 | F4:12:FA:70:5D:80 | MEDA-Car-81 |
| 82 | D8:85:AC:A8:E9:6C | MEDA-Car-82 |
| 83 | 34:B7:DA:63:8C:7C | MEDA-Car-83 |

The code reads the MAC before entering AP mode because the AP MAC can differ from the station MAC. An unmatched address produces `MEDA-Unknown-xxxxxx` and car 0. Do not assume a match from a similar address. Check Serial Monitor and, if necessary, set `kCarOverride` to 81, 82 or 83 for a positively identified board, then upload again. This labels the board only; it does not apply steering limits.

### Control ownership and stopping

- One browser holds the Wi-Fi control lease at a time. Other control claims are rejected until release or expiry. Sequence numbers reject repeated/out-of-order commands within a lease. This is coordination, not a security authentication mechanism; access depends on the hotspot password.
- The panel sends held-slider commands every 100 ms. The main loop requests neutral after a nominal **500 ms without a Wi-Fi motion command**, revokes the lease, and requires an explicit reconnect. Idle heartbeats keep ownership but do not extend a motion command.
- USB motion commands retain the **2-second** timeout. During Wi-Fi ownership, USB ignores other commands but **n/x always requests neutral and cancels wireless control**.
- A stalled/closed/background browser loses its idle lease after 3 seconds. A failed network request disables the controls; the firmware timeout independently requests neutral. The page does not automatically resume a test.
- Loading the full web page requests neutral and revokes any active lease before transferring the page. Do not open another tab while someone is testing.
- WiFiS3 modem calls can block. The timeout is a software deadline, not a guaranteed hardware emergency stop. Validate worst-case wireless latency on the actual board. Keep a physical power disconnect accessible. Neutral is not verified braking, and an incorrect configured neutral may move the motor continuously.

Only control signals are wireless. The board still needs power: USB may remain connected for bench testing; battery operation uses the approved BEC/diode/VIN arrangement after supply validation. No receiver takeover, ultrasonic stop, or autonomous driving has been added.

### Files

Troubleshooting: a 2026-10-23 panel fix trims Arduino CRLF line endings before parsing the CAL2 handshake. Without this fix, Wi-Fi could connect while sliders stayed disabled. Re-upload the sketch (including the regenerated `control_panel_page.h`), reload the board-hosted page, and click **Connect Wi-Fi**. Offline tests now exercise the actual CRLF response format.

- `pwm_bench_test.ino`: PWM outputs, command parsing and motion timeout.
- `wifi_bench.h`: local access point, MAC identity, HTTP transport and single-controller lease.
- `control_panel.html`: English USB/Wi-Fi UI. For Wi-Fi, open the board-hosted page; opening the local file supports USB and displays Wi-Fi instructions.
- `control_panel_page.h`: generated copy served by the board. Keep it beside the sketch.
- `embed_panel.py`: run with Python after changing the HTML, then compile/upload again. Settings remain in RAM and reset on restart.

Wi-Fi implementation reference: [Arduino UNO R4 WiFi network examples](https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples/).

Upload `pwm_bench_test.ino` to Arduino UNO R4 WiFi, then open `control_panel.html` in desktop Chrome or Edge. Close other serial monitors and connect at 115200 baud. The updated panel requires a CAL2 configuration reply before enabling controls; upload the matching sketch if it keeps waiting.

ESC signal: D6. Steering signal: D5. Common ground is required. Lift the driven wheels before enabling ESC power. Neither the startup centers nor neutral requests prove that the vehicle is stopped or mechanically centered.

## What changed

| Setting | Previous panel | CAL2 panel |
|---|---|---|
| ESC scale | 0.1-1.5 us per percent | 0.1-6.0 us per percent |
| ESC output | Neutral minus offset, limited to +/-150 us | Neutral minus offset, clipped to 1000-2000 us |
| ESC neutral | Fixed at 1530 us | Editable, initially 1530 us |
| Steering scale | Up to 4 us per percent | Up to 8 us per percent |
| Steering center | 1422-1622 us | Within the applied limits; supports 1700 us |
| Steering bounds | Symmetric +/-400 us | Independent left/right limits enforced by firmware |

The slider remains -100% to +100%. Percent is a command position, not measured vehicle speed. Scale determines the pulse change per percent; clipping may make the ends of the slider flat. Defaults remain modest: ESC 1.2 us/%, steering 0.5 us/%.

## ESC calibration

1. Confirm the ESC model, mode and battery setting using [ESC_CALIBRATION.md](../ESC_CALIBRATION.md).
2. Start with a known neutral. `Apply ESC neutral` changes the continuously requested neutral pulse; an incorrect value can drive the motor even after releasing the slider. It does not program the ESC's internal neutral or endpoints.
3. Firmware blocks throttle for three seconds after Arduino initialization or an ESC-neutral change. If ESC power is switched on later, independently allow its required initialization time.
4. Hold and drag the throttle slider. Positive positions shorten the pulse; negative positions lengthen it. Record the actual physical direction, starting threshold and neutral interval. Increase scale gradually when the initial span is insufficient.
5. Release to request neutral. The panel sends a held command every 100 ms; firmware returns to neutral after two seconds without a USB control command or a nominal 500 ms without a Wi-Fi motion command. A continuously held slider can therefore run longer than two seconds.

The permitted 1000-2000 us output range is a software envelope, not verified full throttle or brake for every ESC. No automatic endpoint sweep or ESC button-setup sequence is performed. Follow the exact ESC manual for internal calibration and brake/reverse sequencing.

## Steering calibration

Choose car 81, 82 or 83 to fill in recorded values, then click **Apply center and limits**. Selecting a profile alone does not apply it. Profiles affect steering only.

| Car | Left (us) | Center (us) | Right (us) |
|---|---:|---:|---:|
| 81 | 1200 | 1449 | 1660 |
| 82 | 1150 | 1550 | 1950 |
| 83 | 1150 | 1700 | 1950 |

These are previously reported boundaries, with no verified clearance margin. Narrow them when checking the mechanism. On boot the firmware uses center 1522 us and limits 1472-1572 us; these are not a car-specific calibration. Applying a profile moves the servo to its configured center.

Custom limits must satisfy `900 <= left < center < right <= 2100`. The separate center field and +/-1 us buttons adjust center within the applied bounds. Firmware clips steering commands even if a different serial client requests a larger offset. For car 83 at scale 8, full left reaches 1150 us and full right is clipped to 1950 us.

The panel tests one channel at a time. Changing scale, releasing a slider, pressing Space/NEUTRAL or leaving the active window requests neutral on both channels. Settings last only until reset; record confirmed measurements in the calibration Markdown files.

## Serial commands

Commands with numbers must end with a newline.

| Command | Meaning |
|---|---|
| `?` | Help and CAL2 configuration |
| `n` or `x` | Request neutral on both channels |
| `t120` | ESC pulse = configured neutral minus 120 us |
| `s-50` | Steering pulse = configured center minus 50 us, clipped to limits |
| `e1530` | Set ESC neutral to 1530 us |
| `c1700` | Set steering center to 1700 us, if within applied limits |
| `g1700,1150,1950` | Apply center, left limit and right limit together |
| `f` / `b` | Increase/decrease throttle offset by 10 us; names do not prove physical direction |
| `l` / `r` | Request a 50 us steering offset, clipped to limits |

The periods remain ESC 100 Hz and steering 50 Hz. No receiver takeover or automatic obstacle stop is provided. A PWM failure latches out new motion commands; remove ESC power to inspect the hardware.

Validation: UNO R4 compilation and offline JavaScript boundary/protocol checks performed. Physical ESC/servo operation, Wi-Fi hotspot connection, MAC identification, transport latency and Web Serial interaction still require connected-board tests.
