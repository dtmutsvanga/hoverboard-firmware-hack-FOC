---
stepsCompleted: [1, 2, 3, 4]
inputDocuments: ['_bmad-output/prd-push-assist-stroller.md']
session_topic: 'MVP PRD refinement for Push-Assist Stroller - stripping to essential demonstrable features'
session_goals: 'Flawless demo operation, user-controlled torque boost via nunchuk, default behavior when no input'
selected_approach: 'user-selected'
techniques_used: ['First Principles Thinking']
ideas_generated: ['constant-torque-model', 'dual-threshold-system', 'nunchuk-dual-purpose', 'calculated-safety-params', 'mvp-9-features']
context_file: '_bmad/bmm/data/project-context-template.md'
technique_execution_complete: true
session_active: false
workflow_completed: true
---

# Brainstorming Session Results

**Facilitator:** Dan
**Date:** 2026-01-09

## Session Overview

**Topic:** MVP PRD refinement for Push-Assist Stroller System
**Goals:**
- Flawless demo operation
- User-controlled torque boost via nunchuk
- Default behavior when no nunchuk input provided

### Context Guidance

This session focuses on refining the existing PRD to create a demonstration-ready MVP. Key simplification: incline handling via manual user input rather than automatic detection.

### Session Setup

**Key Decisions Made:**
- Incline compensation = user-controlled (nunchuk), not automatic detection
- System defaults to base assist mode when nunchuk idle
- Scope reduced to essential demonstrable features only

**MVP Scope Definition:**

| IN Scope | OUT of Scope (Deferred) |
|----------|------------------------|
| Basic push-assist at constant torque | Automatic incline detection |
| Speed limiting | Differential steering logic |
| Safety timeouts | Hill-hold / parking brake |
| Nunchuk torque boost | Complex speed-proportional assist curves |
| Bidirectional operation | Multiple assist profiles |

## Technique Selection

**Approach:** User-Selected Techniques
**Selected Techniques:**

- **First Principles Thinking**: Strip away ALL assumptions to rebuild requirements from fundamental truths - perfect for defining true MVP essentials

## Technique Execution: First Principles Thinking

### Element 1: What Do We Know For CERTAIN?

**Hardware Facts:**
- Only nunchuk joystick + motors available (no force sensors, no accelerometers)
- Hall sensors measure wheel SPEED, not user push force
- Torque mode tested and provides ACTIVE assist (user felt helped, not just "not resisted")

**Success Criteria Facts:**
- User must FEEL assisted (perceptual, not just mechanical)
- User must NOT feel disturbed (smooth > powerful)
- Torque mode works for active assist - empirically validated

**Bug Reality:**
- Initial push: assistance works
- Continued acceleration: resistance appears (BUG)
- Constant velocity: works fine
- Bug is in acceleration-to-cruise transition

### Element 2: Assumptions Challenged

| Original Assumption | First Principles Truth |
|--------------------|------------------------|
| Adaptive torque based on push force | **IMPOSSIBLE** - no hardware to sense push force |
| Phase detection (accel/cruise/decel) | **ELIMINATED** - just moving vs stopped |
| Automatic incline detection | **REPLACED** - user controls via nunchuk |
| Fixed 5/3 RPM thresholds | **RECALCULATED** - dual threshold system |
| 4A max current (guessed) | **VALIDATED & REFINED** - calculated from hoverboard specs |
| Speed limit vague | **CALCULATED** - 170 RPM = 6.5 kph |

**Bidirectional Operation:** VALIDATED - needed for ramps backward, turning, direction change

### Element 3: Minimum for Demo Success

**9 Must-Work Features:**
1. Push forward → feel assisted
2. Push backward → feel assisted
3. Stop pushing → stroller stops
4. Direction change → smooth, no jerk
5. Nunchuk boost → more torque
6. Nunchuk idle → normal assist
7. Speed cap → won't run away
8. Nunchuk disconnect → safe default
9. Low battery warning

**1 Debug Feature:**
10. Beep on mode change → disabled via MACRO in MVP

### Key Discoveries

