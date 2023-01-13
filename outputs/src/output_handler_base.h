#pragma once

class MOutputHandler {
public:
    virtual ~MOutputHandler() = default;
    virtual void init() = 0;
    virtual void deinit() = 0;
    virtual void handle_output(const char *name, int value) = 0;
};
