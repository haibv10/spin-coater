# Spin Coater

**English** | [Tiếng Việt](README_VN.md)

A two-controller spin coater for thin-film deposition: an **STM32F103** handles real-time
closed-loop motor control, while an **ESP32-S3** with an LVGL touchscreen provides the
user interface. The two boards talk to each other over a framed UART protocol.

**Author:** Bùi Văn Hải

---

## 1. Overview

<p align="center">
  <img src="img/menu_man_hinh.png" alt="Spin Coater HMI main menu" width="480">
</p>

A spin coater deposits a uniform thin film on a substrate by dispensing liquid onto a
spinning chuck; film thickness is governed by the spin speed and duration. This project
implements the full control system for such a machine.

The design splits responsibility across two microcontrollers:

- **STM32F103C8 (Controller / "Slave")** — runs the real-time control loop: reads the
  motor speed from a quadrature encoder, drives a brushless motor through an ESC, and
  regulates RPM with a PID controller and a linear ramp profiler. It also switches the
  vacuum pump and UV LED relays and the buzzer.
- **ESP32-S3 (HMI / "Master")** — drives a 320×480 capacitive touch display through
  LVGL (SquareLine-generated UI) and an EC11 rotary encoder. It lets the operator pick a
  mode, enter setpoints, start/stop the process, and watch the measured RPM in real time.

The operator sets a target on the ESP32; the ESP32 sends commands to the STM32; the STM32
runs the motor and streams back the measured RPM at 20 Hz for live display.

Three operating modes are supported:

| Mode | Description |
| --- | --- |
| **Analog** | Spin at a single target RPM set with the rotary encoder. |
| **Digital** | Spin at a single target RPM entered numerically on the touchscreen. |
| **Ramp** | A multi-stage recipe: ramp to `min_rpm`, hold for `time_min`, ramp to `max_rpm`, hold for `time_max`, then ramp down and finish. |

---

## 2. Hardware

| Component | Part / Detail |
| --- | --- |
| Control MCU | STM32F103C8 ("Blue Pill"), 72 MHz, SPL |
| HMI MCU | ESP32-S3 (N16R8V, 16 MB flash / 8 MB PSRAM) |
| Display | 320×480 IPS, AXS15231B capacitive touch controller |
| Motor | Brushless DC motor driven via an ESC (servo-style 50 Hz PWM) |
| Speed sensor | Quadrature encoder, 16 lines × 4 (quadrature) = 64 CPR |
| Rotary input | EC11 rotary encoder (setpoint entry on ESP32) |
| Actuators | Vacuum pump + UV LED, each on a relay channel |
| Feedback | Piezo buzzer for audible status/confirmation |

### STM32F103 pin map

| Function | Pin | Peripheral |
| --- | --- | --- |
| ESC PWM | PA8 | TIM1_CH1 (PWM output) |
| Encoder A / B | PA1 / PA0 | TIM2 (encoder interface, x4) |
| Buzzer | PA7 | GPIO |
| Relay 1 — UV LED | PB0 | GPIO |
| Relay 2 — Vacuum pump | PB1 | GPIO |
| UART1 | — | Debug / log @ 115200 |
| UART2 | — | Link to ESP32 @ 115200 |

### ESC PWM timing

TIM1 is clocked to a 1 MHz tick (`Prescaler = 24-1` from 24 MHz) with a period of 60000
ticks (20 ms → 50 Hz servo frame). Duty is expressed in timer ticks:

- `ESC_MIN_PWM = 4500` (~1.5 ms, idle / armed)
- `ESC_MAX_PWM = 6000` (~2.0 ms, full speed)

---

## 3. Software Architecture

![System architecture](img/system-architecture.svg)

### STM32 side (`stm32f103c8/`)

Bare-metal super-loop on the SPL. `main.c` runs `app_setup()` once, then loops
`app_loop()` forever. Layered structure:

- **`drivers/`** — low-level peripheral wrappers: `uart` (interrupt-driven RX ring buffer,
  driver-object API for UART1/2/3) and `delay` (SysTick tick counter, `get_tick()`).
- **`components/`** — device drivers: `esc_driver` (PWM + RPM↔duty conversion),
  `encoder` (TIM2 quadrature, RPM measurement), `io_driver` (relays + non-blocking
  buzzer FSM).
- **`control_algorithms/`** — `pid` (velocity/position-form PID with feed-forward bias and
  anti-windup) and `kalmanfilter` (available for speed-signal filtering).
- **`modes/`** — `coater` (PID setpoint + ramp profiler) and `mode` (per-mode logic and the
  Ramp state machine).
