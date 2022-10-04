//
//  realdashclient.h
//
//  Created by Neil Davis on 17/03/2019.
//  See license.txt for more details.
//

#ifndef realdashclient_h
#define realdashclient_h

#include <stdint.h>

//
// Constants
//
/** Maximum revs */
extern const uint16_t RD_MAX_REVS_RPM;

/** c'tor */
void RealDashCanClientInit();

/** d'tor */
void RealDashCanClientDeinit();

/** Init state getter */
uint8_t RealDashCanClientIsInitialized();

/** Start CAN server */
void RealDashCanClientStartServer();

/** Stop CAN server */
void RealDashCanClientStopServer();

/** Update Rev Counter RPM */
void RealDashCanClientUpdateRevs(uint16_t revsRpm);

/** Update Speed MPH */
void RealDashCanClientUpdateSpeed(uint16_t speedMph);

/** Update Fuel Level % */
void RealDashCanClientUpdateFuel(uint16_t fuelPercent);

/** Update Gear */
void RealDashCanClientUpdateGear(uint16_t gear);

/** Reset everything to default/zero states */
void RealDashCanClientResetDefaults();

#endif /* realdashclient_h */
