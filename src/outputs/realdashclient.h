/*
  realdashclient.h

  Created by Neil Davis on 17/03/2019.
  See license.txt for more details.
*/

#ifndef realdashclient_h
#define realdashclient_h

#include <stdint.h>

/*
 Constants:
*/

/** Maximum revs */
extern const uint16_t RD_MAX_REVS_RPM;

/** c'tor */
void RealDashCanClientInit(void);

/** d'tor */
void RealDashCanClientDeinit(void);

/** Init state getter */
uint8_t RealDashCanClientIsInitialized(void);

/** Start CAN server */
void RealDashCanClientStartServer(void);

/** Stop CAN server */
void RealDashCanClientStopServer(void);

/** Update Rev Counter RPM */
void RealDashCanClientUpdateRevs(uint16_t revsRpm);

/** Update Speed MPH */
void RealDashCanClientUpdateSpeed(uint16_t speedMph);

/** Update Fuel Level % */
void RealDashCanClientUpdateFuel(uint16_t fuelPercent);

/** Update Gear */
void RealDashCanClientUpdateGear(uint16_t gear);

/** Reset everything to default/zero states */
void RealDashCanClientResetDefaults(void);

#endif /* realdashclient_h */