- **`protocol_uart/`** — `message` (frame build/parse + CRC), `convert` (byte↔scalar
  unions), `protocol_uart` (command dispatch + telemetry).

### ESP32 side (`esp32_s3_hmi/`)

Arduino framework via PlatformIO. `main.cpp` initialises LVGL and the UI, then in `loop()`
pumps LVGL and, based on the active mode/state, reads incoming frames and updates the
measured-RPM widgets.

- **`lib/lib_screen/`** — SquareLine Studio-exported UI (`ui.c`, screens, fonts, images).
- **`src/lvgl/`** — LVGL glue: display init, `AXS15231B` touch driver, pin/display config.
- **`src/`** — application logic: `app_control` (mode/state machines), `ec11` (rotary
  input), `frame_io` + `msg` + `convert` (the protocol, mirroring the STM32 side), and the
  per-screen controllers (`screen_analog`, `screen_digital`, `screen_ramp`).

---

## 4. Features

- Three spin modes: **Analog**, **Digital**, and multi-stage **Ramp** recipes.
- Closed-loop RPM control with PID + linear ramp profiling for smooth acceleration and
  deceleration.
- Logistic (non-linear) RPM↔duty feed-forward model so the PID only trims a good initial
  estimate.
- Live measured-RPM telemetry streamed to the HMI at ~20 Hz (every 50 ms).
- Safe mode switching: changing modes while spinning first ramps the motor down.
- Emergency-friendly ramp-down that can interrupt an in-progress ramp-up.
- Relay control for vacuum pump and UV LED from the touchscreen.
- Non-blocking buzzer FSM for audible status (ready, mode change, start/stop, ramp finish).
- Robust UART framing with CRC-16 (Modbus) validation and buffer-overflow recovery.
- Touchscreen + rotary-encoder HMI built with LVGL 9.

---

## 5. Control Algorithm

### Measuring speed

The encoder runs in TIM2 quadrature x4 mode (64 counts/rev). `encoder_get_pulse_count()`
samples the counter over a window (default 50 ms), handles 16-bit counter wrap-around with
signed differencing, rejects windows that are too short (<80% of the requested time), and
rescales pulse counts to the nominal window. `encoder_get_rpm()` converts:

```
rpm = (pulses / CPR) * (60000 / sample_time_ms)
```

### Feed-forward: logistic RPM ↔ duty model

The motor+ESC response is non-linear, so instead of a linear map the driver uses a logistic
curve fitted to the measured characteristic:

```
rpm  = L / (1 + exp(-K * (duty - D0)))          // esc_duty_to_rpm_logistic
duty = D0 - ln(L/rpm - 1) / K                    // esc_rpm_to_duty_logistic
```

with `L = 5777`, `K = 0.00945`, `D0 = 4757`. This gives a good open-loop `baseDuty` estimate
for any target RPM; the PID then only has to correct the residual error.

### PID controller

`pid_update()` computes a correction on top of the feed-forward bias:

```
output = ubias(baseDuty) + (P + I + D)
```

- **P** = `kp · error`, where `error = setpoint − measured`.
- **I** = accumulated `ki · error · dt`, with anti-windup: the integral is back-calculated so
  the total correction never exceeds `max_output`.
- **D** = derivative on measurement (`−kd · Δmeasured / dt`) to avoid derivative kick, with
  `dt` clamped to ≤10 ms to reject spikes.
- Optional deadband suppresses P/I within a small error band.

Default tuning (`main.c`): `kp = 0.1`, `ki = 1.0`, `kd = 0.0`, `max_output = 1500`.

When the setpoint drops to ~0, the motor is set to idle and the PID integrator/derivative
are reset to prevent windup.

### Ramp profiling

Speed changes are never step commands to the PID. `coater_ramp_up()` / `coater_ramp_down()`
generate a **linear setpoint profile** from the current RPM to the target across a number of
`steps` over `ramp_time_ms`, advancing one step at a time in `coater_update_ramp_profile()`.
The PID tracks this moving setpoint, so acceleration is smooth and bounded. A ramp-down is
allowed to pre-empt an in-progress ramp-up for safe stops.

### Ramp-mode state machine (`mode_ramp_update`)

```
RAMP_IDLE
  └─(start)→ RAMP_TO_MIN ──(reached)──▶ RAMP_HOLD_MIN ──(time_min)──▶
             RAMP_TO_MAX ──(reached)──▶ RAMP_HOLD_MAX ──(time_max)──▶
             RAMP_FINISH ──(stopped)──▶ (send RAMP_FINISH) → RAMP_IDLE

  (stop) ──▶ RAMP_STOP ──(stopped)──▶ RAMP_IDLE   // ramps down, no FINISH sent
```

