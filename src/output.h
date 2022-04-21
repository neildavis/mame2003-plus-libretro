#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include "driver.h"

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* core initialization */
void output_init(const char *machine_name);

/* core cleanup */
void output_stop(void);

/* set the value for a given output */
void output_set_value(const char *outname, INT32 value);

/* set an indexed value for an output (concatenates basename + index) */
void output_set_indexed_value(const char *basename, int index, int value);


/***************************************************************************
    INLINES
***************************************************************************/

INLINE void output_set_led_value(int index, int value)
{
	output_set_indexed_value("led", index, value ? 1 : 0);
}

INLINE void output_set_lamp_value(int index, int value)
{
	output_set_indexed_value("lamp", index, value);
}

INLINE void output_set_digit_value(int index, int value)
{
	output_set_indexed_value("digit", index, value);
}

#endif	/* __OUTPUT_H__ */
