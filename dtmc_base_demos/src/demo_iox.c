#include <dtcore/dterr.h>

#include <dtcore/dterr.h>
#include <dtcore/dteventlogger.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtpicosdk_helper.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtiox.h>

#include <dtmc_base_demos/demo_iox.h>

#define TAG demo_name

// the demo's privates
typedef struct demo_t
{
    demo_config_t config;

    int32_t cycle_number;

    bool got_first_read;
} demo_t;

// forward declare the cycle function
dterr_t*
demo__cycle(demo_t* self);

// --------------------------------------------------------------------------------------
// return a string description of the demo (the returned string is heap allocated)
dterr_t*
demo_describe(demo_t* self, char** out_description)
{
    dterr_t* dterr = NULL;
    char tmp[256];
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->config.iox_handle);
    DTERR_ASSERT_NOT_NULL(out_description);

    *out_description = NULL;

    char* d = NULL;
    char* s = "\n    ";
    d = dtstr_concat_format(d, s, "Description of the demo:");

    d = dtstr_concat_format(d, s, "The demo's declared node name is \"%s\".", self->config.node_name);

    if (self->config.inhibit_read)
    {
        d = dtstr_concat_format(d, s, "It writes every one second to the IOX device but doesn't read from it.");
    }
    else if (self->config.inhibit_write)
    {
        d = dtstr_concat_format(d, s, "It reads every one second from the IOX device but doesn't write to it.");
    }
    else
    {
        d = dtstr_concat_format(d, s, "It both reads and writes every one second to/from the IOX device.");
    }

    if (self->config.should_read_before_first_write)
    {
        d = dtstr_concat_format(d, s, "It will wait for a successful read before performing the first write.");
    }

    dtobject_to_string((dtobject_handle)self->config.iox_handle, tmp, sizeof(tmp));
    d = dtstr_concat_format(d, s, "The IOX object in use is %s", tmp);

    *out_description = d;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Create a new instance, allocating memory as needed
dterr_t*
demo_create(demo_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    *self = (demo_t*)malloc(sizeof(demo_t));
    if (*self == NULL)
    {
        dterr = dterr_new(
          DTERR_NOMEM, DTERR_LOC, NULL, "failed to allocate %zu bytes for demo_t", sizeof(demo_t));
        goto cleanup;
    }

    memset(*self, 0, sizeof(demo_t));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the demo instance with handles to implementations and settings
dterr_t*
demo_configure( //
  demo_t* self,
  demo_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    self->config = *config;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the demo logic (loops forever, blocking)
dterr_t*
demo_start(demo_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->config.iox_handle);

    {
        char* description = NULL;
        DTERR_C(demo_describe(self, &description));
        dtlog_info(TAG, "%s", description);
    }

    int32_t attach_retries = 0;
    int32_t cycle_count = 0;
    while (true)
    {
        if (self->config.max_attach_retries > 0 && attach_retries >= self->config.max_attach_retries)
        {
            dterr = dterr_new(DTERR_FAIL,
              DTERR_LOC,
              NULL,
              "exceeded max attach retries (%" PRId32 "), quitting demo",
              self->config.max_attach_retries);
            goto cleanup;
        }
        attach_retries++;

        // attach to IOX device hardware driver
        DTERR_C(dtiox_attach(self->config.iox_handle))

        // enable IRQ on the hardware, clear any prior state
        DTERR_C(dtiox_enable(self->config.iox_handle, true));

        while (true)
        {
            if (self->config.max_cycles_to_run > 0 && cycle_count >= self->config.max_cycles_to_run)
            {
                dtlog_info(TAG, "reached max cycles to run (%" PRId32 "), detaching", self->config.max_cycles_to_run);
                break;
            }
            cycle_count++;

            dterr = demo__cycle(self);

            // cycle had no error, do it again
            if (dterr == NULL)
                continue;

            dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "demo__cycle failed");
            dtlog_dterr(TAG, dterr);
            dterr_dispose(dterr);
            dterr = NULL;

            dtlog_info(TAG, "detaching from IOX device and retrying after 2 seconds...");

            // detach the driver and try to attach and run again
            DTERR_C(dtiox_detach(self->config.iox_handle));

            dtruntime_sleep_milliseconds(2000);

            break;
        }
        if (self->config.max_cycles_to_run > 0 && cycle_count >= self->config.max_cycles_to_run)
            break;
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// do a single tx/rx cycle
dterr_t*
demo__cycle(demo_t* self)
{
    dterr_t* dterr = NULL;

#define BUFSIZE (128)
    // every 10th iteration, log status
    if (self->cycle_number % 10 == 0)
    {
        char* s = NULL;
        DTERR_C(dtiox_concat_format(self->config.iox_handle, s, NULL, &s));
        dtlog_info(TAG, "%s %5" PRId32 ". %s", self->config.node_name, self->cycle_number, s);
        dtstr_dispose(s);
    }

    self->cycle_number++;

    char tx_buffer[BUFSIZE] = { 0 };
    char rx_buffer[BUFSIZE] = { 0 };
    char log_buffer[2 * BUFSIZE] = { 0 };

    if (!self->config.inhibit_write)
    {
        if (self->config.should_read_before_first_write && !self->got_first_read)
        {
            sprintf(tx_buffer, "[%s:------]", self->config.node_name);
        }
        else
        {
            // keep messages short so they fit in the fewest frames of MTU-limited transports
            sprintf(tx_buffer, "[%s:%06" PRId32 "]", self->config.node_name, self->cycle_number);
            int32_t written = 0;
            dterr = dtiox_write(self->config.iox_handle, (const uint8_t*)tx_buffer, strlen(tx_buffer) + 1, &written);

            if (dterr)
            {
                if (dterr->error_code == DTERR_BUSY)
                {
                    // not a real error, just log and continue
                    dtlog_info(TAG, "%s %5" PRId32 ". tx busy, will retry", self->config.node_name, self->cycle_number);
                    dterr_dispose(dterr);
                    dterr = NULL;
                    DTERR_C(dtiox_enable(self->config.iox_handle, true));
                }
                else
                    goto cleanup;
            }
            else
            {
                if (written != strlen(tx_buffer) + 1)
                {
                    dterr = dterr_new(
                      DTERR_FAIL, DTERR_LOC, NULL, "only wrote %" PRId32 " bytes from the string \"%s\"", written, tx_buffer);
                    goto cleanup;
                }
            }
        }
        strcat(log_buffer, "    tx=");
        strcat(log_buffer, tx_buffer);
    }

    dtruntime_sleep_milliseconds(500);

    if (!self->config.inhibit_read)
    {
        int32_t received = 0;
        rx_buffer[0] = '\0';
        DTERR_C(dtiox_read(self->config.iox_handle, (uint8_t*)rx_buffer, BUFSIZE, &received));
        strcat(log_buffer, "    rx=");
        if (received <= 0)
        {
            strcat(log_buffer, "<no data>");
        }
        else
        {
            strcat(log_buffer, rx_buffer);
            self->got_first_read = true;
        }
    }

    dtlog_info(TAG, "%s", log_buffer);

    dtruntime_sleep_milliseconds(500);

cleanup:

    return dterr;
}

// --------------------------------------------------------------------------------------
// Stop, unregister and dispose of the demo instance resources
void
demo_dispose(demo_t* self)
{
    if (self == NULL)
        return;

    free(self);
}