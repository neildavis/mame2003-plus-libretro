#ifndef __CHASEHQ_H__
#define __CHASEHQ_H__

#include <memory>
#include <udd.h>
#include <tm1637.h>

#include "output_handler_base.h"

using namespace udd;
using namespace tm1637;

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
    void update_speed(int value);
    void update_siren(int value);
    void update_credits(int value);
    void update_start_button(int value);

private:
    int m_turboCount;    
    int m_speedKPH;
    int m_creditsCount;
    udd::DisplayST7735R m_display;
    std::shared_ptr<Device> m_pTM1637;
};

#endif // __CHASEHQ_H__