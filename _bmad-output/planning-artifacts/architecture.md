---
stepsCompleted: [1, 2, 4, 7]
status: complete
inputDocuments:
  - '_bmad-output/planning-artifacts/prd.md'
workflowType: 'architecture'
project_name: 'Push-Assist Stroller System'
user_name: 'Dan'
date: '2026-01-09'
---

# Architecture Decision Document

_This document builds collaboratively through step-by-step discovery. Sections are appended as we work through each architectural decision together._

## Project Context Analysis

### Requirements Overview

**Functional Requirements:**
22 FRs defining push-assist capabilities:
- Motor assist behavior (constant torque when moving)
- Speed control and limiting (170 RPM cap)
- Nunchuk input processing (boost + threshold control)
- State management (Normal/Eager modes)
- Safety constraints (torque limits, graceful degradation)

**Non-Functional Requirements:**
19 NFRs with critical embedded constraints:
- Performance: < 100ms response, no loop bottlenecks
- Code efficiency: Fixed-point only, static allocation, minimal overhead
- Hardware limits: Defined thresholds and current caps

**Scale & Complexity:**

- Primary domain: Embedded C firmware (STM32/GD32)
- Complexity level: Low-Medium
- Existing codebase: hoverboard-firmware-hack-FOC
- New components: 1 control mode + configuration

### Technical Constraints & Dependencies

| Constraint | Impact |
|------------|--------|
| Existing control loop (16kHz) | Must not disrupt FOC algorithm |
| Main loop (1ms) | Push-assist logic executes here |
| No floating point | Fixed-point arithmetic required |
| No dynamic allocation | Static buffers only |
| Existing patterns | Must follow util.c, control.c conventions |

### Cross-Cutting Concerns Identified

1. **State Management** - Mode switching affects multiple components
2. **Input Processing** - Nunchuk data used for both torque and threshold
3. **Safety Enforcement** - Limits applied across all states
4. **Direction Handling** - Sign propagation through calculations

## Core Architectural Decisions

### Decision Summary

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| 1 | Control Location | Main loop (`main.c`) | Follows existing variant pattern, 5ms sufficient |
| 2 | State Transitions | Immediate | Nunchuk pre-filtered, user expects instant response |
| 3 | Data Flow | Single function | Matches brainstorming pseudocode, minimal complexity |
| 4 | Integration | Enhance existing PCC (rewrite algorithm) | Keep state machine, replace flawed torque logic |
| 5 | Configuration | `Inc/config.h` | Follows existing pattern, easy calibration |

### Existing Architecture (Preserved)

**State Machine (C button toggles):**
```
state 0: handleStatePushCruiseControl() ← REWRITE THIS
state 1: handleStateNunChuckSpeedDiffCtrl()
state 2: handleStateBabyRocker()
```

**Control Mode Usage:**
- `OPEN_MODE` → Freewheeling when idle
- `TRQ_MODE` → Active torque assist when moving

### New Algorithm Architecture

**Replace existing PCC algorithm with constant torque model:**

```c
// In handleStatePushCruiseControl():
int16_t calculateAssistTorque(int16_t speed, int8_t nunchuk_y) {
    bool nunchuk_active = (nunchuk_y > NUNCHUK_DEADBAND);

    int16_t engage = nunchuk_active ? ENGAGE_EAGER : ENGAGE_NORMAL;
    int16_t disengage = nunchuk_active ? DISENGAGE_EAGER : DISENGAGE_NORMAL;
    int16_t abs_speed = ABS(speed);

    if (abs_speed > engage && abs_speed < SPEED_CAP) {
        int16_t torque = TORQUE_BASE;
        if (nunchuk_active) {
            torque += (nunchuk_y * (TORQUE_MAX - TORQUE_BASE)) / 127;
        }
        return (speed < 0) ? -torque : torque;
    }
    return 0;  // Freewheel
}
```

### Configuration Structure

**New section in `Inc/config.h`:**

```c
// ######################## PUSH-ASSIST PARAMETERS #########################
#define PA_ENGAGE_NORMAL     20    // RPM - Normal mode engage threshold
#define PA_DISENGAGE_NORMAL  10    // RPM - Normal mode disengage threshold
#define PA_ENGAGE_EAGER      3     // RPM - Eager mode engage threshold
#define PA_DISENGAGE_EAGER   2     // RPM - Eager mode disengage threshold
#define PA_SPEED_CAP         170   // RPM - Maximum assisted speed (6.5 kph)
#define PA_TORQUE_BASE       150   // Base assist torque (of 1000)
#define PA_TORQUE_MAX        300   // Maximum assist torque (of 1000)
#define PA_NUNCHUK_DEADBAND  10    // Nunchuk Y-axis deadband
```

### Files Affected

| File | Change |
|------|--------|
| `Inc/config.h` | Add push-assist parameter section |
| `Src/main.c` | Rewrite `handleStatePushCruiseControl()` |

### What Stays the Same

- State machine structure (state 0/1/2)
- C button mode switching
- Nunchuk I2C communication
- FOC control loop (16kHz DMA)
- All other existing functionality

## Implementation Guidelines

### Code Organization

Helper functions and inline functions MAY be used to improve readability without affecting performance:

```c
// Example: Extract threshold selection for clarity
static inline int16_t getEngageThreshold(bool eager_mode) {
    return eager_mode ? PA_ENGAGE_EAGER : PA_ENGAGE_NORMAL;
}

static inline int16_t getDisengageThreshold(bool eager_mode) {
    return eager_mode ? PA_DISENGAGE_EAGER : PA_DISENGAGE_NORMAL;
}
```

**Guidelines:**
- Use `static inline` for small helper functions (compiler will inline, zero overhead)
- Keep functions small and single-purpose
- Follow existing naming conventions (`camelCase` for functions)
- Place helpers near the top of `handleStatePushCruiseControl()` or as file-scope statics

### Existing Conventions to Follow

| Convention | Example |
|------------|---------|
| Variable naming | `snake_case` for locals, `camelCase` for globals |
| Macro naming | `PA_TORQUE_BASE` (prefix + UPPER_CASE) |
| Function naming | `handleStatePushCruiseControl()` |
| Fixed-point math | Use existing `ABS()`, avoid floats |
| Control modes | `ctrlModReq = TRQ_MODE` / `OPEN_MODE` |

## Architecture Validation

### Coherence Check

| Validation | Status |
|------------|--------|
| Decisions work together | ✅ Main loop + immediate transitions + single function = coherent |
| No conflicts with FOC loop | ✅ Only sets `cmdL`/`cmdR`, doesn't touch DMA interrupt |
| Existing patterns preserved | ✅ Same state machine, same control mode API |
| Configuration follows convention | ✅ `PA_` prefix macros in config.h |

### Requirements Coverage

| Category | Coverage |
|----------|----------|
| 22 Functional Requirements | ✅ All mapped to algorithm design |
| 19 Non-Functional Requirements | ✅ Fixed-point, no allocation, < 100ms response |
| Safety constraints | ✅ Speed cap, torque limits, graceful freewheel |

### Implementation Readiness

- [x] Algorithm pseudocode provided
- [x] Configuration parameters defined with defaults
- [x] Files to modify identified (2 files)
- [x] Integration points clear (state 0 handler)
- [x] Existing code patterns documented

## Architecture Status: COMPLETE

**Ready for implementation.** Proceed to epic/story creation or direct implementation.

