# Story 2.2: Safe Defaults on Disconnect

Status: ready-for-dev

## Story

As a **parent using the stroller**,
I want **the system to continue working safely if the nunchuk disconnects**,
So that **I'm not stranded with a non-functional stroller**.

## Acceptance Criteria

1. **Given** nunchuk is connected and working
   **When** the cable disconnects or communication fails
   **Then** system defaults to Normal mode (20/10 RPM, PA_TORQUE_BASE) (FR13)
   **And** assist continues working - does not suddenly stop (FR20)

2. **Given** nunchuk was in Eager mode with boost
   **When** disconnect occurs
   **Then** torque drops to PA_TORQUE_BASE (not zero)
   **And** thresholds revert to Normal (20/10 RPM)

3. **Given** nunchuk reconnects after disconnect
   **When** valid data is received again
   **Then** system resumes responding to nunchuk input normally

## Tasks / Subtasks

- [ ] Task 1: Add nunchuk connection state check (AC: #1)
  - [ ] Subtask 1.1: Add extern for `nunchukState` or use `isNunchukConnected()`
  - [ ] Subtask 1.2: Check connection state at start of handleStatePushCruiseControl()
  - [ ] Subtask 1.3: If disconnected, force Normal mode parameters

- [ ] Task 2: Ensure graceful degradation (AC: #1, #2)
  - [ ] Subtask 2.1: On disconnect, use PA_ENGAGE_NORMAL / PA_DISENGAGE_NORMAL
  - [ ] Subtask 2.2: On disconnect, use PA_TORQUE_BASE (ignore boost)
  - [ ] Subtask 2.3: Do NOT set enable=0 or ctrlModReq=OPEN_MODE on disconnect

- [ ] Task 3: Handle reconnection (AC: #3)
  - [ ] Subtask 3.1: No special code needed - algorithm naturally reads live state
  - [ ] Subtask 3.2: Verify transition is smooth when nunchuk reconnects

- [ ] Task 4: Test disconnect scenarios (AC: all)
  - [ ] Subtask 4.1: Disconnect while in Normal mode - assist continues
  - [ ] Subtask 4.2: Disconnect while in Eager mode - drops to Normal, assist continues
  - [ ] Subtask 4.3: Reconnect after disconnect - responds to nunchuk again

## Dev Notes

### Architecture-Specified Behavior

**Key principle:** Disconnect = safe default, NOT disable.

The existing firmware timeout handler (`handleTimeout()` in util.c) sets `ctrlModReq = OPEN_MODE` when nunchuk times out. This would STOP assist entirely - which is NOT what we want.

Our push-assist function runs AFTER the timeout handler and can override this behavior by checking nunchuk state and applying Normal mode instead of stopping.

[Source: _bmad-output/planning-artifacts/epics.md#Story 2.2]

### Algorithm Enhancement

Add connection check to the algorithm from Story 2.1:

```c
static void handleStatePushCruiseControl(void)
{
    int16_t abs_speed = ABS(speedAvg);

    // Check nunchuk connection state
    uint8_t nunchuk_connected = (nunchukState == NUNCHUK_CONNECTED);
    // Alternative: uint8_t nunchuk_connected = isNunchukConnected();

    // Read nunchuk Y-axis (only if connected)
    int16_t nunchuk_y = 0;
    if (nunchuk_connected) {
        nunchuk_y = nunchuk_data[1] - 128;
        if (nunchuk_y < 0) nunchuk_y = 0;
    }
    // If disconnected, nunchuk_y stays 0 → Normal mode

    // Determine if nunchuk is actively requesting boost (AND connected)
    uint8_t nunchuk_active = nunchuk_connected && (nunchuk_y > PA_NUNCHUK_DEADBAND);

    // Select thresholds - defaults to Normal if disconnected
    int16_t engage = nunchuk_active ? PA_ENGAGE_EAGER : PA_ENGAGE_NORMAL;
    int16_t disengage = nunchuk_active ? PA_DISENGAGE_EAGER : PA_DISENGAGE_NORMAL;

    // Core logic unchanged
    if (abs_speed > engage && abs_speed < PA_SPEED_CAP) {
        ctrlModReq = TRQ_MODE;
        enable = 1;

        // Calculate torque - defaults to base if disconnected
        int16_t torque = PA_TORQUE_BASE;
        if (nunchuk_active) {
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
        ctrlModReq = OPEN_MODE;
        enable = 0;
        cmdL = 0;
        cmdR = 0;
    }
}
```

**Key changes from Story 2.1:**
1. Add `nunchuk_connected` check
2. Only read nunchuk_y if connected (default to 0)
3. `nunchuk_active` requires connection AND joystick pushed

### Nunchuk State Detection

**Option A: Direct state check (recommended)**
```c
extern nunchuk_state nunchukState;  // From control.c

uint8_t nunchuk_connected = (nunchukState == NUNCHUK_CONNECTED);
```

**Option B: Helper function**
```c
// Declared in util.c, checks nunchukState == NUNCHUK_CONNECTED
uint8_t nunchuk_connected = isNunchukConnected();
```

[Source: Inc/defines.h lines 232-237, Src/util.c lines 1798-1801]

### Existing Timeout Behavior (to be overridden)

```c
// In util.c handleTimeout() - lines 1050-1052
if (timeoutFlgADC || timeoutFlgSerial || timeoutFlgGen) {
    ctrlModReq = OPEN_MODE;  // <-- This would STOP assist
    input1[inIdx].cmd = 0;
    // ...
}
```

**Our override:** Since `handleStatePushCruiseControl()` runs in the main loop AFTER `handleTimeout()`, we can set `ctrlModReq = TRQ_MODE` when assist conditions are met, regardless of timeout state.

The algorithm naturally handles this because:
1. If nunchuk disconnects → `nunchuk_connected = false`
2. → `nunchuk_active = false` (can't be active if not connected)
3. → Normal mode thresholds and base torque
4. → Assist continues in Normal mode

### Behavior Matrix

| Nunchuk State | Joystick Position | Mode | Thresholds | Torque |
|---------------|-------------------|------|------------|--------|
| Connected | Centered | Normal | 20/10 RPM | 150 |
| Connected | Forward | Eager | 3/2 RPM | 150-300 |
| Disconnected | N/A | Normal | 20/10 RPM | 150 |
| Reconnecting | N/A | Normal | 20/10 RPM | 150 |

### Testing Checklist

| Test | Expected Result |
|------|-----------------|
| Normal operation, disconnect cable | Beep warning, assist continues in Normal mode |
| Eager mode + boost, disconnect cable | Drops to Normal mode, assist continues at base torque |
| Assist engaged, disconnect | No sudden stop or jerk |
| Reconnect after disconnect | Nunchuk control resumes |
| Disconnect during direction change | Safe freewheel through zero, Normal mode assist |

### Dependencies

- **Story 1.1 MUST be complete** - PA_ parameters
- **Story 1.2 MUST be complete** - Base algorithm
- **Story 2.1 MUST be complete** - Nunchuk integration (this story adds disconnect handling)

### Previous Story Intelligence

**From Story 2.1:**
- Nunchuk Y-axis reading implemented
- Eager mode threshold selection implemented
- Variable torque calculation implemented

**This story adds:**
- `nunchukState` check
- Graceful degradation to Normal mode on disconnect

### Project Structure Notes

- **File to modify:** `Src/main.c` - add connection check to `handleStatePushCruiseControl()`
- **Add extern:** `extern nunchuk_state nunchukState;` if not already present
- **No new files required**
- **Minimal code change** - just wrap existing nunchuk logic with connection check

### References

- [Architecture: Safety] _bmad-output/planning-artifacts/architecture.md (safety model)
- [PRD: FR13, FR20] _bmad-output/planning-artifacts/prd.md#Functional Requirements
- [PRD: Safety Model] _bmad-output/planning-artifacts/prd.md#Safety Model
- [Epics: Story 2.2] _bmad-output/planning-artifacts/epics.md#Story 2.2
- [Timeout handler] Src/util.c lines 959-1052
- [Nunchuk state enum] Inc/defines.h lines 232-237

## Dev Agent Record

### Agent Model Used

{{agent_model_name_version}}

### Debug Log References

### Completion Notes List

### File List

- Src/main.c (modified - add nunchuk connection check to handleStatePushCruiseControl)
