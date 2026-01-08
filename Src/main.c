/*
 * This file is part of the hoverboard-firmware-hack project.
 *
 * Copyright (C) 2017-2018 Rene Hopf <renehopf@mac.com>
 * Copyright (C) 2017-2018 Nico Stute <crinq@crinq.de>
 * Copyright (C) 2017-2018 Niklas Fauth <niklas.fauth@kit.fail>
 * Copyright (C) 2019-2020 Emanuel FERU <aerdronix@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h> // for abs()
#include <math.h>
#include "stm32f1xx_hal.h"
#include "defines.h"
#include "setup.h"
#include "config.h"
#include "util.h"
#include "BLDC_controller.h" /* BLDC's header file */
#include "rtwtypes.h"
#include "comms.h"

#if defined(DEBUG_I2C_LCD) || defined(SUPPORT_LCD)
    #include "hd44780.h"
#endif

void SystemClock_Config(void);

//------------------------------------------------------------------------
// Global variables set externally
//------------------------------------------------------------------------
extern TIM_HandleTypeDef htim_left;
extern TIM_HandleTypeDef htim_right;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern volatile adc_buf_t adc_buffer;
#if defined(DEBUG_I2C_LCD) || defined(SUPPORT_LCD)
extern LCD_PCF8574_HandleTypeDef lcd;
extern uint8_t LCDerrorFlag;
#endif

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

volatile uint8_t uart_buf[200];

// Matlab defines - from auto-code generation
//---------------
extern P rtP_Left;     /* Block parameters (auto storage) */
extern P rtP_Right;    /* Block parameters (auto storage) */
extern ExtY rtY_Left;  /* External outputs */
extern ExtY rtY_Right; /* External outputs */
extern ExtU rtU_Left;  /* External inputs */
extern ExtU rtU_Right; /* External inputs */
//---------------

extern uint8_t inIdx; // input index used for dual-inputs
extern uint8_t inIdx_prev;
extern InputStruct input1[]; // input structure
extern InputStruct input2[]; // input structure

extern int16_t speedAvg;                // Average measured speed
extern int16_t speedAvgAbs;             // Average measured speed in absolute
extern volatile uint32_t timeoutCntGen; // Timeout counter for the General timeout (PPM, PWM,
                                        // Nunchuk)
extern volatile uint8_t timeoutFlgGen;  // Timeout Flag for the General timeout (PPM, PWM, Nunchuk)
extern uint8_t timeoutFlgADC; // Timeout Flag for for ADC Protection: 0 = OK, 1 = Problem detected
                              // (line disconnected or wrong ADC data)
extern uint8_t timeoutFlgSerial; // Timeout Flag for Rx Serial command: 0 = OK, 1 = Problem detected
                                 // (line disconnected or wrong Rx data)

extern volatile int pwml; // global variable for pwm left. -1000 to 1000
extern volatile int pwmr; // global variable for pwm right. -1000 to 1000

extern uint8_t enable; // global variable for motor enable
extern uint8_t cruiseCtrlAcv; // cruise control active flag (defined in util.c)
extern uint8_t ctrlModReq; // control mode request (defined in util.c) - 0:OPEN, 1:VLT, 2:SPD, 3:TRQ

extern int16_t batVoltage; // global variable for battery voltage

#if defined(SIDEBOARD_SERIAL_USART2)
extern SerialSideboard Sideboard_L;
#endif
#if defined(SIDEBOARD_SERIAL_USART3)
extern SerialSideboard Sideboard_R;
#endif
#if (defined(CONTROL_PPM_LEFT) && defined(DEBUG_SERIAL_USART3)) ||                                 \
    (defined(CONTROL_PPM_RIGHT) && defined(DEBUG_SERIAL_USART2))
extern volatile uint16_t ppm_captured_value[PPM_NUM_CHANNELS + 1];
#endif
#if (defined(CONTROL_PWM_LEFT) && defined(DEBUG_SERIAL_USART3)) ||                                 \
    (defined(CONTROL_PWM_RIGHT) && defined(DEBUG_SERIAL_USART2))
extern volatile uint16_t pwm_captured_ch1_value;
extern volatile uint16_t pwm_captured_ch2_value;
#endif

//------------------------------------------------------------------------
// Global variables set here in main.c
//------------------------------------------------------------------------
uint8_t backwardDrive;
extern volatile uint32_t buzzerTimer;
volatile uint32_t main_loop_counter;
int16_t batVoltageCalib;  // global variable for calibrated battery voltage
int16_t board_temp_deg_c; // global variable for calibrated temperature in degrees Celsius
int16_t left_dc_curr;     // global variable for Left DC Link current
int16_t right_dc_curr;    // global variable for Right DC Link current
int16_t dc_curr;          // global variable for Total DC Link current
int16_t cmdL;             // global variable for Left Command
int16_t cmdR;             // global variable for Right Command

//------------------------------------------------------------------------
// Local variables
//------------------------------------------------------------------------
#if defined(FEEDBACK_SERIAL_USART2) || defined(FEEDBACK_SERIAL_USART3)
typedef struct {
    uint16_t start;
    int16_t cmd1;
    int16_t cmd2;
    int16_t speedR_meas;
    int16_t speedL_meas;
    int16_t batVoltage;
    int16_t boardTemp;
    uint16_t cmdLed;
    uint16_t checksum;
} SerialFeedback;

static SerialFeedback Feedback;
#endif
#if defined(FEEDBACK_SERIAL_USART2)
static uint8_t sideboard_leds_L;
#endif
#if defined(FEEDBACK_SERIAL_USART3)
static uint8_t sideboard_leds_R;
#endif

#ifdef VARIANT_TRANSPOTTER
uint8_t nunchuk_connected;
extern float setDistance;

static uint8_t checkRemote = 0;
static uint16_t distance;
static float steering;
static int distanceErr;
static int lastDis tance = 0;
static uint16_t transpotter_counter = 0;
#endif

static int16_t speed; // local variable for speed. -1000 to 1000
#ifndef VARIANT_TRANSPOTTER
static int16_t steer;          // local variable for steering. -1000 to 1000
static int16_t steerRateFixdt; // local fixed-point variable for steering rate limiter
static int16_t speedRateFixdt; // local fixed-point variable for speed rate limiter
static int32_t steerFixdt;     // local fixed-point variable for steering low-pass filter
static int32_t speedFixdt;     // local fixed-point variable for speed low-pass filter
#endif

