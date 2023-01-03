
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

const char *proc_name = NULL;

int get_resource_path(char *path, int bufSize) {
    const char *homedir;

    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
   
    return snprintf(path, bufSize, "%s/.local/lrmame2003osvr", homedir);
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
