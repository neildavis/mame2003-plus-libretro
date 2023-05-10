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

// Consts for drawing on ST7789
const Color NN_GREEN = Color(61, 131, 59);
const Color AMBER(255, 192, 0);
const Color &bgColor = NN_GREEN;
const int minX = 21, maxX= 226, minY = 0, maxY = 205;
const Point imgP1(minX, minY);
const Point imgP2(maxX, maxY);
const int imgW = maxX - minX + 1, imgH = maxY - minY + 1;
const int imgCtrX = (imgW / 2), imgCtrY = (imgH / 2);
// time arc consts
const int timeArcRadius = imgW / 2;
const int timeArcThickness = 20;
const float timeArcStartDeg = 150.0;
const float timeArcWarnDeg = -80;
const float timeArcDangerDeg = -130;
const float timeArcEndDeg = -180.0;
const float timeArcDegreeRange = timeArcStartDeg - timeArcEndDeg;
const float timeArcDegreeInc = timeArcDegreeRange / 100;
// cars passed arc consts
const int kCarsPassedTarget = 30;
const int kCarsPassedMax = 41;
const int carsPassedArcRadius = 50;
const int carsPassedArcThickness = timeArcThickness;
const float carsPassedArcStartDeg = 180.0;
const float carsPassedArcEndDeg = -180.0;
const float carsPassedArcDegreeRange = carsPassedArcStartDeg - carsPassedArcEndDeg;
const float carsPassedArcDegreeInc = carsPassedArcDegreeRange / kCarsPassedMax;

TurboOutputHandler::TurboOutputHandler() :
    m_image(imgW, imgH, bgColor),
    m_logoImage(imgW, imgH, bgColor) {
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
    // Load BMP images
    char res_path[4096];
    get_resource_path(res_path, sizeof(res_path)/sizeof(char));
    char logoImagePath[4096];
    snprintf(logoImagePath, 4096, "%s/nn_206.bmp", res_path);
    m_logoImage.loadBMP(logoImagePath, 0, 0);
    // Setup initial state
    reset_state();
 }

