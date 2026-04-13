/*
 * dtradioconfig -- WiFi and MQTT credential bundle with NV persistence.
 *
 * Holds the node name, WiFi SSID/password, and MQTT host, port, and
 * credentials required to connect an embedded node to a broker. The
 * structure is read from and written to a dtnvblob handle for persistence
 * across reboots, and exposes packx serialization for over-the-air delivery
 * or factory provisioning.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtpackable.h>

#include <dtcore/dtpackx.h>

#include <dtmc_base/dtnvblob.h>

#define DTRADIOCONFIG_SIGNATURE "dtradioconfig_t mark 1"

typedef struct dtradioconfig_t
{
    /* Data members */
    char* signature;
    char* self_node_name;
    char* wifi_ssid;
    char* wifi_password;

    char* mqtt_host;
    int32_t mqtt_port;
    int32_t mqtt_wsport;
    char* mqtt_user;
    char* mqtt_password;

} dtradioconfig_t;

extern dterr_t*
dtradioconfig_init(dtradioconfig_t* config);
extern dterr_t*
dtradioconfig_write_nvblob(dtradioconfig_t* self, dtnvblob_handle nvblob_handle);
extern dterr_t*
dtradioconfig_read_nvblob(dtradioconfig_t* self, dtnvblob_handle nvblob_handle);
extern void
dtradioconfig_to_string(dtradioconfig_t* self, char* buffer, size_t length);

extern dterr_t*
dtradioconfig_packx_length(dtradioconfig_t* self DTPACKABLE_PACKX_LENGTH_ARGS);
extern dterr_t*
dtradioconfig_packx(dtradioconfig_t* self DTPACKABLE_PACKX_ARGS);
extern dterr_t*
dtradioconfig_unpackx(dtradioconfig_t* self DTPACKABLE_UNPACKX_ARGS);
extern void
dtradioconfig_dispose(dtradioconfig_t* self);
