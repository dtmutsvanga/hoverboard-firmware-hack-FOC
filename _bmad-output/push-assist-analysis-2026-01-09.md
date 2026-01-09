# Push-Assist Stroller System: Analysis & Recommendations

**Date:** 2026-01-09
**Project:** Hoverboard FOC Firmware - Baby Stroller Push Assist
**Baseline Commit:** `c81fafc`

---

## Executive Summary

The current push-assist implementation has a fundamental algorithm flaw causing resistance during constant-velocity pushing. The acceleration-detection approach incorrectly reduces motor assist when the user maintains steady speed, creating the sensation of resistance.

**Recommendation:** Rewrite `handleStatePushCruiseControl()` with a simpler speed-proportional assist algorithm.

---

## 1. Project Context

### 1.1 Hardware Configuration

| Component | Specification |
|-----------|---------------|
| MCU | STM32F103RCT6 / GD32F103RCT6 |
| Motors | 2x BLDC hub motors (hoverboard stock) |
| Wheels | 8-inch diameter |
| Control | FOC (Field Oriented Control) via Simulink-generated code |
| Input | Wii Nunchuk (I2C on USART3) |
| Target Load | ~25kg baby stroller |

### 1.2 Firmware Architecture

```
Input Layer (Nunchuk)
    ↓
Processing Layer (util.c, control.c)
    ↓
FOC Control Layer (BLDC_controller.c) ← 16 kHz interrupt
    ↓
PWM Output (bldc.c) → Motors
```

**Control Loop Timing:**
- **16 kHz DMA interrupt:** FOC algorithm execution, ADC sampling
- **1 ms application loop:** Input processing, speed calculation
- **5 ms main loop delay:** State machine, safety checks

### 1.3 Current Configuration (config.h)

```c
CTRL_TYP_SEL    = FOC_CTRL   // Field Oriented Control
CTRL_MOD_REQ    = SPD_MODE   // Default (overridden to TRQ_MODE in PCC)
I_MOT_MAX       = 4          // 4A current limit (reduced for stroller safety)
N_MOT_MAX       = 200        // 200 RPM max (walking pace ~3-4 mph)
```

### 1.4 Use Case Requirements

1. **Natural feel:** Motor amplifies user push effort, never resists
2. **Bidirectional:** Equal assist forward and backward
3. **Speed limited:** Max = normal walking pace (~3-4 mph)
4. **Safe stopping:** No sudden braking or direction changes
5. **Freewheel at idle:** No resistance when stationary
6. **Nunchuk modulation:** Optional torque limit adjustment via joystick

---

## 2. Problem Statement

### 2.1 Reported Bug

> "The current solution kinda works but when pushing, at first I feel the assistance. Then upon further pushing I feel resistance."

### 2.2 Observed Behavior

| Phase | Expected | Actual |
|-------|----------|--------|
| Initial push | Assist | Assist (correct) |
| Continued acceleration | Assist | Assist (correct) |
| Cruise (constant velocity) | Assist | **Resistance** (bug) |
| Deceleration | Reduced assist | Varies |
| Stop | Freewheel | Freewheel (correct) |

### 2.3 User's Assessment

> "I feel the code is too complicated for its tasks and that there is a lot of redundant useless code in the assist mode."

---

## 3. Theoretical Foundation

### 3.1 Best Practices for Torque-Assist Systems

**Core Principle:** The motor should *amplify* user effort, never *resist* it.

#### Industry Approaches

| Method | Description | Hardware Required | Suitability |
|--------|-------------|-------------------|-------------|
| Torque Sensing | Measures user force via strain gauge | Torque sensor | Best feel, not available |
| Cadence-Based | Detects motion, applies fixed assist | Motion sensor | Binary feel, too simple |
| Speed-Proportional | Assist proportional to wheel speed | Speed sensor (Hall) | Good feel, available |

**Selected Approach:** Speed-Proportional Assist
- Uses existing Hall sensor speed feedback
- User controls speed via push force
- Faster = more assist (up to limit)
- Zero speed = zero assist (pure freewheel)

