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
#define OUTPUTS_BUF_FMT "%s:%s:%d:"

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

/*
 Turbo Output Values
*/

/* Number of credits */
#define OUTPUT_TURBO_CREDITS_NAME "credits" 
/* Attract Mode Gameplay active (does not include start/scoreboard screens)*/
#define OUTPUT_TURBO_ATTRACT_MODE_NAME "attract" 
/* Start Screen active */
#define OUTPUT_TURBO_START_SCREEN_NAME "strtscr" 
/* Race start lights: 0=none, 1-3=reds, 4=green */
#define OUTPUT_TURBO_RACE_START_LIGHTS_NAME "strtlght" 
/* Ambulance Yellow Flags: range=0-0xa: 0=none, odd=left, even=right */
#define OUTPUT_TURBO_RACE_YELLOW_FLAG_NAME "ylwflg" 
/* Time remaining (secs): 0-99 (0x63) */
#define OUTPUT_TURBO_TIME_NAME "time"
/* Cars passed: 0-41 (0x29) */
#define OUTPUT_TURBO_CARS_PASSED_NAME "passed"
/* Lives: 0-4 */
#define OUTPUT_TURBO_LIVES_NAME "lives"
/* Stage: 0+ (first stage=0) */
#define OUTPUT_TURBO_STAGE_NAME "stage"
/* Start Button LED */
#define TURBO_LED_START 0

/*
 Chase HQ output Values
*/

/* Chase Light/Siren animation frame 0-6 */
#define CHQ_CHASE_LAMP_STATE_NAME "siren"
/* Gear Low/High (0/1)*/
#define CHQ_GEAR_NAME "gear"
/* Revs 0-8000 */
#define CHQ_REVS_NAME "rpm"
#define CHQ_REVS_MAX 8000
/* Speed kph */
#define CHQ_SPEED_KPH_NAME "kph"
/* Time */
#define CHQ_TIME_NAME "time"
/* Turbo/Nitro count 0-5 */
#define CHQ_TURBO_COUNT_NAME "trb_c"
/* Active Turbo/Nitro duaration/spent 0-0xd2 (210) */
#define CHQ_TURBO_DURATION_NAME "trb_d"
/* Number of credits */
#define CHQ_CREDITS_NAME "credits" 

#endif /* __OUTPUT_DEF_H__ */
