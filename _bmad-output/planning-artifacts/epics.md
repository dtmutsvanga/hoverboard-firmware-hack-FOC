---
stepsCompleted: [1, 2, 3, 4]
status: complete
inputDocuments:
  - '_bmad-output/planning-artifacts/prd.md'
  - '_bmad-output/planning-artifacts/architecture.md'
workflowType: 'epics'
project_name: 'Push-Assist Stroller System'
user_name: 'Dan'
date: '2026-01-09'
amended: '2026-01-11'
---

# Push-Assist Stroller System - Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for the Push-Assist Stroller System, decomposing the requirements from the PRD and Architecture into implementable stories.

## Requirements Inventory

### Functional Requirements

**Motor Assist (FR1-FR5):**
- FR1: System provides constant torque assist when stroller speed exceeds engage threshold
- FR2: System applies torque in the direction of travel (follows speed sign)
- FR3: System freewheels (zero torque) when speed drops below disengage threshold
- FR4: User experiences assist during forward motion
- FR5: User experiences equal assist during backward motion

**Speed Control (FR6-FR8):**
- FR6: System limits motor assist to speeds below maximum speed cap
- FR7: System cuts torque output when speed exceeds cap (no active braking)
- FR8: System resumes assist when speed drops below cap

**User Input - Nunchuk (FR9-FR13):**
- FR9: User can increase torque output via nunchuk Y-axis
- FR10: User can lower engagement threshold via nunchuk input (eager mode)
- FR11: System detects nunchuk connection state
- FR12: System defaults to normal mode when nunchuk is idle
- FR13: System defaults to normal mode when nunchuk is disconnected

**State Management (FR14-FR17):**
- FR14: System transitions smoothly between idle and assist states
- FR15: System transitions smoothly during direction changes (through zero)
- FR16: System maintains state without jerky or abrupt behavior
- FR17: System operates in two modes: Normal (20/10 RPM) and Eager (3/2 RPM)

**Safety (FR18-FR20):**
- FR18: System limits torque output to allow user physical override
- FR19: System disengages motor when stroller is stopped
- FR20: System provides graceful degradation on input failure

**User Input - Thumb Throttle (FR23-FR28):**
- FR23: System shall read thumb throttle input from PA2 via ADC
- FR24: System shall ignore ADC values below low deadband threshold (0.8V / 993 counts)
- FR25: System shall ignore ADC values above high deadband threshold (2.1V / 2606 counts)
- FR26: System shall map working range (0.8V-2.1V) to torque boost (same as nunchuk Y-axis)
- FR27: System shall allow independent enable/disable of nunchuk and thumb throttle via compile-time macros
- FR28: When both inputs enabled, system shall use maximum of both for boost calculation

**User Input - Power Off Button (FR29-FR33):**
- FR29: System shall configure PA3 as digital input with internal pullup
- FR30: System shall detect button press (active low)
- FR31: System shall require 2 second continuous press to trigger power off
- FR32: System shall ignore button presses shorter than 2 seconds
- FR33: System shall latch OFF (shutdown) when 2 second threshold reached

### Non-Functional Requirements

**Performance (NFR1-NFR4):**
- NFR1: Assist response latency < 100ms (imperceptible to user)
- NFR2: Control loop execution - no bottlenecks at 16 kHz
- NFR3: State transition time - single loop iteration
- NFR4: Speed measurement accuracy sufficient for threshold detection

**Reliability (NFR5-NFR8):**
- NFR5: Continuous operation 30+ minutes without overheating
- NFR6: Thermal stability - no shutdown during normal use
- NFR7: Graceful degradation - safe defaults on component failure
- NFR8: State consistency - no undefined states during transitions

**Code Efficiency (NFR9-NFR12):**
- NFR9: Minimal main loop impact - push-assist logic must not block or delay
- NFR10: No floating point operations - fixed-point arithmetic only
- NFR11: No dynamic memory allocation - static buffers only
- NFR12: Follow existing codebase patterns - leverage proven implementations