**Constant Torque Model:**
```
IF speed > threshold:
    torque = BASE_TORQUE + (nunchuk_input * BOOST_MULTIPLIER)
    direction = sign(speed)
ELSE:
    torque = 0
```

**Dual-Threshold System:**
| Mode | Trigger | Engage | Disengage |
|------|---------|--------|-----------|
| Normal | Nunchuk idle | 20 RPM | 10 RPM |
| Eager | Nunchuk pressed | 3 RPM | 2 RPM |

**Nunchuk Dual-Purpose:**
1. Boost torque (increase assist power)
2. Lower engagement threshold (assist starts sooner)

**Calculated Safety Parameters:**
| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Hardware current limit | 10A | Safe ceiling below 15A max |
| Torque cap (firmware) | 300/1000 | ~5A effective, user can overpower |
| Speed cap | 170 RPM | 6.5 kph, very fast walk |

---

## Idea Organization and Prioritization

### Thematic Organization

**Theme 1: Core Control Logic** - Radical simplification
- Constant torque model eliminates phase detection
- Moving = torque, stopped = zero
- Direction automatically follows speed sign

**Theme 2: Dual-Threshold System** - User intent drives behavior
- Normal mode (20/10 RPM) for smooth everyday operation
- Eager mode (3/2 RPM) for immediate assist when requested
- Nunchuk serves dual purpose: torque boost + threshold lowering

**Theme 3: Calculated Safety Parameters** - Evidence-based limits
- Hardware current: 10A (safe ceiling)
- Torque cap: 300/1000 (~5A effective)
- Speed cap: 170 RPM (6.5 kph)

**Theme 4: Bug Root Cause** - Targeted fix
- Problem isolated to acceleration-to-cruise transition
- Core physics validated (initial push and constant velocity work)
- Fix target: whatever causes resistance during acceleration

### Prioritization Results

**Top Priority - Bug Fix:**
- Fix acceleration-phase resistance (MVP blocker)
- Likely direction/sign calculation issue

**Quick Wins:**
- Apply calculated safety parameters (config.h changes)
- Implement dual-threshold constants

**Implementation-Ready:**
- Full MVP algorithm documented in pseudocode
- 9 must-work features defined with clear acceptance criteria

---

## Action Planning

### Immediate Next Steps

| Priority | Action | Files Affected | Success Criteria |
|----------|--------|----------------|------------------|
| 1 | Fix acceleration resistance bug | `main.c`, `bldc.c` | Constant assist throughout acceleration |
| 2 | Implement dual-threshold system | `main.c`, `config.h` | Smooth normal + instant eager mode |
| 3 | Set calculated safety limits | `config.h` | 10A hw, 300 torque, 170 RPM cap |
| 4 | Test 9 must-work features | - | All pass acceptance criteria |

### Implementation Checklist

```
[ ] Review current torque calculation in acceleration phase
[ ] Identify where resistance is introduced
[ ] Implement constant torque model (moving = torque, stopped = zero)
[ ] Add dual-threshold logic with nunchuk detection
[ ] Set ENGAGE_NORMAL = 20, DISENGAGE_NORMAL = 10
[ ] Set ENGAGE_EAGER = 3, DISENGAGE_EAGER = 2
[ ] Set I_MOT_MAX or equivalent to 10A hardware limit
[ ] Set torque output cap to 300/1000
[ ] Set N_MOT_MAX to 170 RPM
[ ] Test forward assist
[ ] Test backward assist
[ ] Test stop behavior
[ ] Test direction change smoothness
[ ] Test nunchuk boost
[ ] Test nunchuk idle (normal mode)
[ ] Test speed cap
[ ] Verify nunchuk disconnect defaults to normal
[ ] Verify low battery warning
```

---

## Refined MVP PRD Requirements

### Functional Requirements (Revised)

