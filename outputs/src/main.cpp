
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

#include <wiringPi.h>
#include <udd.h>

#include "output-def.h"

using namespace udd;

DisplayConfigruation displayConfig;
DisplayST7789R display = DisplayST7789R();

/* Define GPIO pins for After Burner lamp outputs */
const int GPIO_LAMP_DANGER = 14;    /* Danger lamp will be on GPIO/BCM pin 14 */

const int kSpiSpeed = 90000000;
// Note: Although our display is sold as 280x240 it is actually 320x240 from the driver point of view.
// Hence use of Point offsets below
// It is also rendered in portrait orientation (hence use of DEGREE_270 rotation everywhere!)
const int kDisplayWidth = 240;
const int kDisplayHeight = 320;

// Images
Image bmp_lock= Image(280, 104, BLACK);
Image bmp_clear_lock= Image(280, 104, BLACK);
// Positions to place images
Point point_lock_tl = Point(20, 68);
Point point_lock_br = Point(299, 171);

void configureDisplay() {
    displayConfig.width = kDisplayWidth;
    displayConfig.height = kDisplayHeight;
    displayConfig.spiSpeed = kSpiSpeed;

    displayConfig.CS = 5;   // SPI Chip Select (we're using SPI0 with MOSI/SCLK on BCM 10/11 but overriding use of CE0/CE1)
    displayConfig.DC = 6;   // TFT SPI Data or Command selector
    displayConfig.RST = 13; // Display Reset
    displayConfig.BLK = -1; // ?? Not used

    display.openDisplay(displayConfig);
    display.clearScreen(BLACK);
}

void initWiringPi() {
	wiringPiSetupGpio();  // use BCM pin numbers
	pinMode(GPIO_LAMP_DANGER, OUTPUT);
    digitalWrite(GPIO_LAMP_DANGER, 0);
}

void initImages() {
    const char *homedir;

    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
   
    char path[4096];
    sprintf(path, "%s/.local/lrmame2003osvr/lock.bmp", homedir);
    bmp_lock.loadBMP(path, 0, 0);
}

int parseLampOutputName(const char *output_name) {
   int value;
   if (1 == sscanf(output_name, "lamp%d", &value)) {
       return value;
   }
   return -1;
 }

int parseLedOutputName(const char *output_name) {
   int value;
   if (1 == sscanf(output_name, "led%d", &value)) {
       return value;
   }
   return -1;
 }

 void aburner_lock(bool lock) {
    Image &image = lock ? bmp_lock : bmp_clear_lock;
    display.showImage(image, point_lock_tl, point_lock_br, DEGREE_270);
}

 void aburner_danger(bool danger) {
    digitalWrite(GPIO_LAMP_DANGER, danger ? 1 : 0);
 }

 void aburner_output(const char *output_name, int value) {
    int n;
    n = parseLampOutputName(output_name);
    if (n >= 0) {
        if (AFTER_BURNER_LAMP_LOCK_ON == n) {
            /* After Burner 'Lock-On' lamp */
            aburner_lock(value > 0);
        } else if (AFTER_BURNER_LAMP_DANGER == n) {
            /* After Burner 'Danger' lamp */
            aburner_danger(value > 0);
        }
        return;
    }
    n = parseLedOutputName(output_name);
    if (n >= 0) {
        if (AFTER_BURNER_LED_START == n) {
            /* TODO: After Burner Start LED */
        }
        return;
    }
}

int main(int argc, char **argv) {
    int fd, err, n;
    char buf[OUTPUTS_PIPE_MAX_BUF_SIZE];

    char machine_name[OUTPUTS_PIPE_MAX_MACHINE_NAME_SIZE];
    char output_name[OUTPUTS_PIPE_MAX_OUTPUT_NAME_SIZE];
    int output_value;

    /* WiringPi Setup */
    initWiringPi();

    /* Configure the ST7789 display */
    configureDisplay();

    /* Pre-load images */
    initImages();

    /* Make the FIFO pipe */
    fprintf(stdout, "%s: Creating FIFO pipe at %s\n", argv[0], OUTPUTS_PIPE_NAME);
    err = mkfifo(OUTPUTS_PIPE_NAME, 0666);
    if (err != 0 && EEXIST != errno) {
        fprintf(stderr, "%s: Unable to create FIFO pipe at %s errno=%d\n", argv[0], OUTPUTS_PIPE_NAME, errno);
        exit(errno);
    }

    while (1) {
        /* Open the FIFO pipe for reading */
        fprintf(stdout, "%s: Opening FIFO pipe at %s\n", argv[0], OUTPUTS_PIPE_NAME);
        fd = open(OUTPUTS_PIPE_NAME, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "%s: Unable to open FIFO pipe for reading at %s errno=%d\n", argv[0], OUTPUTS_PIPE_NAME, errno);
            exit(errno);
        }

        /* Continually read output commands from the pipe */
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        while (1) {
            n = poll(&pfd, 1, -1);
            if (pfd.revents & POLLHUP) {
                /* client closed their end of the pipe */
                fprintf(stdout, "%s: Client hung-up pipe\n", argv[0]);
                break;
            }
            n = read(fd, buf, sizeof(buf));
            if (n > 0 && 3 == sscanf(buf, OUTPUTS_BUF_FMT, machine_name, output_name, &output_value)) {
                /* We successfully read an output command */
                fprintf(stdout, "%s: Read output %s=%d for machine '%s'\n", argv[0], output_name, output_value, machine_name);
                if (strcmp(machine_name, "aburner") == 0) {
                    /* After Burner */
                    aburner_output(output_name, output_value);
                }
            }
        }

        /* Blank screen and clear LEDs */
        display.clearScreen(BLACK);
        digitalWrite(GPIO_LAMP_DANGER, 0);

        /* Close the pipe ready to re-open for next client */
        fprintf(stdout, "%s: Closing pipe\n", argv[0]);
        close(fd);
    }
}