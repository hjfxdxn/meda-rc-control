# Steering calibration

Panel update (2026-10-22): the [CAL2 panel](pwm_bench_test/README.md) now supports these three profiles, including car 83's 1700 us center and asymmetric travel. It enforces independently configured left/right bounds. The older panel limitations and symmetric-scale examples below are retained as historical context, not instructions for the updated panel. Measurements themselves are unchanged.

Record each car separately. Values are RC servo command pulse widths in microseconds (us), not steering angles. Left and right refer to the vehicle's direction of travel.

## Calibration records

| Car number | Center (us) | Left limit (us) | Right limit (us) | Left offset (us) | Right offset (us) | Date | Status |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 81 | 1449 | 1200 | 1660 | -249 | +211 | 2026-10-09 | User-reported measurements; loaded driving verification pending |
| 82 | 1550 | 1150 | 1950 | -400 | +400 | 2026-10-09 | User-reported measurements; loaded driving verification pending |
| 83 | 1700 | 1150 | 1950 | -550 | +250 | 2026-10-09 | Historical measurements retained; steering unresponsive as reported on 2026-10-23; wiring suspected, cause unconfirmed |

### Car 83 troubleshooting update (2026-10-23)

The user reports no steering response and suspects a wiring problem. This is not a confirmed diagnosis. Retain the previous 1700 us center and 1150/1950 us boundaries as historical measurements, not a fresh verification. Check servo power, common ground, connector polarity and the D5 signal path before repeating calibration. Current `pwm_bench_test` drives steering on D5; the separate `start_test` sketch does not initialize steering. Do not widen limits to compensate for an unresponsive servo.

## Car 81

- Use **1449 us** as the recorded steering center, subject to confirming that the front wheels point straight.
- Treat **1200 us** and **1660 us** as the reported travel boundaries. Mechanical clearance and a suitable operating margin have not been verified.
- Do not replace the center with the endpoint average of 1430 us solely to make the range symmetrical. The linkage may have asymmetric travel around the straight-ahead position.
- Relative to 1449 us, the available reported travel is 249 us to the left and 211 us to the right.
- A future car-specific controller should map each side separately and clamp the final pulse to the recorded limits (or narrower verified operating limits).

Example mapping for a normalized steering command `position` from -1 (left) through 0 (center) to +1 (right):

```cpp
constexpr int kCar81SteeringCenterUs = 1449;
constexpr int kCar81SteeringLeftLimitUs = 1200;
constexpr int kCar81SteeringRightLimitUs = 1660;

// Clamp position to [-1, 1] before applying this mapping.
// Negative position uses the left span; positive position uses the right span.
const float pulseUs = kCar81SteeringCenterUs + position *
    (position < 0.0f ? 249.0f : 211.0f);
```

## Current bench-panel settings

This document records measurements; it does not change the sketch's startup defaults or add automatic car-profile selection.

For car 81, apply a center of **1449 us** in the panel. With the current symmetric slider, a scale of **2.10 us per 1%** gives a full-travel range of **1239–1659 us**, inside the reported boundaries. This leaves only 1 us below the reported right boundary and is not a verified mechanical safety margin; start with a smaller scale when checking clearance.

Do not use the panel's full **4.0** scale on car 81: center 1449 us with a symmetric 400 us offset would request **1049–1849 us**, beyond both reported boundaries.

The panel's applied center lasts only until Arduino resets. Keep this record when uploading or reconnecting, and reapply the center as needed.

## Car 82

- Use **1550 us** as the recorded steering center, subject to confirming that the front wheels point straight.
- The reported left and right boundaries are **1150 us** and **1950 us**.
- Their arithmetic midpoint is exactly **1550 us**; travel is symmetric at **-400 us / +400 us** relative to center. Endpoint symmetry alone does not establish straight-ahead alignment.
- On the current bench panel, center **1550 us** and scale **4.0 us per 1%** map full travel to **1150–1950 us**. This reaches the reported boundaries with no additional operating margin; begin at a smaller scale and verify mechanical clearance.
- Mechanical clearance, a suitable operating margin, and loaded straight-line driving remain unverified.
- Do not use these settings on car 81, whose travel boundaries are narrower and asymmetric.

Example mapping for a normalized steering command `position` from -1 (left) through 0 (center) to +1 (right):

```cpp
constexpr int kCar82SteeringCenterUs = 1550;
constexpr int kCar82SteeringLeftLimitUs = 1150;
constexpr int kCar82SteeringRightLimitUs = 1950;

// Clamp position to [-1, 1] before applying this mapping.
const float pulseUs = kCar82SteeringCenterUs + position * 400.0f;
```

## Car 83

- Record **1700 us** as the user-reported straight-ahead center, pending confirmation that the front wheels point straight at this command.
- Reported travel boundaries are **1150 us** left and **1950 us** right: offsets of **-550 us** and **+250 us** from center.
- The endpoint midpoint is 1550 us, which is 150 us below the reported center. Do not substitute this midpoint for measured straight-ahead alignment. Check servo-horn alignment, steering linkage, and trim if the asymmetry is unexpected.
- Use separate left and right spans in a future car-specific controller. A symmetric 550 us span would exceed the reported right boundary.
- The current bench panel and firmware accept center values only from **1422 to 1622 us**, so **1700 us is not currently supported**. The steering-offset cap is also **400 us**, insufficient to reach this car's reported left boundary from 1700 us. These records do not change those software settings.
- Mechanical clearance, operating margins, and loaded straight-line driving remain unverified.

Example mapping for a normalized steering command `position` from -1 (left) through 0 (center) to +1 (right):

```cpp
constexpr int kCar83SteeringCenterUs = 1700;
constexpr int kCar83SteeringLeftLimitUs = 1150;
constexpr int kCar83SteeringRightLimitUs = 1950;

// Clamp position to [-1, 1] before applying this mapping.
const float pulseUs = kCar83SteeringCenterUs + position *
    (position < 0.0f ? 550.0f : 250.0f);
```
