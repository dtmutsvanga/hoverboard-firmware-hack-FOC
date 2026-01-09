# Story 2.1: Nunchuk Boost & Eager Mode

Status: review

## Story

As a **parent pushing uphill or needing extra help**,
I want **to push the nunchuk joystick forward for more torque and quicker response**,
So that **I get instant, stronger assist when I need it**.

## Acceptance Criteria

1. **Given** nunchuk Y-axis is below PA_NUNCHUK_DEADBAND
   **When** I push the stroller
   **Then** system operates in Normal mode (20/10 RPM thresholds, PA_TORQUE_BASE) (FR12)

2. **Given** nunchuk Y-axis exceeds PA_NUNCHUK_DEADBAND
   **When** I push the joystick forward
   **Then** system switches to Eager mode (3/2 RPM thresholds) (FR10, FR17)
   **And** torque increases proportionally: `PA_TORQUE_BASE + (Y * (PA_TORQUE_MAX - PA_TORQUE_BASE) / 127)` (FR9)

3. **Given** I am in Eager mode with boost
   **When** I release the joystick to center
   **Then** system returns to Normal mode immediately (FR12)
   **And** transition is smooth, no jerk

4. **Given** I am at low speed (e.g., 5 RPM) in Normal mode
   **When** I push joystick forward (Eager mode)
   **Then** assist engages immediately (3 RPM threshold met) (FR10)

## Tasks / Subtasks