static uint32_t buzzerTimer_prev = 0;
static uint32_t inactivity_timeout_counter;
static MultipleTap MultipleTapBrake; // define multiple tap functionality for the Brake pedal

static uint16_t rate = RATE; // Adjustable rate to support multiple drive modes on startup

// Static variable to track PCC (Push Cruise Control) state reset - needs to be reset when entering State 0
static uint8_t pcc_needs_reset = 0;

/*******************************************************************************
 * PUSH CRUISE CONTROL (PCC) - Full Implementation
 * E-Bike Style Assistive Speed Control
 * 
 * Concept: User pushes the device to initiate movement. System detects the
 * wheel speed, measures it over a sampling window, then engages motor assist
 * to maintain that speed, reducing user effort.
 ******************************************************************************/

/* PCC Configuration Parameters - Tuned for 25kg stroller
 * 
 * TORQUE-BASED ASSIST APPROACH (TRQ_MODE):
 * - Motor applies constant assist TORQUE in direction of wheel movement
 * - Doesn't care about exact speed - just adds gentle push
 * - Never fights user: torque is always in direction of travel
 * - No back-EMF issues: torque mode bypasses voltage control
 */
#define PCC_ENGAGE_SPEED_RPM        25      // Speed to START assist
#define PCC_DISENGAGE_SPEED_RPM     5       // Speed to STOP assist (hysteresis)
#define PCC_MAX_SPEED_RPM           400     // Maximum allowed assist speed
#define PCC_HOLD_TIME_MS            10000   // Max coast time after user releases
#define PCC_RAMP_RATE_RPM_PER_SEC   200     // How fast motor ramps (RPM per second)
#define PCC_COOLDOWN_MS             150     // Minimum time in IDLE before re-engaging
#define PCC_ACCEL_THRESHOLD         3       // RPM/loop to detect acceleration (lowered)

// Torque-Based Assist Parameters (TRQ_MODE)
// In TRQ_MODE, PWM controls motor current (torque) directly, range -1000 to +1000
// We apply a CONSTANT assist torque in the direction of wheel movement
// This doesn't care about exact speed - just adds gentle push in direction of travel
#define PCC_ASSIST_TORQUE_BASE      (80 * 3)      // Base assist (out of 1000), ~24% - gentle push
#define PCC_ASSIST_TORQUE_ACCEL     300     // Assist when accelerating - stronger pull (MUST be > BASE)
#define PCC_ASSIST_TORQUE_MAX       1000    // Maximum assist torque (no aggressive clamping)

/* PCC State definitions */
typedef enum {
    PCC_STATE_IDLE,         // Waiting for push, motors disabled
    PCC_STATE_TRACKING,     // Motor tracking user's push speed (assist mode)
    PCC_STATE_COASTING,     // User released, maintaining last speed briefly
    PCC_STATE_RAMP_DOWN     // Gracefully disengaging
} PCC_State_t;

/* PCC Internal context structure */
typedef struct {
    PCC_State_t state;
    uint32_t    stateEntryTime;     // HAL_GetTick() when state was entered
    uint32_t    idleEntryTime;      // When we entered IDLE (for cooldown)
    int16_t     targetL;            // Current target speed for left motor
    int16_t     targetR;            // Current target speed for right motor
    int16_t     lastSpeedL;         // Previous loop's speed (for accel detection)
    int16_t     lastSpeedR;         // Previous loop's speed (for accel detection)
    int8_t      assistDirL;         // Locked assist direction: +1=forward, -1=backward, 0=none
    int8_t      assistDirR;         // Locked assist direction: +1=forward, -1=backward, 0=none
    uint8_t     initialized;        // Init flag
} PCC_Context_t;

