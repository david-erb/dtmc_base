/*
 * dtflowmonitor -- Data-flow rate monitor with heartbeat logging and timeout alerting.
 *
 * Tracks the arrival time and byte count of incoming data streams. A
 * configurable no-data timeout triggers a warning log when the stream goes
 * silent. Periodic heartbeat events record throughput (scan count and byte
 * count) since the last interval, providing a lightweight diagnostic trace
 * for serial or network data paths without external tooling.
 *
 * cdox v1.0.2
 */
#include <dtcore/dterr.h>

#include <dtmc_base/dtruntime.h>

typedef struct dtflowmonitor_t
{
    char title[32];

    dtruntime_milliseconds_t last_received_at;
    dtruntime_milliseconds_t notify_if_no_data_timeout_ms;
    bool has_logged_no_data_warning;

    dtruntime_milliseconds_t last_heartbeat_at;
    dtruntime_milliseconds_t heartbeat_interval_ms;
    int32_t scan_count_since_last_heartbeat;
    int32_t byte_count_since_last_heartbeat;
} dtflowmonitor_t;

dterr_t*
dtflowmonitor_init(dtflowmonitor_t* flow_monitor, const char* title);
dterr_t*
dtflowmonitor_update(dtflowmonitor_t* flow_monitor, int32_t ngot);