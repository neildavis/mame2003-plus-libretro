#include "monacogp.h"

using namespace tm1637;

/* Player score */
#define OUTPUTS_MONACOGP_SCORE_NAME "score"
#define OUTPUTS_MONACOGP_SCORE_MAX 9999


MonacoGpOutputHandler::MonacoGpOutputHandler() :
    TurboOutputHandler() {
}

void MonacoGpOutputHandler::handle_output(const char *name, int value) {
    if (0 == strcmp(name, OUTPUTS_MONACOGP_SCORE_NAME)) {
        update_score(value);
        return;
    }
    TurboOutputHandler::handle_output(name, value);
}
void MonacoGpOutputHandler::update_score(int value) {
    if (value == m_score) {
        return;
    }
    m_score = value;
    if (m_score > OUTPUTS_MONACOGP_SCORE_MAX) {
        m_score = OUTPUTS_MONACOGP_SCORE_MAX;
    }
    m_pTM1637->showIntegerLiteral(m_score);
}
