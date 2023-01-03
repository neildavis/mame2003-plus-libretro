
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "output-def.h"
#include "afterburner.h"
#include "turbo.h"

void main_event_loop() {
    int fd, err, n;
    char buf[OUTPUTS_PIPE_MAX_BUF_SIZE];
    char machine_name[OUTPUTS_PIPE_MAX_MACHINE_NAME_SIZE];
    char output_name[OUTPUTS_PIPE_MAX_OUTPUT_NAME_SIZE];
    int output_value;
    
    while (1) {
        /* Open the FIFO pipe for reading */
        fprintf(stdout, "%s: Opening FIFO pipe at %s\n", proc_name, OUTPUTS_PIPE_NAME);
        fd = open(OUTPUTS_PIPE_NAME, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "%s: Unable to open FIFO pipe for reading at %s errno=%d\n", proc_name, OUTPUTS_PIPE_NAME, errno);
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
                fprintf(stdout, "%s: Client hung-up pipe\n", proc_name);
                break;
            }
            n = read(fd, buf, sizeof(buf));
            if (n > 0 && 3 == sscanf(buf, OUTPUTS_BUF_FMT, machine_name, output_name, &output_value)) {
                /* We successfully read an output command */
                fprintf(stdout, "%s: Read output %s=%d for machine '%s'\n", proc_name, output_name, output_value, machine_name);
                if (0 == strcmp(machine_name, "aburner")) {
                    /* After Burner */
                    aburner_output(output_name, output_value);
                } else if (0 == strcmp(machine_name, "turbo")) {
                    turbo_output(output_name, output_value);
                }
            }
        }

        /* Close the pipe ready to re-open for next client */
        fprintf(stdout, "%s: Closing pipe\n", proc_name);
        close(fd);
    }
}

int main(int argc, char **argv) {
    int err;
    proc_name = argv[0];

    /* Make the FIFO pipe */
    fprintf(stdout, "%s: Creating FIFO pipe at %s\n", argv[0], OUTPUTS_PIPE_NAME);
    err = mkfifo(OUTPUTS_PIPE_NAME, 0666);
    if (err != 0 && EEXIST != errno) {
        fprintf(stderr, "%s: Unable to create FIFO pipe at %s errno=%d\n", argv[0], OUTPUTS_PIPE_NAME, errno);
        exit(errno);
    }

    /* Run the main evenbt loop forever */
    main_event_loop();
}