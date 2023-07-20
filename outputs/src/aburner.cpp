#include <string.h>
#include <stdio.h>

#include <wiringPi.h>

#include "aburner.h"
#include "utils.h"
#include "output-def.h"

using namespace udd;

/* Define GPIO pins for After Burner lamp outputs */
static const int GPIO_LAMP_DANGER = 14;    /* Danger lamp will be on GPIO/BCM pin 14 */

static const int kSpiSpeed = 90000000;
// Note: Although our display is sold as 280x240 it is actually 320x240 from the driver point of view.
// Hence use of Point offsets below
// It is also rendered in portrait orientation (hence use of kDisplayRotation rotation everywhere!)
static const int kDisplayWidth = 240;
static const int kDisplayHeight = 320;
static const Rotation kDisplayRotation = DEGREE_90;

// Colors
Color SEGA_BLUE = Color(0, 96, 168);
// Positions to place images
Point point_tl = Point(20, 0);
Point point_br = Point(299, 239);
Point point_start_tl = Point(30, 62);
Point point_start_br = Point(289, 178);
Point point_lock_tl = Point(20, 68);
Point point_lock_br = Point(299, 171);

AfterBurnerOutputHandler::~AfterBurnerOutputHandler() {
}

void AfterBurnerOutputHandler::init() {
    m_display.reset(new DisplayST7789R());
    DisplayConfiguration displayConfig;
    displayConfig.width = kDisplayWidth;
    displayConfig.height = kDisplayHeight;
    displayConfig.spiSpeed = kSpiSpeed;

    displayConfig.CS = 5;   // SPI Chip Select (we're using SPI0 with MOSI/SCLK on BCM 10/11 but overriding use of CE0/CE1)
    displayConfig.DC = 6;   // TFT SPI Data or Command selector
    displayConfig.RST = 13; // Display Reset
    displayConfig.BLK = -1; // ?? Not used

    m_display->openDisplay(displayConfig);
    m_display->clearScreen(SEGA_BLUE);

	wiringPiSetupGpio();  // use BCM pin numbers
	pinMode(GPIO_LAMP_DANGER, OUTPUT);
    digitalWrite(GPIO_LAMP_DANGER, 0);

    // Images
    m_bmp_press_start.reset(new Image(260, 115, BLACK));
    m_bmp_clear_press_start.reset(new Image(260, 115, BLACK));
    m_bmp_lock.reset(new Image(280, 104, BLACK));
    m_bmp_clear_lock.reset(new Image(280, 104, BLACK));
    char res_path[4096];
    get_resource_path(res_path, sizeof(res_path)/sizeof(char));
    char path[4096];
    snprintf(path, 4096, "%s/lock.bmp", res_path);
    m_bmp_lock->loadBMP(path, 0, 0);
    snprintf(path, 4096, "%s/startbut.bmp", res_path);
    m_bmp_press_start->loadBMP(path, 0, 0);
}

void AfterBurnerOutputHandler::deinit() {
    // Blank screen and clear LEDs
    m_display->clearScreen(BLACK);
    digitalWrite(GPIO_LAMP_DANGER, 0);
    // Free resources
    m_bmp_press_start.reset();
    m_bmp_clear_press_start.reset();
    m_bmp_lock.reset();
    m_bmp_clear_lock.reset();
    m_display.reset();
}

void AfterBurnerOutputHandler::handle_output(const char *name, int value) {

    int n;

    n = parseLampOutputName(name);
    if (n >= 0) {
        if (AFTER_BURNER_LAMP_LOCK_ON == n) {
            /* After Burner 'Lock-On' lamp */
            update_lock(value);
        } else if (AFTER_BURNER_LAMP_DANGER == n) {
            /* After Burner 'Danger' lamp */
            update_danger(value);
        } else if (AFTER_BURNER_LAMP_ALTITUDE_WARNING == n) {
            /* After Burner 'Altitude Warning' lamp */
            update_altitude_warning(value);
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

void AfterBurnerOutputHandler::update_danger(int value) {
   digitalWrite(GPIO_LAMP_DANGER, value > 0 ? 1 : 0);
}

void AfterBurnerOutputHandler::update_lock(int value) {
    std::unique_ptr<Image> &image = value > 0 ? m_bmp_lock : m_bmp_clear_lock;
    m_display->showImage(*image, point_lock_tl, point_lock_br, kDisplayRotation);
}

void AfterBurnerOutputHandler::update_altitude_warning(int value) {
     /* Not sure this is used? */
     printf("%s: After Burner ALTITUDE WARNING %s\n", proc_name, value > 0 ? "ON" : "OFF");
}

void AfterBurnerOutputHandler::update_start_led(int value) {
    std::unique_ptr<Image> &image = value > 0 ? m_bmp_press_start : m_bmp_clear_press_start;
    m_display->showImage(*image, point_start_tl, point_start_br, kDisplayRotation);
}