#### Key Design Requirements

1. **Zero resistance at idle:** TRQ_MODE with pwm=0 must freewheel
2. **Progressive assist:** Gentle at low speed, stronger as speed builds
3. **No direction fighting:** Torque always matches wheel direction
4. **Smooth transitions:** No abrupt on/off switching
5. **Deceleration transparency:** Assist reduces proportionally with speed

### 3.2 User Flow Analysis

| Phase | User Action | Wheel Speed | Motor Behavior |
|-------|-------------|-------------|----------------|
| **Idle** | Standing still | 0 RPM | Disabled, freewheel |
| **Initiate** | Start pushing | 0→5 RPM | Freewheel (no assist yet) |
| **Accelerate** | Continuous push | 5→50 RPM | Light assist, ramping up |
| **Cruise** | Maintain push | ~50 RPM steady | **Constant moderate assist** |
| **Coast** | Release handle | Speed decaying | Reduced assist (proportional) |
| **Decelerate** | Apply drag | 50→10 RPM | Minimal assist |
| **Stop** | Hold stationary | 0 RPM | Disable, freewheel |
| **Reverse** | Push backward | 0→-30 RPM | Same assist, opposite direction |

**Critical Insight:** The bug occurs in the **Cruise** phase where user maintains push but algorithm reduces assist.C

### 3.3 Recommended Algorithm

```
ALGORITHM: Speed-Proportional Torque Assist

CONSTANTS:
  MIN_SPEED     = 5 RPM      // Below this: freewheel zone
  MAX_SPEED     = 150 RPM    // Safety limit
  BASE_TORQUE   = 100        // Minimum assist (out of 1000)
  MAX_TORQUE    = 250        // Maximum assist
  RAMP_ZONE     = 30 RPM     // Speed range for ramp-up

STATE: direction_lock (persists until speed crosses zero)

EVERY 5ms:
  speed_L = measure_left_wheel_rpm()
  speed_R = measure_right_wheel_rpm()
  abs_speed = (|speed_L| + |speed_R|) / 2

  IF abs_speed < MIN_SPEED:
    // FREEWHEEL ZONE
    set_mode(OPEN_MODE)
    pwm = 0
    direction_lock = NONE

  ELSE:
    // ASSIST ZONE
    set_mode(TRQ_MODE)

    // Lock direction on entry
    IF direction_lock == NONE:
      direction_lock = SIGN(avg_speed)

    // Calculate assist (linear interpolation)
    IF abs_speed < RAMP_ZONE:
      assist = BASE_TORQUE + (abs_speed / RAMP_ZONE) * (MAX_TORQUE - BASE_TORQUE) / 2
    ELSE IF abs_speed < MAX_SPEED:
      assist = lerp(BASE_TORQUE, MAX_TORQUE, (abs_speed - RAMP_ZONE) / (MAX_SPEED - RAMP_ZONE))
    ELSE:
      assist = taper_off(MAX_TORQUE, abs_speed - MAX_SPEED)

    // Apply in locked direction only
    pwm = assist * direction_lock * SIGN(current_speed)
```

**Why This Works:**
- No acceleration detection needed
- At cruise speed, assist is constant (no reduction = no resistance)
- When user slows, assist naturally reduces
- Direction lock prevents reversal when user stops mid-push

---

## 4. Current Implementation Analysis

### 4.1 Code Structure

**Location:** `Src/main.c` lines 178-1074

**State Machine:**
```
PCC_STATE_IDLE
    ↓ (speed > 5 RPM)
PCC_STATE_TRACKING
    ↓ (no acceleration for 500ms)
PCC_STATE_COASTING
    ↓ (speed < 2 RPM OR timeout)
PCC_STATE_RAMP_DOWN
    ↓ (torque ramped to 0)
PCC_STATE_IDLE
```

