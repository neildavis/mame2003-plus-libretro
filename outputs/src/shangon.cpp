#include "shangon.h"
#include "output-def.h"
#include "utils.h"
#include "wiringpi_xtra.h"

#include <string.h>
#include <stdio.h>
#include <chrono>
#include <thread>

#include <pigpiod_if2.h>
#include <wiringPi.h>
#include <sr595.h>
#include <tm1637.h>
#include <tm1637_sayer.h>

const int SHO_SPEED_KPH_MAX    = 324;          // with turbo
const int SHO_REVS_RPM_MAX     = 18000;        // at max speed
const int SHO_REVS_RPM_IDLE    = 2000;         // idle revs at 0 kph during race
const int SHO_REVS_RPM_START   = SHO_REVS_RPM_IDLE;        // artificial revs at race start (SHO_REVS_RPM_IDLE == disabled)
// Revs equivalent Speeds. COMPUTED. DO NOT CHANGE!
const int SHO_RPM_IDLE_EQUIV_SPEED = SHO_REVS_RPM_IDLE * SHO_SPEED_KPH_MAX / SHO_REVS_RPM_MAX;
const int SHO_RPM_START_EQUIV_SPEED = SHO_REVS_RPM_START * SHO_SPEED_KPH_MAX / SHO_REVS_RPM_MAX;
// Servo min/max movement limits
const int SERVO_SPEED_PULSEWIDTH_MIN   = 500;  // 0 kph
const int SERVO_SPEED_PULSEWIDTH_MAX   = 2500; // 324 kph
const int SERVO_RPM_PULSEWIDTH_MIN     = 500;  // 0 revs
const int SERVO_RPM_PULSEWIDTH_MAX     = 2500; // max revs at 324 kph

// Pins for servos
const int PIN_SPEED_SERVO      = 26;   // BCM
const int PIN_RPM_SERVO        = 16;   // BCM
// Pins for Shift Register (74x595)
static const int PIN_SR_BASE        = 100;  // arbitrary as long as above valid BCM GPIO pin range
static const int PIN_SR_NUM         = 8;    // Single 74x595, 8 outputs: Qa-Qh
static const int PIN_SR_DATA        = 23;   // BCM
static const int PIN_SR_OE          = 24;   // BCM
static const int PIN_SR_CLK         = 22;   // BCM
static const int PIN_SR_LATCH       = 27;   // BCM
// Pins (virtual via 74x595 SR) for LEDs
static const int PIN_BRAKE_LIGHT    = PIN_SR_BASE + 0;  // 74x595 Qa
static const int PIN_START_LIGHT_1  = PIN_SR_BASE + 1;  // 74x595 Qb
static const int PIN_START_LIGHT_2  = PIN_SR_BASE + 2;  // 74x595 Qc
static const int PIN_START_LIGHT_3  = PIN_SR_BASE + 3;  // 74x595 Qd
static const int PIN_TURBO_R        = PIN_SR_BASE + 4;  // 74x595 Qe
static const int PIN_TURBO_G        = PIN_SR_BASE + 5;  // 74x595 Qf
static const int PIN_TURBO_B        = PIN_SR_BASE + 6;  // 74x595 Qg
static const int PIN_START_BTN      = PIN_SR_BASE + 7;  // 74x595 Qh - MUST BE LAST
// Pins for TM1637 Display
static const int PIN_TM1637_CLK     = 3;    // BCM
static const int PIN_TM1637_DIO     = 2;    // BCM

SuperHangOnOutputHandler::SuperHangOnOutputHandler() : 
    m_game_in_progress(false), m_credits(0), m_stage(0), m_heartbeat(0), 
    m_turbo_available(false), m_turbo_active(false), m_start_lights(0),
    m_time(0), m_pi_handle(-1) {
}

SuperHangOnOutputHandler::~SuperHangOnOutputHandler() {
}

void SuperHangOnOutputHandler::init() {
    // Init pigpiod interface
    m_pi_handle = pigpio_start(NULL, NULL);
    if (m_pi_handle < 0) {
        printf("shangon: Failed to connect to pigpiod daemon\n");
    }
    // Initialize to zero speed
    handle_speed_output(0);
    // Init TM1637 Display (alos inits wirinpPi)
    m_pTM1637 = std::make_shared<tm1637::Device>(PIN_TM1637_CLK, PIN_TM1637_DIO, tm1637::GpioWiringPiBCM);
    m_pTM1637->clear();
    m_pSayer.reset(new tm1637::Sayer(m_pTM1637));
    m_pSayer->begin("SEGA   SUPER HANG-ON   PRESS START");
    // Init wiringPi and Shift Register for LEDs
    pinMode(PIN_SR_OE, OUTPUT);
    digitalWrite(PIN_SR_OE, HIGH);  // Pull 74x595 OE HIGH to disable outputs
    sr595Setup(PIN_SR_BASE, PIN_SR_NUM, PIN_SR_DATA, PIN_SR_CLK, PIN_SR_LATCH);
    // - Set all LEDs to off
    for (int i = PIN_SR_BASE; i < PIN_SR_BASE + PIN_SR_NUM; i++) {
        digitalWrite(i, LOW);   
    }
    digitalWrite(PIN_SR_OE, LOW);   // Pull 74x595 OE LOW to enable outputs
}