static PCC_Context_t pcc = {PCC_STATE_IDLE, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#ifdef MULTI_MODE_DRIVE
static uint8_t drive_mode;
static uint16_t max_speed;
#endif

#define PWROFF_BTN_PRESS_MAX 5U
#define MAX_SET_STEPS        4U
#define SPD_UPD_TM_ms        1U
/* regulated by Up Down joystick */
#define MAX_MOVE_TIME_ms  3000U
#define MIN_MOVE_TIME_ms  1500U
#define TIME_AMPLITUDE_ms (MAX_MOVE_TIME_ms - MIN_MOVE_TIME_ms)
#define TIME_SET_STEP_ms  (TIME_AMPLITUDE_ms / MAX_SET_STEPS)
#define STRT_MOVE_TIME_ms (MIN_MOVE_TIME_ms + TIME_AMPLITUDE_ms / 2U)
#define STRT_ROCKER_DELAY 2000U
#define time_tick buzzerTimer
/* regulated by left right joystick */
#define MAX_SPEED          N_MOT_MAX
#define MIN_SPEED          100U
#define MAX_ROCKER_TIME_ms (25UL * 60UL * 1000UL) /* 25 minutes */
#define SPEED_AMPLITUDE    (MAX_SPEED - MIN_SPEED)
#define SPEED_SET_STEPS    (SPEED_AMPLITUDE / MAX_SET_STEPS)
#define STRT_MAX_SPEED     (SPEED_AMPLITUDE / 2U + MIN_SPEED)

/* Defaults */
#define DEFAULT_MOVE_TIME (STRT_MOVE_TIME_ms + TIME_SET_STEP_ms)
#define DEFAULT_SPEED     (STRT_MAX_SPEED + SPEED_SET_STEPS * 1U)

#define TICKS_1MS    (16)
#define IS_TIMEOUT_1MS() ((buzzerTimer - buzzerTimer_prev) > (TICKS_1MS * DELAY_IN_MAIN_LOOP))
static uint8_t rockerEnable = 0; /* Enable the motors */
static uint16_t moveTime = STRT_MOVE_TIME_ms;
static uint16_t maxSpeed = STRT_MAX_SPEED;
static uint32_t lastRegulationTime = 0;
static int16_t currAvgSpeed = 0;

/**
 * @brief Calculate cumulative wheel displacement
 * @param reset: 1 = reset displacement to zero, 0 = accumulate
 * @return Current displacement in milli-rotations
 * @note 1000 milli-rotations = 1 full wheel rotation
 */
static int32_t calcDispacement(uint8_t reset)
{
    static int32_t _currDisplacement = 0;
    static uint32_t prevTime = 0;  // FIX BR-3: Changed to uint32_t for consistency

    if (reset) {
        _currDisplacement = 0;
        prevTime = HAL_GetTick();
        return 0;
    }

    uint32_t currTm = HAL_GetTick();
    uint32_t dt_ms = currTm - prevTime;
    
    // FIX BR-3: Correct unit conversion
    // Result in milli-rotations: (RPM * ms) / 60 = milli-rotations
    // Prevent division issues and overflow with sanity check
    if (dt_ms > 0 && dt_ms < 1000) {  // < 1 second sanity check
        _currDisplacement += ((int32_t)currAvgSpeed * (int32_t)dt_ms) / 60;
    }
    
    prevTime = currTm;
    return _currDisplacement;
}

static int16_t speedSinus(uint8_t reset)
{
    static uint32_t lastTime = 0;
    static int16_t prevSpd = 0;
    static uint32_t startTime = 0;

    static uint16_t _moveTime = DEFAULT_MOVE_TIME;
    static uint16_t _maxSpeed = DEFAULT_SPEED;
    static uint8_t first = 1;
    
    if (reset) {
        lastTime = 0;
        prevSpd = 0;
        startTime = HAL_GetTick();
        _moveTime = moveTime;
        _maxSpeed = maxSpeed;
        first = 1;
    }
    
    // Handle startup delay - return zero speed during this period
    if (first) {
        if (HAL_GetTick() - startTime > STRT_ROCKER_DELAY) {
            startTime = HAL_GetTick();  // Synchronize cycle start after delay
            first = 0;
        } else {
            return 0;  // Return zero during startup delay for safety
        }
    }

    uint32_t tm = HAL_GetTick();
    
    // Rate limit updates
    if (tm - lastTime < SPD_UPD_TM_ms && lastTime > 0) {
        return prevSpd;
    }
    lastTime = tm;
    
    // FIX BR-1: Calculate RELATIVE time within period (not absolute time!)
    // This ensures the sine wave is periodic and predictable
    uint32_t relative_tm = (tm - startTime) % _moveTime;
    
    float angle = 2.0f * 3.141593f * (float)relative_tm / (float)_moveTime;
    float spd_f = cosf(angle);
    prevSpd = (int16_t)(spd_f * (float)(_maxSpeed)); /* Amplitude */

    // FIX BR-2: Check LOCAL variable prevSpd (not global speed) for zero-crossing
    // FIX BR-5: Use != instead of XOR for inequality check (better readability)
    /* Only update parameters at the end/start of a cycle (near zero crossing) */
    int16_t threshold = (int16_t)(_maxSpeed / 20);  // 5% of max speed
    if (prevSpd >= 0 && prevSpd < threshold)
    {
        if ((_moveTime != moveTime) || (_maxSpeed != maxSpeed)) {
            _moveTime = moveTime;
            _maxSpeed = maxSpeed;
            startTime = HAL_GetTick();
            lastTime = 0;
            prevSpd = 0;
            // Note: Don't reset first here - we want smooth parameter transitions
        }
    }
    return prevSpd;
}

static void setBoardTemperature(int16_t *pboard_temp_adcFilt, int32_t *pboard_temp_adcFixdt)
{
    // ####### CALC BOARD TEMPERATURE #######
    filtLowPass32(adc_buffer.temp, TEMP_FILT_COEF, pboard_temp_adcFixdt);
    *pboard_temp_adcFilt = (int16_t)((*pboard_temp_adcFixdt) >> 16); // convert fixed-point to
                                                                     // integer
    board_temp_deg_c = (TEMP_CAL_HIGH_DEG_C - TEMP_CAL_LOW_DEG_C) *
                           (*pboard_temp_adcFilt - TEMP_CAL_LOW_ADC) /
                           (TEMP_CAL_HIGH_ADC - TEMP_CAL_LOW_ADC) +
                       TEMP_CAL_LOW_DEG_C;
}

static void setDcLinkCurrent(void)
{
    left_dc_curr = -(rtU_Left.i_DCLink * 100) / A2BIT_CONV;   // Left DC Link Current * 100
    right_dc_curr = -(rtU_Right.i_DCLink * 100) / A2BIT_CONV; // Right DC Link Current * 100
    dc_curr = left_dc_curr + right_dc_curr;                   // Total DC Link Current * 100
}

static void updateSpeedGlobals()
{
    currAvgSpeed = getSpeed();
}

static void init_hw(void)
{
    HAL_Init();
    __HAL_RCC_AFIO_CLK_ENABLE();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    /* System interrupt init*/
    /* MemoryManagement_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
    /* BusFault_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
    /* UsageFault_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
    /* SVCall_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
    /* DebugMonitor_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
    /* PendSV_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

    SystemClock_Config();

    __HAL_RCC_DMA1_CLK_DISABLE();
    MX_GPIO_Init();
    MX_TIM_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();
    BLDC_Init(); // BLDC Controller Init

    HAL_GPIO_WritePin(OFF_PORT, OFF_PIN, GPIO_PIN_SET); // Activate Latch
    Input_Lim_Init();                                   // Input Limitations Init
    Input_Init();                                       // Input Init

    HAL_ADC_Start(&hadc1);
    HAL_ADC_Start(&hadc2);

    poweronMelody();
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

static void sendDebugInformation(int16_t board_temp_adcFilt)
{
#if defined(DEBUG_SERIAL_USART2) || defined(DEBUG_SERIAL_USART3)
    if (main_loop_counter % 25 == 0) { // Send data periodically every 125 ms
    #if defined(DEBUG_SERIAL_PROTOCOL)
        process_debug();
    #else
        printf("in1:%i in2:%i cmdL:%i cmdR:%i BatADC:%i BatV:%i TempADC:%i Temp:%i \r\n",
               input1[inIdx].raw,  // 1: INPUT1
               input2[inIdx].raw,  // 2: INPUT2
               cmdL,               // 3: output command: [-1000, 1000]
               cmdR,               // 4: output command: [-1000, 1000]
               adc_buffer.batt1,   // 5: for battery voltage calibration
               batVoltageCalib,    // 6: for verifying battery voltage calibration
               board_temp_adcFilt, // 7: for board temperature calibration
               board_temp_deg_c);  // 8: for verifying board temperature calibration
    #endif /* defined(DEBUG_SERIAL_PROTOCOL) */
    }
#endif /* defined(DEBUG_SERIAL_USART2) || defined(DEBUG_SERIAL_USART3) */
}

