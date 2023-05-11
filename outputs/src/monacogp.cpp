#include "monacogp.h"
#include "utils.h"

using namespace tm1637;

/* Player score */
#define OUTPUTS_MONACOGP_SCORE_NAME "score"
#define OUTPUTS_MONACOGP_SCORE_MAX 9999
#define OUTPUTS_MONACOGP_SCORE_LED 1


MonacoGpOutputHandler::MonacoGpOutputHandler() :
    TurboOutputHandler() {
}

void MonacoGpOutputHandler::handle_output(const char *name, int value) {
    if (0 == strcmp(name, OUTPUTS_MONACOGP_SCORE_NAME)) {
        update_score(value, true);
        return;
    } else if (OUTPUTS_MONACOGP_SCORE_LED == parseLedOutputName(name)) {
        update_score(m_score, value > 0);
        return;
    }
    TurboOutputHandler::handle_output(name, value);
}
void MonacoGpOutputHandler::update_score(int value, bool visible) {
    if (value == m_score && visible == m_score_visible) {
        return;
    }
    m_score = value;
    m_score_visible = visible;
    if (m_score > OUTPUTS_MONACOGP_SCORE_MAX) {
        m_score = OUTPUTS_MONACOGP_SCORE_MAX;
    }
    if (visible) {
        m_pTM1637->showIntegerLiteral(m_score);
    } else {
        m_pTM1637->clear();
    }
}
