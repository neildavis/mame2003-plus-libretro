
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
#include <wiringPi.h>

#include "output-def.h"

void initWiringPi(void) {
    wiringPiSetupGpio();
	pinMode(GPIO_LAMP_LOCK_ON, OUTPUT);
	pinMode(GPIO_LAMP_DANGER, OUTPUT);
    digitalWrite(GPIO_LAMP_LOCK_ON, 0);
    digitalWrite(GPIO_LAMP_DANGER, 0);
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

 void aburner_output(const char *output_name, int value) {
    int n;
    n = parseLampOutputName(output_name);
    if (n >= 0) {
        if (AFTER_BURNER_LAMP_LOCK_ON == n) {
            /* After Burner 'Lock-On' lamp */
            digitalWrite(GPIO_LAMP_LOCK_ON, value);
        } else if (AFTER_BURNER_LAMP_DANGER == n) {
            /* After Burner 'Danger' lamp */
            digitalWrite(GPIO_LAMP_DANGER, value);
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

        /* Close the pipe ready to re-open for next client */
        fprintf(stdout, "%s: Closing pipe\n", argv[0]);
        close(fd);
    }
}