Analog/Digital modes reuse the same primitives: given a new target, they ramp up or down to
it depending on the sign of the error (with a 15 RPM deadzone). RPM setpoints are clamped to
`[0, 6000]`.

---

## 6. UART Protocol

Both boards exchange fixed-layout, little-endian frames over UART at **115200 8-N-1**.

### Frame format

```
┌─────────────┬──────────┬────────┬───────────────┬──────────────┐
│ Start (2 B) │ Type (1) │ Len(1) │ Data (0..12 B)│  CRC-16 (2 B) │
│   0xAA55    │  0xNN    │  = N   │   payload     │  Modbus, LE   │
└─────────────┴──────────┴────────┴───────────────┴──────────────┘
```

- **Start** — `0xAA55` marker (little-endian on the wire: `0x55 0xAA`).
- **Type** — message type (see tables below).
- **Len** — payload length in bytes (0–12).
- **Data** — payload; multi-byte scalars are little-endian (`uint16` RPM/time, `float` RPM).
- **CRC-16** — Modbus CRC (poly `0xA001`, init `0xFFFF`) over `Start..Data`.

The STM32 receiver (`uart_parser_from_esp`) resynchronises on the start marker, validates the
length, checks the CRC, and recovers from overflow by resetting its buffer. It processes at
most 3 frames per loop pass to avoid starving the control loop.

### Master → Slave (ESP32 → STM32)

| Type | Value | Payload | Meaning |
| --- | --- | --- | --- |
| `SET_ANALOG` | `0x10` | — | Enter Analog mode |
| `SET_DIGITAL` | `0x11` | — | Enter Digital mode |
| `SET_RAMP` | `0x12` | — | Enter Ramp mode |
| `RELAY_VACUUM_ON` | `0x13` | — | Vacuum pump on |
| `RELAY_VACUUM_OFF` | `0x14` | — | Vacuum pump off |
| `RELAY_UV_LED_ON` | `0x15` | — | UV LED on |
| `RELAY_UV_LED_OFF` | `0x16` | — | UV LED off |
| `ANALOG_START` | `0x20` | `u16 rpm` | Start Analog at RPM |
| `ANALOG_STOP` | `0x21` | — | Stop Analog |
| `DIGITAL_START` | `0x40` | `u16 rpm` | Start Digital at RPM |
| `DIGITAL_STOP` | `0x41` | — | Stop Digital |
| `RAMP_START` | `0x60` | `u16 min_rpm, u16 t_min(s), u16 max_rpm, u16 t_max(s)` | Start Ramp recipe |
| `RAMP_STOP` | `0x61` | — | Stop Ramp |

> Note: in `RAMP_START` the two time fields are sent in **seconds** and converted to
> milliseconds on the STM32.

### Slave → Master (STM32 → ESP32)

| Type | Value | Payload | Meaning |
| --- | --- | --- | --- |
| `ANALOG_MRPM_UPDATE` | `0x31` | `float rpm` | Measured RPM (Analog) |
| `DIGITAL_MRPM_UPDATE` | `0x51` | `float rpm` | Measured RPM (Digital) |
| `RAMP_MRPM_UPDATE` | `0x71` | `float rpm` | Measured RPM (Ramp) |
| `RAMP_FINISH` | `0x72` | 1 B | Ramp recipe completed |

Telemetry (`MRPM_UPDATE`) is sent every 50 ms while a mode is running.

---

## 7. Directory Structure

```
spin_coater/
├── stm32f103c8/                 # Real-time controller firmware (Keil MDK-ARM, SPL)
│   ├── core/                       # main.c/.h — app_setup() + app_loop()
│   ├── drivers/                    # uart, delay (SysTick)
│   ├── components/                 # esc_driver, encoder, io_driver (relay/buzzer)
│   ├── control_algorithms/         # pid, kalmanfilter
│   ├── modes/                      # coater (PID+ramp), mode (mode logic + ramp FSM)
│   ├── protocol_uart/              # message (framing/CRC), convert, protocol_uart
│   ├── board_config/Inc/           # pinmap.h
│   └── MDK-ARM/                    # Keil project (spin_coating.uvprojx), RTE, SPL
│
└── esp32_s3_hmi/                    # Touchscreen HMI firmware (PlatformIO / Arduino)
    ├── platformio.ini              # env: esp32-s3-n16r8v, LVGL 9.3, GFX, ESP32Encoder
    ├── include/                    # app_control, frame_io, msg, convert, ec11, screen_* headers
    ├── src/
    │   ├── main.cpp                # setup()/loop(), telemetry → UI
    │   ├── app_control.cpp         # mode/state machines
    │   ├── ec11.cpp                # rotary encoder input
    │   ├── frame_io/msg/convert    # protocol (mirrors STM32)
    │   ├── screen_*.cpp            # per-screen controllers
    │   └── lvgl/                    # LVGL init + AXS15231B touch driver + display config
    └── lib/lib_screen/            # SquareLine-exported UI (ui.c, screens, fonts, images)
```

