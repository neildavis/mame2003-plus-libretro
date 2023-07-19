#ifndef __CHASEHQ_H__
#define __CHASEHQ_H__

#include <memory>
#include <udd.h>
#include "output_handler_base.h"
using namespace udd;


class ChaseHqOutputHandler : public MOutputHandler {
public:
    // MOutputHandler overrides
    virtual ~ChaseHqOutputHandler() override;
    virtual void init() override;
    virtual void deinit() override;
    virtual void handle_output(const char *name, int value) override;

private:
    void update_turbo_count(int value);
    void update_turbo_duration(int value);
    void update_revs(int value);
    void update_siren(int value);

private:
    int m_turboCount;
};

#endif // __CHASEHQ_H__