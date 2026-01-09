# Story 1.1: Push-Assist Configuration

Status: review

## Story

As a **developer**,
I want **push-assist parameters defined in config.h**,
So that **the algorithm thresholds and limits can be easily tuned without code changes**.

## Acceptance Criteria

1. **Given** the `Inc/config.h` file
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

2. **And** parameters follow existing naming convention (prefix + UPPER_CASE)

3. **And** parameters include descriptive comments

4. **And** code compiles without errors

## Tasks / Subtasks

- [x] Task 1: Add push-assist parameters section to config.h (AC: #1, #2, #3)
  - [x] Subtask 1.1: Locate appropriate insertion point (after CRUISE CONTROL SETTINGS section, before DEBUG SERIAL)
  - [x] Subtask 1.2: Add section header comment following existing pattern
  - [x] Subtask 1.3: Define all 8 required parameters with descriptive comments
  - [x] Subtask 1.4: Add section footer comment
- [x] Task 2: Verify compilation (AC: #4)
  - [x] Subtask 2.1: Build project with Keil uVision or equivalent
  - [x] Subtask 2.2: Confirm no compiler errors or warnings related to new defines

## Dev Notes

### Architecture Compliance

This story implements the configuration structure specified in the Architecture Decision Document:

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

[Source: _bmad-output/planning-artifacts/architecture.md#Configuration Structure]

### Existing Code Patterns to Follow

| Convention | Example from config.h |
|------------|----------------------|
| Section headers | `// ############################### SECTION NAME ###############################` |
| Section footers | `// ########################### END OF SECTION NAME ############################` |
| Macro naming | `PA_` prefix + `UPPER_CASE` (follows `BAT_`, `TEMP_` pattern) |
| Comment style | `// [unit] Description` or inline after value |
| Value alignment | Values aligned for readability |

[Source: Inc/config.h lines 69-89, 93-111]

### Insertion Point

Insert the new section between:
- **AFTER:** `// ######################### END OF CRUISE CONTROL SETTINGS ##########################` (line 230)
- **BEFORE:** `// ############################### DEBUG SERIAL ###############################` (line 234)

This placement groups motor-related configuration together and follows the logical flow of the file.

### Parameter Rationale

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| `PA_ENGAGE_NORMAL` | 20 RPM | Smooth everyday operation - engage after clear movement detected |
| `PA_DISENGAGE_NORMAL` | 10 RPM | 10 RPM hysteresis prevents oscillation |
| `PA_ENGAGE_EAGER` | 3 RPM | Instant response when user requests via nunchuk |
| `PA_DISENGAGE_EAGER` | 2 RPM | Minimal hysteresis for responsive feel |
| `PA_SPEED_CAP` | 170 RPM | 6.5 kph - fast walking pace, safety limit |
| `PA_TORQUE_BASE` | 150 | ~2.5A effective - noticeable assist, easily overpowered |
| `PA_TORQUE_MAX` | 300 | ~5A effective - strong assist for inclines |
| `PA_NUNCHUK_DEADBAND` | 10 | Filters nunchuk noise when centered |

[Source: _bmad-output/planning-artifacts/prd.md#Calculated Parameters, NFR13-NFR19]

### Hardware Context

- **Torque scale:** 0-1000 maps to motor current request
- **Current relationship:** I_MOT_MAX = 4A (line 157), so 300/1000 = ~1.2A actual
- **Speed measurement:** RPM from Hall sensors, available as `speedAvg` in main loop
- **Existing limits:** N_MOT_MAX = 200 RPM (line 159) - speed cap should be below this

[Source: Inc/config.h lines 136-173]

### Project Structure Notes

- **File to modify:** `Inc/config.h` (single file change)
- **No new files required**
- **No dependencies on other stories** - this is foundational configuration
- **Story 1.2 will consume these defines** in the algorithm implementation

### Build Verification

After adding parameters, verify build succeeds:
1. Open `MDK-ARM/mainboard-hack.uvprojx` in Keil uVision
2. Select VARIANT_NUNCHUK target
3. Build (F7)
4. Confirm 0 errors, 0 warnings related to PA_ defines

### References

- [Architecture: Configuration Structure] _bmad-output/planning-artifacts/architecture.md#Configuration Structure
- [PRD: Hardware Limits] _bmad-output/planning-artifacts/prd.md#Hardware Limits NFR13-NFR19
- [Epics: Story 1.1] _bmad-output/planning-artifacts/epics.md#Story 1.1
- [Existing config patterns] Inc/config.h lines 69-111 (BATTERY, TEMPERATURE sections)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5 (claude-opus-4-5-20251101)

### Debug Log References

- Build output: 0 errors, 0 warnings related to PA_ defines
- Build artifacts: build/hover.elf (48,752 bytes text), build/hover.hex, build/hover.bin

### Completion Notes List

- Added push-assist parameters section to Inc/config.h at lines 234-247
- Section placed after CRUISE CONTROL SETTINGS, before DEBUG SERIAL (as specified)
- All 8 PA_ parameters defined with descriptive comments following existing naming conventions
- Build verified with arm-none-eabi-gcc via Makefile for VARIANT_NUNCHUK target
- Pre-existing warnings in util.c:190 and main.c:712 are unrelated to this story

### Change Log

- 2026-01-09: Added PUSH-ASSIST PARAMETERS section to config.h with 8 configuration defines

### File List

- Inc/config.h (modified) - Added PA_ parameter definitions at lines 234-247