**Hardware Limits (NFR13-NFR19):**
- NFR13: Hardware current limit 10A per motor
- NFR14: Software torque cap 300/1000 (~5A effective)
- NFR15: Speed cap 170 RPM (6.5 kph)
- NFR16: Normal engage threshold 20 RPM
- NFR17: Normal disengage threshold 10 RPM
- NFR18: Eager engage threshold 3 RPM
- NFR19: Eager disengage threshold 2 RPM

**Thumb Throttle & Power Button (NFR20-NFR24):**
- NFR20: Thumb throttle response latency < 100ms (match nunchuk)
- NFR21: ADC sampling uses existing 16kHz DMA (no additional CPU overhead)
- NFR22: Power button debounce - 2 second hold requirement inherently debounces
- NFR23: Thumb throttle low deadband 0-993 ADC counts (0-0.8V)
- NFR24: Thumb throttle high deadband 2606-4095 ADC counts (2.1-2.5V)

### Additional Requirements

**From Architecture:**
- Rewrite `handleStatePushCruiseControl()` in `Src/main.c`
- Add push-assist parameters section in `Inc/config.h`
- Use `static inline` helper functions for readability (zero overhead)
- Follow existing naming conventions (camelCase functions, snake_case locals, UPPER_CASE macros)
- Use existing `ABS()` macro, avoid floating point
- Set `ctrlModReq = TRQ_MODE` when assisting, `OPEN_MODE` when freewheeling

**From PRD Amendment (2026-01-11):**
- Add control method macros: `CONTROL_METHOD_NUNCHUK`, `CONTROL_METHOD_THUMB_THROTTLE`, `CONTROL_METHOD_POWER_OFF_BUTTON`
- Thumb throttle on PA2 uses existing `adc_buffer.l_tx2` (already sampled at 16kHz)
- Power off button on PA3 requires GPIO config change (digital input with pullup vs ADC)
- Requires `DEBUG_SERIAL_USART2` disabled when thumb throttle enabled (PA2 GPIO conflict)

### FR Coverage Map

| FR | Epic | Description |
|----|------|-------------|
| FR1 | 1 | Constant torque above engage threshold |
| FR2 | 1 | Torque follows direction of travel |
| FR3 | 1 | Freewheel below disengage threshold |
| FR4 | 1 | Forward motion assist |
| FR5 | 1 | Backward motion assist |
| FR6 | 1 | Speed cap limit |
| FR7 | 1 | Torque cut above cap |
| FR8 | 1 | Resume assist below cap |
| FR9 | 2 | Nunchuk Y-axis torque increase |
| FR10 | 2 | Nunchuk lowers engagement threshold |
| FR11 | 2 | Detect nunchuk connection state |
| FR12 | 2 | Normal mode when nunchuk idle |
| FR13 | 2 | Normal mode on nunchuk disconnect |
| FR14 | 1 | Smooth idle/assist transitions |
| FR15 | 1 | Smooth direction changes |
| FR16 | 1 | No jerky behavior |
| FR17 | 2 | Normal/Eager mode operation |
| FR18 | 1 | Torque allows physical override |
| FR19 | 1 | Disengage when stopped |
| FR20 | 2 | Graceful degradation on failure |
| FR23 | 3 | Read thumb throttle from PA2 ADC |
| FR24 | 3 | Thumb throttle low deadband |
| FR25 | 3 | Thumb throttle high deadband |
| FR26 | 3 | Map thumb throttle to torque boost |
| FR27 | 3 | Independent enable/disable macros |
| FR28 | 3 | Max of both inputs when both enabled |
| FR29 | 3 | Configure PA3 as digital input |
| FR30 | 3 | Detect button press (active low) |
| FR31 | 3 | 2 second press for power off |
| FR32 | 3 | Ignore presses < 2 seconds |
| FR33 | 3 | Latch OFF on 2s threshold |

## Epic List

### Epic 1: Core Push-Assist Control

**Goal:** Parent can push the stroller and feel assisted in either direction, with natural stopping behavior. Replaces existing buggy/overcomplicated `handleStatePushCruiseControl()` with a clean constant-torque model.

**FRs covered:** FR1, FR2, FR3, FR4, FR5, FR6, FR7, FR8, FR14, FR15, FR16, FR18, FR19

