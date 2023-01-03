#ifndef __OUTPUT_H__
#define __OUTPUT_H__

// Process name used for logging
extern const char *proc_name;

int get_resource_path(char *path, int bufSize);
int parseLampOutputName(const char *output_name);
int parseLedOutputName(const char *output_name);

#endif // __OUTPUT_H__