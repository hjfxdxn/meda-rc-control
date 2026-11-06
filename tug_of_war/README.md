# Tug of war: UNO R4 WiFi

[2026 tug-of-war rules, start and stop](https://sites.google.com/edu.teu.ac.jp/applied-creative-mecha2026/競技/綱引き競技ルール力比べ#h.sl9okkvpmokx)

For the competition, open the dedicated [car 81 sketch](../tug_car_81/tug_car_81.ino) or [car 82 sketch](../tug_car_82/tug_car_82.ino) with its accompanying classifier header. Each dedicated folder can be copied and uploaded independently. The original combined sketch is also available for development. Select the physical chassis using CAR_NUMBER (82 by default, 81 also supported). Board MAC has no role. ESC D6 at 100 Hz, steering D5 at 50 Hz, microphone analog output A0, front HC-SR04 TRIG D8 and ECHO D12. The course lists GY-MAX4466 as the specified microphone and KY-037 separately as a previous class stock module. Use the analog output and a common ground.

The onboard matrix shows the chassis number alongside a dash while preparing, a check mark when armed, a right chevron during the pull, and a cross after a latched stop. Read the [Chinese](GUIDE.zh-CN.md), [Japanese](GUIDE.ja.md), or [English](GUIDE.en.md) guide for examples.

Before the whistle, allow three seconds at neutral and three clear 256 ms windows. A steady board LED means armed. Two consecutive valid windows with the shared RA0020 carrier trigger the front-range check, then forward PWM. RMS threshold 4 ADC counts, tone ratio 0.65, 12-bit ADC at 10 kHz. Similar high-frequency sounds can also trigger this detector. The dedicated builds default to automatic start. The original combined sketch supports automatic start only.

## Optional reset backup on the dedicated builds

Use automatic start for normal operation. To prepare the optional manual fallback, change ENABLE_RESET_BACKUP to true in the correct car sketch and upload. D4 open still selects automatic whistle start. Keep D4 open during power-up. Before a manual bout, hold RESET, fit a D4-to-GND jumper while RESET is held, and release RESET only after the whistle. Keep the jumper fitted until startup has sampled it. Firmware waits at least three seconds after PWM initialization before requesting forward, with the same front-range check, 10-second limit and stop latch as automatic start. A cold boot with the option enabled and jumper fitted also selects the timed start. Remove the jumper before power cycling. Check that D4 is free on the actual shield before using this option.

Place a visible ultrasonic reflector in front of the vehicle, within the HC-SR04 range throughout the test. A reading above 20 cm and up to 400 cm permits pulling. A close target, lost echo or invalid reading requests neutral and latches STOP until reset. The 20 cm threshold comes from the earlier team trial sketch. Measure stopping distance and adjust the threshold and reflector placement before competition. The independent 10-second timer starts with the first forward command. Serial x or n also stops the trial. Reset only between bouts.

Car 82 uses 1530 us provisional neutral, 1430 us measured forward onset and 1550 us steering center. Car 81 uses 1510 us candidate neutral, 1340 us measured forward onset and 1449 us steering center. Verify neutral, loaded pull pulse and vehicle direction separately for each chassis before competing.

## Evidence to collect

1. With wheels lifted, confirm neutral, wheel direction, clean arming, and no start from background noise. Test both reference sounds using the actual competition microphone.
2. Film whistle onset and first wheel motion together under representative load. The rule requires motion within one second. Two 256 ms windows consume 512 ms plus alignment, processing, ranging and ESC/mechanical delay. A missed window adds latency.
3. Film the automatic stop after ten seconds, then independently demonstrate early stopping with a cardboard target. Confirm the actual wheels stop and record the distance travelled after neutral. Remove the target after stopping and verify the drive stays off until reset.
4. Prepare the design, stopping calculation and pre-test video. The organizers check operation starting five minutes before each match.

Offline verification used the two saved RA0020 MP3 recordings and the actual sketch with simulated peripherals. Both recordings triggered the pull with a valid range. Timeout, near target, missing echo, pre-start range rejection and the latched stop were exercised. Wheel motion, braking and track performance require physical measurement.

## Match inspection record (complete separately for each car)

Arrive at least five minutes before the match, with the rope carabiner attachable to the chassis. The TA/SA checks both stop functions from five minutes before each match. Missing or malfunctioning stop functions can result in a forfeit. The referee may prohibit further participation for unsafe operation or environmental damage.

| Measurement | Car 81 | Car 82 |
|---|---|---|
| ESC neutral accepted and wheels stationary | Pending | Pending |
| Whistle A / B to first loaded wheel motion, each <= 1 s | Pending | Pending |
| Drive start to neutral at 10 s and actual wheel stop | Pending | Pending |
| Independent ultrasound stop before timeout, clearance at rest | Pending | Pending |
| Test video, design and stopping calculation | Pending | Pending |

For the stopping calculation, measure approach speed v in m/s and travel from the neutral request to rest in metres. Add v times the worst measured sensor/loop response time in seconds, the sensor-to-bumper offset, and a clearance margin. Compare the resulting distance in metres to the configured 0.20 m trigger. The software polls at 60 ms intervals with a 25 ms echo timeout; verify timing and actual stopping distance on both chassis. For whistle timing, two full windows take 0.512 s; a partial first window can add up to 0.256 s, before processing, range checking, ESC response and wheel motion. Use video to measure the whole interval.

## Acceleration update

Car 81 launches at 1240 us; car 82 at 1330 us. Both ramp to 1100 us over 300 ms, then hold. Smaller pulses request more forward throttle. START_US, PULL_US and RAMP_US configure the ramp. These are trial settings, not calibrated full throttle. Table values remain measured onset records. All stops remain active during acceleration; the 10-second deadline starts with the first drive command.