**Implementation Notes:**
- **REMOVE** existing buggy `handleStatePushCruiseControl()` code entirely
- **REPLACE** with new constant-torque algorithm from Architecture document
- Add push-assist configuration parameters to `Inc/config.h`
- Use `static inline` helpers for readability
- Core logic: `moving = TRQ_MODE + constant torque`, `stopped = OPEN_MODE`

**Standalone:** Delivers complete basic push-assist. No nunchuk interaction required - uses hardcoded Normal mode thresholds (20/10 RPM).

---

### Epic 2: Nunchuk Boost & Safety

**Goal:** Parent can request more torque for inclines via nunchuk, and system fails safely if nunchuk disconnects.

**FRs covered:** FR9, FR10, FR11, FR12, FR13, FR17, FR20

**Implementation Notes:**
- Add nunchuk Y-axis reading to modulate torque output
- Implement Eager mode (3/2 RPM thresholds) when nunchuk active
- Handle nunchuk disconnect → safe default to Normal mode
- Graceful degradation on any input failure

**Builds on Epic 1:** Adds variable behavior on top of working core algorithm.

---

### Epic 3: Alternative Input Methods

**Goal:** Provide alternative/supplementary control methods (thumb throttle and power off button) that can work independently or alongside nunchuk, with compile-time enable/disable macros.

**FRs covered:** FR23, FR24, FR25, FR26, FR27, FR28, FR29, FR30, FR31, FR32, FR33

**Implementation Notes:**
- Add control method macros to `Inc/config.h`
- Thumb throttle reads existing `adc_buffer.l_tx2` (PA2 already sampled)
- Power off button requires PA3 GPIO reconfiguration (digital input with pullup)
- Disable `DEBUG_SERIAL_USART2` when thumb throttle enabled
- When both nunchuk and thumb throttle enabled, use max of both for boost

**Standalone:** Can be implemented independently of Epic 2 (nunchuk). If nunchuk disabled, thumb throttle provides sole boost input.

---

## Epic 1: Core Push-Assist Control

Parent can push the stroller and feel assisted in either direction, with natural stopping behavior. Replaces existing buggy/overcomplicated code with a clean constant-torque model.

### Story 1.1: Push-Assist Configuration

**As a** developer,
**I want** push-assist parameters defined in config.h,
**So that** the algorithm thresholds and limits can be easily tuned without code changes.

**Acceptance Criteria:**

**Given** the `Inc/config.h` file
**When** the push-assist section is added
**Then** the following parameters are defined:
- `PA_ENGAGE_NORMAL` = 20 (RPM)
- `PA_DISENGAGE_NORMAL` = 10 (RPM)
- `PA_ENGAGE_EAGER` = 3 (RPM)
- `PA_DISENGAGE_EAGER` = 2 (RPM)
- `PA_SPEED_CAP` = 170 (RPM)
- `PA_TORQUE_BASE` = 150 (of 1000)
- `PA_TORQUE_MAX` = 300 (of 1000)
- `PA_NUNCHUK_DEADBAND` = 10

**And** parameters follow existing naming convention (prefix + UPPER_CASE)
**And** parameters include descriptive comments
**And** code compiles without errors

---

### Story 1.2: Core Constant Torque Algorithm

**As a** parent pushing the stroller,
**I want** constant torque assist when moving and freewheeling when stopped,
**So that** pushing feels effortless while maintaining natural control.

**Acceptance Criteria:**

**Given** the stroller is stationary (speed < PA_DISENGAGE_NORMAL)
**When** I start pushing
**Then** the motor freewheels (OPEN_MODE) until speed exceeds PA_ENGAGE_NORMAL

**Given** speed exceeds PA_ENGAGE_NORMAL (20 RPM)
**When** I continue pushing forward
**Then** motor provides constant torque (PA_TORQUE_BASE) in direction of travel (TRQ_MODE)
**And** assist feels consistent, not surging or pulsing (FR16)

**Given** I am pushing backward
**When** speed exceeds PA_ENGAGE_NORMAL in reverse
**Then** motor provides equal assist in reverse direction (FR5)

**Given** I stop pushing and speed drops below PA_DISENGAGE_NORMAL (10 RPM)
**When** the stroller coasts to a stop
**Then** motor disengages and wheels freewheel (OPEN_MODE) (FR3, FR19)

