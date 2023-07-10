#include <string.h>
#include <stdio.h>

#include <wiringPi.h>

#include "chasehq.h"
#include "utils.h"
#include "output-def.h"

using namespace udd;

ChaseHqOutputHandler::~ChaseHqOutputHandler() {
}

void ChaseHqOutputHandler::init() {
}

void ChaseHqOutputHandler::deinit() {
}

void ChaseHqOutputHandler::handle_output(const char *name, int value) {
}
