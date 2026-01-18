#ifndef __AFTERBURNER_H__
#define __AFTERBURNER_H__

#ifdef ROM_ABURNER2

#include <memory>
#include <udd.h>
#include "output_handler_base.h"
using namespace udd;


class AfterBurnerOutputHandler : public OutputHandlerBase {
public:
    // OutputHandlerBase overrides
    virtual ~AfterBurnerOutputHandler() override;
    virtual void init(OutputHandlerMode mode) override;
    virtual void deinit() override;
    virtual void handle_output(const char *name, int value) override;
    virtual void do_boot();
private:
    void show_splash_screen(const char *filename);
    void update_danger(int value);
    void update_lock(int value);
    void update_force_feedback(int value);
    void update_start_led(int value);
    void update_horizon_h(int value);
    void update_horizon_v(int value);
private:
    int     m_pi_handle;
    std::unique_ptr<Image> m_bmp_press_start;
    std::unique_ptr<Image> m_bmp_clear_press_start;
    std::unique_ptr<Image> m_bmp_lock;
    std::unique_ptr<Image> m_bmp_clear_lock;
    std::unique_ptr<DisplayST7789R> m_display;
};

#endif
#endif // __AFTERBURNER_H__