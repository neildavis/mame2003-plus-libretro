#ifndef __SHANGON_H__
#define __SHANGON_H__

#ifdef ROM_SHANGON

#include "output_handler_base.h"

#include <memory>

// forward declaration
namespace tm1637 {
    class Device;
    class Sayer;
}

class SuperHangOnOutputHandler : public OutputHandlerBase {
public:
    SuperHangOnOutputHandler();
public:
    // OutputHandlerBase overrides
    virtual ~SuperHangOnOutputHandler() override;
    virtual void init(OutputHandlerMode mode) override;
    virtual void deinit() override;
    virtual void handle_output(const char *name, int value) override;

private:
    int shoSpeedToPulseWidth(int speed_kph);
    int shoSpeedToRpmPulseWidth(int speed_kph);
    void handle_speed_output(int value);
    void handle_start_button_output(int value);
    void handle_time_secs_output(int value);
    void handle_time_frames_output(int value);
    void handle_turbo_available_state();
    void handle_turbo_active_state();
    void handle_start_lights_state();
    void handle_game_start();
    void handle_game_over();
private:
    bool    m_game_in_progress;
    int     m_credits;
    int     m_stage;
    int     m_heartbeat;
    bool    m_turbo_available;
    bool    m_turbo_active;
    int     m_start_lights;
    int     m_time;
    //
    int     m_pi_handle;
    std::shared_ptr<tm1637::Device> m_pTM1637;
    std::unique_ptr<tm1637::Sayer> m_pSayer;

};

#endif // ROM_SHANGON
#endif // __SHANGON_H__