- [x] Task 1: Add nunchuk Y-axis reading to push-assist algorithm (AC: #1, #2)
  - [x] Subtask 1.1: Access `input2[inIdx].cmd` in handleStatePushCruiseControl() (uses processed input with deadband)
  - [x] Subtask 1.2: Deadband handled by input processing; cmd > 0 indicates Eager mode
  - [x] Subtask 1.3: Select appropriate engage/disengage thresholds based on mode

- [x] Task 2: Implement variable torque calculation (AC: #2)
  - [x] Subtask 2.1: Calculate boost torque proportionally based on joystick position
  - [x] Subtask 2.2: Clamp result to PA_TORQUE_MAX
  - [x] Subtask 2.3: Apply calculated torque instead of fixed PA_TORQUE_BASE

- [x] Task 3: Ensure smooth mode transitions (AC: #3, #4)
  - [x] Subtask 3.1: Verified no state persistence - mode recalculated each loop iteration
  - [x] Subtask 3.2: Input filtering provides smooth transitions; torque scales proportionally

- [x] Task 4: Test and verify (AC: all)
  - [x] Subtask 4.1: Build successful with VARIANT_NUNCHUK (make -e VARIANT=VARIANT_NUNCHUK)
  - [ ] Subtask 4.2: Test Normal mode (nunchuk centered) - requires hardware testing
  - [ ] Subtask 4.3: Test Eager mode (nunchuk pushed forward) - requires hardware testing
  - [ ] Subtask 4.4: Test torque scaling (partial vs full joystick) - requires hardware testing

## Dev Notes

### Architecture-Specified Algorithm Enhancement

Extend the Story 1.2 algorithm to include nunchuk input:

```c
static void handleStatePushCruiseControl(void)
{
    // Get average speed from Hall sensors
    int16_t abs_speed = ABS(speedAvg);

    // Read nunchuk Y-axis (0-255, centered at 128)
    // nunchuk_data[1] is raw value, or use input2[inIdx].raw (scaled to ~-1000 to +1000)
    int16_t nunchuk_y = nunchuk_data[1] - 128;  // Convert to signed (-128 to +127)
    if (nunchuk_y < 0) nunchuk_y = 0;           // Only use forward push (positive Y)

    // Determine if nunchuk is actively requesting boost
    uint8_t nunchuk_active = (nunchuk_y > PA_NUNCHUK_DEADBAND);

    // Select thresholds based on mode
    int16_t engage = nunchuk_active ? PA_ENGAGE_EAGER : PA_ENGAGE_NORMAL;
    int16_t disengage = nunchuk_active ? PA_DISENGAGE_EAGER : PA_DISENGAGE_NORMAL;

    // Core logic: moving above threshold + below cap = assist
    if (abs_speed > engage && abs_speed < PA_SPEED_CAP) {
        ctrlModReq = TRQ_MODE;
        enable = 1;

        // Calculate torque with boost
        int16_t torque = PA_TORQUE_BASE;
        if (nunchuk_active) {
            // Proportional boost: 0 at deadband, max at full joystick
            int16_t boost_range = PA_TORQUE_MAX - PA_TORQUE_BASE;
            torque = PA_TORQUE_BASE + (nunchuk_y * boost_range) / 127;
            if (torque > PA_TORQUE_MAX) torque = PA_TORQUE_MAX;
        }

        // Apply torque in direction of travel
        if (speedAvg > 0) {
            cmdL = torque;
            cmdR = torque;
        } else {
            cmdL = -torque;
            cmdR = -torque;
        }
    } else {
        // FREEWHEEL MODE
        ctrlModReq = OPEN_MODE;
        enable = 0;
        cmdL = 0;
        cmdR = 0;
    }
}
```

[Source: _bmad-output/planning-artifacts/architecture.md#New Algorithm Architecture]

### Nunchuk Data Access

**Option A: Raw nunchuk data (recommended for precision)**
```c
extern uint8_t nunchuk_data[6];  // Declared in control.c, extern in util.c

// nunchuk_data[0] = X axis (0-255, center 127) - steering
// nunchuk_data[1] = Y axis (0-255, center 128) - speed/boost
// nunchuk_data[5] = buttons (bit 0 = Z, bit 1 = C)
```

**Option B: Processed input (already scaled)**
```c
extern InputStruct input1[];  // X axis scaled to ~-1000 to +1000
extern InputStruct input2[];  // Y axis scaled to ~-1000 to +1000

// Access via: input2[inIdx].raw or input2[inIdx].cmd (filtered)
```

[Source: Src/util.c lines 864-865, Src/control.c line 17]

### Nunchuk Connection State

```c
// In defines.h
typedef enum {
  NUNCHUK_CONNECTING,
  NUNCHUK_DISCONNECTED,
  NUNCHUK_RECONNECTING,
  NUNCHUK_CONNECTED
} nunchuk_state;

// Check connection (util.c line 1801)
extern nunchuk_state nunchukState;  // From control.c
uint8_t isNunchukConnected(void);   // Returns true if NUNCHUK_CONNECTED
```

**Note:** Story 2.2 handles disconnect fallback. For Story 2.1, assume nunchuk is connected.

[Source: Inc/defines.h lines 232-237, Src/util.c lines 1798-1801]

### Torque Calculation Details

| Joystick Position | nunchuk_y (after -128) | Torque Output |
|-------------------|------------------------|---------------|
| Centered | 0 | PA_TORQUE_BASE (150) |
| At deadband (10) | 10 | PA_TORQUE_BASE (150) - Normal mode |
| Quarter forward | ~32 | ~188 |
| Half forward | ~64 | ~225 |
| Full forward | 127 | PA_TORQUE_MAX (300) |

**Formula:** `torque = 150 + (nunchuk_y * 150) / 127`

### Mode Comparison

| Mode | Engage Threshold | Disengage Threshold | Torque | When Active |
|------|------------------|---------------------|--------|-------------|
| Normal | 20 RPM | 10 RPM | 150 (fixed) | Nunchuk centered |
| Eager | 3 RPM | 2 RPM | 150-300 (variable) | Nunchuk pushed forward |

### Dependencies

- **Story 1.1 MUST be complete** - PA_ parameters must exist
- **Story 1.2 MUST be complete** - Base algorithm must be implemented

### Previous Story Intelligence

**From Story 1.2:**
- `handleStatePushCruiseControl()` provides the base algorithm
- Speed variables: `speedAvg`, `speedAvgAbs` available
- Control mode switching: `ctrlModReq = TRQ_MODE` / `OPEN_MODE`
- Motor commands: `cmdL`, `cmdR`

**Changes from 1.2 to 2.1:**
- Add nunchuk_data extern declaration
- Add nunchuk_y reading and processing
- Add threshold selection logic
- Replace fixed torque with calculated torque

### Testing Checklist

| Test | Expected Result |
|------|-----------------|
| Nunchuk centered, push stroller | Normal mode: engage at 20 RPM, base torque |
| Nunchuk forward 50%, push | Eager mode: engage at 3 RPM, ~225 torque |
| Nunchuk forward 100%, push | Eager mode: engage at 3 RPM, 300 torque |
| At 5 RPM, push nunchuk forward | Assist engages immediately (3 RPM threshold met) |
| Release nunchuk while assisted | Returns to Normal mode, torque drops to base |
| Rapid nunchuk on/off | Smooth transitions, no jerking |

### Project Structure Notes

- **File to modify:** `Src/main.c` - enhance `handleStatePushCruiseControl()`
- **Add extern:** `extern uint8_t nunchuk_data[6];` near top of function or file
- **No new files required**

### References

- [Architecture: Algorithm] _bmad-output/planning-artifacts/architecture.md#New Algorithm Architecture
- [PRD: FR9-FR12, FR17] _bmad-output/planning-artifacts/prd.md#Functional Requirements
- [Epics: Story 2.1] _bmad-output/planning-artifacts/epics.md#Story 2.1
- [Nunchuk handling] Src/util.c lines 861-869
- [Nunchuk data structure] Src/control.c lines 17-20

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- Build verified with `make -e VARIANT=VARIANT_NUNCHUK` - successful compilation

### Completion Notes List

- **2026-01-10**: Implemented nunchuk boost & eager mode in handleStatePushCruiseControl()
  - Used `input2[inIdx].cmd` for nunchuk Y-axis (already has deadband applied by input processing)
  - Normal mode: PA_ENGAGE_NORMAL (20 RPM) / PA_DISENGAGE_NORMAL (10 RPM), PA_TORQUE_BASE (220)
  - Eager mode: PA_ENGAGE_EAGER (3 RPM) / PA_DISENGAGE_EAGER (2 RPM), variable torque up to PA_TORQUE_MAX (440)
  - Torque scales proportionally: `PA_TORQUE_BASE + (nunchuk_y * boost_range) / 500`
  - Mode selection is real-time per-loop - no state persistence issues
  - Hardware testing (subtasks 4.2-4.4) deferred to user

### Change Log

- **2026-01-10**: Story 2.1 implementation complete - Nunchuk boost & eager mode

### File List

- Src/main.c (modified - enhanced handleStatePushCruiseControl with nunchuk input, threshold selection, and variable torque calculation)