**Given** I change direction (forward to backward or vice versa)
**When** speed crosses zero
**Then** motor freewheels through transition, re-engages in new direction (FR15)
**And** no jerk or motor fighting the change (FR16)

**Given** speed approaches PA_SPEED_CAP (170 RPM)
**When** speed exceeds the cap
**Then** torque output cuts to zero (FR7)
**And** no active braking applied - stroller coasts naturally

**Given** speed was above cap and drops below
**When** I continue pushing at normal pace
**Then** assist resumes automatically (FR8)

**Given** I grab the wheels to stop suddenly
**When** I apply manual braking force
**Then** I can overpower the motor (torque capped at PA_TORQUE_BASE) (FR18)

**Implementation Notes:**
- **REMOVE** existing buggy `handleStatePushCruiseControl()` entirely
- **REPLACE** with clean constant-torque algorithm
- Use `static inline` helpers for readability
- No floating point, no dynamic allocation

---

## Epic 2: Nunchuk Boost & Safety

Parent can request more torque for inclines via nunchuk, and system fails safely if nunchuk disconnects.

### Story 2.1: Nunchuk Boost & Eager Mode

**As a** parent pushing uphill or needing extra help,
**I want** to push the nunchuk joystick forward for more torque and quicker response,
**So that** I get instant, stronger assist when I need it.

**Acceptance Criteria:**

**Given** nunchuk Y-axis is below PA_NUNCHUK_DEADBAND
**When** I push the stroller
**Then** system operates in Normal mode (20/10 RPM thresholds, PA_TORQUE_BASE) (FR12)

**Given** nunchuk Y-axis exceeds PA_NUNCHUK_DEADBAND
**When** I push the joystick forward
**Then** system switches to Eager mode (3/2 RPM thresholds) (FR10, FR17)
**And** torque increases proportionally: PA_TORQUE_BASE + (Y * (PA_TORQUE_MAX - PA_TORQUE_BASE) / 127) (FR9)

**Given** I am in Eager mode with boost
**When** I release the joystick to center
**Then** system returns to Normal mode immediately (FR12)
**And** transition is smooth, no jerk

**Given** I am at low speed (e.g., 5 RPM) in Normal mode
**When** I push joystick forward (Eager mode)
**Then** assist engages immediately (3 RPM threshold met) (FR10)

---

### Story 2.2: Safe Defaults on Disconnect

**As a** parent using the stroller,
**I want** the system to continue working safely if the nunchuk disconnects,
**So that** I'm not stranded with a non-functional stroller.

**Acceptance Criteria:**

**Given** nunchuk is connected and working
**When** the cable disconnects or communication fails
**Then** system defaults to Normal mode (20/10 RPM, PA_TORQUE_BASE) (FR13)
**And** assist continues working - does not suddenly stop (FR20)

**Given** nunchuk was in Eager mode with boost
**When** disconnect occurs
**Then** torque drops to PA_TORQUE_BASE (not zero)
**And** thresholds revert to Normal (20/10 RPM)

**Given** nunchuk reconnects after disconnect
**When** valid data is received again
**Then** system resumes responding to nunchuk input normally

**Implementation Notes:**
- Leverage existing nunchuk timeout detection in firmware
- Default behavior = Normal mode (safe, not disabled)

---

## Epic 3: Alternative Input Methods

Provide alternative/supplementary control methods (thumb throttle and power off button) that can work independently or alongside nunchuk, with compile-time enable/disable macros.

### Story 3.1: Control Method Configuration Macros

**As a** developer,
**I want** compile-time macros to enable/disable each control method independently,
**So that** I can configure the exact input methods needed for my build.

**Acceptance Criteria:**

**Given** the `Inc/config.h` file
**When** the control method section is added
**Then** the following macros are defined:
- `CONTROL_METHOD_NUNCHUK` - Enable/disable nunchuk Y-axis boost
- `CONTROL_METHOD_THUMB_THROTTLE` - Enable/disable thumb throttle ADC boost
- `CONTROL_METHOD_POWER_OFF_BUTTON` - Enable/disable PA3 power off button