void TurboOutputHandler::deinit() {
    m_pTM1637->clear();
    m_pTM1637.reset();
    m_pSayer.reset();
    m_display.clearScreen(BLACK);
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
    if (0 == strcmp(name, OUTPUT_TURBO_LIVES_NAME)) {
        update_lives(value);
        return;
    }
    if (0 == strcmp(name, OUTPUT_TURBO_STAGE_NAME)) {
        update_stage(value);
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

void TurboOutputHandler::draw_time_arc() {
    Color arcSegmentColor = GREEN;
    float degreeTimeThis = timeArcEndDeg + (timeArcDegreeRange * m_time / m_time_max);
    m_image = m_logoImage;
    for (float degree = degreeTimeThis; degree > timeArcEndDeg; degree -= timeArcDegreeInc) {    
        if (degree < timeArcDangerDeg) {
            arcSegmentColor = RED;
        } else if (degree < timeArcWarnDeg) {
            arcSegmentColor = AMBER;
        }
        m_image.drawArc(imgCtrX, imgCtrY, timeArcRadius, timeArcRadius -timeArcThickness, degree - timeArcDegreeInc, degree, arcSegmentColor, 3);
    }
}

void TurboOutputHandler::update_time(int value) {
    if (m_attract_mode_active || m_start_mode_active) {
        return; // Ignore time during attract and start screen modes
    }

    // Time on ST7789
    if (value > m_time) {
        m_time_max = value;
    }
    m_time = value;
    draw_time_arc();
    draw_cars_passed_arc();
    m_display.showImage(m_image, imgP1, imgP2, DEGREE_0);
}

void TurboOutputHandler::draw_cars_passed_arc() {
    if (0 == m_cars_passed) {
        return;
    }
    
    Color cpColor(RED);
    if (m_last_yellow_flag > 0) {
        cpColor = YELLOW;
    } else if (m_cars_passed >= kCarsPassedTarget) {
        cpColor = GREEN;
    }
    float degreeCpThis = carsPassedArcStartDeg - (carsPassedArcDegreeRange * m_cars_passed / kCarsPassedMax);
    // Draw CP on m_image        
    m_image.drawArc(imgCtrX, imgCtrY, carsPassedArcRadius, carsPassedArcRadius - carsPassedArcThickness, degreeCpThis, carsPassedArcStartDeg, cpColor, 2);
}

void TurboOutputHandler::update_cars_passed(int value) {
    if (m_attract_mode_active || m_start_lights_last < 0) {
        return; // Ignore cars passed during attract mode
    }
    
    m_cars_passed = value;
    draw_time_arc();
    draw_cars_passed_arc();
    m_display.showImage(m_image, imgP1, imgP2, DEGREE_0);
}

void TurboOutputHandler::update_start_lights(int value) {
    if (value >= m_start_lights_last) {
        if (3 == value) {
            m_pSayer->begin("GO");
            for (int i = 0; i < 6; i++) {
                m_pSayer->next();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            update_tm1637();
            return;
        }        
        if (0 == value) {
            m_pSayer->begin("3-2-1");
        }
        m_pSayer->next();
        m_pSayer->next();
    }
    m_start_lights_last = value;
}

void TurboOutputHandler::update_yellow_flag(int value) {
    bool redrawCP = false;
    if ((1 == value &&  0 == m_last_yellow_flag) || 
        (0 == value && m_last_yellow_flag != 0)) {
        // Start of yellow flags - redraw cp on yellow bg
        redrawCP = true;
    }
    m_last_yellow_flag = value;
    if (redrawCP) {
        update_cars_passed(m_cars_passed);
    }
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
    reset_state();
    m_attract_mode_active = (value > 0);
}

void TurboOutputHandler::update_start_mode(int value) {
    if (value) {
        reset_state();
        m_display.showImage(m_logoImage, imgP1, imgP2, DEGREE_0);
    } else {
        digitalWrite(kPinStartBtn, LOW);
    }
    m_start_mode_active = (value > 0);
}
void TurboOutputHandler::update_lives(int value) {
    if (0 == m_time) {
        return;
    }
    m_lives = value;
    update_tm1637();
}

void TurboOutputHandler::update_stage(int value) {
    if (0 == m_time) {
        return;
    }
    m_stage = value;
    update_tm1637();
}

void TurboOutputHandler::update_tm1637() {
    // Round
    static const int8_t NUM_DIGITS[16] = {DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4, DIGIT_5, DIGIT_6, DIGIT_7, DIGIT_8, 
                                    DIGIT_9, DIGIT_A, DIGIT_b, DIGIT_C, DIGIT_d, DIGIT_E, DIGIT_F};
    m_tm1637_digits[0] = DIGIT_r;
    m_tm1637_digits[1] = DIGIT_QUESTION_MARK;
    if ((unsigned int)m_stage < sizeof(NUM_DIGITS) / sizeof(int8_t)) {
        m_tm1637_digits[1] = NUM_DIGITS[m_stage + 1];
    }
    if (m_stage > 0) {
        // stage is zero indexed. Lives only apply after first round/stage
        m_tm1637_digits[2] = DIGIT_L;
        m_tm1637_digits[3] = DIGIT_QUESTION_MARK;
        if ((unsigned int)m_lives < sizeof(NUM_DIGITS) / sizeof(int8_t)) {
            m_tm1637_digits[3] = NUM_DIGITS[m_lives];
        }    
    } else {
        m_tm1637_digits[2] = 0;
        m_tm1637_digits[3] = 0;
    }
    m_pTM1637->showRawDigits(m_tm1637_digits);
    m_pTM1637->setColon(true);
}

void TurboOutputHandler::reset_state() {
    m_start_lights_last = -1;
    m_time = 0;
    m_time_max = 0;
    m_cars_passed = 0;
    m_last_yellow_flag = 0;
    m_lives = 0;
    m_stage = 0;
    m_attract_mode_active = false;
    m_start_mode_active = false;
    for (int i=0; i<4; i++) {
        m_tm1637_digits[i] = -1;    
    }
    m_pTM1637->setColon(false);
    m_pTM1637->clear();
    digitalWrite(kPinStartBtn, LOW);
    m_display.showImage(m_logoImage, imgP1, imgP2, DEGREE_0);
}