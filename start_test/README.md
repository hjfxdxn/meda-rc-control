# Sound-triggered ESC start test

Board: Arduino UNO R4 WiFi. Upload `start_test.ino` with `ra0020_classifier.h` in the same folder. Serial speed: 115200 baud. No steering servo is initialized or required.

## Wiring

Keep the existing microphone (AO=A0, DO=D7) and HC-SR04 (Trig=D9, Echo=D11) wiring. Add:

| ESC connection | Destination |
|---|---|
| Control signal | Arduino D6 |
| Control ground | Arduino GND |
| BEC supply, if present | Leave disconnected and insulated while Arduino is USB-powered |
| Main power input | Original vehicle battery, as specified for the ESC |
| Motor output | Existing motor connections |

Identify pins from the ESC labels/manual; wire colors vary. Do not connect the battery voltage to D6 or the Arduino 5V pin. Disconnect the ESC signal from any receiver output before attaching Arduino D6; do not connect two outputs together. Arduino and ESC must share control ground.

## Test sequence

1. Lift the driven wheels and disconnect ESC battery power while wiring.
2. Power Arduino from USB and upload the sketch.
3. Connect the ESC battery and switch on the ESC. Let it complete its normal neutral initialization. The code holds neutral for at least 3 seconds after MCU reset; the correct arming procedure depends on the ESC.
4. Keep quiet for three analysis blocks (about one second). Wait for `State:ARMED` before playing either start signal.
5. Two matching audio blocks trigger a single ESC test: 1580 us for 2 seconds, then a neutral request of 1530 us. Further sounds cannot restart it until reset.
6. Send `x` or `n` in Serial Monitor to request neutral and cancel the test. Reset is required afterward. Disconnect ESC power if necessary.

## Parameters and limits

ESC period is 10000 us (100 Hz), and neutral remains 1530 us. `START_ESC_PULSE_US=1580` uses the pulse the user confirmed causes reverse motion. The test therefore commands REVERSE for 2 seconds. The code limits the configured pulse to within 150 us of neutral. Neutral still requires verification on the actual ESC.

Neutral is not a verified brake command. This is a wheel-lifted bench test, not an autonomous driving controller. It has no RC override, steering, or obstacle-based stopping. HC-SR04 distance is diagnostic only; it is not used as a driving interlock.

The microphone uses the shared approximately 3.2 kHz A/B carrier feature. Similar sustained tones can trigger it. START is decided entirely on Arduino. The LED latches on detection.

While the motor test runs, audio processing, ultrasonic measurement, and normal telemetry are paused. The loop services the 2-second neutral deadline and serial stop commands. Before the motor starts, audio blocks and echo waiting may delay a stop command by a sampling/processing cycle. Serial stop is a software request, not a hardware emergency stop.

Serial output `Motor:START_REQUEST` announces the requested test. `Motor:STOPPED Reason:TIMEOUT` indicates that neutral was requested at the end; it does not prove the physical motor stopped. On a PWM fault, disconnect ESC power and inspect the setup.

## Experimental A/B diagnostics

Classification runs on the Arduino using the analog microphone waveform. A uses
the 54-68 Hz envelope band (recording: approximately 61 Hz); B uses 21-32 Hz
(approximately 26 Hz). Both share a carrier near 3.2 kHz.

Two consecutive valid windows must agree before `Class:A` or `Class:B` is
reported. Otherwise `Class:UNKNOWN` is reported. `Candidate` is the current
window only. A/B classification is diagnostic: an unclassified matching carrier
can still start the same 1580 us reverse test for 2000 ms.

The start-event line includes the classification from both confirmation windows.
`Score_A` and `Score_B` are normalized peak modulation powers, not confidence
probabilities. The winning band must exceed the other by at least 1.5 times.
`ModDepth` shows envelope variation; `ClassReason` explains rejection or waiting
for a second window. Existing RMS, carrier, clipping and timing checks remain.

For testing, lift the driven wheels, open Serial Monitor at 115200 baud, reset,
wait for `State:ARMED`, and play A alone. Save the start-event line. Reset and
repeat with B alone, using the same distance and volume. Repeat several times.
If the result is UNKNOWN or incorrect, retain the full diagnostic lines for
tuning; do not assume that classification has been validated. Processing pauses
during the motor run, and each reset permits only one start.
