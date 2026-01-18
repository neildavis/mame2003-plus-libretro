#include <string.h>
#include <stdio.h>
#include <string>

#include <pigpiod_if2.h>
#include <wiringPi.h>

#include "aburner.h"
#include "utils.h"
#include "output-def.h"

using namespace udd;

/* Define GPIO pins for outputs */
// Pins for LEDs
static const int PIN_LAMP_DANGER    = 14;    /* Danger lamp will be on GPIO/BCM pin 14 */
// Pins for ST7789 LCD Display
static const int PIN_ST7798_CS      = 5;    // Chip Select
static const int PIN_ST7798_DC      = 6;    // SPI Data/Cmd select
static const int PIN_ST7798_RST     = 13;   // ReSeT
static const int PIN_ST7798_BLK     = -1;   // Backlight (not used)
// Pins for servos
const int PIN_HORIZ_H_SERVO         = 26;   // BCM
const int PIN_HORIZ_V_SERVO         = 16;   // BCM

// Artificial Horizon Servo Control
// - Horiz: Range == 730
const int SERVO_HORIZ_PWM_MIN   = 790;  // Full Left
const int SERVO_HORIZ_PWM_MID   = 1520; // Mid point (level)
const int SERVO_HORIZ_PWM_MAX   = 2250; // Full Right
// - Vert: Range == 350
const int SERVO_VERT_PWM_MIN   = 1100;  // Full Up
const int SERVO_VERT_PWM_MID   = 1450; // Mid point
const int SERVO_VERT_PWM_MAX   = 1800; // Full Down

static const int kSpiSpeed = 90000000;
// Note: Although our display is sold as 280x240 it is actually 320x240 from the driver point of view.
// Hence use of Point offsets below
// It is also rendered in portrait orientation (hence use of kDisplayRotation rotation everywhere!)
static const int kDisplayWidth = 240;
static const int kDisplayHeight = 320;
static const Rotation kDisplayRotation = DEGREE_90;

// Splash Screen durations
static const int kSplashScreenDuration = 5;    // seconds
static const int kTitleScreenDuration  = 5;    // seconds

// Colors
Color SEGA_BLUE = Color(0, 96, 168);
// Positions to place images
Point point_tl = Point(20, 0);
Point point_br = Point(299, 239);
Point point_start_tl = Point(30, 62);
Point point_start_br = Point(289, 178);
Point point_lock_tl = Point(20, 68);
Point point_lock_br = Point(299, 171);

/******************************************************************************
*
* OutputHandlerBase virtual override methods
*
******************************************************************************/

AfterBurnerOutputHandler::~AfterBurnerOutputHandler() {
}

void AfterBurnerOutputHandler::init(OutputHandlerMode mode) {
    // Init pigpiod interface
    m_pi_handle = pigpio_start(NULL, NULL);
    if (m_pi_handle < 0) {
        printf("Failed to connect to pigpiod daemon\n");
    }
    // Init ST7789
    m_display.reset(new DisplayST7789R());
    DisplayConfiguration displayConfig;
    displayConfig.width = kDisplayWidth;
    displayConfig.height = kDisplayHeight;
    displayConfig.spiSpeed = kSpiSpeed;

    displayConfig.CS    = PIN_ST7798_CS;   // SPI Chip Select (we're using SPI0 with MOSI/SCLK on BCM 10/11 but overriding use of CE0/CE1)
    displayConfig.DC    = PIN_ST7798_DC;   // TFT SPI Data or Command selector
    displayConfig.RST   = PIN_ST7798_RST; // Display Reset
    displayConfig.BLK   = PIN_ST7798_BLK; // ?? Not used

	wiringPiSetupGpio();  // use BCM pin numbers
    m_display->openDisplay(displayConfig);
    m_display->clearScreen(BLACK);

    // Setup LEDs
	pinMode(PIN_LAMP_DANGER, OUTPUT);
    digitalWrite(PIN_LAMP_DANGER, 0);

    // Centre Servos
    set_servo_pulsewidth(m_pi_handle, PIN_HORIZ_H_SERVO, SERVO_HORIZ_PWM_MID);
    set_servo_pulsewidth(m_pi_handle, PIN_HORIZ_V_SERVO, SERVO_VERT_PWM_MID);

    // Images - only needed in 'Game' mode
    if (OutputHandlerModeGame == mode) {
        m_bmp_press_start.reset(new Image(260, 115, BLACK));
        m_bmp_clear_press_start.reset(new Image(260, 115, BLACK));
        m_bmp_lock.reset(new Image(280, 104, BLACK));
        m_bmp_clear_lock.reset(new Image(280, 104, BLACK));
        char res_path[4096];
        get_resource_path(res_path, sizeof(res_path)/sizeof(char));
        std::string path = std::string(res_path) + "/lock.bmp";
        m_bmp_lock->loadBMP(path.c_str(), 0, 0);
        path = std::string(res_path) + "/startbut.bmp";
        m_bmp_press_start->loadBMP(path.c_str(), 0, 0);
    }
}

