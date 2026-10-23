# ESC configuration and calibration

Updated: 2026-10-23. Board: Arduino UNO R4 WiFi.

This document records the supplied vehicle inventory, manufacturer guidance, and pending measurements. It does not change firmware settings. Steering values are recorded separately in [STEERING_CALIBRATION.md](STEERING_CALIBRATION.md).

## Vehicle inventory

The installed components below were reported by the user; verify labels before testing.

| Car | Name | Chassis | Motor | ESC |
|---|---|---|---|---|
| 81 | KURUMA | Tamiya TT-02, on-road | MABUCHI RS-540SH-6527, 9.6 V specification | Tamiya TEU-105BK |
| 82 | Shako chan GO | Tamiya TT-02B, off-road | Tamiya OP.930 Super Stock BZ, 23T | Hobbywing QuicRun WP 1060 Brushed |
| 83 | BU-BU | Tamiya TC-01, Formula E GEN2 | Tamiya OP.1895 Brushless Motor 02 Sensored, 21.5T | Tamiya TBLE-04S, item 45069 |

Motor KV and torque constants do not determine ESC neutral, direction, deadband, or starting pulse width.

## Car 81: TEU-105BK

- Brushed motor controller with forward, brake, and reverse operation.
- Manufacturer supply specification: 6.6-7.2 V. The motor's 9.6 V designation does not authorize a 9.6 V supply to this ESC.
- Manufacturer-listed motor compatibility: Sport-Tuned motors and brushed motors of 25 turns or more. Compatibility of the exact RS-540SH-6527 must still be checked against motor current and load; the model name and voltage alone are insufficient.
- Manufacturer-listed output: forward 100%, reverse 50%. Do not assume symmetric response around neutral.
- Receiver/BEC output voltage: 6 V. Confirm the available current rating from the exact unit's documentation before finalizing the shared power budget.
- Record the actual neutral, endpoints, reverse configuration, and brake-to-reverse behavior. Do not copy the other cars' values.

