# AGENTS.md

Instructions for agents (and humans) working on this repository: firmware for the Zénith (Polytechnique Montréal) motor test bench.

## Project

Two Teensy 4.1 boards and a Raspberry Pi:

- **Control Teensy**: operator interface; sends commands over RF (nRF24L01) at 20 Hz.
- **Sensor Teensy**: drives the ESC, reads the sensors (load cell, ICM-42688-P IMU, PM08-CAN), and owns all safety logic.
- **Raspberry Pi**: receives JSON telemetry over UART and shows the dashboard. Read-only.

The team often doesn't have the hardware on hand, so everything must be testable on a PC with mocks.

## Source of truth: `docs/`

Read these before writing code. The code implements these documents, not the other way around. The docs are written in French.

| File | Content |
| --- | --- |
| `srs-commandes-puissance.md` | Power commands, seq/ack, link watchdog, ramp limit (CMD, TX, ACK, LNK, ST, SAF) |
| `srs-sequence-demarrage.md` | Startup, precharge, arming, abort (PRE, TST, HSK, PWR, ARM, ABT) |
| `srs-tst-01-peripheriques.md` | Per-peripheral self-test checks (TST-01.*) |
| `srs-securite-fonctionnement.md` | Runtime shutdown conditions (SUR, ARR, COM, ELE, MEC, MAT) |
| `messages.md` | Byte-level format of every message, states, error codes |
| `telemetrie.md` | Per-sensor acquisition, SD log, telemetry distribution |

Rules:

- Cite the ID of the requirement you implement in a comment (`// LNK-01`) and in the PR description.
- If behavior changes, update the SRS in the same PR.
- Values marked « à confirmer » (to be confirmed) go in named constants, never hard-coded, with the requirement ID in a comment.
- Never resolve an open point from an SRS on your own: flag it in the PR.

## Document implementation choices

This is a project priority: every choice must be explainable. For any non-trivial decision (structure, algorithm, library, protocol, value), the PR or the relevant doc states:

1. **What was chosen.**
2. **Why.**
3. **Which alternatives were rejected, and why.**

Example: "Telemetry to the control Teensy rides in the nRF24 ack payload instead of a separate transmission, because the nRF24 is half-duplex, so the sensor Teensy stays in receive mode at all times. Rejected: switching TX/RX every cycle (commands are lost while transmitting)."

## Target architecture

```
firmware/src/
  communication/   HAL: bus interfaces (spi, i2c, can, uart, pwm, gpio) + Teensy impl + mocks
  sensors/         Chip drivers (load_cell, imu, power_module, esc, nrf24), built on the HAL
  protocol/        Structs and enums from messages.md; header-only, shared
  safety/          State machine, link watchdog, shutdown conditions
  logging/         SD log
  modules/         sensor_teensy, control_teensy, telemetry_pi; wire everything together
```

The current code still lives under `firmware/lib/` (ESCcontroller, IMUReader, NRF24Link, PowerMonitor, LoadCellReader) and will be migrated to this layout.

Dependency rules:

- Dependencies only point downward: `modules` → `safety` / `logging` → `sensors` → `communication`.
- `protocol/` can be used by every layer and depends on nothing.
- Modules never include each other; they communicate through the structs in `protocol/`.
- Outside `communication/`, never call `Wire`, `SPI`, `FlexCAN_T4`, `Serial`, or hardware registers directly: go through the HAL interface. The one exception is the WDOG, which is configured in `safety/`.

## Build and test

```bash
# Firmware (PlatformIO, from firmware/)
pio run -e teensy41
pio run -e teensy41 -t upload

# With Docker (CI / no local install)
make build

# Native tests (coming: env:native + Unity)
pio test -e native
```

Planned environments: `env:sensor`, `env:control`, `env:native`. Only `teensy41` exists today.

Before opening a PR, the firmware must build without warnings (`-Wall`) and the native tests must pass.

## Safety rules (non-negotiable)

- **Nothing blocks the critical path.** No `delay()`, no busy-waiting, and no blocking SD or `Serial` writes before command handling and monitoring.
- **Loop order:** monitoring → commands (kill first) → telemetry → logging. Telemetry and logging are best-effort: skip a send rather than wait.
- **Keep ISRs short.** They set a flag or force the motor output to 0. No SPI, I2C, CAN, Serial, or SD inside an ISR.
- **The kill is never filtered:** no seq check, no ramp limit, no delay (SAF-03, ARR-03).
- **Never auto-restart.** After a shutdown, only the full startup sequence can resume operation (ST-05, ARR-04).
- **Fail-safe defaults.** At boot: disarmed, setpoint 0, contactor open (ST-01, PRE-02). Invalid or missing data counts as out of bounds (SUR-04).
- **Don't change a safety threshold** without updating the matching SRS.

## Code conventions

- C++ (Teensyduino), one header per interface (`.hpp`).
- Units in names: `timeout_ms`, `t_us`, `setpoint_pct`, `voltage_v`.
- Constants are `constexpr`; no magic numbers.
- Time: compare with unsigned subtraction (`millis() - last_ms > TIMEOUT_MS`), never with addition.
- Message structs: `__attribute__((packed))` + `static_assert(sizeof(...) == N)`, matching `messages.md`.
- Variables shared with an ISR are `volatile`; copy structs between `noInterrupts()` / `interrupts()`.
- No dynamic allocation after `setup()`.
- Every driver has a mock in the HAL, and every new feature ships with native tests.
- Identifiers in English; docs in `docs/` stay in French.

## Git

- One branch per topic, PR into `main`, reviewed before merge.
- Commit prefixes: `feat:`, `fix:`, `docs:`, `test:`, `refactor:`.
- Never commit `.pio/` or `.vscode/`.
- PlatformIO dependencies go in `firmware/platformio.ini` (`lib_deps`), with a pinned version.