**Key Parameters:**
```c
#define PCC_ENGAGE_SPEED_RPM        5       // Start assist threshold
#define PCC_DISENGAGE_SPEED_RPM     2       // Stop assist threshold
#define PCC_MAX_SPEED_RPM           150     // Speed limit
#define PCC_HOLD_TIME_MS            10000   // Coast timeout
#define PCC_ACCEL_THRESHOLD         3       // RPM/loop for accel detection
#define PCC_ASSIST_TORQUE_BASE      320     // 32% torque (80*4)
#define PCC_ASSIST_TORQUE_ACCEL     150     // 15% during acceleration
#define PCC_ASSIST_TORQUE_MAX       200     // 20% max (inconsistent!)
```

### 4.2 Root Cause Analysis

**The Bug Location:** `main.c:877-956` (TRACKING state handler)

```c
// Acceleration detection
uint8_t isAccelerating = (ABS(actualL) > absLastL + PCC_ACCEL_THRESHOLD) ||
                         (ABS(actualR) > absLastR + PCC_ACCEL_THRESHOLD);

// Torque selection based on acceleration
int16_t assistTorque = isAccelerating ? PCC_ASSIST_TORQUE_ACCEL : PCC_ASSIST_TORQUE_BASE;

// Premature state transition
if (!isAccelerating && timeInState > 500) {
    PCC_EnterState(PCC_STATE_COASTING);  // ← BUG: Transitions too early
}
```

**Bug Sequence:**
1. User pushes → `isAccelerating = true` → `assistTorque = 150`
2. User reaches cruise → `isAccelerating = false` → `assistTorque = 320`
3. After 500ms at cruise → Transitions to COASTING
4. In COASTING: `assistTorque = PCC_ASSIST_TORQUE_BASE / 3 = ~107`

**Result:** User is still pushing but motor drops to 1/3 assist = feels like resistance.

### 4.3 Additional Issues Found

#### Issue 1: Inconsistent Parameters
```c
PCC_ASSIST_TORQUE_BASE = 320  // "Base" assist
PCC_ASSIST_TORQUE_MAX  = 200  // "Max" is LESS than base!
```

#### Issue 2: Aggressive Acceleration Threshold
```c
PCC_ACCEL_THRESHOLD = 3  // RPM per 5ms loop = 600 RPM/sec
```
Normal pushing acceleration is ~50-100 RPM/sec. Threshold is 6x too high.

#### Issue 3: Premature COASTING Transition
```c
if (!isAccelerating && timeInState > 500)  // Only 500ms!
```
User maintaining constant push for >500ms triggers "coasting" behavior.

### 4.4 Code Metrics

| Metric | Current | Recommended |
|--------|---------|-------------|
| Lines of code | ~350 | ~80 |
| States | 4 | 2 zones |
| Tuning parameters | 12 | 5 |
| Conditional branches | 40+ | ~10 |
| Bug surface | High | Low |

---

## 5. Comparison: Current vs. Ideal

| Aspect | Current | Ideal |
|--------|---------|-------|
| **Control Signal** | Acceleration detection | Speed proportional |
| **Cruise Behavior** | Reduced assist (bug) | Constant assist |
| **State Complexity** | 4 states + transitions | 2 zones |
| **Parameter Count** | 12 interrelated | 5 simple |
| **Direction Lock** | Correct | Same |
| **TRQ_MODE Usage** | Correct | Same |
| **Failure Modes** | Many (timing, detection) | Few (thresholds) |

### 5.1 What Works in Current Implementation

- Direction locking mechanism is correctly implemented
- TRQ_MODE usage is appropriate for torque control
- Safety limits (max speed, ramp down) are reasonable
- State machine structure is clean and maintainable
- Integration with other modes (Nunchuk, Baby Rocker) is clean

### 5.2 What Doesn't Work

- Acceleration-based assist logic is fundamentally flawed for push-assist
- Premature transition to COASTING state
- Inconsistent torque constants cause confusion
- Over-engineered for the actual use case

---

