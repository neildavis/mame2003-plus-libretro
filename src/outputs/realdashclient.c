/*
  realdashclient.c

  Created by Neil Davis on 17/03/2019.
  See license.txt for more details.
*/

#include "realdashclient.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>

/* fwd delcs */
void dbusMethodCallSync(const char *methodName);
void dbusMethodCallIgnoreReturn(const char *methodName, int type, const void *value);

/*
 Constants:
*/
const uint16_t RD_MAX_REVS_RPM = 9000;

/* Client DBus config */
#define DBUS_SERVICE_NAME "nd.mame2010.main"
/* RealDashCanServerQt DBus config */
#define CAN_SERVER_SERVICE_NAME "nd.realdash.canserver"
#define CAN_SERVER_INTERFACE    "nd.realdash.canserver.DashBoard"
#define CAN_SERVER_OBJECT_PATH  "/nd/realdash/canserver"

/* DBus connection */
struct DBusConnection *m_conn;
/* Internal cached data values */
uint16_t m_revsRpm;
uint16_t m_speedMph;
uint16_t m_fuelPercent;
uint16_t m_gear;
uint8_t m_initialized;

/** c'tor */
void RealDashCanClientInit() {
    m_initialized = 0;
#ifdef REALDASH
    DBusError err;
    int ret;
    /* initialise the errors */
    dbus_error_init(&err);
    
    /* connect to the bus */
    m_conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == m_conn) {
        exit(1);
    }
    /* request a name on the bus */
    ret = dbus_bus_request_name(m_conn, DBUS_SERVICE_NAME,
                                DBUS_NAME_FLAG_REPLACE_EXISTING
                                , &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        exit(1);
    }
    m_initialized = 1;
#endif
}

/** d'tor */
void RealDashCanClientDeinit() {
#ifdef REALDASH
    dbus_connection_close(m_conn);
    m_conn = NULL;
#endif
    m_initialized = 0;
}

/** Init state getter */
uint8_t RealDashCanClientIsInitialized() {
    return m_initialized;
}

/** Start CAN server */
void RealDashCanClientStartServer() {
#ifdef REALDASH
    dbusMethodCallSync("startServer");
#endif
}

/** Stop CAN server */
void RealDashCanClientStopServer() {
#ifdef REALDASH
    dbusMethodCallSync("stopServer");
#endif
}

/** Update Rev Counter RPM */
void RealDashCanClientUpdateRevs(uint16_t revsRpm) {
#ifdef REALDASH
    if (revsRpm != m_revsRpm) {
        m_revsRpm = revsRpm;
        dbusMethodCallIgnoreReturn("setRevs", DBUS_TYPE_UINT16, &m_revsRpm);
    }
#endif
}

/** Update Speed MPH */
void RealDashCanClientUpdateSpeed(uint16_t speedMph) {
#ifdef REALDASH
    if (m_speedMph != speedMph) {
        m_speedMph = speedMph;
        dbusMethodCallIgnoreReturn("setSpeed", DBUS_TYPE_UINT16, &m_speedMph);
    }
#endif
}

/** Update Fuel Level % */
void RealDashCanClientUpdateFuel(uint16_t fuelPercent) {
#ifdef REALDASH
    if (m_fuelPercent != fuelPercent) {
        m_fuelPercent = fuelPercent;
        dbusMethodCallIgnoreReturn("setFuelLevel", DBUS_TYPE_UINT16, &m_fuelPercent);
    }
#endif
}

/** Update Gear */
void RealDashCanClientUpdateGear(uint16_t gear) {
#ifdef REALDASH
    if (m_gear != gear) {
        m_gear = gear;
        dbusMethodCallIgnoreReturn("setGear", DBUS_TYPE_UINT16, &m_gear);
    }
#endif
}

/** Reset everything to default/zero states */
void RealDashCanClientResetDefaults()
{
#ifdef REALDASH
    RealDashCanClientUpdateRevs(0);
    RealDashCanClientUpdateSpeed(0);
    RealDashCanClientUpdateFuel(0);
    RealDashCanClientUpdateGear(0);
#endif
}

/*
 Private methods
*/

void dbusMethodCallSync(const char *methodName) {
    DBusMessage* msg = dbus_message_new_method_call(CAN_SERVER_SERVICE_NAME,
                                       CAN_SERVER_OBJECT_PATH,
                                       CAN_SERVER_INTERFACE,
                                       methodName);
    if (NULL == msg) {
        fprintf(stderr, "Message Null\n");
        exit(1);
    }
    
    /* send message and get a handle for a reply */
    DBusPendingCall* pending = NULL;
    if (!dbus_connection_send_with_reply (m_conn, msg, &pending, -1)) {
        fprintf(stderr, "Out Of Memory!\n");
        /* free message */
        dbus_message_unref(msg);
        return;
    }
    if (NULL == pending) {
        fprintf(stderr, "Pending Call Null\n");
         /* free message */
        dbus_message_unref(msg);
        return;
    }
    dbus_connection_flush(m_conn);
    
    /* free message */
    dbus_message_unref(msg);
    /* block until we receive a reply */
    dbus_pending_call_block(pending);
    /*  free the pending message handle */
    dbus_pending_call_unref(pending);
}

void dbusMethodCallIgnoreReturn(const char *methodName, int type, const void *value) {
    DBusMessage* msg;
    DBusMessageIter args;
    
    msg = dbus_message_new_method_call(CAN_SERVER_SERVICE_NAME,
                                       CAN_SERVER_OBJECT_PATH,
                                       CAN_SERVER_INTERFACE,
                                       methodName);
    if (NULL == msg) {
        fprintf(stderr, "Message Null\n");
        return;
    }
    
    /* append arguments */
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, type, value)) {
        fprintf(stderr, "Out Of Memory!\n");
        /* free message */
        dbus_message_unref(msg);
        return;
    }
    
    /* send message (don't care about reply) */
    if (!dbus_connection_send (m_conn, msg, NULL)) {
        fprintf(stderr, "Out Of Memory!\n");
        /* free message */
        dbus_message_unref(msg);
        return;
    }
    dbus_connection_flush(m_conn);
    
    /* free message */
    dbus_message_unref(msg);
}
