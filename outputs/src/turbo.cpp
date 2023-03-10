#include <string.h>
#include <sys/timeb.h>
#include <chrono>
#include <thread>
#include <wiringPi.h>
#include <tm1637_digits.h>

#include "output-def.h"
#include "turbo.h"
#include "utils.h"

using namespace tm1637;
using namespace udd;

const int kPinStartBtn = 17;
const int kSpiSpeed = 90000000;

// Consts for srawing on ST7789
const Color &bgColor = BLACK;
const Color AMBER(255, 192, 0);
const int minX = 21, maxX= 226, minY = 0, maxY = 205;
const Point imgP1(minX, minY);
const Point imgP2(maxX, maxY);
const int imgW = maxX - minX + 1, imgH = maxY - minY + 1;
const int imgCtrX = (imgW / 2), imgCtrY = (imgH / 2);
const int lampRadius = 50;
// const Point lampP1(minX + (imgW - lampImage.getWidth()) / 2 + 1, minY + (imgH - lampImage.getHeight()) / 2 + 1);
// const Point lampP2(lampP1.x + lampImage.getWidth() - 1, lampP1.y + lampImage.getHeight() - 1);
const int timeArcRadius = imgW / 2;
const int timeArcThickness = 20;
const float timeArcStartDeg = 150.0;
const float timeArcWarnDeg = -80;
const float timeArcDangerDeg = -130;
const float timeArcEndDeg = -180.0;
const float timeArcDegreeRange = timeArcStartDeg - timeArcEndDeg;
const float timeArcDegreeInc = 10.0;
const bool timeArcGradient = false;
//
const int kCarsPassedTarget = 30;
const int kCarsPassedMax = 41;
const int carsPassedPieRadius = timeArcRadius - timeArcThickness * 3;
const float carsPassedStartDeg = 180.0;
const float carsPassedEndDeg = -180.0;
const float carsPassedDegreeRange = carsPassedStartDeg - carsPassedEndDeg;
const float carsPassedDegreeInc = carsPassedDegreeRange / kCarsPassedMax;

TurboOutputHandler::TurboOutputHandler() :
    m_image(imgW, imgH, bgColor) {

}
 
TurboOutputHandler::~TurboOutputHandler() {
}

void TurboOutputHandler::init() {

    // Setup start button
    wiringPiSetupGpio();
    pinMode(kPinStartBtn, OUTPUT);
    // Setup TM1637
    m_pTM1637 = std::make_shared<Device>(3, 2, GpioGPIOD);
    m_pSayer.reset(new Sayer(m_pTM1637));
    // Setup ST7789
    DisplayConfiguration displayConfig;
    displayConfig.width = 240;
    displayConfig.height = 240;
    displayConfig.spiSpeed = kSpiSpeed;
    displayConfig.spiMode = 3;
    displayConfig.CS = 4;
    displayConfig.DC = 6;
    displayConfig.RST = 5;
    displayConfig.BLK = 13;
    m_display.openDisplay(displayConfig);
    // Setup initial state
    reset_state();
 }

void TurboOutputHandler::deinit() {
    m_pTM1637->clear();
    m_pTM1637.reset();
    m_pSayer.reset();
    m_display.clearScreen(bgColor);
    m_image.clear(bgColor);
    m_display.reset();
    m_display.closeSPI();
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
    if (m_attract_mode_active || m_start_mode_active) {
        return; // Ignore time during attract and start screen modes
    }

    // Time on ST7789
    if (value > m_time_last) {
        m_time_max = value;
        // Draw initial full arc
        Color arcSegmentColor = GREEN;
        for (float degree = timeArcStartDeg; degree > timeArcEndDeg; degree -= timeArcDegreeInc) {
            if (timeArcGradient) {
                int color_green = (degree + 176) * 255 / 320;
                int color_red = 255 - color_green;
                arcSegmentColor = Color(color_red, color_green, 0);
            } else { 
                if (degree < timeArcDangerDeg) {
                    arcSegmentColor = RED;
                } else if (degree < timeArcWarnDeg) {
                    arcSegmentColor = AMBER;
                }
            }
            m_image.drawArc(imgCtrX, imgCtrY, timeArcRadius, timeArcRadius -timeArcThickness, degree - timeArcDegreeInc, degree, arcSegmentColor, 3);
        }
    } else if (value != m_time_last) {
        // Time decrement, blank out corresponding arc portion
        float degreeTimeLast = timeArcEndDeg + (timeArcDegreeRange * m_time_last / m_time_max) + 1;
        float degreeTimeThis = timeArcEndDeg + (timeArcDegreeRange * value / m_time_max);
        m_image.drawArc(imgCtrX, imgCtrY, timeArcRadius + 1, timeArcRadius - timeArcThickness - 1, degreeTimeThis, timeArcStartDeg + 1, bgColor, 3);
    }
    m_display.showImage(m_image, imgP1, imgP2, DEGREE_0);
    m_time_last = value;
}

