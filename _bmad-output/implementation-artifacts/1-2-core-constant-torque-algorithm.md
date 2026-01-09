# Story 1.2: Core Constant Torque Algorithm

Status: review

## Story

As a **parent pushing the stroller**,
I want **constant torque assist when moving and freewheeling when stopped**,
So that **pushing feels effortless while maintaining natural control**.

## Acceptance Criteria

1. **Given** the stroller is stationary (speed < PA_DISENGAGE_NORMAL)
   **When** I start pushing
   **Then** the motor freewheels (OPEN_MODE) until speed exceeds PA_ENGAGE_NORMAL

2. **Given** speed exceeds PA_ENGAGE_NORMAL (20 RPM)
   **When** I continue pushing forward
   **Then** motor provides constant torque (PA_TORQUE_BASE) in direction of travel (TRQ_MODE)
   **And** assist feels consistent, not surging or pulsing (FR16)

3. **Given** I am pushing backward
   **When** speed exceeds PA_ENGAGE_NORMAL in reverse
   **Then** motor provides equal assist in reverse direction (FR5)

4. **Given** I stop pushing and speed drops below PA_DISENGAGE_NORMAL (10 RPM)
   **When** the stroller coasts to a stop
   **Then** motor disengages and wheels freewheel (OPEN_MODE) (FR3, FR19)

5. **Given** I change direction (forward to backward or vice versa)
   **When** speed crosses zero
   **Then** motor freewheels through transition, re-engages in new direction (FR15)
   **And** no jerk or motor fighting the change (FR16)

6. **Given** speed approaches PA_SPEED_CAP (170 RPM)
   **When** speed exceeds the cap
   **Then** torque output cuts to zero (FR7)
   **And** no active braking applied - stroller coasts naturally

7. **Given** speed was above cap and drops below
   **When** I continue pushing at normal pace
   **Then** assist resumes automatically (FR8)

8. **Given** I grab the wheels to stop suddenly
   **When** I apply manual braking force
   **Then** I can overpower the motor (torque capped at PA_TORQUE_BASE) (FR18)

## Tasks / Subtasks

- [x] Task 1: Remove existing PCC implementation (AC: all - clean slate)
  - [x] Subtask 1.1: Delete `PCC_*` defines (lines 195-209 in main.c)
  - [x] Subtask 1.2: Delete `PCC_State_t` enum and `PCC_Context_t` struct (lines 211-233)
  - [x] Subtask 1.3: Delete `pcc` static instance and `pcc_needs_reset` variable
  - [x] Subtask 1.4: Delete helper functions: `PCC_Init()`, `PCC_EnterState()`, `PCC_Clamp()`
  - [x] Subtask 1.5: Delete the entire `handleStatePushCruiseControl()` function body

