#ifndef __TURBO_H__
#define __TURBO_H__

#include <memory>
#include <tm1637.h>
#include <gpiod.hpp>

#include "output_handler_base.h"

using namespace tm1637;
using namespace gpiod;

class TurboOutputHandler : public MOutputHandler {
public:
    // MOutputHandler overrides
    virtual ~TurboOutputHandler() override;
    virtual void init() override;
    virtual void deinit() override;
    virtual void handle_output(const char *name, int value) override;
private:
    void update_time(int value);
    void update_cars_passed(int value);
    void update_start_lights(int value);
    void update_yellow_flag(int value);
    void update_start_button(int value);
    void update_attract_mode(int value);
    void update_start_mode(int value);
    void reset_state();
private:
    chip m_chip;
    line m_line_r3;
    line m_line_r2;
    line m_line_r1;
    line m_line_g;
    line m_line_y1;
    line m_line_y2;
    std::unique_ptr<Device> m_pTM1637;
    int m_tm1637_digits[4];
    int m_start_lights_last;
    bool m_attract_mode_active;
    bool m_start_mode_active;
};

#endif // __TURBO_H__