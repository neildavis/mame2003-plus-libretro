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
 After Burner Output Values
*/

#define AFTER_BURNER_LED_START 0
#define AFTER_BURNER_LAMP_LOCK_ON 0
#define AFTER_BURNER_LAMP_DANGER 1

/* Define GPIO pins for After Burner lamp outputs */
#define GPIO_LAMP_LOCK_ON	2	/* Lock-on lamp will be on GPIO/BCM pin 2 */
#define GPIO_LAMP_DANGER	3	/* Danger lamp will be on GPIO/BCM pin 3 */

#endif /* __OUTPUT_DEF_H__ */