## 6. Recommendation

### Decision: REWRITE `handleStatePushCruiseControl()`

### 6.1 Rationale

1. **Fundamental Algorithm Flaw:** Acceleration-detection cannot be tuned to work. The core logic is wrong for push-assist.

2. **Complexity vs. Benefit:** 4-state machine adds debugging surface without benefit. Speed-proportional assist is simpler and correct.

3. **Parameter Chaos:** Current constants are inconsistent. Fresh implementation avoids inherited confusion.

4. **Low Risk:** PCC code is isolated. No changes needed to FOC core, config.h, or other modes.

### 6.2 Scope

**Files to Modify:**
- `Src/main.c` - Replace PCC implementation

**Files Unchanged:**
- `Inc/config.h` - Motor limits already correct
- `Src/BLDC_controller.c` - Auto-generated, don't touch
- `Src/bldc.c` - FOC core unchanged
- Other modes (State 1, State 2) unchanged

**Code Changes:**
- Remove: ~270 lines (old PCC states and logic)
- Add: ~80 lines (new speed-proportional algorithm)
- Net: -190 lines

### 6.3 Implementation Plan

1. **Keep:**
   - `PCC_Context_t` structure (direction lock)
   - `PCC_EnterState()` helper (clean state entry)
   - Integration with state machine (handleSwitchStateReq)

2. **Remove:**
   - `PCC_STATE_COASTING` and `PCC_STATE_RAMP_DOWN`
   - Acceleration detection logic
   - Complex torque selection

3. **Write:**
   - 2-zone algorithm (freewheel / assist)
   - Speed-proportional torque curve
   - Simplified parameter set

### 6.4 New Parameters

```c
// Speed-Proportional Assist Configuration
#define PCC_MIN_SPEED_RPM       5       // Below this: freewheel
#define PCC_MAX_SPEED_RPM       150     // Speed limit
#define PCC_BASE_TORQUE         100     // Minimum assist (10%)
#define PCC_MAX_TORQUE          250     // Maximum assist (25%)
#define PCC_RAMP_ZONE_RPM       30      // Ramp-up range
```

---

## 7. Testing Plan

### 7.1 Pre-Implementation Verification

1. Build current firmware: `make -e VARIANT=VARIANT_NUNCHUK`
2. Verify baseline behavior matches bug description
3. Document current speed/torque readings via debug serial

### 7.2 Post-Implementation Testing

| Test | Expected Result |
|------|-----------------|
| Push from stop | Smooth acceleration with assist |
| Maintain cruise | **Constant assist, no resistance** |
| Release and coast | Natural deceleration |
| Push backward | Equal assist in reverse |
| Stop mid-push | Immediate freewheel |
| Exceed max speed | Assist tapers off |

### 7.3 Safety Validation

- [ ] Motor stops when stroller stops
- [ ] No sudden direction changes
- [ ] Current stays within I_MOT_MAX (4A)
- [ ] Speed stays within N_MOT_MAX (200 RPM)
- [ ] Nunchuk button state changes work correctly

---

## Appendix A: File References

| File | Lines | Relevance |
|------|-------|-----------|
| `Src/main.c` | 178-1074 | PCC implementation (to be rewritten) |
| `Inc/config.h` | 140-173 | Motor control parameters |
| `Src/BLDC_controller.c` | All | FOC algorithm (auto-generated, read-only) |
| `Inc/defines.h` | All | Hardware pin mappings |

## Appendix B: Glossary

| Term | Definition |
|------|------------|
| FOC | Field Oriented Control - motor control algorithm |
| TRQ_MODE | Torque mode - PWM controls motor current directly |
| SPD_MODE | Speed mode - closed-loop RPM control |
| PCC | Push Cruise Control - the assist system |
| Hall Sensor | Magnetic sensor for motor position/speed |
| Back-EMF | Voltage generated by spinning motor |

---

**Document Status:** Analysis Complete
**Next Action:** Awaiting approval to implement rewrite