void AfterBurnerOutputHandler::deinit() {
    // Blank screen and clear LEDs
    m_display->clearScreen(BLACK);
    digitalWrite(PIN_LAMP_DANGER, 0);
    // Stop Servos
    if (m_pi_handle >= 0) {
        set_servo_pulsewidth(m_pi_handle, PIN_HORIZ_H_SERVO, 0); // stop servo
        set_servo_pulsewidth(m_pi_handle, PIN_HORIZ_V_SERVO, 0); // stop servo
        pigpio_stop(m_pi_handle);
        m_pi_handle = -1;
    }
    // Free resources
    m_bmp_press_start.reset();
    m_bmp_clear_press_start.reset();
    m_bmp_lock.reset();
    m_bmp_clear_lock.reset();
    m_display.reset(); // Closes SPI via udd::~Display()
}

void AfterBurnerOutputHandler::handle_output(const char *name, int value) {

    int n;

    if (strcmp(OUTPUT_AFTER_BURNER_HORIZ_H_NAME, name) == 0) {
        update_horizon_h(value);
        return;
    } else if (strcmp(OUTPUT_AFTER_BURNER_HORIZ_V_NAME, name) == 0) {
        update_horizon_v(value);
        return;
    }
    n = parseLampOutputName(name);
    if (n >= 0) {
        if (AFTER_BURNER_LAMP_LOCK_ON == n) {
            /* After Burner 'Lock-On' lamp */
            update_lock(value);
        } else if (AFTER_BURNER_LAMP_DANGER == n) {
            /* After Burner 'Danger' lamp */
            update_danger(value);
        } else if (AFTER_BURNER_LAMP_FF == n) {
            /* After Burner Force Feedback Motor */
            update_force_feedback(value);
        }
        return;
    }
    n = parseLedOutputName(name);
    if (n >= 0) {
        if (AFTER_BURNER_LED_START == n) {
            /* After Burner Start LED */
            update_start_led(value);
        }
        return;
    }
}

void AfterBurnerOutputHandler::do_boot() {
    /* Show Splash screens */
    show_splash_screen("splash.bmp");
    sleep(kSplashScreenDuration);
    show_splash_screen("title.bmp");
    sleep(kTitleScreenDuration);
    m_display->clearScreen(BLACK);
}


/******************************************************************************
*
* Private methods
*
******************************************************************************/

void AfterBurnerOutputHandler::show_splash_screen(const char *filename) {
    Image splash = Image(280, 240, SEGA_BLUE);
    char res_path[4096];
    get_resource_path(res_path, 4096);
    std::string path = std::string(res_path) + '/' + filename;
    splash.loadBMP(path.c_str(), 0, 0);
    m_display->showImage(splash, point_tl, point_br, kDisplayRotation);
}

void AfterBurnerOutputHandler::update_danger(int value) {
    digitalWrite(PIN_LAMP_DANGER, value > 0 ? 1 : 0);
}

void AfterBurnerOutputHandler::update_lock(int value) {
    std::unique_ptr<Image> &image = value > 0 ? m_bmp_lock : m_bmp_clear_lock;
    m_display->showImage(*image, point_lock_tl, point_lock_br, kDisplayRotation);
}

void AfterBurnerOutputHandler::update_force_feedback(int value) {
    /* For now, until we have a force feedback motor we just light the Danger lamps for a bit more effect */
    digitalWrite(PIN_LAMP_DANGER, value > 0 ? 1 : 0);
}

void AfterBurnerOutputHandler::update_start_led(int value) {
    std::unique_ptr<Image> &image = value > 0 ? m_bmp_press_start : m_bmp_clear_press_start;
    m_display->showImage(*image, point_start_tl, point_start_br, kDisplayRotation);
}


static const int ab_horiz_h_min = -92;
static const int ab_horiz_h_max = 92;
void AfterBurnerOutputHandler::update_horizon_h(int value) {
    int hval = int((int16_t)value);
    if (hval < ab_horiz_h_min) hval = ab_horiz_h_min;
    else if (hval > ab_horiz_h_max) hval = ab_horiz_h_max;
    int hpwm = SERVO_HORIZ_PWM_MIN + ((hval - ab_horiz_h_min) * (SERVO_HORIZ_PWM_MAX - SERVO_HORIZ_PWM_MIN) / (ab_horiz_h_max - ab_horiz_h_min));
	//printf("aburner2: horiz H: %d : %d\n", hval, hpwm);
    set_servo_pulsewidth(m_pi_handle, PIN_HORIZ_H_SERVO, hpwm);   
}

static const int ab_horiz_v_min = -80;
static const int ab_horiz_v_max = 110;
void AfterBurnerOutputHandler::update_horizon_v(int value) {
    int vval = int((int16_t)value);
    if (vval < ab_horiz_v_min) vval = ab_horiz_v_min;
    else if (vval > ab_horiz_v_max) vval = ab_horiz_v_max;
    int vpwm = SERVO_VERT_PWM_MAX - ((vval - ab_horiz_v_min) * (SERVO_VERT_PWM_MAX - SERVO_VERT_PWM_MIN) / (ab_horiz_v_max - ab_horiz_v_min));
	//printf("aburner2: horiz V: %d : %d\n", vval, vpwm);
    set_servo_pulsewidth(m_pi_handle, PIN_HORIZ_V_SERVO, vpwm);   
}