static void handleSystemWarningsAndErrors(void)
{
    if (BAT_DEAD_ENABLE && batVoltage < BAT_DEAD && speedAvgAbs < 20) {
#if defined(DEBUG_SERIAL_USART2) || defined(DEBUG_SERIAL_USART3)
        printf("Powering off, battery voltage is too low\r\n");
#endif
        poweroff();
    } else if (rtY_Left.z_errCode || rtY_Right.z_errCode) {
        enable = 0;
        beepCount(1, 24, 1);
    } else if (timeoutFlgADC) {
        beepCount(2, 24, 1);
    } else if (timeoutFlgSerial) {
        beepCount(3, 24, 1);
    } else if (timeoutFlgGen) {
        beepCount(4, 24, 1);
    } else if (TEMP_WARNING_ENABLE && board_temp_deg_c >= TEMP_WARNING) {
        beepCount(5, 24, 1);
    } else if (BAT_LVL1_ENABLE && batVoltage < BAT_LVL1) {
        beepCount(0, 10, 6);
    } else if (BAT_LVL2_ENABLE && batVoltage < BAT_LVL2) {
        beepCount(0, 10, 30);
    } else if (BEEPS_BACKWARD &&
               (((cmdR < -50 || cmdL < -50) && speedAvg < 0) || MultipleTapBrake.b_multipleTap)) {
        beepCount(0, 5, 1);
        backwardDrive = 1;
    } else {
        beepCount(0, 0, 0);
        backwardDrive = 0;
    }
}

static void handleTemperaturePowerOff(void)
{
    if (TEMP_POWEROFF_ENABLE && board_temp_deg_c >= TEMP_POWEROFF &&
        speedAvgAbs < 20) { // poweroff
                            // before
                            // mainboard
                            // burns OR
                            // low bat
                            // 3
#if defined(DEBUG_SERIAL_USART2) || defined(DEBUG_SERIAL_USART3)
        printf("Powering off, temperature is too high\r\n");
#endif
        poweroff();
    }
}

static void handleCruiseControlInactivity(void)
{
#if defined(CRUISE_CONTROL_SUPPORT) || defined(STANDSTILL_HOLD_ENABLE)
    if ((abs(rtP_Left.n_cruiseMotTgt) > 50 && rtP_Left.b_cruiseCtrlEna) ||
        (abs(rtP_Right.n_cruiseMotTgt) > 50 && rtP_Right.b_cruiseCtrlEna)) {
        inactivity_timeout_counter = 0;
    }
#endif
}

static void handleInactivityPowerOff(void)
{
    if (abs(cmdL) > 50 || abs(cmdR) > 50) {
        inactivity_timeout_counter = 0;
    }
    if (inactivity_timeout_counter >
        (INACTIVITY_TIMEOUT * 60 * 1000) / (DELAY_IN_MAIN_LOOP + 1)) { // rest of main loop
                                                                       // needs maybe 1ms
#if defined(DEBUG_SERIAL_USART2) || defined(DEBUG_SERIAL_USART3)
        printf("Powering off, wheels were inactive for too long\r\n");
#endif
        poweroff();
    }
}

void handleMotorControlModeAndEnable(void)
{
    if (rockerEnable == 0 && !rtY_Left.z_errCode && !rtY_Right.z_errCode &&
        ABS(input1[inIdx].cmd) < 50 && ABS(input2[inIdx].cmd) < 50) {
        beepShort(56); // make 2 beeps indicating the motor enable
        beepShort(54);
        HAL_Delay(100);
        steerFixdt = speedFixdt = 0; // reset filters
        rockerEnable = 1;            // enable motors
    } else if (rockerEnable && (ABS(input1[inIdx].cmd) > 25 || ABS(input2[inIdx].cmd) > 25)) {
        enable = 1;
    }
}

uint8_t isPowerOffRequest(uint8_t btnStateZ, uint8_t currBtnStateZ)
{
    return !btnStateZ && currBtnStateZ;
}

uint8_t isSwitchStatesRequest(uint8_t btnStateC, uint8_t currBtnStateC)
{
    return !btnStateC && currBtnStateC;
}

void handlePowerOffRequest(void)
{
    static uint32_t lastPwrOffReq = 0;
    static int ctr = 0;
    if (HAL_GetTick() - lastPwrOffReq > 2000U) {
        ctr = 0; /* Reset counter if previous press was a long time ago */
    }
    ctr++;
    if (ctr > PWROFF_BTN_PRESS_MAX) {
        poweroff();
    }
    lastPwrOffReq = HAL_GetTick();
}

#ifdef CRUISE_CONTROL_SUPPORT
static void handleCruiseControlActivate(int btn)
{
    if (btn) {
        cruiseControl(1);
    }
}

static uint8_t isCruiseControlActive(void)
{
    return rtP_Left.b_cruiseCtrlEna;
}
#endif /* CRUISE_CONTROL_SUPPORT */

static void handleStateNunChuckSpeedDiffCtrl(void)
{
    // ####### DIRECTION CHANGE FIX #######
    // Reset rate limiter states when joystick is centered (in deadband zone)
    // The input processing already applies deadband from config.h (PRI_INPUT1/2),
    // so cmd will be exactly 0 when joystick is in the center deadband zone.
    // This prevents momentum when changing direction - motors won't briefly
    // move in old direction before reversing.
    
    if (input1[inIdx].cmd == 0) {
        steerRateFixdt = 0;  // Reset steering rate limiter
        steerFixdt = 0;      // Reset steering filter
    }
    if (input2[inIdx].cmd == 0) {
        speedRateFixdt = 0;  // Reset speed rate limiter
        speedFixdt = 0;      // Reset speed filter
    }
    
    // ####### LOW-PASS FILTER #######
    rateLimiter16(input1[inIdx].cmd, rate, &steerRateFixdt);
    rateLimiter16(input2[inIdx].cmd, rate, &speedRateFixdt);
    filtLowPass32(steerRateFixdt >> 4, FILTER, &steerFixdt);
    filtLowPass32(speedRateFixdt >> 4, FILTER, &speedFixdt);
    steer = (int16_t)(steerFixdt >> 16); // convert fixed-point to integer
    speed = (int16_t)(speedFixdt >> 16); // convert fixed-point to integer

    // ####### MIXER #######
    mixerFcn(speed << 4, steer << 4, &cmdR, &cmdL); // This function implements the
                                                    // equations above

    // ####### SET OUTPUTS #######
    // Note: Right motor wired with reverse polarity, hence cmdR is negated
    pwmr = -cmdR;
    pwml = cmdL;
}

