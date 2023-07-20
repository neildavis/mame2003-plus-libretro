#include <string.h>
#include <stdio.h>
#include <math.h>

#include <wiringPi.h>
#include <sr595.h>
#include <tm1637_digits.h>

#include "chasehq.h"
#include "utils.h"
#include "output-def.h"

using namespace udd;

static const int NUM_LEDS_REVS  = 8;
static const int NUM_LEDS_TURBO = 5;


// Pins for LEDs Shift Register
static const int PIN_SR_BASE    = 100;  // arbitrary as long as outside RPi GPIO pin range
static const int PIN_SR_NUM     = NUM_LEDS_REVS + NUM_LEDS_TURBO;
static const int PIN_SR_DATA    = 2;
static const int PIN_SR_LATCH   = 3;
static const int PIN_SR_CLK     = 4;
// Pin to drive NPN for siren light
static const int PIN_SIREN      = 17;
// SPI LCD Pins
static const int PIN_LCD_DC     = 5;
static const int PIN_LCD_CS     = 6;
static const int PIN_LCD_RST    = 13;
static const int PIN_LCD_BLK    = 12;
// SPI LCD Confing
static const int LCD_SPI_MODE = 0;
static const int LCD_SPI_SPEED = 35000000;
// TM1637 config
static const int PIN_TM1637_CLK = 22;
static const int PIN_TM1637_DIO = 27;

ChaseHqOutputHandler::~ChaseHqOutputHandler() {
}

void ChaseHqOutputHandler::init() {
    m_turboCount = 0;
    wiringPiSetupGpio();
    pinMode(PIN_SIREN, OUTPUT);
    sr595Setup(PIN_SR_BASE, PIN_SR_NUM, PIN_SR_DATA, PIN_SR_CLK, PIN_SR_LATCH);
    // Set all LEDs to off
    for (int i = PIN_SR_BASE; i < PIN_SR_BASE + PIN_SR_NUM; i++) {
        digitalWrite(i, LOW);
    }
    // Drive siren off
    digitalWrite(PIN_SIREN, LOW);    // Setup ST7735
    // Setup ST7735 Display
    // DisplayConfiguration displayConfig;
    // displayConfig.displayType = ST7735;
    // displayConfig.width = 128;
    // displayConfig.height = 160;
    // displayConfig.xOffset = 2;
    // displayConfig.yOffset = 1;    
    // displayConfig.spiSpeed = LCD_SPI_SPEED;
    // displayConfig.spiMode = LCD_SPI_MODE;
    // displayConfig.CS = PIN_LCD_CS;
    // displayConfig.DC = PIN_LCD_DC;
    // displayConfig.RST = PIN_LCD_RST;
    // displayConfig.BLK = PIN_LCD_BLK;
    // m_display.openDisplay(displayConfig);
    // m_display.clearScreen(YELLOW);
    // Setup TM1637
    m_pTM1637 = std::make_shared<Device>(PIN_TM1637_CLK, PIN_TM1637_DIO, GpioGPIOD);
    m_pTM1637->setColon(false);
    m_speedKPH = 928;
    m_pTM1637->showIntegerLiteral(m_speedKPH, RadixDecimal);
}

void ChaseHqOutputHandler::deinit() {
    // Set all LEDs to off
    for (int i = PIN_SR_BASE; i < PIN_SR_BASE + PIN_SR_NUM; i++) {
        digitalWrite(i, LOW);
    }
    // Clear TM1637
    m_pTM1637->clear();
    // Drive siren off
    digitalWrite(PIN_SIREN, LOW);    // Setup ST7735
}

void ChaseHqOutputHandler::handle_output(const char *name, int value) {
    if (0 == strcmp(name, CHQ_REVS_NAME)) {
        update_revs(value);
        return;
    }
    if (0 == strcmp(name, CHQ_SPEED_KPH_NAME)) {
        update_speed(value);
        return;
    }
    if (0 == strcmp(name, CHQ_TURBO_COUNT_NAME)) {
        update_turbo_count(value);
        return;
    }
    if (0 == strcmp(name, CHQ_TURBO_DURATION_NAME)) {
        update_turbo_duration(value);
        return;
    }
    if (0 == strcmp(name, CHQ_CHASE_LAMP_STATE_NAME)) {
        update_siren(value);
        return;
    }
}

void ChaseHqOutputHandler::update_turbo_count(int value) {
    m_turboCount = value;
    for (int led = 0; led < NUM_LEDS_TURBO; led++) {
        digitalWrite(PIN_SR_BASE + NUM_LEDS_REVS + led, value > led ? HIGH : LOW);
    }
}

void ChaseHqOutputHandler::update_turbo_duration(int value) {
    int ledVal = ((value % 30) > 14) ? HIGH : LOW;
    int ledIdx = PIN_SR_BASE + NUM_LEDS_REVS + m_turboCount;
    digitalWrite(ledIdx, ledVal);
}

void ChaseHqOutputHandler::update_revs(int value) {
    // This could be done more efficiently!
    int rK = floor(value / 1000.0);
    for (int led = 0; led < NUM_LEDS_REVS; led++) {
        digitalWrite(PIN_SR_BASE + led, rK >= led + 1 ? HIGH : LOW);
    }
}

void ChaseHqOutputHandler::update_speed(int value) {
    // Since we dont want to update TM1637 too often, slice off lower 4 bits
    value = value & ~0xf;
    if (value != m_speedKPH) {
        m_speedKPH = value;
        m_pTM1637->showIntegerLiteral(m_speedKPH, RadixDecimal);
    }
}

void ChaseHqOutputHandler::update_siren(int value) {
    int pinVal = (value < 3) ? LOW : HIGH;
    digitalWrite(PIN_SIREN, pinVal);
}