void SuperHangOnOutputHandler::deinit() {
    if (m_pi_handle >= 0) {
        set_servo_pulsewidth(m_pi_handle, PIN_SPEED_SERVO, 0); // stop servo
        set_servo_pulsewidth(m_pi_handle, PIN_RPM_SERVO, 0); // stop servo
        pigpio_stop(m_pi_handle);
        m_pi_handle = -1;
    }
    // Clear TM1637
    if (m_pTM1637) {
        m_pTM1637->setColon(false);
        m_pTM1637->clear();
    }
    // Set all LEDs to off
    for (int i = PIN_SR_BASE; i < PIN_SR_BASE + PIN_SR_NUM; i++) {
        digitalWrite(i, LOW);   
    }
    digitalWrite(PIN_SR_OE, HIGH);  // Pull 74x595 OE HIGH to disable outputs
    // Deinit sr595
    struct wiringPiNodeStruct *sr595Node = wiringPiRemoveNode(PIN_SR_BASE);
    if (NULL != sr595Node) {
        free(sr595Node); // release memory
    }
}

void SuperHangOnOutputHandler::handle_output(const char *name, int value) {
    if (0 == strcmp(SHO_SPEED_NAME, name)) {
        handle_speed_output(value);
        return;
    } else if (m_game_in_progress && (0 == strcmp(name, SHO_TIME_NAME))) {
        // printf("shangon: Time/Frames changed: %04x\n", value);
        handle_time_secs_output((value >> 8) & 0xff);   // time (secs) is in MSB
        handle_time_frames_output(value & 0xff);        // frame count (15,30,45,60) is in LSB
        return;
    } else if (0 == strcmp(SHO_TURBO_AVAILABLE_NAME, name)) {
        //printf("shangon: Turbo %s\n", value > 0 ? "Available" : "Unavailable");
        m_turbo_available = value > 0 ? true : false;
        handle_turbo_available_state();
        return;
    } else if (0 == strcmp(SHO_TURBO_ACTIVE_NAME, name)) {
        //printf("shangon: Turbo %s\n", value > 0 ? "Active" : "Deactivated");
        m_turbo_active = value > 0 ? true : false;
        handle_turbo_active_state();
        return;
    } else if (0 == strcmp(SHO_BRAKE_LIGHT_NAME, name)) {
        //printf("shangon: Brake Light: %s\n", value > 0 ? "ON" : "OFF");
        digitalWrite(PIN_BRAKE_LIGHT, value > 0 ? HIGH : LOW);
        return;
    } else if (0 == strcmp(SHO_START_LIGHTS_NAME, name)) {
        //printf("shangon: Start Lights changed: %d\n", value);
        m_start_lights = value;
        handle_start_lights_state();
        return;
    } else if (0 == strcmp(name, SHO_CREDITS_NAME)) {
        //printf("shangon: Credits changed: %d -> %d\n", m_credits, value);
        if (value == m_credits -1) {
            /* Credit(s) used */
            //printf("shangon: Credit used: Game Started!\n");
            handle_game_start();
        }
        m_credits = value;
        return;
    } else if (0 == strcmp(name, SHO_STAGE_BCD_NAME)) {
        m_stage = value; // wait for timer to update to update TM1637
        //printf("shangon: Stage: %02x\n", value);
        return;
    } else if (0 == strcmp(name, SHO_START_BTN_NAME)) {
        //printf("shangon: Start Button LED: %s\n", value > 0 ? "ON" : "OFF");
        handle_start_button_output(value);
        return;
    }
}

int SuperHangOnOutputHandler::shoSpeedToPulseWidth(int speed_kph) {
    int pulsewidth = SERVO_SPEED_PULSEWIDTH_MIN + 
        (SHO_SPEED_KPH_MAX - speed_kph) * (SERVO_SPEED_PULSEWIDTH_MAX - SERVO_SPEED_PULSEWIDTH_MIN) / SHO_SPEED_KPH_MAX;
    return pulsewidth;
}

int SuperHangOnOutputHandler::shoSpeedToRpmPulseWidth(int speed_kph) {
    if (m_game_in_progress) {
        if (0x01 == m_start_lights) {
            // Artificial revs for race start
            speed_kph = SHO_RPM_START_EQUIV_SPEED;
        } else if (speed_kph < SHO_RPM_IDLE_EQUIV_SPEED) { 
            // Idle revs during game 
            speed_kph =SHO_RPM_IDLE_EQUIV_SPEED;
        }
    }
    int pulsewidth = SERVO_RPM_PULSEWIDTH_MIN + 
        (SHO_SPEED_KPH_MAX - speed_kph) * (SERVO_RPM_PULSEWIDTH_MAX - SERVO_RPM_PULSEWIDTH_MIN) / SHO_SPEED_KPH_MAX;
    return pulsewidth;
}

