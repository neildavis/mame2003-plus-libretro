
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <memory>
#include <poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <sys/timeb.h>

#include "utils.h"
#include "output-def.h"
#include "output_handler_base.h"
#include "aburner.h"
#include "turbo.h"

void main_event_loop() {
    int fd;
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
        FILE *stream = fdopen(fd, "r");
        if (NULL == stream) {
            fprintf(stderr, "%s: Unable to open FIFO pipe stream for reading. errno=%d\n", proc_name, errno);
            exit(errno);
        }

        /* Continually read output commands from the pipe */
        struct timeb time_now;
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        std::unique_ptr<MOutputHandler> pOutputHandler;
        while (1) {
            poll(&pfd, 1, -1);
            if (pfd.revents & POLLHUP) {
                /* client closed their end of the pipe */
                fprintf(stdout, "%s: Client hung-up pipe\n", proc_name);
                break;
            }
            if (3 == fscanf(stream, "%[^:]:%[^:]:%d", machine_name, output_name, &output_value)) {
                /* We successfully read an output command */
                ftime(&time_now);
                fprintf(stdout, "%s: T%ld.%03d Read output %s=%d for machine '%s'\n", proc_name, time_now.time, time_now.millitm, output_name, output_value, machine_name);
                if (!pOutputHandler && 0 == strcmp(OUTPUTS_INIT_NAME, output_name)) {
                    // Initialize output handler
                    if (0 == strcmp(machine_name, "aburner")) {
                        /* After Burner */
                        fprintf(stdout, "%s: Initializing new instance of TurboOutputHandler\n", proc_name);
                        pOutputHandler.reset(new AfterBurnerOutputHandler());
                    } else if (0 == strcmp(machine_name, "turbo")) {
                        fprintf(stdout, "%s: Initializing new instance of TurboOutputHandler\n", proc_name);
                        pOutputHandler.reset(new TurboOutputHandler());
                    }
                    pOutputHandler->init();
                    continue;                
                }
                if (pOutputHandler) {
                    pOutputHandler->handle_output(output_name, output_value);
                }

            } else {
                fprintf(stdout, "%s: Failed to scan\n", proc_name);
            }
        }

        // Deinitialze output handler
        fprintf(stdout, "%s: Deinit output handler\n", proc_name);
        pOutputHandler->deinit();

        /* Close the pipe ready to re-open for next client */
        fprintf(stdout, "%s: Closing stream\n", proc_name);
        fclose(stream);
        fprintf(stdout, "%s: Closing pipe\n", proc_name);
        close(fd);
    }
}

int main(int /*argc*/, char **argv) {
    int err;
    proc_name = argv[0];

    /* Make the FIFO pipe */
    fprintf(stdout, "%s: Creating FIFO pipe at %s\n", argv[0], OUTPUTS_PIPE_NAME);
    err = mkfifo(OUTPUTS_PIPE_NAME, 0666);
    if (err != 0 && EEXIST != errno) {
        fprintf(stderr, "%s: Unable to create FIFO pipe at %s errno=%d\n", argv[0], OUTPUTS_PIPE_NAME, errno);
        exit(errno);
    }

    /* Run the main event loop forever */
    main_event_loop();
}