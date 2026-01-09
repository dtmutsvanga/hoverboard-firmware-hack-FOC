# Product Requirements Document: Push-Assist Stroller System

**Version:** 1.0
**Date:** 2026-01-09
**Author:** Dan
**Status:** Draft

---

## 1. Overview

### 1.1 Product Vision

Transform a hoverboard mainboard into a push-assist system for a baby stroller, providing motorized assistance that amplifies the user's pushing effort while maintaining a natural, safe, and intuitive experience.

### 1.2 Problem Statement

Pushing a loaded baby stroller (~25kg) over extended distances or inclines causes user fatigue. Existing powered stroller solutions are either fully motorized (removing user control) or expensive dedicated products. Repurposing hoverboard hardware provides a cost-effective solution that augments rather than replaces user effort.

### 1.3 Target User

Parent or caregiver pushing a baby stroller who wants:
- Reduced physical effort during walks
- Maintained control over stroller movement
- Safe, predictable motor behavior around their child

---

## 2. Product Requirements

### 2.1 Functional Requirements

#### FR-1: Push Assistance

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-1.1 | System SHALL provide motor assist when user pushes the stroller | Critical |
| FR-1.2 | Assist SHALL be felt during acceleration phase | Critical |
| FR-1.3 | Assist SHALL be felt during constant velocity phase | Critical |
| FR-1.4 | Assist SHALL feel natural and intuitive | Critical |
| FR-1.5 | Motor SHALL NOT resist user's pushing motion at any time | Critical |

