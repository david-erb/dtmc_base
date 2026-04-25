#include <string.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtflowmonitor.h>

#define TAG "dtflowmonitor"

// ---------------------------------------------------------------------------------------------
dterr_t*
dtflowmonitor_init(dtflowmonitor_t* flow_monitor, const char* title)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(flow_monitor);

    memset(flow_monitor, 0, sizeof(*flow_monitor));
    if (title != NULL)
    {
        snprintf(flow_monitor->title, sizeof(flow_monitor->title), "%s", title);
    }
    else
    {
        snprintf(flow_monitor->title, sizeof(flow_monitor->title), "flow");
    }

    flow_monitor->last_received_at = dtruntime_now_milliseconds();
    flow_monitor->notify_if_no_data_timeout_ms = 5000;
    flow_monitor->has_logged_no_data_warning = false;

    flow_monitor->last_heartbeat_at = dtruntime_now_milliseconds();
    flow_monitor->heartbeat_interval_ms = 10000;
    flow_monitor->byte_count_since_last_heartbeat = 0;

cleanup:
    return dterr;
}

// ---------------------------------------------------------------------------------------------
dterr_t*
dtflowmonitor_update(dtflowmonitor_t* flow_monitor, int32_t ngot)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(flow_monitor);
    dtruntime_milliseconds_t now = dtruntime_now_milliseconds();

    if (ngot > 0)
    {
        if (flow_monitor->has_logged_no_data_warning)
        {
            dtlog_info(TAG,
              "%s flowed again after %" DTRUNTIME_MILLISECONDS_PRI " ms",
              flow_monitor->title,
              now - flow_monitor->last_received_at);
        }

        flow_monitor->has_logged_no_data_warning = false;
        flow_monitor->last_received_at = now;

        flow_monitor->byte_count_since_last_heartbeat += ngot;

        if (now - flow_monitor->last_heartbeat_at > flow_monitor->heartbeat_interval_ms)
        {
            dtlog_info(TAG,
              "%s flowed %d bytes in the last %" DTRUNTIME_MILLISECONDS_PRI " ms",
              flow_monitor->title,
              flow_monitor->byte_count_since_last_heartbeat,
              now - flow_monitor->last_heartbeat_at);
            flow_monitor->last_heartbeat_at = now;
            flow_monitor->byte_count_since_last_heartbeat = 0;
        }
    }
    else
    {
        if (!flow_monitor->has_logged_no_data_warning)
        {
            if (now - flow_monitor->last_received_at > flow_monitor->notify_if_no_data_timeout_ms)
            {
                dtlog_info(TAG,
                  "%s hasn't flowed data for over %" DTRUNTIME_MILLISECONDS_PRI " ms",
                  flow_monitor->title,
                  flow_monitor->notify_if_no_data_timeout_ms);
                flow_monitor->has_logged_no_data_warning = true;
            }
        }
    }

cleanup:
    return dterr;
}
