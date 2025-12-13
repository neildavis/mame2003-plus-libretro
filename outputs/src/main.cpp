
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
#include <signal.h>

#include "utils.h"
#include "output-def.h"
#include "output_handler_base.h"
#include "aburner.h"
#include "turbo.h"
#include "monacogp.h"
#include "chasehq.h"


static volatile sig_atomic_t keep_running = 1;

static void sig_handler(int _)
{
    (void)_;
    keep_running = 0;
}

void main_event_loop() {
    FILE *stream = NULL;
    int fd  = -1;
    char machine_name[OUTPUTS_PIPE_MAX_MACHINE_NAME_SIZE];
    char output_name[OUTPUTS_PIPE_MAX_OUTPUT_NAME_SIZE];
    int output_value;
    
    while (keep_running) {
        /* Open the FIFO pipe for reading */
        fprintf(stdout, "%s: Opening FIFO pipe at %s\n", proc_name, OUTPUTS_PIPE_NAME);
        fd = open(OUTPUTS_PIPE_NAME, O_RDONLY|O_NONBLOCK);
        if (fd < 0) {
            fprintf(stderr, "%s: Unable to open FIFO pipe for reading at %s errno=%d\n", proc_name, OUTPUTS_PIPE_NAME, errno);
            exit(errno);
        }
        stream = fdopen(fd, "r");
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
        int poll_ret = 0;
        std::unique_ptr<MOutputHandler> pOutputHandler;
        while (poll_ret >= 0 && keep_running) {
            poll_ret = poll(&pfd, 1, -1);
            if (poll_ret < 0) {
                /* poll failed */
                fprintf(stdout, "%s: Poll failed, errno=%d", proc_name, errno);
                if (EINTR == errno) {
                    /* Interrupted by signal */
                    fprintf(stdout, " (Poll interrupted by signal)\n");
                } else {
                    /* Other unspecified failure */
                    fprintf(stdout, "\n");
                }
                break;
            }
            if (pfd.revents & POLLHUP) {
                /* client closed their end of the pipe */
                fprintf(stdout, "%s: Client hung-up pipe\n", proc_name);
                break;
            }
            if (3 == fscanf(stream, "%[^:]:%[^:]:%d:", machine_name, output_name, &output_value)) {
                /* We successfully read an output command */
                ftime(&time_now);
                fprintf(stdout, "%s: T%ld.%03d Read output %s=%d for machine '%s'\n", proc_name, time_now.time, time_now.millitm, output_name, output_value, machine_name);
                if (!pOutputHandler && 0 == strcmp(OUTPUTS_INIT_NAME, output_name)) {
                    // Initialize output handler
#ifdef ROM_ABURNER2
                    if (0 == strcmp(machine_name, "aburner")) {
                        /* After Burner */
                        fprintf(stdout, "%s: Initializing new instance of AfterBurnerOutputHandler\n", proc_name);
                        pOutputHandler.reset(new AfterBurnerOutputHandler());
                        pOutputHandler->init();
                        continue;                
                    }
#endif
#ifdef ROM_TURBO
                    if (0 == strcmp(machine_name, "turbo")) {
                        fprintf(stdout, "%s: Initializing new instance of TurboOutputHandler\n", proc_name);
                        pOutputHandler.reset(new TurboOutputHandler());
                        pOutputHandler->init();
                        continue;                
                    }
#endif
#ifdef ROM_MONACOGP
                    if (0 == strcmp(machine_name, "monacogp")) {
                        fprintf(stdout, "%s: Initializing new instance of MonacoGpOutputHandler\n", proc_name);
                        pOutputHandler.reset(new MonacoGpOutputHandler());
                        pOutputHandler->init();
                        continue;                
                    } 
#endif
#ifdef ROM_CHASEHQ
                    if (0 == strcmp(machine_name, "chasehq")) {
                        fprintf(stdout, "%s: Initializing new instance of ChaseHqOutputHandler\n", proc_name);
                        pOutputHandler.reset(new ChaseHqOutputHandler());
                        pOutputHandler->init();
                        continue;                
                    }
#endif
                    fprintf(stdout, "%s: No output handler available for machine '%s'\n", proc_name, machine_name);
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
        if (pOutputHandler) {
            pOutputHandler->deinit();
        }

        /* Close the pipe ready to re-open for next client */
        fprintf(stdout, "%s: Closing stream\n", proc_name);
        fclose(stream);
        stream = NULL;
        fprintf(stdout, "%s: Closing pipe\n", proc_name);
        close(fd);
        fd = -1;
    }
    // Clean up on exit
    fprintf(stdout, "%s: Exiting main event loop, cleaning up\n", proc_name);
    if (NULL != stream) {
        fclose(stream);
    }
    if (fd >= 0) {
        close(fd); 
    }
}

int main(int /*argc*/, char **argv) {
    int err;
    proc_name = argv[0];

    /* Setup signal handlers */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    /* Make the FIFO pipe */
    fprintf(stdout, "%s: Creating FIFO pipe at %s\n", argv[0], OUTPUTS_PIPE_NAME);
    err = mkfifo(OUTPUTS_PIPE_NAME, 0666);
    if (err != 0 && EEXIST != errno) {
        fprintf(stderr, "%s: Unable to create FIFO pipe at %s errno=%d\n", argv[0], OUTPUTS_PIPE_NAME, errno);
        exit(errno);
    }

    /* Run the main event loop */
    main_event_loop();

    /* Cleanup FIFO */
    err = unlink(OUTPUTS_PIPE_NAME);
    if (err != 0) {
        fprintf(stderr, "%s: Unable to remove FIFO pipe at %s errno=%d\n", argv[0], OUTPUTS_PIPE_NAME, errno);
        exit(errno);
    }
}