static void handleStateBabyRocker(int *pstate, uint8_t *pabortrock)
{
    {
        /* Update Frequency */
        static int16_t prevY = 0, prevX = 0;
        static uint32_t prevCheck = 0;
        int32_t absDiff = abs((int32_t)prevY - (int32_t)(input2[inIdx].cmd));
        if (HAL_GetTick() - prevCheck > 250U) {
            if (absDiff > 100) {
                if (input2[inIdx].cmd > 150) {
                    moveTime += TIME_SET_STEP_ms;
                    if (moveTime > MAX_MOVE_TIME_ms) {
                        moveTime = MAX_MOVE_TIME_ms;
                    } else {
                        beepShort(35);
                        beepShort(37);
                    }
                    prevCheck = HAL_GetTick();
                    lastRegulationTime = HAL_GetTick();
                } else if (input2[inIdx].cmd < -150) {
                    moveTime -= TIME_SET_STEP_ms;
                    if (moveTime < MIN_MOVE_TIME_ms) {
                        moveTime = MIN_MOVE_TIME_ms;
                    } else {
                        beepShort(37);
                        beepShort(35);
                    }
                    prevCheck = HAL_GetTick();
                    lastRegulationTime = HAL_GetTick();
                }
                prevY = input2[inIdx].cmd;
            } else {

                /* Update speed if there was a command */
                absDiff = abs((int32_t)prevX - (int32_t)(input1[inIdx].cmd));
                if (absDiff > 50) {
                    if (input1[inIdx].cmd > 150) {
                        maxSpeed += SPEED_SET_STEPS;
                        if (maxSpeed > MAX_SPEED) {
                            maxSpeed = MAX_SPEED;
                        } else {
                            beepShort(38);
                            beepShort(40);
                        }
                        prevCheck = HAL_GetTick();
                        lastRegulationTime = HAL_GetTick();
                    } else if (input1[inIdx].cmd < -150) {

                        maxSpeed -= SPEED_SET_STEPS;
                        if (maxSpeed < MIN_SPEED) {
                            maxSpeed = MIN_SPEED;
                        } else {
                            beepShort(40);
                            beepShort(38);
                        }
                        prevCheck = HAL_GetTick();
                        lastRegulationTime = HAL_GetTick();
                    }
                    prevX = input1[inIdx].cmd;
                }
            }
        }
        calcDispacement(0);
        int16_t sinSpd = speedSinus(0);
        rateLimiter16(0, 32767, &steerRateFixdt);
        rateLimiter16(sinSpd, 32767, &speedRateFixdt);
        filtLowPass32(steerRateFixdt >> 4, FILTER, &steerFixdt);
        filtLowPass32(speedRateFixdt >> 4, FILTER, &speedFixdt);
    }
    steer = (int16_t)(steerFixdt >> 16);            // convert fixed-point to integer
    speed = (int16_t)(speedFixdt >> 16);            // convert fixed-point to integer
    mixerFcn(speed << 4, steer << 4, &cmdR, &cmdL); // This function implements the
                                                    // equations above
    pwmr = -cmdR;
    pwml = cmdL;

    /* Check timeout and switch off rocker function */
    if (HAL_GetTick() - lastRegulationTime > MAX_ROCKER_TIME_ms || *pabortrock) {

        if (currAvgSpeed < 10U && currAvgSpeed >= 0) {
            *pstate = 0;
            *pabortrock = 0;
            enable = 0;
        }
    }
}

/*******************************************************************************
 * PCC Helper Functions
 ******************************************************************************/