void SuperHangOnOutputHandler::handle_speed_output(int value) {
    if (m_pi_handle < 0) {
        return;
    }
    int speed_kph = bcd16_to_decimal(value); /* convert from game speed units in BDC to decimal*/
    int pulse_width_speed = shoSpeedToPulseWidth(speed_kph);
    int pulse_width_rpm = shoSpeedToRpmPulseWidth(speed_kph);
    set_servo_pulsewidth(m_pi_handle, PIN_SPEED_SERVO, pulse_width_speed);
    set_servo_pulsewidth(m_pi_handle, PIN_RPM_SERVO, pulse_width_rpm);
    //printf("shangon: Set servo pulse widths speed: %d, rpm: %d for speed %d kph\n", pulse_width_speed, pulse_width_rpm, speed_kph);
}

void SuperHangOnOutputHandler::handle_start_button_output(int value) {
    //printf("shangon: Start Button frames: %02x\n", value);
    digitalWrite(PIN_START_BTN, (value > 0x00 && value < 0x1f) ? HIGH : LOW);    // flash LED
    // Scroll text on tm1637
    if (m_pSayer->finished()) {
        m_pSayer->reset();
    }
    m_pSayer->next();
}

void SuperHangOnOutputHandler::handle_time_secs_output(int value) {
    if (value != m_time) {
        //printf("shangon: Time changed: %02x\n", time);
        m_time = value;
        m_pTM1637->showIntegerLiteral(value | (m_stage << 8), tm1637::RadixHex);
        if (0 == value) {
            /* Time reached zero during game - Game Over! */
            handle_game_over();
            return;
        }
    }
}

void SuperHangOnOutputHandler::handle_time_frames_output(int value) {
    if (m_turbo_available && !m_turbo_active) {
        // Blink turbo available light on frame count
        digitalWrite(PIN_TURBO_R, (value & 0x1));
        digitalWrite(PIN_TURBO_G, LOW);
        digitalWrite(PIN_TURBO_B, LOW);
    }
}

void SuperHangOnOutputHandler::handle_turbo_available_state() {
    if (!m_turbo_available) {
        // Disable turbo available light.
        // ( Enabled state is blinked via handle_time_frames_output() )
        digitalWrite(PIN_TURBO_R, LOW);
        digitalWrite(PIN_TURBO_G, LOW);
        digitalWrite(PIN_TURBO_B, LOW);
    }
}

void SuperHangOnOutputHandler::handle_turbo_active_state() {
    if (m_turbo_active) {
        digitalWrite(PIN_TURBO_R, LOW);
        digitalWrite(PIN_TURBO_G, LOW);
        digitalWrite(PIN_TURBO_B, HIGH);
    } else {
        handle_turbo_available_state();
    }
}

void SuperHangOnOutputHandler::handle_start_lights_state() {
    digitalWrite(PIN_START_LIGHT_1, (m_start_lights & 0b11) ? HIGH : LOW);  // light 1 on for 1 or 2 or 3
    digitalWrite(PIN_START_LIGHT_2, (m_start_lights & 0b10) ? HIGH : LOW);  // light 2 on for 2 or 3
    digitalWrite(PIN_START_LIGHT_3, (m_start_lights == 0b11) ? HIGH : LOW); // light 3 on for 3 only
    // Force some artificial high revs at the start
    if (0x01 == m_start_lights) {
        handle_speed_output(0);
    }
}

void SuperHangOnOutputHandler::handle_game_start() {
    m_game_in_progress = true;
    m_stage = 1;
    /* 
        Do other game start stuff here. 
        e.g. disable start button lamp manually since flag we are using is not updated by the game 
    */
    digitalWrite(PIN_START_BTN, LOW);
    m_pTM1637->setColon(true);
    // Since speed was rest to zero before and doesn't change again until we accelerate
    // we need to force a speed update to get idle revs
    handle_speed_output(0);
}

void SuperHangOnOutputHandler::handle_game_over() {
    /* 
        Do game over stuff here. 
        e.g. set speed to zero since the value we are using is not updated by the game
    */
    //printf("shangon: Time Up! Game Over!\n");
	m_game_in_progress = false;
    handle_speed_output(0);
    // Set all LEDs to off - excluding 'Start Button'
    for (int i = PIN_SR_BASE; i < PIN_START_BTN; i++) {
        digitalWrite(i, LOW);   
    }
    digitalWrite(PIN_START_BTN, HIGH);
    m_pTM1637->setColon(false);
    m_pTM1637->clear();
}
