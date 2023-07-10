#ifndef __TURBO_H__
#define __TURBO_H__

#include <memory>
#include <tm1637.h>
#include <tm1637_sayer.h>
#include <udd.h>

#include "output_handler_base.h"

using namespace tm1637;

class TurboOutputHandler : public MOutputHandler {
public:
    TurboOutputHandler();
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
    void update_lives(int value);
    void update_stage(int value);
    void update_tm1637();
    void reset_state();
    void draw_time_arc();
    void draw_cars_passed_arc();
protected:
    std::shared_ptr<Device> m_pTM1637;
private:
    std::unique_ptr<Sayer> m_pSayer;
    udd::DisplayST7789R m_display;
    udd::Image m_image;
    udd::Image m_logoImage;
    uint8_t m_tm1637_digits[4];
    int m_start_lights_last;
    int m_time;
    int m_time_max;
    int m_cars_passed;
    int m_last_yellow_flag;
    int m_lives;
    int m_stage;
    bool m_attract_mode_active;
    bool m_start_mode_active;
};

#endif // __TURBO_H__