| ID | Requirement | Implementation |
|----|-------------|----------------|
| FR-1 | Motor provides constant assist when moving | Torque = BASE when speed > threshold |
| FR-2 | Motor freewheels when stopped | Torque = 0 when speed < disengage |
| FR-3 | Bidirectional assist (forward and backward) | Direction = sign(speed) |
| FR-4 | Smooth direction changes | No phase detection, continuous torque |
| FR-5 | Nunchuk boost increases torque | Torque += nunchuk_y * BOOST |
| FR-6 | Nunchuk boost lowers engage threshold | 20 RPM → 3 RPM when pressed |
| FR-7 | Speed capped at fast walking pace | Torque = 0 when speed > 170 RPM |
| FR-8 | Safe default on nunchuk disconnect | Normal mode (20/10 RPM, BASE torque) |

### Non-Functional Requirements (Revised)

| ID | Requirement | Value |
|----|-------------|-------|
| NFR-1 | Hardware current limit | 10A |
| NFR-2 | Software torque cap | 300/1000 |
| NFR-3 | Speed cap | 170 RPM (6.5 kph) |
| NFR-4 | Normal engage threshold | 20 RPM |
| NFR-5 | Normal disengage threshold | 10 RPM |
| NFR-6 | Eager engage threshold | 3 RPM |
| NFR-7 | Eager disengage threshold | 2 RPM |

### MVP Algorithm (Reference Implementation)

```c
// Constants (config.h)
#define ENGAGE_NORMAL    20   // RPM
#define DISENGAGE_NORMAL 10   // RPM
#define ENGAGE_EAGER     3    // RPM
#define DISENGAGE_EAGER  2    // RPM
#define SPEED_CAP        170  // RPM
#define TORQUE_BASE      150  // of 1000
#define TORQUE_MAX       300  // of 1000

// Core logic (main.c)
int16_t calculateAssistTorque(int16_t speed, int8_t nunchuk_y) {
    int16_t torque = 0;
    bool nunchuk_active = (nunchuk_y > NUNCHUK_DEADBAND);

    int16_t engage = nunchuk_active ? ENGAGE_EAGER : ENGAGE_NORMAL;
    int16_t disengage = nunchuk_active ? DISENGAGE_EAGER : DISENGAGE_NORMAL;

    int16_t abs_speed = abs(speed);

    if (abs_speed > engage && abs_speed < SPEED_CAP) {
        // Calculate torque
        torque = TORQUE_BASE;
        if (nunchuk_active) {
            torque += (nunchuk_y * (TORQUE_MAX - TORQUE_BASE)) / 127;
        }
        // Apply direction
        if (speed < 0) torque = -torque;
    }
    // else: torque remains 0 (freewheel)

    return torque;
}
```

---

## Session Summary and Insights

### Key Achievements

1. **Eliminated 6 complexity sources** from original PRD through First Principles analysis
2. **Identified bug root cause** - isolated to acceleration-to-cruise transition
3. **Calculated evidence-based parameters** - no more guessing on thresholds, current, speed
4. **Defined dual-threshold system** - elegant solution for normal vs demanding conditions
5. **Created implementation-ready algorithm** - pseudocode ready to translate to C

### Session Reflections

**What Worked Well:**
- First Principles questioning exposed hidden assumptions
- Calculating from known hoverboard specs grounded safety parameters
- User's real-world testing validated torque mode as correct approach
- Dual-threshold concept emerged organically from use case analysis

**Breakthrough Moments:**
- Realizing adaptive torque is IMPOSSIBLE without force sensors
- Understanding that "phases" are unnecessary - just moving vs stopped
- Nunchuk as dual-purpose input (torque + threshold) - elegant simplification
- Bug isolation: the problem is specific to acceleration phase, not fundamental

### Creative Partnership

Dan brought empirical testing insights (torque mode works, bug description) while the First Principles technique systematically challenged every assumption. The result: a dramatically simplified MVP that addresses the real problem (acceleration resistance) without over-engineering.

---

## Next Steps

1. **Review** this session document
2. **Begin** with Priority 1: Fix acceleration resistance bug
3. **Apply** calculated safety parameters to config.h
4. **Implement** dual-threshold system
5. **Test** all 9 must-work features
6. **Demo** the flawless MVP

---

*Session completed: 2026-01-09*
*Technique: First Principles Thinking*
*Outcome: Refined MVP PRD with implementation-ready specifications*