void TurboOutputHandler::update_cars_passed(int value) {
    if (m_attract_mode_active || m_start_lights_last < 0) {
        return; // Ignore cars passed during attract mode
    }
    // int valTens = value / 10;
    // if (m_tm1637_digits[2] != valTens) {
    //     m_tm1637_digits[2] = valTens;
    //     m_pTM1637->showInteger(2, valTens);
    // }
    // m_tm1637_digits[3] = value % 10;
    // m_pTM1637->showInteger(3, m_tm1637_digits[3]);
    
    // Draw CP on ST7789
    Color cpColor(RED);
    if (value >= kCarsPassedTarget) {
       cpColor = GREEN;
    }
    float cpDegree = carsPassedStartDeg - value * carsPassedDegreeInc;
    m_image.drawPieSlice(imgCtrX, imgCtrY, carsPassedPieRadius, carsPassedStartDeg, cpDegree, cpColor, SOLID, 2); 
    // fill any remiaing space with BG color
    if (value < kCarsPassedMax) {
        m_image.drawPieSlice(imgCtrX, imgCtrY, carsPassedPieRadius, cpDegree, carsPassedEndDeg, bgColor, SOLID, 2); 
    }
    m_display.showImage(m_image, imgP1, imgP2, DEGREE_0);
    m_cars_passed = value;
}

void TurboOutputHandler::update_start_lights(int value) {
    if (value >= m_start_lights_last) {
        if (3 == value) {
            m_pSayer->begin("-GO-");
            for (int i = 0; i < 6; i++) {
                m_pSayer->next();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return;
        }        
        if (0 == value) {
            m_pSayer->begin("3-2-1");
        }
        m_pSayer->next();
        m_pSayer->next();
    } else {
        m_pSayer->clear();
        m_pTM1637->setColon(true);
    }
    m_start_lights_last = value;
}

void TurboOutputHandler::update_yellow_flag(int value) {
    // if (0 == value) {
    //     m_line_y1.set_value(0);
    //     m_line_y2.set_value(0);
    // } else {
    //     // struct timeb time_now;
    //     // ftime(&time_now);
    //     int odd = value % 2;
    //     m_line_y1.set_value(odd);
    //     m_line_y2.set_value(!odd);
    // }
}

void TurboOutputHandler::update_start_button(int value) {
    if (m_pSayer->finished()) {
        m_pSayer->begin("PRESS START bUTTON  ");
    }
    m_pSayer->next();
    m_start_lights_last = -1;
    digitalWrite(kPinStartBtn, (1 - (value %2)));
}

void TurboOutputHandler::update_attract_mode(int value) {
    m_attract_mode_active = (value > 0);
    reset_state();
}

void TurboOutputHandler::update_start_mode(int value) {
    m_start_mode_active = (value > 0);
    if (m_start_mode_active) {
        reset_state();
        // m_line_g.set_value(1);
    } else {
        digitalWrite(kPinStartBtn, LOW);
    }
}

void TurboOutputHandler::reset_state() {
    m_start_lights_last = -1;
    // for (int i=0; i<4; i++) {
    //     m_tm1637_digits[i] = -1;    
    // }
    m_pTM1637->clear();
    m_pTM1637->setColon(false);
    digitalWrite(kPinStartBtn, LOW);
    m_display.clearScreen(bgColor);
}