#pragma once

/* OutputHandlerMode determines the mode an output handler is being initialized for */
typedef enum {
    /* Boot sequence if a 'Boot ROM' is requested */
    OutputHandlerModeBoot    = 0,

    /* Normal game mode */
    OutputHandlerModeGame
} OutputHandlerMode;

class OutputHandlerBase {
public:
    /* Virtual d'tor*/
    virtual ~OutputHandlerBase() = default;

    /* Initialize the output handler */
    virtual void init(OutputHandlerMode /*mode*/) {} ;

    /* De-initialse the output handler (cleanup resources prior to init() or destruction) */
    virtual void deinit() {} ;

    /* Handle an output from a running game */
    virtual void handle_output(const char* /*name*/, int /*value*/) {};

    /* Boot sequence if a 'Boot ROM' was passed on command line (See also BOOT_ROM in Makefile) */
    virtual void do_boot() {};
};
