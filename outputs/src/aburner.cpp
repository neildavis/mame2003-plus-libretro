#include <string.h>
#include <stdio.h>

#include <wiringPi.h>
#include <udd.h>

#include "utils.h"
#include "output-def.h"

using namespace udd;

DisplayConfigruation displayConfig;
DisplayST7789R display = DisplayST7789R();

/* Define GPIO pins for After Burner lamp outputs */
const int GPIO_LAMP_DANGER = 14;    /* Danger lamp will be on GPIO/BCM pin 14 */

const int kSpiSpeed = 90000000;
// Note: Although our display is sold as 280x240 it is actually 320x240 from the driver point of view.
// Hence use of Point offsets below
// It is also rendered in portrait orientation (hence use of kDisplayRotation rotation everywhere!)
const int kDisplayWidth = 240;
const int kDisplayHeight = 320;
const Rotation
 kDisplayRotation = DEGREE_90;

// Colors
Color SEGA_BLUE = Color(0, 96, 168);
// Images
Image bmp_press_start = Image(260, 115, BLACK);
Image bmp_clear_press_start = Image(260, 115, BLACK);
Image bmp_lock = Image(280, 104, BLACK);
Image bmp_clear_lock = Image(280, 104, BLACK);
// Positions to place images
Point point_tl = Point(20, 0);
Point point_br = Point(299, 239);
Point point_start_tl = Point(30, 62);
Point point_start_br = Point(289, 178);
Point point_lock_tl = Point(20, 68);
Point point_lock_br = Point(299, 171);

void aburner_initImages() {
    char res_path[4096];
    get_resource_path(res_path, sizeof(res_path)/sizeof(char));
    char path[4096];
    snprintf(path, 4096, "%s/lock.bmp", res_path);
    bmp_lock.loadBMP(path, 0, 0);
    snprintf(path, 4096, "%s/startbut.bmp", res_path);
    bmp_press_start.loadBMP(path, 0, 0);
}

void aburner_configureDisplay() {
    displayConfig.width = kDisplayWidth;
    displayConfig.height = kDisplayHeight;
    displayConfig.spiSpeed = kSpiSpeed;

    displayConfig.CS = 5;   // SPI Chip Select (we're using SPI0 with MOSI/SCLK on BCM 10/11 but overriding use of CE0/CE1)
    displayConfig.DC = 6;   // TFT SPI Data or Command selector
    displayConfig.RST = 13; // Display Reset
    displayConfig.BLK = -1; // ?? Not used

    display.openDisplay(displayConfig);
    display.clearScreen(SEGA_BLUE);
}

void aburner_initWiringPi() {
	wiringPiSetupGpio();  // use BCM pin numbers
	pinMode(GPIO_LAMP_DANGER, OUTPUT);
    digitalWrite(GPIO_LAMP_DANGER, 0);
}

void aburner_lock(bool lock) {
    Image &image = lock ? bmp_lock : bmp_clear_lock;
    display.showImage(image, point_lock_tl, point_lock_br, kDisplayRotation);
}

 void aburner_danger(bool danger) {
    digitalWrite(GPIO_LAMP_DANGER, danger ? 1 : 0);
 }

 void aburner_altitude_warning(bool warn) {
     /* Not sure this is used? */
     printf("%s: After Burner ALTITUDE WARNING %s\n", proc_name, warn? "ON" : "OFF");
 }

 void aburner_start_led(bool on) {
    Image &image = on ? bmp_press_start : bmp_clear_press_start;
    display.showImage(image, point_start_tl, point_start_br, kDisplayRotation);
}

void aburner_init() {
    /* WiringPi Setup */
    aburner_initWiringPi();

    /* Configure the ST7789 display */
    aburner_configureDisplay();

    /* Pre-load images */
    aburner_initImages();
}

void aburner_stop() {
    /* Blank screen and clear LEDs */
    display.clearScreen(BLACK);
    digitalWrite(GPIO_LAMP_DANGER, 0);
}

void aburner_output(const char *output_name, int value) {
    int n;
    if (0 == strcmp(output_name, OUTPUTS_INIT_NAME)) {
        aburner_init();
        return;
    }
    if (0 == strcmp(output_name, OUTPUTS_STOP_NAME)) {
        aburner_stop();
        return;
    }
    n = parseLampOutputName(output_name);
    if (n >= 0) {
        if (AFTER_BURNER_LAMP_LOCK_ON == n) {
            /* After Burner 'Lock-On' lamp */
            aburner_lock(value > 0);
        } else if (AFTER_BURNER_LAMP_DANGER == n) {
            /* After Burner 'Danger' lamp */
            aburner_danger(value > 0);
        } else if (AFTER_BURNER_LAMP_ALTITUDE_WARNING == n) {
            /* After Burner 'Altitude Warning' lamp */
            aburner_altitude_warning(value > 0);
        }
        return;
    }
    n = parseLedOutputName(output_name);
    if (n >= 0) {
        if (AFTER_BURNER_LED_START == n) {
            /* After Burner Start LED */
            aburner_start_led(value != 0);
        }
        return;
    }
}

