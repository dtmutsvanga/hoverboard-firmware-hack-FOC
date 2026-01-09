---
stepsCompleted: [1, 2, 3, 4, 7, 9, 10, 11]
inputDocuments:
  - '_bmad-output/prd-push-assist-stroller.md'
  - '_bmad-output/analysis/brainstorming-session-2026-01-09.md'
workflowType: 'prd'
lastStep: 7
documentCounts:
  briefs: 0
  research: 0
  brainstorming: 1
  projectDocs: 1
---

# Product Requirements Document - Push-Assist Stroller System

**Author:** Dan
**Date:** 2026-01-09

## Executive Summary

Transform a hoverboard mainboard into a push-assist system for a baby stroller. Motor provides constant torque assistance when the stroller is moving, freewheels when stopped, and follows the user's direction automatically.

**Problem:** Pushing a loaded baby stroller (~25kg) over extended distances or inclines causes fatigue. Commercial powered strollers are either fully motorized (removing user control) or prohibitively expensive.

**Solution:** Repurpose hoverboard FOC hardware with a radically simplified control model:
- **Moving = Torque** (constant assist in direction of travel)
- **Stopped = Freewheel** (no motor engagement)
- **No phase detection** - eliminated acceleration/cruise/decel complexity

**Target User:** Parent or caregiver who wants reduced pushing effort while maintaining full physical control.

### What Makes This Special

| Differentiator | Implementation |
|----------------|----------------|
| **Augments, never fights** | Torque direction = sign(speed). Motor can never oppose user. |
| **$50 vs $500** | Repurposed hoverboard mainboard vs commercial motorized strollers |
| **Constant torque model** | No adaptive algorithms. Simple: moving → assist, stopped → off |
| **Dual-threshold UX** | Normal mode (20/10 RPM) for smooth operation; Eager mode (3/2 RPM) for instant response via nunchuk |
| **User-controlled boost** | Nunchuk Y-axis increases torque AND lowers engagement threshold |

### MVP Scope (9 Must-Work Features)

1. Push forward → feel assisted
2. Push backward → feel assisted
3. Stop pushing → stroller stops
4. Direction change → smooth, no jerk
5. Nunchuk boost → more torque + lower threshold
6. Nunchuk idle → normal assist mode
7. Speed cap at 170 RPM (6.5 kph)
8. Nunchuk disconnect → safe default (normal mode)
9. Low battery warning

### Calculated Parameters

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Hardware current limit | 10A | Safe ceiling below 15A max |
| Software torque cap | 300/1000 | ~5A effective, user can overpower |
| Speed cap | 170 RPM | 6.5 kph, fast walking pace |
| Normal engage/disengage | 20/10 RPM | Smooth everyday operation |
| Eager engage/disengage | 3/2 RPM | Instant response when requested |

## Project Classification

**Technical Type:** iot_embedded (STM32 firmware, FOC motor control, Hall sensor feedback)
**Domain:** Consumer electronics / personal mobility assist
**Complexity:** Medium (safety-relevant, user maintains physical override)
**Project Context:** Brownfield - extending hoverboard-firmware-hack-FOC codebase
**Known Bug:** Acceleration-to-cruise transition causes resistance (targeted fix required)

## Success Criteria

### User Success

| Criteria | Target | Measurement |
|----------|--------|-------------|
| Perceived push effort | 50% reduction vs unpowered | Subjective feel during use |
| Resistance during cruise | None perceived | User reports no "fighting" sensation |
| Direction change smoothness | No jerk or hesitation | Seamless forward/backward transitions |
| Stopping behavior | Natural coast to stop | No sudden braking or roll-away |
| Assist engagement | Imperceptible latency | User feels "helped" not "delayed" |

**User Success Moment:** "This stroller feels lighter than it should" - the assist is invisible, just makes everything easier.

### Project Success (Hobby/Personal)

| Milestone | Criteria |
|-----------|----------|
| Demo-ready | All 9 must-work features pass manual testing |
| Daily-driver | 30+ minute walks without issues or overheating |
| Bug-free | Acceleration-to-cruise transition feels smooth (no resistance) |
| Safe default | Nunchuk disconnect results in graceful normal-mode operation |

### Technical Success

| Criteria | Target | Validation |
|----------|--------|------------|
| Assist response time | < 100ms | Imperceptible to user |
| Speed cap enforcement | 170 RPM max | Cannot exceed fast walking pace |
| Current limit | Hardware 10A, Software ~5A effective | User can always overpower motor |
| Thermal stability | 30+ min continuous operation | No overheating shutdowns |
| Freewheel when stopped | Zero motor engagement below disengage threshold | Stroller rolls freely |

### Measurable Outcomes

**Pass/Fail Acceptance Tests:**

| Test | Pass Criteria |
|------|---------------|
| Push from standstill | Assist engages within 0.5s |
| Maintain cruise | Constant assist, no resistance buildup |
| Release handle | Natural deceleration, motor disengages |
| Push backward | Equal assist as forward |
| Direction change | Smooth transition, no jerk |
| Nunchuk boost | Increased torque + lower threshold |
| Nunchuk idle | Normal mode (20/10 RPM thresholds) |
| Nunchuk disconnect | Safe default to normal mode |
| Speed cap | Torque cuts at 170 RPM |
| Overpower test | User can stop wheels by hand |