---

## 8. Build & Flash

### STM32 (Keil MDK-ARM)

1. Open `stm32f103c8/MDK-ARM/spin_coating.uvprojx` in Keil µVision (MDK-ARM).
2. Target device: **STM32F103C8**. The project uses the Standard Peripheral Library (SPL);
   dependencies are managed through the RTE folder.
3. Build (F7) and flash via ST-Link (Load / F8).

### ESP32-S3 (PlatformIO)

```bash
cd esp32_s3_hmi

# Build
pio run

# Build, upload, and open the serial monitor
pio run -t upload -t monitor
```

Environment and dependencies are defined in `platformio.ini`:

- Board: `esp32-s3-n16r8v`, framework: `arduino`, monitor speed: `115200`
- Libraries: `lvgl@^9.3.0`, `GFX Library for Arduino@1.5.0`, `ESP32Encoder@^0.11.8`

### Wiring the link

Cross-connect the ESP32 UART to the STM32 **UART2** (TX↔RX, RX↔TX) and share a common
ground. Both run at 115200 8-N-1. STM32 **UART1** is available for debug logging.

---

## 9. Demonstration

A typical Ramp run:

1. On the HMI, select **Ramp** mode → ESP32 sends `SET_RAMP` (`0x12`); STM32 beeps.
2. Enter `min_rpm`, `time_min`, `max_rpm`, `time_max` and press start → ESP32 sends
   `RAMP_START` (`0x60`) with the 8-byte payload.
3. STM32 ramps to `min_rpm`, holds, ramps to `max_rpm`, holds, then ramps down, streaming
   `RAMP_MRPM_UPDATE` (`0x71`) every 50 ms so the display shows live RPM.
4. On completion the STM32 sends `RAMP_FINISH` (`0x72`) and beeps twice.

Analog/Digital modes are simpler: pick a target (rotary encoder or numeric entry), start,
and watch the measured RPM converge to the setpoint.

### Results — thin films spin-coated with the machine

Thin films patterned on glass substrates, spun on this coater:

| ![PTIT logo](img/output_logo_PTIT.png) | ![Mercedes logo](img/output_logo_mercedes.png) | ![Bear logo](img/output_logo_con_gau.png) |
| :---: | :---: | :---: |
| **PTIT logo** — university logo | **Mercedes logo** | **Bear logo** |

### Video

[![Watch the demo](img/background_spin.jpeg)](https://github.com/Rephania/spin-coater-/releases/download/v1.0.0/demo_spin_coater.mp4)

▶️ Click the image above to watch the full demo (hosted on the
[Releases](https://github.com/Rephania/spin-coater-/releases/tag/v1.0.0) page).

<!--
  To play the video INLINE instead of a click-to-download link, drag the .mp4 into
  any GitHub issue/comment box, copy the generated
  https://github.com/user-attachments/assets/... URL, and paste it on its own line here.
-->


---

## 10. Lessons Learned

- **Feed-forward beats brute-force PID.** Fitting a logistic RPM↔duty curve and letting the
  PID trim only the residual made tuning far easier and the response much better than a
  linear map with a heavy integral term.
- **Never step the setpoint.** Feeding the PID a linearly ramped setpoint instead of a target
  step eliminated overshoot and mechanical stress at start/stop.
- **Anti-windup matters.** Back-calculating the integral against `max_output` (and resetting
  it near zero RPM) prevented the long recovery lags typical of a saturated integrator.
- **Robust framing pays off.** A start marker + length + CRC-16, plus buffer-overflow reset
  and a per-loop frame cap, kept the UART link reliable without ever blocking the control
  loop.
- **Split concerns across MCUs.** Keeping hard real-time control on the STM32 and the
  graphics-heavy HMI on the ESP32-S3 let each side stay responsive.
- **Non-blocking everything.** A buzzer FSM and tick-based scheduling (`get_tick()`) kept the
  super-loop free of `delay()` calls that would have stalled control.

---

## License

Released under the [MIT License](LICENSE) © 2026 Bùi Văn Hải.