Source: [Tamiya official drive-set manual containing TEU-105BK specifications](https://d7z22c0gz59ng.cloudfront.net/cms/japan/download/rcmanual/45053.pdf). [Standalone TEU-105BK manual](https://d7z22c0gz59ng.cloudfront.net/cms/japan/download/rcmanual/45055.pdf).

## Car 82: QuicRun WP 1060 Brushed

- Brushed motor controller. For a 540/550 motor with 2S LiPo or six-cell NiMH, the published turn limit is 12T or more; the reported 23T motor meets this turn-count condition. Gearing, cooling, and load still matter.
- Hold neutral during ESC power-up and wait approximately three seconds for automatic throttle calibration; confirm the completion indication before requesting motion.
- Jumper caps select running mode and battery type. Record their actual positions rather than assuming factory defaults.
- BEC specification: 6 V / 3 A. This is the shared supply capacity, not a separate allowance for each connected device.

| Running mode | Behavior |
|---|---|
| Fwd/Br | Forward and brake; reverse disabled |
| Fwd/Rev/Br | First reverse-side action brakes; a subsequent action can reverse after the motor stops |
| Fwd/Rev | Direct reverse operation; do not treat this as the same braking behavior |

Select the battery jumper to match the actual battery chemistry. A command that does not produce reverse may reflect the selected mode or sequence rather than insufficient pulse offset.

Sources: [Hobbywing product specifications](https://www.hobbywing.com/en/products/quicrun-wp-1060-brushed55), [official manual](https://www.hobbywing.com/en/uploads/file/20221015/f60b7ebe160a7b283927ae8916d36763.pdf).

## Car 83: TBLE-04S (45069)

- This is **TBLE-04S**, not the TBLE-04SR mentioned in the course example.
- Supports Tamiya sensored brushless motors of 15.5T or more; the reported Tamiya 21.5T motor is within that range.
- Use brushless motor mode, the specified three motor-phase connections, and the sensor cable. Do not apply the brushed-motor wire-swapping procedure to this sensored system.
- Perform the manufacturer's High Point Setup with the intended control signal range. Do not assume existing transmitter calibration matches Arduino output.
- Reverse can be enabled or disabled. Reverse operation requires the specified neutral-return sequence; verify behavior on the actual unit.
- Neutral brake and brake output are configurable. Requesting neutral is therefore not a universal definition of coasting or braking.
- Supply specification: 6.6-7.2 V. BEC: 6 V / 1.5 A. Manufacturer-listed output: forward 100%, reverse 50%.

Sources: [Tamiya product page](https://www.tamiya.com/japan/products/45069/index.html), [official manual](https://www.tamiyausa.com/media/files/45069ml-1207-ad8e.pdf), [Japanese manual](https://d7z22c0gz59ng.cloudfront.net/cms/japan/download/rcmanual/45069.pdf).

## Existing sound-triggered test settings

These are settings in `start_test/start_test.ino`, not completed calibration records for all three cars.

| Parameter | Current value | Evidence / limitation |
|---|---|---|
| ESC signal pin | D6 | Current firmware assignment |
| Signal period | 10000 us (100 Hz) | Existing bench setting; not verified across all three ESCs |
| Neutral request | 1530 us | Current setting; exact neutral interval remains unmeasured |
| Test drive request | 1580 us | User reported reverse motion; vehicle number was not recorded |
| Motor run duration | 2000 ms | Firmware then requests neutral |
| Neutral initialization interval | At least 3000 ms | Measured from Arduino PWM initialization, not from a separately switched ESC's power-on |

Earlier sound-triggered tests reported no wheel movement at 1480 us, with no confirmed result for the subsequent 1430 us change at that time. The vehicle was not identified. The separate car 82 measurements below do not retrospectively verify that earlier sound-triggered sequence.

The latest 1580 us sound-triggered sequence still needs hardware verification. Do not assign this observation to car 81, 82, or 83 without confirming which vehicle was tested.

The sketch has no steering output, receiver takeover, obstacle-based stop, or brake-to-reverse state machine. Its two-second motor test duration is separate from sound detection: starting currently requires two matching audio windows, not two seconds of continuous whistle detection.

## Measurements to record

Use `TBD` until physically measured. Steering center values must never be substituted for ESC neutral.

| Car | Mode / battery setting | Neutral interval (us) | Selected neutral (us) | Forward start (us) | Reverse start (us) | Verified signal period (us) | Status |
|---|---|---|---|---|---|---|---|
| 81 | TBD | TBD | TBD | 1340 | 1680 | TBD | Starting thresholds reported by user, 2026-10-23; 1510 us midpoint is a neutral candidate, not verified |
| 82 | TBD | TBD | TBD | 1430 | 1580 | TBD | Starting thresholds reported by user, 2026-10-23; partial calibration |
| 83 | TBD | TBD | TBD | 1560 | 1470 | TBD | Starting thresholds reported by user, 2026-10-23; 1515 us midpoint is a neutral candidate, not verified |

Car 81 observation (user report, 2026-10-23): forward rotation starts at **1340 us**, and reverse rotation starts at **1680 us**. Their arithmetic midpoint is **1510 us**, proposed as a neutral candidate only. Verify that this pulse stops the motor and is accepted during ESC initialization before recording it as the selected neutral. These starting thresholds are not full-throttle endpoints and do not establish the exact neutral interval or braking behavior. Test conditions and signal period were not supplied. Firmware defaults remain unchanged.

Car 82 observation (user report, 2026-10-23): forward rotation starts at **1430 us**, and reverse rotation starts at **1580 us**. These are observed starting thresholds, not full-throttle endpoints. The exact neutral interval, selected neutral, test load, signal period and ESC jumper settings were not supplied. Do not infer the neutral interval from these two thresholds or automatically change firmware settings based on this record.

Car 83 observation (user report, 2026-10-23): reverse rotation starts at **1470 us**, and forward rotation starts at **1560 us**. Their arithmetic midpoint is **1515 us**, proposed as a neutral candidate only. Verify that this pulse stops the motor and is accepted during ESC initialization before recording it as the selected neutral. Starting thresholds alone do not establish the exact neutral interval or braking behavior. The test neutral setting and signal period were not reported. Do not assume car 82's pulse-to-direction mapping applies to car 83. Steering is also currently unresponsive; the user suspects wiring, but the cause is not confirmed. See [STEERING_CALIBRATION.md](STEERING_CALIBRATION.md). Firmware defaults remain unchanged.

For each test, also record battery voltage/chemistry, motor wiring, unloaded or loaded condition, brake/reverse sequence, LED indications, date, and operator. Record full endpoints only after following the relevant manufacturer's setup procedure.

1. Identify the vehicle and ESC, lift the driven wheels, and check wiring and operating mode.
2. Establish a stable neutral signal before enabling ESC power; allow that ESC to initialize.
3. Adjust pulse width in small steps to identify the neutral interval and each direction's starting threshold. Return to neutral between trials and follow the ESC's reversal procedure.
4. Record the physical direction and distinguish braking from reverse motion. Do not infer direction from whether the pulse is larger or smaller than neutral.
5. Apply the measured settings to a car-specific firmware profile. Validate braking distance separately under controlled loaded conditions.

All three use an RC command signal, but their internal motor-drive PWM frequency is not the Arduino command repetition rate. Likewise, ESC current ratings do not specify available BEC current or spare battery power for a fan.
