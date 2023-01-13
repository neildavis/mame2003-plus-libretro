/***************************************************************************

    output.c

    General purpose output routines.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "output.h"
#include "output-def.h"

/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    output_init - initialize everything
-------------------------------------------------*/

char outputs_machine_name[OUTPUTS_PIPE_MAX_MACHINE_NAME_SIZE];
int fd_outputs = -1;
FILE *stream_outputs = NULL;

void output_init(const char *machine_name) {
	if (outputs_machine_name != machine_name && strlen(machine_name) > 0) {
		strcpy(outputs_machine_name, machine_name);	
	}  
	if (strlen(outputs_machine_name) == 0) {
		strcpy(outputs_machine_name, "unknown");	
	}

	if (fd_outputs < 0) {
		fd_outputs = open(OUTPUTS_PIPE_NAME, O_WRONLY | O_NONBLOCK);
		if (fd_outputs > -1) {
			// Successfully opened pipe. Now get a FILE stream
			stream_outputs = fdopen(fd_outputs, "w");
			if (NULL == stream_outputs) {
				close(fd_outputs);
				fd_outputs = -1;
			} else {
				//Send a special 'hello' output to allow server to initialize for specific ROM
				output_set_value(OUTPUTS_INIT_NAME, 0);
			}
		}
	}
}

/*-------------------------------------------------
    output_stop - cleanup on exit
-------------------------------------------------*/

void output_stop() {
	if (stream_outputs) {
		output_set_value(OUTPUTS_STOP_NAME, 0);
		fclose(stream_outputs);
		stream_outputs = NULL;
	}
	if (fd_outputs > -1) {
		close(fd_outputs);
		fd_outputs = -1;
	}
	outputs_machine_name[0] = 0;
}


/*-------------------------------------------------
    output_set_value - set the value of an output
-------------------------------------------------*/

void output_set_value(const char *outname, INT32 value) {
	if (!stream_outputs && strlen(outputs_machine_name) > 0) {
		/* outputs were initialized, but the pipe failed to open. Try again */
		output_init(outputs_machine_name);
	}
	if (stream_outputs) {
		fprintf(stream_outputs, OUTPUTS_BUF_FMT, outputs_machine_name, outname, value);
		fflush(stream_outputs);
	}
}

/*-------------------------------------------------
    output_set_indexed_value - set the value of an
    indexed output
-------------------------------------------------*/

void output_set_indexed_value(const char *basename, int index, int value) {
	char buffer[100];
	char *dest = buffer;

	/* copy the string */
	while (*basename != 0)
		*dest++ = *basename++;

	/* append the index */
	if (index >= 1000) *dest++ = '0' + ((index / 1000) % 10);
	if (index >= 100) *dest++ = '0' + ((index / 100) % 10);
	if (index >= 10) *dest++ = '0' + ((index / 10) % 10);
	*dest++ = '0' + (index % 10);
	*dest++ = 0;	// null terminator

	/* set the value */
	output_set_value(buffer, value);
}