## Product Scope

### MVP - Minimum Viable Product

The 9 must-work features from brainstorming session:
1. Push forward → feel assisted
2. Push backward → feel assisted
3. Stop pushing → stroller stops
4. Direction change → smooth, no jerk
5. Nunchuk boost → more torque + lower threshold
6. Nunchuk idle → normal assist mode
7. Speed cap at 170 RPM
8. Nunchuk disconnect → safe default
9. Low battery warning

**Bug Fix Required:** Acceleration-to-cruise resistance must be eliminated.

### Growth Features (Post-MVP)

- Multiple assist profiles (eco/normal/sport)
- Automatic incline detection (requires accelerometer hardware)
- Speed display via serial output
- Configurable threshold tuning via nunchuk buttons

### Vision (Future)

- Differential steering assist for easier turning
- Hill-hold / parking brake mode
- Battery level indicator integration
- Regenerative braking on downhills

## User Journeys

### Journey 1: Sarah - Morning Walk to Daycare

Sarah is a parent with a 10-month-old and a fully-loaded stroller (~25kg with baby, diaper bag, and groceries). She walks 1.5km to daycare every morning, including a gradual incline near the end. Before the push-assist, she'd arrive tired and sweaty. Today is different.

**The Walk Begins:**
Sarah unlocks the stroller and starts pushing. Within the first step, she feels something strange - the stroller is *lighter* than it should be. The motor has detected movement and is providing constant assist in her direction of travel. She's pushing, but the effort feels halved.

**Cruising Along:**
On the flat section, she maintains a comfortable walking pace. The assist is invisible - no surging, no resistance, just smooth forward motion. She barely notices the motor is there, which is exactly the point.

**The Incline:**
Approaching the hill, Sarah reaches for the nunchuk and pushes the joystick forward. Two things happen: the torque increases, and the motor becomes more responsive (eager mode). The stroller practically pulls itself up the incline while she guides it.

**Quick Stop for Traffic:**
A car pulls out unexpectedly. Sarah stops pushing - the stroller coasts naturally to a stop. No sudden braking, no roll-away. The motor disengages and the wheels freewheel. She waits, then pushes again. Assist re-engages smoothly.

**Backing Up:**
She needs to back out of a tight spot. Pulling backward, the motor provides the same assist in reverse - no hesitation, no fighting her direction. The transition is seamless.

**Arrival:**
Sarah arrives at daycare feeling fresh. The assist didn't drive for her - she was always in control - but it removed the fatigue from the equation.

**Success Moment:** "This stroller feels lighter than it should."

---

### Journey 2: Edge Cases - Direction Changes, Stops, and Failures

**Scenario A: Quick Direction Change**
Dan is testing the system. He pushes forward, then immediately reverses direction. The motor:
1. Detects speed crossing zero
2. Disengages briefly (freewheel)
3. Re-engages in the new direction

Result: Smooth transition, no jerk, no motor fighting the change.

**Scenario B: Sudden Stop Mid-Push**
While cruising at speed, Dan grabs the wheels to stop suddenly (simulating an emergency). The motor:
1. Cannot overpower his grip (torque capped at 300/1000)
2. Speed drops to zero, motor disengages
3. Wheels freewheel immediately

Result: User always wins. Motor never fights the stop.

**Scenario C: Nunchuk Disconnects**
The nunchuk cable comes loose during a walk. The system:
1. Detects communication timeout
2. Defaults to Normal mode (20/10 RPM thresholds, base torque)
3. Continues providing assist - doesn't suddenly stop

Result: Safe degradation. Walk continues without drama.

**Scenario D: Speed Cap Test**
Dan pushes the stroller to a jog. As speed approaches 170 RPM:
1. Torque output cuts to zero
2. Stroller coasts (no sudden braking)
3. Speed drops below cap, assist resumes

Result: Can't run away. Natural speed limiting.

**Scenario E: Low Battery**
*(Handled by existing firmware - not MVP scope)*

---

### Journey Requirements Summary

| Journey Element | Required Capability |
|-----------------|---------------------|
| Initial push detection | Speed threshold monitoring (20 RPM normal, 3 RPM eager) |
| Constant assist feel | Torque output = constant when moving |
| Incline boost | Nunchuk Y-axis increases torque + lowers threshold |
| Natural stopping | Torque = 0 below disengage threshold |
| Direction change | Direction = sign(speed), freewheel through zero |
| Emergency stop | User can overpower motor (300/1000 torque cap) |
| Nunchuk disconnect | Safe default to normal mode |
| Speed limiting | Torque cut at 170 RPM |
| Low battery | *(Existing firmware functionality)* |

## Embedded System Requirements

### Hardware Platform

| Component | Specification |
|-----------|---------------|
| MCU | STM32F103RCT6 / GD32F103RCT6 |
| Motors | Hoverboard BLDC hub motors (2x) |
| Feedback | Hall sensors (3 per motor) |
| Wheels | 8" diameter |
| Input | Wii Nunchuk via I2C |

