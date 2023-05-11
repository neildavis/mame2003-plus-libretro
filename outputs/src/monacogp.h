#ifndef ___MONACOGP_H__
#define ___MONACOGP_H__

#include <memory>
#include <tm1637.h>
#include <tm1637_sayer.h>
#include <udd.h>

#include "output_handler_base.h"
#include "turbo.h"

using namespace tm1637;

class MonacoGpOutputHandler : public TurboOutputHandler {
public:
    MonacoGpOutputHandler();
    // MOutputHandler overrides
    virtual void handle_output(const char *name, int value) override;
private:
    void update_score(int score, bool visible);
private:
    int m_score;
    bool m_score_visible;
};

#endif // ___MONACOGP_H__