#### FR-2: Speed Control

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-2.1 | Maximum assisted speed SHALL be limited to normal walking pace | Critical |
| FR-2.2 | Walking pace defined as approximately 3-4 mph (~150-200 RPM on 8" wheels) | Critical |
| FR-2.3 | System SHALL allow bidirectional operation (forward and backward) | High |
| FR-2.4 | Assist level SHALL be equal in both directions | High |

#### FR-3: Stopping Behavior

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-3.1 | When user stops pushing, stroller movement SHALL stop | Critical |
| FR-3.2 | System SHALL NOT cause sudden or jerky stops | Critical |
| FR-3.3 | System SHALL NOT apply sudden braking | Critical |
| FR-3.4 | Motor SHALL freewheel when stroller is stationary | High |

#### FR-4: Direction Changes

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-4.1 | System SHALL NOT resist direction changes | Critical |
| FR-4.2 | System SHALL NOT cause jerky motion during direction reversal | Critical |
| FR-4.3 | Transition between forward and backward SHALL be smooth | High |

#### FR-5: Manual Override (Nunchuk)

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-5.1 | Nunchuk joystick SHALL allow adjustment of maximum assist torque | Medium |
| FR-5.2 | User SHALL be able to reduce assist level when desired | Medium |
| FR-5.3 | Nunchuk buttons SHALL allow mode switching | Medium |

---

### 2.2 Non-Functional Requirements

#### NFR-1: Safety

| ID | Requirement | Priority |
|----|-------------|----------|
| NFR-1.1 | Motor current SHALL be limited to safe levels (4A max) | Critical |
| NFR-1.2 | System SHALL disable motors on communication timeout | Critical |
| NFR-1.3 | System SHALL power off on low battery | Critical |
| NFR-1.4 | System SHALL power off on overtemperature | Critical |
| NFR-1.5 | User SHALL always be able to physically overpower motor | Critical |

#### NFR-2: Performance

| ID | Requirement | Priority |
|----|-------------|----------|
| NFR-2.1 | Control loop SHALL execute without bottlenecks | Critical |
| NFR-2.2 | Motor response latency SHALL be imperceptible to user | High |
| NFR-2.3 | System SHALL handle 25kg stroller load | High |

#### NFR-3: Usability

| ID | Requirement | Priority |
|----|-------------|----------|
| NFR-3.1 | No specialized training required to use | High |
| NFR-3.2 | Behavior SHALL be predictable and consistent | High |
| NFR-3.3 | Audio feedback (beeps) SHALL indicate state changes | Medium |

---

### 2.3 Constraints

#### Hardware Constraints

| ID | Constraint |
|----|------------|
| HC-1 | Uses existing hoverboard mainboard (STM32F103RCT6) |
| HC-2 | Uses existing hoverboard BLDC hub motors |
| HC-3 | Wheel diameter: 8 inches |
| HC-4 | Input device: Wii Nunchuk via I2C |
| HC-5 | Battery: 10S lithium pack (~36V nominal) |

#### Software Constraints

| ID | Constraint |
|----|------------|
| SC-1 | Based on hoverboard-firmware-hack-FOC codebase |
| SC-2 | FOC control algorithm is auto-generated (read-only) |
| SC-3 | Main loop must remain efficient (no blocking operations) |
| SC-4 | Must coexist with other operating modes |

---

## 3. User Experience

### 3.1 User Flow

```
[Stationary] → User pushes handle
     ↓
[Accelerating] → Motor provides increasing assist
     ↓
[Cruising] → Motor maintains steady assist
     ↓
[User releases] → Assist reduces, natural coast
     ↓
[Stopping] → Motor disengages, freewheel
     ↓
[Stationary] → Ready for next push
```

### 3.2 Expected Feel

| Phase | User Perception |
|-------|-----------------|
| **Idle** | Stroller rolls freely, no motor engagement |
| **Initial Push** | Light resistance as motors wake up, then assist kicks in |
| **Acceleration** | Stroller feels lighter than expected, pulls forward gently |
| **Cruising** | Minimal effort to maintain speed, like pushing on flat ground |
| **Slowing** | Natural deceleration, motor doesn't fight slowdown |
| **Stopping** | Stroller stops when user stops, no roll-away |
| **Reversing** | Same assist feel in opposite direction |

### 3.3 What Users Should NOT Experience

- Motor fighting their push direction
- Sudden acceleration or braking
- Jerky or unpredictable motion
- Resistance when trying to slow down
- Motor continuing to push after user stops
- Different behavior forward vs. backward

---

## 4. Acceptance Criteria

### 4.1 Core Functionality

| Test | Pass Criteria |
|------|---------------|
| Push from standstill | Assist engages smoothly within 0.5 seconds |
| Maintain cruise speed | Constant assist without reduction or resistance |
| Release handle | Natural deceleration, no sudden stop |
| Push backward | Equal assist as forward |
| Stop mid-push | Immediate freewheel, no motor resistance |
| Change direction | Smooth transition, no jerking |

### 4.2 Safety Tests

| Test | Pass Criteria |
|------|---------------|
| Overpower motor | User can stop stroller by grabbing wheels |
| Nunchuk disconnect | Motors disable within timeout period |
| Low battery | System powers off safely |
| Obstacle collision | Stroller stops when user stops pushing |

### 4.3 Performance Tests

| Test | Pass Criteria |
|------|---------------|
| Max speed | Does not exceed walking pace (~150 RPM) |
| Response time | Assist response < 100ms |
| Current draw | Stays within 4A per motor limit |
| Continuous operation | 30+ minutes without overheating |

---

## 5. Out of Scope

The following are explicitly NOT part of this product:

- Autonomous driving (user must always push)
- Obstacle detection/avoidance
- GPS tracking or navigation
- Remote control operation
- Balancing (like a hoverboard)
- Regenerative braking
- Mobile app integration
- Speed display/dashboard

---

## 6. Technical Parameters

### 6.1 Motor Control Settings

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Control Type | FOC (Field Oriented Control) | Smoothest torque output |
| Control Mode | TRQ_MODE (Torque) | Allows freewheeling, natural feel |
| Max Current | 4A per motor | Safe for 25kg load, user can overpower |
| Max Speed | 200 RPM | Walking pace on 8" wheels |

### 6.2 Assist Parameters (Recommended)

| Parameter | Value | Description |
|-----------|-------|-------------|
| Engage Speed | 5 RPM | Speed at which assist activates |
| Disengage Speed | 3 RPM | Speed at which assist deactivates |
| Base Torque | 10% | Minimum assist level |
| Max Torque | 25% | Maximum assist level |
| Ramp Zone | 30 RPM | Range over which assist increases |

---

## 7. Success Metrics

| Metric | Target |
|--------|--------|
| User-perceived resistance during cruise | None |
| Push effort compared to unpowered | 50% reduction |
| Safety incidents | Zero |
| Mode switching reliability | 100% |
| Battery life impact | < 20% reduction vs. normal walking |

---

## 8. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-09 | Dan | Initial draft |

---

## 9. Appendix: Reference Materials

- Original firmware: [hoverboard-firmware-hack-FOC](https://github.com/EFeru/hoverboard-firmware-hack-FOC)
- Technical Analysis: `_bmad-output/push-assist-analysis-2026-01-09.md`
- Task Definition: `tech-task.md`