### Connectivity

| Interface | Details |
|-----------|---------|
| Nunchuk I2C | USART3 right cable, 5V tolerant, address 0xA4 |
| Debug Serial | USART3 @ 115200 baud (optional) |
| Programming | J-Link via SWD |

### Power Profile

| Parameter | Value |
|-----------|-------|
| Battery | 10S lithium pack (~36V nominal) |
| Hardware current limit | 10A per motor |
| Software torque cap | 300/1000 (~5A effective) |
| PWM frequency | 16 kHz |

### Control Architecture

| Layer | Implementation |
|-------|----------------|
| Control loop | 16 kHz DMA interrupt (`bldc.c`) |
| Application loop | 1 ms (`main.c`) |
| Main loop delay | 5 ms (configurable via `DELAY_IN_MAIN_LOOP`) |
| Control type | FOC (Field Oriented Control) |
| Control mode | TRQ_MODE (Torque) |

### Code Efficiency Requirements

| Requirement | Rationale |
|-------------|-----------|
| **Minimal main loop impact** | Push-assist logic must not introduce blocking operations or significant delays |
| **No floating point** | Use fixed-point arithmetic to maintain performance |
| **Avoid dynamic allocation** | No malloc/free - use static buffers only |
| **Efficient state transitions** | State machine must complete within single loop iteration |
| **Leverage existing patterns** | Follow codebase conventions for speed calculations and input processing |

### Safety Model

| Mechanism | Implementation |
|-----------|----------------|
| Physical override | User can always overpower motor (torque capped) |
| Communication timeout | Existing firmware handles nunchuk disconnect |
| State detection | Control loop detects nunchuk state for mode switching |
| Low battery | Existing firmware handles shutdown |
| Overtemperature | Existing firmware handles shutdown |

### Update Mechanism

- **Method:** Manual flash via J-Link + Keil uVision
- **No OTA:** Direct hardware access required
- **Build variant:** `VARIANT_NUNCHUK`

## Functional Requirements

### Motor Assist

- FR1: System provides constant torque assist when stroller speed exceeds engage threshold
- FR2: System applies torque in the direction of travel (follows speed sign)
- FR3: System freewheels (zero torque) when speed drops below disengage threshold
- FR4: User experiences assist during forward motion
- FR5: User experiences equal assist during backward motion

### Speed Control

- FR6: System limits motor assist to speeds below maximum speed cap
- FR7: System cuts torque output when speed exceeds cap (no active braking)
- FR8: System resumes assist when speed drops below cap

### User Input (Nunchuk)

- FR9: User can increase torque output via nunchuk Y-axis
- FR10: User can lower engagement threshold via nunchuk input (eager mode)
- FR11: System detects nunchuk connection state
- FR12: System defaults to normal mode when nunchuk is idle
- FR13: System defaults to normal mode when nunchuk is disconnected

### State Management

- FR14: System transitions smoothly between idle and assist states
- FR15: System transitions smoothly during direction changes (through zero)
- FR16: System maintains state without jerky or abrupt behavior
- FR17: System operates in two modes: Normal (20/10 RPM) and Eager (3/2 RPM)

### Safety

- FR18: System limits torque output to allow user physical override
- FR19: System disengages motor when stroller is stopped
- FR20: System provides graceful degradation on input failure

### Existing Firmware (Not MVP Scope)

- FR21: System warns user on low battery *(existing)*
- FR22: System handles communication timeout *(existing)*

## Non-Functional Requirements

### Performance

| NFR | Requirement | Target |
|-----|-------------|--------|
| NFR1 | Assist response latency | < 100ms (imperceptible to user) |
| NFR2 | Control loop execution | No bottlenecks at 16 kHz |
| NFR3 | State transition time | Single loop iteration |
| NFR4 | Speed measurement accuracy | Sufficient for threshold detection |

### Reliability

| NFR | Requirement | Target |
|-----|-------------|--------|
| NFR5 | Continuous operation | 30+ minutes without overheating |
| NFR6 | Thermal stability | No shutdown during normal use |
| NFR7 | Graceful degradation | Safe defaults on component failure |
| NFR8 | State consistency | No undefined states during transitions |

### Code Efficiency

| NFR | Requirement | Rationale |
|-----|-------------|-----------|
| NFR9 | Minimal main loop impact | Push-assist logic must not block or delay |
| NFR10 | No floating point operations | Fixed-point arithmetic only |
| NFR11 | No dynamic memory allocation | Static buffers only (no malloc/free) |
| NFR12 | Follow existing codebase patterns | Leverage proven implementations |

### Hardware Limits

| NFR | Parameter | Value |
|-----|-----------|-------|
| NFR13 | Hardware current limit | 10A per motor |
| NFR14 | Software torque cap | 300/1000 (~5A effective) |
| NFR15 | Speed cap | 170 RPM (6.5 kph) |
| NFR16 | Normal engage threshold | 20 RPM |
| NFR17 | Normal disengage threshold | 10 RPM |
| NFR18 | Eager engage threshold | 3 RPM |
| NFR19 | Eager disengage threshold | 2 RPM |