/* Clamp a value between min and max */
static int16_t PCC_Clamp(int16_t value, int16_t minVal, int16_t maxVal)
{
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

/* Ramp a value toward target with maximum step size */
static int16_t PCC_RampToward(int16_t current, int16_t target, int16_t maxStep)
{
    int16_t diff = target - current;
    if (diff > maxStep) return current + maxStep;
    if (diff < -maxStep) return current - maxStep;
    return target;
}

/* Enter a new PCC state */
static void PCC_EnterState(PCC_State_t newState)
{
    uint32_t now = HAL_GetTick();
    
    switch (newState) {
        case PCC_STATE_IDLE:
            pcc.idleEntryTime = now;
            // Disable motors and allow freewheeling
            enable = 0;
            rtP_Left.b_cruiseCtrlEna = 0;
            rtP_Right.b_cruiseCtrlEna = 0;
            rtP_Left.n_cruiseMotTgt = 0;
            rtP_Right.n_cruiseMotTgt = 0;
            pwml = pwmr = 0;
            pcc.targetL = pcc.targetR = 0;
            pcc.assistDirL = 0;  // Clear locked direction
            pcc.assistDirR = 0;
            cruiseCtrlAcv = 0;
            // CRITICAL: Use OPEN_MODE to allow freewheeling
            ctrlModReq = OPEN_MODE;
            // Only beep if transitioning from active state
            if (pcc.state != PCC_STATE_IDLE) {
                beepShort(30);
            }
            break;
            
        case PCC_STATE_TRACKING:
            // TRQ_MODE: PWM controls torque directly - perfect for push assist
            // PWM will be set in the main loop based on direction of movement
            ctrlModReq = TRQ_MODE;
            rtP_Left.b_cruiseCtrlEna = 0;
            rtP_Right.b_cruiseCtrlEna = 0;
            cruiseCtrlAcv = 1;  // Mark assist as active
            // PWM will be set by state handler, enable after initial values
            enable = 1;
            beepShort(50);
            break;
            
        case PCC_STATE_COASTING:
            ctrlModReq = TRQ_MODE;
            // Silent transition, keep motors enabled
            break;
            
        case PCC_STATE_RAMP_DOWN:
            ctrlModReq = TRQ_MODE;
            beepShort(40);  // Disengage beep
            break;
    }
    
    pcc.state = newState;
    pcc.stateEntryTime = now;
}

/*******************************************************************************
 * PCC Initialization
 ******************************************************************************/
static void PCC_Init(void)
{
    pcc.state = PCC_STATE_IDLE;
    pcc.stateEntryTime = HAL_GetTick();
    pcc.idleEntryTime = HAL_GetTick();
    pcc.targetL = 0;
    pcc.targetR = 0;
    pcc.lastSpeedL = 0;
    pcc.lastSpeedR = 0;
    pcc.assistDirL = 0;  // No locked direction
    pcc.assistDirR = 0;
    pcc.initialized = 1;
    
    enable = 0;
    rtP_Left.b_cruiseCtrlEna = 0;
    rtP_Right.b_cruiseCtrlEna = 0;
    pwml = pwmr = 0;
    cruiseCtrlAcv = 0;
    ctrlModReq = OPEN_MODE;  // Allow freewheeling when idle
}

/*******************************************************************************
 * PCC Main Update - Industry Best Practice Implementation
 * 
 * Key behaviors:
 * 1. TRACKING: Motor matches user's current speed (doesn't fight acceleration)
 * 2. COASTING: When user releases, briefly maintains last speed
 * 3. Hysteresis: Different thresholds for engage vs disengage
 * 4. Cooldown: Prevents rapid on/off cycling
 ******************************************************************************/
static void handleStatePushCruiseControl(void)
{
    if (pcc_needs_reset || !pcc.initialized) {
        PCC_Init();
        pcc_needs_reset = 0;
        return;
    }
    
    uint32_t now = HAL_GetTick();
    uint32_t timeInState = now - pcc.stateEntryTime;
    
    // Get current motor speeds (right motor has reversed polarity)
    int16_t actualL = rtY_Left.n_mot;
    int16_t actualR = -rtY_Right.n_mot;
    int16_t absAvgSpeed = (ABS(actualL) + ABS(actualR)) / 2;
    
    // Detect if user is actively accelerating (comparing to last loop)
    // FIX: Check if MAGNITUDE is increasing (works for both forward and reverse)
    // Old bug: only detected positive acceleration, not negative (reverse direction)
    int16_t absLastL = ABS(pcc.lastSpeedL);
    int16_t absLastR = ABS(pcc.lastSpeedR);
    uint8_t isAccelerating = (ABS(actualL) > absLastL + PCC_ACCEL_THRESHOLD) || 
                             (ABS(actualR) > absLastR + PCC_ACCEL_THRESHOLD);
    
    // Calculate ramp rate (RPM per loop iteration)
    int16_t rampStep = (PCC_RAMP_RATE_RPM_PER_SEC * DELAY_IN_MAIN_LOOP) / 1000;
    if (rampStep < 1) rampStep = 1;
    
    // Save for next iteration's acceleration detection
    pcc.lastSpeedL = actualL;
    pcc.lastSpeedR = actualR;
    
    switch (pcc.state) {
        
        /*-----------------------------------------------------------------
         * IDLE: Motors off, waiting for user to push above threshold
         * Uses OPEN_MODE to allow freewheeling (SPD_MODE would resist movement)
         *-----------------------------------------------------------------*/
        case PCC_STATE_IDLE:
            enable = 0;
            pwml = pwmr = 0;
            ctrlModReq = OPEN_MODE;  // Freewheel - don't resist manual movement
            
            // Check for engage (with cooldown to prevent rapid cycling)
            if (absAvgSpeed > PCC_ENGAGE_SPEED_RPM && 
                (now - pcc.idleEntryTime) > PCC_COOLDOWN_MS) {
                // Start tracking immediately at current speed
                pcc.targetL = PCC_Clamp(actualL, -PCC_MAX_SPEED_RPM, PCC_MAX_SPEED_RPM);
                pcc.targetR = PCC_Clamp(actualR, -PCC_MAX_SPEED_RPM, PCC_MAX_SPEED_RPM);
                PCC_EnterState(PCC_STATE_TRACKING);
            }
            break;
            
        /*-----------------------------------------------------------------
         * TRACKING: TORQUE-BASED ASSIST (TRQ_MODE)
         * 
         * In TRQ_MODE, PWM controls motor CURRENT (torque) directly.
         * We apply a constant assist torque in the DIRECTION of movement:
         *   - Wheel spinning forward → positive torque (helps push forward)
         *   - Wheel spinning backward → negative torque (helps push backward)
         * 
         * This approach:
         * ✓ Never fights the user (torque is always in direction of travel)
         * ✓ Doesn't care about exact speed (no back-EMF issues)
         * ✓ Naturally assists acceleration (constant force = acceleration)
         * ✓ Doesn't fight deceleration (when user slows, assist reduces)
         * 
         * WHY NOT VLT_MODE: Voltage mode requires exceeding back-EMF.
         * At 50 RPM, we'd need ~500+ PWM. Setting pwml=50 causes BRAKING!
         *-----------------------------------------------------------------*/
        case PCC_STATE_TRACKING:
            ctrlModReq = TRQ_MODE;  // CRITICAL: Torque mode, not voltage!
            enable = 1;
            
            {
                // DIRECTION LOCKING: Lock direction on first entry, prevent reversal on stop
                // This fixes the bug where stopping the wheel causes it to reverse
                if (pcc.assistDirL == 0) {
                    // First time in TRACKING - lock direction based on current speed
                    if (actualL > PCC_DISENGAGE_SPEED_RPM) {
                        pcc.assistDirL = 1;   // Lock to forward
                    } else if (actualL < -PCC_DISENGAGE_SPEED_RPM) {
                        pcc.assistDirL = -1;  // Lock to backward
                    }
                }
                if (pcc.assistDirR == 0) {
                    if (actualR > PCC_DISENGAGE_SPEED_RPM) {
                        pcc.assistDirR = 1;
                    } else if (actualR < -PCC_DISENGAGE_SPEED_RPM) {
                        pcc.assistDirR = -1;
                    }
                }
                
                // Choose assist level based on acceleration
                int16_t assistTorque = isAccelerating ? PCC_ASSIST_TORQUE_ACCEL : PCC_ASSIST_TORQUE_BASE;
                
                // SAFETY: Reduce assist at high speeds to prevent runaway
                // As we approach max speed, taper off the assist
                if (absAvgSpeed > PCC_MAX_SPEED_RPM) {
                    assistTorque = 0;  // No more assist above max
                } else if (absAvgSpeed > PCC_MAX_SPEED_RPM - 30) {
                    // Taper off in the last 30 RPM before max
                    int16_t margin = PCC_MAX_SPEED_RPM - absAvgSpeed;  // 0-30
                    assistTorque = (assistTorque * margin) / 30;
                }
                
                // Clamp to safety limit
                if (assistTorque > PCC_ASSIST_TORQUE_MAX) {
                    assistTorque = PCC_ASSIST_TORQUE_MAX;
                }
                
                // Apply torque using LOCKED direction (not instantaneous speed)
                // This prevents direction reversal when user stops the wheel
                // Left motor
                if (pcc.assistDirL > 0 && actualL > 0) {
                    pwml = assistTorque;       // Assist forward (locked forward, still moving forward)
                } else if (pcc.assistDirL < 0 && actualL < 0) {
                    pwml = -assistTorque;      // Assist backward (locked backward, still moving backward)
                } else {
                    pwml = 0;                  // Speed crossed zero or direction mismatch - stop assist
                }
                
                // Right motor (PWM needs negation due to wiring)
                if (pcc.assistDirR > 0 && actualR > 0) {
                    pwmr = -assistTorque;      // Assist forward (negated for right motor)
                } else if (pcc.assistDirR < 0 && actualR < 0) {
                    pwmr = assistTorque;       // Assist backward (negated for right motor)
                } else {
                    pwmr = 0;
                }
                
                // Track last speed for state transitions
                pcc.targetL = actualL;
                pcc.targetR = actualR;
            }
            
            // No cruise control - we're using direct torque
            rtP_Left.b_cruiseCtrlEna = 0;
            rtP_Right.b_cruiseCtrlEna = 0;
            
            // Transition to COASTING if user stops actively pushing
            // This prevents continuous assist from becoming runaway
            if (!isAccelerating && timeInState > 500) {
                PCC_EnterState(PCC_STATE_COASTING);
            }
            // Safety: disengage if speed drops below threshold
            else if (absAvgSpeed < PCC_DISENGAGE_SPEED_RPM) {
                PCC_EnterState(PCC_STATE_RAMP_DOWN);
            }
            break;
            
        /*-----------------------------------------------------------------
         * COASTING: User has released but wheel still moving
         * - Minimal assist to maintain momentum (NOT accelerate!)
         * - Returns to TRACKING if user pushes again
         * - Times out to RAMP_DOWN after PCC_HOLD_TIME_MS
         * 
         * The reduced torque ensures we don't keep accelerating when
         * user wants to coast/stop. Just enough to maintain speed.
         *-----------------------------------------------------------------*/
        case PCC_STATE_COASTING:
            ctrlModReq = TRQ_MODE;
            enable = 1;
            
            {
                // MINIMAL assist during coast - just enough to fight friction
                // This should NOT be enough to accelerate the wheel
                int16_t assistTorque = PCC_ASSIST_TORQUE_BASE / 3;  // ~27 out of 1000
                
                // Further reduce at higher speeds (speed limit)
                if (absAvgSpeed > PCC_MAX_SPEED_RPM) {
                    assistTorque = 0;
                } else if (absAvgSpeed > PCC_MAX_SPEED_RPM - 30) {
                    int16_t margin = PCC_MAX_SPEED_RPM - absAvgSpeed;
                    assistTorque = (assistTorque * margin) / 30;
                }
                
                // Apply torque using LOCKED direction (same as TRACKING)
                // Left motor
                if (pcc.assistDirL > 0 && actualL > 0) {
                    pwml = assistTorque;
                } else if (pcc.assistDirL < 0 && actualL < 0) {
                    pwml = -assistTorque;
                } else {
                    pwml = 0;  // Speed crossed zero - stop assist
                }
                
                // Right motor (PWM needs negation)
                if (pcc.assistDirR > 0 && actualR > 0) {
                    pwmr = -assistTorque;
                } else if (pcc.assistDirR < 0 && actualR < 0) {
                    pwmr = assistTorque;
                } else {
                    pwmr = 0;
                }
                
                pcc.targetL = actualL;
                pcc.targetR = actualR;
            }
            
            rtP_Left.b_cruiseCtrlEna = 0;
            rtP_Right.b_cruiseCtrlEna = 0;
            
            // Check if user started pushing again → back to TRACKING
            if (isAccelerating) {
                PCC_EnterState(PCC_STATE_TRACKING);
                break;
            }
            
            // Exit conditions
            if (absAvgSpeed < PCC_DISENGAGE_SPEED_RPM) {
                PCC_EnterState(PCC_STATE_RAMP_DOWN);
            } else if (timeInState >= PCC_HOLD_TIME_MS) {
                // Coast timeout - user hasn't pushed, start disengaging
                PCC_EnterState(PCC_STATE_RAMP_DOWN);
            }
            break;
            
        /*-----------------------------------------------------------------
         * RAMP_DOWN: Gracefully reduce motor assist to zero
         * Uses a time-based ramp down of assist torque
         *-----------------------------------------------------------------*/
        case PCC_STATE_RAMP_DOWN:
            ctrlModReq = TRQ_MODE;
            enable = 1;
            
            {
                // Calculate remaining assist based on time in ramp down
                // Ramp from PCC_ASSIST_TORQUE_BASE to 0 over ~500ms
                int16_t remainingTorque = PCC_ASSIST_TORQUE_BASE - 
                    (int16_t)((timeInState * PCC_ASSIST_TORQUE_BASE) / 500);
                
                if (remainingTorque < 0) remainingTorque = 0;
                
                // Apply remaining torque using LOCKED direction
                // Left motor
                if (pcc.assistDirL > 0 && actualL > 0) {
                    pwml = remainingTorque;
                } else if (pcc.assistDirL < 0 && actualL < 0) {
                    pwml = -remainingTorque;
                } else {
                    pwml = 0;
                }
                
                // Right motor (PWM needs negation)
                if (pcc.assistDirR > 0 && actualR > 0) {
                    pwmr = -remainingTorque;
                } else if (pcc.assistDirR < 0 && actualR < 0) {
                    pwmr = remainingTorque;
                } else {
                    pwmr = 0;
                }
                
                // When torque is zero, transition to IDLE
                if (remainingTorque == 0) {
                    PCC_EnterState(PCC_STATE_IDLE);
                }
            }
            
            rtP_Left.b_cruiseCtrlEna = 0;
            rtP_Right.b_cruiseCtrlEna = 0;
            break;
            
        default:
            PCC_EnterState(PCC_STATE_IDLE);
            break;
    }
}

int handleSwitchStateReq(int state, uint8_t *pabortrock)
{
    state++;
    if (state > 2) {
        state = 0;
    }
    switch (state) {
        case 0:
            // Entering Push Cruise Control mode
            *pabortrock = 1;
            enable = 0;
            
            // Disable any active cruise control from previous state
            rtP_Left.b_cruiseCtrlEna = 0;
            rtP_Right.b_cruiseCtrlEna = 0;
            
            // Reset PWM outputs
            pwml = pwmr = 0;
            cmdL = cmdR = 0;
            
            // Initialize PCC state machine
            pcc_needs_reset = 1;
            
            // Audio feedback moved to PCC_Init via state entry
            beepShort(30);
            beepShort(45);
            break;
        case 1:
            // First disable motors while we reset everything
            enable = 0;
            
            // Disable any active cruise control from previous state
            rtP_Left.b_cruiseCtrlEna = 0;
            rtP_Right.b_cruiseCtrlEna = 0;
            
            // FIX NC-3: Reset filters to prevent initial jump when entering mode
            steerFixdt = speedFixdt = 0;
            steerRateFixdt = speedRateFixdt = 0;
            
            // Reset PWM outputs and commands to zero
            pwml = pwmr = 0;
            cmdL = cmdR = 0;
            
            beepShort(60);
            beepShort(62);
            *pabortrock = 1;
            
            // Now enable motors - they'll start from zero
            enable = 1;
            break;
        case 2:
            // First disable motors while we reset everything
            enable = 0;
            
            // Disable any active cruise control from previous state
            rtP_Left.b_cruiseCtrlEna = 0;
            rtP_Right.b_cruiseCtrlEna = 0;
            
            // Reset filters for baby rocker mode
            steerFixdt = speedFixdt = 0;
            steerRateFixdt = speedRateFixdt = 0;
            
            // Reset PWM outputs and commands to zero
            pwml = pwmr = 0;
            cmdL = cmdR = 0;
            
            *pabortrock = 0;
            rockerEnable = 1;
            beepShort(62);
            beepShort(60);
            lastRegulationTime = HAL_GetTick();
            speedSinus(1);      /* Reset the speed */
            calcDispacement(1); /* Reset the displacement calculation */
            
            // Now enable motors - they'll start from zero
            enable = 1;
            break;
    }
    return state;
}
int main(void)
{
    int32_t board_temp_adcFixdt = adc_buffer.temp << 16; // Fixed-point filter output initialized
                                                         // with current ADC converted to
                                                         // fixed-point
    int16_t board_temp_adcFilt = adc_buffer.temp;
    int state = 0;
    uint8_t abortrock = 0;
    uint8_t btnStateZ = 0, btnStateC = 0;
    uint8_t currBtnStateZ = 0, currBtnStateC = 0;
   // uint32_t prevBtnStateZChg = 0, prevBtnStateCChg = 0;

    init_hw();
    /* Main loop */
    for (;;) {
        if (IS_TIMEOUT_1MS()) { // 1 ms = 16 ticks
                                // buzzerTimer
            /* Nunchuk buttons, and analog stick */
            readCommand(); // Read Command: input1[inIdx].cmd, input2[inIdx].cmd
            getNunchuckBtnPressState(&currBtnStateZ, &currBtnStateC);
            calcAvgSpeed();       // Calculate average measured speed: speedAvg, speedAvgAbs
            updateSpeedGlobals(); // Update global speed variables in this functions

            // ####### MOTOR ENABLING: Only if the initial input is very small (for SAFETY) #######
            /* !!!!!!!!!!!!!! STATE 0 !!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
            handleMotorControlModeAndEnable();

            /* Determine the nunchuck button commd*/
            if (isNunchuckConnectd() && rockerEnable) {
                if (isPowerOffRequest(btnStateZ, currBtnStateZ)) { /* Turn off the device */
                    handlePowerOffRequest();
                } else if (isSwitchStatesRequest(btnStateC, currBtnStateC)) {
                    state = handleSwitchStateReq(state, &abortrock);
                }
            }

            /* Handle the current state */
            switch (state) {
                case 0:
#ifdef CRUISE_CONTROL_SUPPORT
                    handleStatePushCruiseControl();
#endif /* CRUISE_CONTROL_SUPPORT */
                    break;
                case 1:
                    // Nunchuck differenctioal motor control
                    handleStateNunChuckSpeedDiffCtrl();
#ifdef CRUISE_CONTROL_SUPPORT
                    /* Handle Cruise control */
                    if (currBtnStateZ && main_loop_counter > 5) {
                        handleCruiseControlActivate(currBtnStateZ != btnStateZ);
                    }
#endif /* CRUISE_CONTROL_SUPPORT */
                    /* Disable motors on no input (free wheeling) */
                    if(!isCruiseControlActive())
                    {
                        // FIX NC-1: Use inIdx instead of hardcoded 0 for proper dual-input support
                        if(input1[inIdx].cmd == 0 && input2[inIdx].cmd == 0)
                        {
                            if (enable) {
                                enable = 0;
                            }
                        } else {
                            if (!enable) {
                                enable = 1;
                            }
                        }
                    } else {
                        enable = 1;
                    }
                    break;
                case 2:
                    // Baby rocker control
                    handleStateBabyRocker(&state, &abortrock);
                    break;
                default:
                    break;
            }
            /* Save previos button states (to handle fronts and for deboiuncing) */
            btnStateC = currBtnStateC;
            btnStateZ = currBtnStateZ;

            // ####### CALC BOARD TEMPERATURE #######
            setBoardTemperature(&board_temp_adcFilt, &board_temp_adcFixdt);

            // ####### CALC CALIBRATED BATTERY VOLTAGE #######
            batVoltageCalib = batVoltage * BAT_CALIB_REAL_VOLTAGE / BAT_CALIB_ADC;

            // ####### CALC DC LINK CURRENT #######
            setDcLinkCurrent();

            // ####### DEBUG SERIAL OUT #######
            sendDebugInformation(board_temp_adcFilt);

            // ####### POWEROFF BY POWER-BUTTON #######
             poweroffPressCheck();

            // ####### BEEP AND EMERGENCY POWEROFF #######
            handleTemperaturePowerOff();
            handleSystemWarningsAndErrors();
            handleInactivityPowerOff();
            handleCruiseControlInactivity();
            // HAL_GPIO_ToggleP(LED_PORT, LED_PIN);                 // This is to measure the
            // main() loop duration with an oscilloscope connected to LED_PIN Update states
            inIdx_prev = inIdx;
            buzzerTimer_prev = buzzerTimer;
            main_loop_counter++;
        }
    }
}

// ===========================================================
/** System Clock Configuration
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = 16;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /**Initializes the CPU, AHB and APB busses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    // PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV8;  // 8 MHz
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4; // 16 MHz
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    /**Configure the Systick interrupt time
     */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

    /**Configure the Systick
     */
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
