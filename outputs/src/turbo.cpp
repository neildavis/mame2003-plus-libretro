#include <string.h>
#include <sys/timeb.h>

#include "output-def.h"
#include "turbo.h"
#include "utils.h"

using namespace tm1637;
using namespace gpiod;

const char *kGpiodChipName = "gpiochip0";
const char *kGpiodConsumer = "lrmame2003osvr";

TurboOutputHandler::~TurboOutputHandler() {
}

void TurboOutputHandler::init() {

    m_pTM1637.reset(new Device(15, 14));
    m_chip = chip(kGpiodChipName);
    m_line_r3 = m_chip.get_line(25);
    m_line_r3.request({kGpiodConsumer, line_request::DIRECTION_OUTPUT, 0});
    m_line_r2 = m_chip.get_line(8);
    m_line_r2.request({kGpiodConsumer, line_request::DIRECTION_OUTPUT, 0});
    m_line_r1 = m_chip.get_line(7);
    m_line_r1.request({kGpiodConsumer, line_request::DIRECTION_OUTPUT, 0});
    m_line_g = m_chip.get_line(18);
    m_line_g.request({kGpiodConsumer, line_request::DIRECTION_OUTPUT, 0});
    m_line_y1 = m_chip.get_line(23);
    m_line_y1.request({kGpiodConsumer, line_request::DIRECTION_OUTPUT, 0});
    m_line_y2 = m_chip.get_line(24);
    m_line_y2.request({kGpiodConsumer, line_request::DIRECTION_OUTPUT, 0});
    reset_state();
 }

void TurboOutputHandler::deinit() {
    m_pTM1637->clear();
    m_pTM1637.reset();
}

void TurboOutputHandler::handle_output(const char *name, int value) {
    if (0 == strcmp(name, OUTPUT_TURBO_TIME_NAME)) {
        update_time(value);
        return;
    }
    if (0 == strcmp(name, OUTPUT_TURBO_CARS_PASSED_NAME)) {
        update_cars_passed(value);
        return;
    }
    if (0 == strcmp(name, OUTPUT_TURBO_RACE_START_LIGHTS_NAME)) {
        update_start_lights(value);
        return;
    }
    if (0 == strcmp(name, OUTPUT_TURBO_RACE_YELLOW_FLAG_NAME)) {
        update_yellow_flag(value);
        return;
    }
    if (0 == strcmp(name, OUTPUT_TURBO_ATTRACT_MODE_NAME)) {
        update_attract_mode(value);
        return;
    }
    if (0 == strcmp(name, OUTPUT_TURBO_START_SCREEN_NAME)) {
        update_start_mode(value);
        return;
    }
    int n = parseLedOutputName(name);
    if (n == TURBO_LED_START) {
        update_start_button(value);
        return;
    }
}

void TurboOutputHandler::update_time(int value) {
    if (m_attract_mode_active) {
        return; // Ignore time during attract mode
    }
    if (m_start_mode_active) {
        // During start screen, use time as start button state since TURBO_LED_START is inconsistent timing
        m_line_g.set_value(value %2);
        return;
    }
    int valTens = value / 10;
    if (m_tm1637_digits[0] != valTens) {
        m_tm1637_digits[0] = valTens;
        m_pTM1637->showInteger(0, valTens);
    }
    m_tm1637_digits[1] = value % 10;
    m_pTM1637->showInteger(1, m_tm1637_digits[1]);
}

void TurboOutputHandler::update_cars_passed(int value) {
    if (m_attract_mode_active) {
        return; // Ignore cars passed during attract mode
    }
    int valTens = value / 10;
    if (m_tm1637_digits[2] != valTens) {
        m_tm1637_digits[2] = valTens;
        m_pTM1637->showInteger(2, valTens);
    }
    m_tm1637_digits[3] = value % 10;
    m_pTM1637->showInteger(3, m_tm1637_digits[3]);
}

void TurboOutputHandler::update_start_lights(int value) {
    m_line_r3.set_value(value >= m_start_lights_last && value > -1);
    m_line_r2.set_value(value >= m_start_lights_last && value > 0);
    m_line_r1.set_value(value >= m_start_lights_last && value > 1);
    m_line_g.set_value(value >= m_start_lights_last && value > 2);
    m_start_lights_last = value;

}

void TurboOutputHandler::update_yellow_flag(int value) {
    if (0 == value) {
        m_line_y1.set_value(0);
        m_line_y2.set_value(0);
    } else {
        // struct timeb time_now;
        // ftime(&time_now);
        int odd = value % 2;
        m_line_y1.set_value(odd);
        m_line_y2.set_value(!odd);
    }
}

void TurboOutputHandler::update_start_button(int /*value*/) {
    m_start_lights_last = -1;
}

void TurboOutputHandler::update_attract_mode(int value) {
    m_attract_mode_active = (value > 0);
    reset_state();
}

void TurboOutputHandler::update_start_mode(int value) {
    m_start_mode_active = (value > 0);
    if (m_start_mode_active) {
        reset_state();
        m_line_g.set_value(1);
    } else {
        m_pTM1637->setColon(true);
    }
}

void TurboOutputHandler::reset_state() {
    m_start_lights_last = -1;
    for (int i=0; i<4; i++) {
        m_tm1637_digits[i] = -1;    
    }
    m_pTM1637->clear();
    m_pTM1637->setColon(false);
    m_line_r3.set_value(0);
    m_line_r2.set_value(0);
    m_line_r1.set_value(0);
    m_line_g.set_value(0);
    m_line_y1.set_value(0);
    m_line_y2.set_value(0);
}