**And** each macro can be commented/uncommented independently (FR27)
**And** code compiles with any combination of enabled/disabled macros
**And** when `CONTROL_METHOD_THUMB_THROTTLE` is enabled, `DEBUG_SERIAL_USART2` must be disabled (add compile-time check)

**Given** both `CONTROL_METHOD_NUNCHUK` and `CONTROL_METHOD_THUMB_THROTTLE` are enabled
**When** boost is calculated
**Then** system uses maximum of both inputs for torque boost (FR28)

**Implementation Notes:**
- Add macros in push-assist configuration section of config.h
- Add `#if defined(CONTROL_METHOD_THUMB_THROTTLE) && defined(DEBUG_SERIAL_USART2)` error check
- Default: All three enabled for maximum flexibility

---

### Story 3.2: Thumb Throttle ADC Input

**As a** parent pushing the stroller,
**I want** to use a thumb throttle for torque boost instead of/alongside nunchuk,
**So that** I have an alternative, simpler control method.

**Acceptance Criteria:**

**Given** `CONTROL_METHOD_THUMB_THROTTLE` is enabled
**When** the system initializes
**Then** PA2 is configured for ADC input (already done by existing ADC2 setup)
**And** `DEBUG_SERIAL_USART2` is disabled (GPIO conflict)

**Given** thumb throttle ADC value is below 993 counts (0.8V)
**When** the boost is calculated
**Then** thumb throttle contribution is zero (low deadband) (FR24)

**Given** thumb throttle ADC value is above 2606 counts (2.1V)
**When** the boost is calculated
**Then** thumb throttle contribution is maximum/clamped (high deadband) (FR25)

**Given** thumb throttle ADC value is in working range (993-2606 counts)
**When** I press the throttle
**Then** value is mapped linearly to torque boost (FR26)
**And** boost calculation matches nunchuk Y-axis behavior
**And** response latency < 100ms (NFR20)

**Given** both nunchuk and thumb throttle are enabled and active
**When** both have non-zero boost values
**Then** system uses the maximum of both inputs (FR28)

**Given** `CONTROL_METHOD_THUMB_THROTTLE` is disabled
**When** code compiles
**Then** all thumb throttle code is excluded (no overhead)

**Implementation Notes:**
- Read from `adc_buffer.l_tx2` (PA2 already sampled at 16kHz by ADC2)
- Add deadband constants: `PA_THUMB_DEADBAND_LOW` = 993, `PA_THUMB_DEADBAND_HIGH` = 2606
- Map working range to 0-127 (same scale as nunchuk Y-axis)
- Use `#ifdef CONTROL_METHOD_THUMB_THROTTLE` guards

---

### Story 3.3: Power Off Button

**As a** user of the stroller system,
**I want** a dedicated button to power off the board with a long press,
**So that** I can safely shut down without accessing the main power button.

**Acceptance Criteria:**

**Given** `CONTROL_METHOD_POWER_OFF_BUTTON` is enabled
**When** the system initializes
**Then** PA3 is configured as digital input with internal pullup (FR29)
**And** PA3 is NOT used for ADC (reconfigure from analog to digital)

**Given** the button is not pressed
**When** PA3 is read
**Then** value is HIGH (pullup to VCC)

**Given** the button is pressed (connected to ground)
**When** PA3 is read
**Then** value is LOW (active low) (FR30)

**Given** button is pressed for less than 2 seconds
**When** released before 2 seconds
**Then** no action is taken (FR32)
**And** press timer resets

**Given** button is pressed and held
**When** 2 second threshold is reached
**Then** system latches OFF (shutdown) (FR31, FR33)
**And** shutdown is immediate upon reaching threshold (don't wait for release)

**Given** `CONTROL_METHOD_POWER_OFF_BUTTON` is disabled
**When** code compiles
**Then** PA3 remains as ADC input (default behavior)
**And** all power off button code is excluded

**Implementation Notes:**
- Add GPIO init for PA3: `GPIO_MODE_INPUT` with `GPIO_PULLUP`
- Use a counter in main loop (1ms tick) to track press duration
- 2 seconds = 2000 counts at 1ms loop
- Call existing power-off/shutdown function when threshold reached
- Use `#ifdef CONTROL_METHOD_POWER_OFF_BUTTON` guards

---
