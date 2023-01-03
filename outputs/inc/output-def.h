#ifndef __OUTPUT_DEF_H__
#define __OUTPUT_DEF_H__

/* The path of the named pipe used for output comms */
#define OUTPUTS_PIPE_NAME "/tmp/lr-mame2003plus-outputs"

/* The max size of a machine name */
#define OUTPUTS_PIPE_MAX_MACHINE_NAME_SIZE 32

/* The max size of an outpu name */
#define OUTPUTS_PIPE_MAX_OUTPUT_NAME_SIZE 32

/* The max size of a command buffer sent to the pipe */
#define OUTPUTS_PIPE_MAX_BUF_SIZE 256

/*
 The format specifier for output strings
 1. Machine name, e.g. "aburner"
 2. The output name e.g. "lamp1"
 3. The output value, e.g. 1
*/
#define OUTPUTS_BUF_FMT "%s %s %d"

/*
 Non-ROM-specific output values
*/

/* When outputs are initilazed, this output is sent immediatley 
   to allow server to initialize for specific ROM*/
#define OUTPUTS_INIT_NAME "hello"

/* When outputs are stopped, this output is sent 
   to allow server to clear up for specific ROM*/
#define OUTPUTS_STOP_NAME "goodbye"

/*
 After Burner Output Values
*/

#define AFTER_BURNER_LED_START 0
#define AFTER_BURNER_LAMP_LOCK_ON 0
#define AFTER_BURNER_LAMP_DANGER 1
#define AFTER_BURNER_LAMP_ALTITUDE_WARNING 2

#endif /* __OUTPUT_DEF_H__ */