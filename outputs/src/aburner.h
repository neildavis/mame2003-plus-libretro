#ifndef __AFTERBURNER_H__
#define __AFTERBURNER_H__

#include <memory>
#include <udd.h>
#include "output_handler_base.h"
using namespace udd;


class AfterBurnerOutputHandler : public MOutputHandler {
public:
    // MOutputHandler overrides
    virtual ~AfterBurnerOutputHandler() override;
    virtual void init() override;
    virtual void deinit() override;
    virtual void handle_output(const char *name, int value) override;
private:
    void update_danger(int value);
    void update_lock(int value);
    void update_altitude_warning(int value);
    void update_start_led(int value);
private:
    std::unique_ptr<Image> m_bmp_press_start;
    std::unique_ptr<Image> m_bmp_clear_press_start;
    std::unique_ptr<Image> m_bmp_lock;
    std::unique_ptr<Image> m_bmp_clear_lock;
    std::unique_ptr<DisplayST7789R> m_display;
};


#endif // __AFTERBURNER_H__