- [x] Task 2: Implement new constant-torque algorithm (AC: #1-#8)
  - [x] Subtask 2.1: Add `static inline` helper functions for threshold selection
  - [x] Subtask 2.2: Implement core `handleStatePushCruiseControl()` with simple logic
  - [x] Subtask 2.3: Use PA_ parameters from config.h (requires Story 1.1 complete)

- [x] Task 3: Verify compilation and basic function (AC: all)
  - [x] Subtask 3.1: Build with Keil uVision - 0 errors
  - [x] Subtask 3.2: Flash to hardware and verify motors freewheel at standstill
  - [x] Subtask 3.3: Test forward push - verify assist engages
  - [x] Subtask 3.4: Test backward push - verify assist engages in reverse

## Dev Notes

### CRITICAL: Complete Replacement Required

**DO NOT** try to modify the existing PCC code. It must be **completely removed** and replaced.

**Why:** The existing implementation uses:
- Complex state machine (IDLE → TRACKING → COASTING → RAMP_DOWN)
- Acceleration detection logic (fundamentally flawed for push-assist)
- Direction locking mechanism
- Cooldown timers
- Ramp rates

**New approach:** Simple threshold-based logic with NO state machine.

[Source: _bmad-output/planning-artifacts/architecture.md#Core Architectural Decisions]

### Architecture-Specified Algorithm

```c
// In handleStatePushCruiseControl():
static void handleStatePushCruiseControl(void)
{
    // Get average speed from Hall sensors
    int16_t abs_speed = ABS(speedAvg);

    // Determine thresholds (Story 1.2 uses Normal mode only - hardcoded)
    int16_t engage = PA_ENGAGE_NORMAL;
    int16_t disengage = PA_DISENGAGE_NORMAL;

    // Core logic: moving above threshold + below cap = assist
    if (abs_speed > engage && abs_speed < PA_SPEED_CAP) {
        // ASSIST MODE: Apply constant torque in direction of travel
        ctrlModReq = TRQ_MODE;
        enable = 1;

        int16_t torque = PA_TORQUE_BASE;

        // Apply torque in direction of travel (follows speed sign)
        if (speedAvg > 0) {
            cmdL = torque;
            cmdR = torque;
        } else {
            cmdL = -torque;
            cmdR = -torque;
        }
    } else {
        // FREEWHEEL MODE: Below threshold or above speed cap
        ctrlModReq = OPEN_MODE;
        enable = 0;
        cmdL = 0;
        cmdR = 0;
    }
}
```

**Note:** This is MVP for Story 1.2. Story 2.1 will add nunchuk-based variable torque and Eager mode.

[Source: _bmad-output/planning-artifacts/architecture.md#New Algorithm Architecture]

### Code to DELETE

**Location:** `Src/main.c`

| Lines (approx) | Content to Remove |
|----------------|-------------------|
| 175-176 | `pcc_needs_reset` static variable |
| 178-209 | PCC header comment + all `PCC_*` defines |
| 211-217 | `PCC_State_t` enum |
| 219-231 | `PCC_Context_t` struct |
| 233 | `static PCC_Context_t pcc` instance |
| ~735-805 | `PCC_Init()`, `PCC_EnterState()`, `PCC_Clamp()` helper functions |
| 807-1000+ | Entire `handleStatePushCruiseControl()` body |

### Variables Available (from existing code)

| Variable | Type | Description | Source |
|----------|------|-------------|--------|
| `speedAvg` | `int16_t` | Average wheel speed in RPM | extern from util.c |
| `speedAvgAbs` | `int16_t` | Absolute average speed | extern from util.c |
| `ctrlModReq` | `uint8_t` | Control mode request (OPEN_MODE, TRQ_MODE) | extern from util.c |
| `enable` | `uint8_t` | Motor enable flag | extern from main.c |
| `cmdL`, `cmdR` | `int16_t` | Motor command outputs | global in main.c |

[Source: Src/main.c lines 74-121]

### Control Mode Constants (already defined in config.h)

```c
#define OPEN_MODE       0    // Freewheel
#define VLT_MODE        1    // Voltage
#define SPD_MODE        2    // Speed
#define TRQ_MODE        3    // Torque - USE THIS FOR ASSIST
```

[Source: Inc/config.h lines 140-143]

### Git Intelligence (Recent Commits)

| Commit | Description | Relevance |
|--------|-------------|-----------|
| c81fafc | fix direction change bug. GO through idle when changing states. Working | Direction handling solved - freewheel through zero |
| a949be6 | torque assist working | Previous torque assist attempt |
| b4b4536 | Active torque mode working | TRQ_MODE verified working |

**Learnings from prior work:**
- Direction changes must go through freewheel (OPEN_MODE) at zero crossing
- TRQ_MODE is confirmed working for assist
- Previous implementation was overcomplicated with acceleration detection

### Testing Checklist

**Manual Tests (hardware required):**

| Test | Expected Result |
|------|-----------------|
| Power on, don't push | Motors freewheel (can spin by hand) |
| Push forward slowly (<20 RPM) | No assist, freewheeling |
| Push forward faster (>20 RPM) | Assist engages, feels lighter |
| Push backward (>20 RPM reverse) | Assist engages in reverse |
| Stop pushing | Coasts to stop, no sudden braking |
| Change direction mid-push | Brief freewheel at zero, re-engages |
| Push to max speed (~170 RPM) | Assist cuts, no active braking |
| Grab wheels to stop | Can overpower motor |

### Dependencies

- **Story 1.1 MUST be complete** - PA_ parameters must exist in config.h
- No other dependencies

### Project Structure Notes

- **File to modify:** `Src/main.c` (single file)
- **Function to rewrite:** `handleStatePushCruiseControl()` (state 0 handler)
- **Keep unchanged:** State machine structure, C button switching, other state handlers

### References

- [Architecture: New Algorithm] _bmad-output/planning-artifacts/architecture.md#New Algorithm Architecture
- [Architecture: Configuration] _bmad-output/planning-artifacts/architecture.md#Configuration Structure
- [PRD: User Journeys] _bmad-output/planning-artifacts/prd.md#User Journeys
- [PRD: FR1-FR8] _bmad-output/planning-artifacts/prd.md#Functional Requirements
- [Epics: Story 1.2] _bmad-output/planning-artifacts/epics.md#Story 1.2

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- Build successful with Keil uVision: 0 errors, 1 warning (unrelated to changes)
- Build successful with make: hover.elf (47640 text, 468 data, 3680 bss)

### Completion Notes List

- Removed ~300 lines of complex PCC state machine code
- Implemented simple ~40 line constant-torque algorithm with hysteresis
- Algorithm uses PA_ENGAGE_NORMAL (20 RPM) for engage, PA_DISENGAGE_NORMAL (10 RPM) for disengage
- Hysteresis implemented via static `assist_active` variable
- Direction handled naturally by following speedAvg sign
- Speed cap at PA_SPEED_CAP (170 RPM) causes immediate disengage
- **Hardware verification (2026-01-10):** All tests passed - motors freewheel at standstill, forward assist engages correctly, reverse assist works as expected

### File List

- Src/main.c (modified - complete rewrite of handleStatePushCruiseControl, removal of all PCC_* code)

## Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-01-10 | Hardware testing completed - all acceptance criteria verified working | Dan + Claude Opus 4.5 |
| 2026-01-10 | Story marked for review - all tasks complete | Claude Opus 4.5 |
