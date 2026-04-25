#include <dtcore/dterr.h>

#include <dtcore/dterr.h>
#include <dtcore/dteventlogger.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtpicosdk_helper.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtgpiopin.h>

#include <dtmc_base_demos/demo_gpiopin_record.h>

#define TAG "dtmc_base_demo_gpiopin_record"

// the demo's privates
typedef struct dtmc_base_demo_gpiopin_record_t
{
    // demo configuration
    dtmc_base_demo_gpiopin_record_config_t config;
    dtcpu_t cpu;                   // maintains irq-to-irq timing delta
    dteventlogger_t* event_logger; // event logger for interrupt events
    volatile int32_t event_count;  // separate total event count
    bool should_process_callbacks; // set false during shutdown to ignore further interrupts
} dtmc_base_demo_gpiopin_record_t;

// ----------------------------------------------------------------------------
static void
raw_callback(dtgpiopin_edge_t edge, void* caller_context)
{
    dtmc_base_demo_gpiopin_record_t* self = (dtmc_base_demo_gpiopin_record_t*)caller_context;
    if (!self)
        return;
    if (!self->should_process_callbacks)
        return;

    // add an event log entry
    dteventlogger_item1_t item = { 0 };
    item.timestamp = dtruntime_now_milliseconds();
    item.value1 = dtcpu_elapsed_microseconds(&self->cpu);
    dtcpu_mark(&self->cpu);
    if (edge & DTGPIOPIN_IRQ_RISING)
    {
        item.value2 = 1;
    }
    if (edge & DTGPIOPIN_IRQ_FALLING)
    {
        item.value2 = 0;
    }
    dteventlogger_append(self->event_logger, &item);
    self->event_count++;
}

// --------------------------------------------------------------------------------------
// Create a new instance, allocating memory as needed
dterr_t*
dtmc_base_demo_gpiopin_record_create(dtmc_base_demo_gpiopin_record_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    *self = (dtmc_base_demo_gpiopin_record_t*)malloc(sizeof(dtmc_base_demo_gpiopin_record_t));
    if (*self == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM,
          DTERR_LOC,
          NULL,
          "failed to allocate %zu bytes for dtmc_base_demo_gpiopin_record_t",
          sizeof(dtmc_base_demo_gpiopin_record_t));
        goto cleanup;
    }

    memset(*self, 0, sizeof(dtmc_base_demo_gpiopin_record_t));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the demo instance with handles to implementations and settings
dterr_t*
dtmc_base_demo_gpiopin_record_configure( //
  dtmc_base_demo_gpiopin_record_t* self,
  dtmc_base_demo_gpiopin_record_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    self->config = *config;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the demo logic, typically returning leaving tasks running and callbacks registered
dterr_t*
dtmc_base_demo_gpiopin_record_start(dtmc_base_demo_gpiopin_record_t* self)
{
    dterr_t* dterr = NULL;
    dteventlogger_t _event_logger = { 0 }, *event_logger = &_event_logger;

    DTERR_ASSERT_NOT_NULL(self);

    // create event logger for interrupt timing
    DTERR_C(dteventlogger_init(event_logger, 21, sizeof(dteventlogger_item1_t)));
    self->event_logger = event_logger;

    // attach interrupt callback to GPIO pin and enable interrupts
    DTERR_C(dtgpiopin_attach(self->config.gpiopin_handle, raw_callback, self))
    DTERR_C(dtgpiopin_enable(self->config.gpiopin_handle, true));
    self->should_process_callbacks = true;

    dtlog_info(TAG, "monitoring %s for %" PRIu32 " edge events...", self->config.pin_name, event_logger->item_count);

    dtcpu_mark(&self->cpu);
    while (self->event_count < event_logger->item_count + 1)
    {
        // sleep before polling again
        dtruntime_sleep_milliseconds(100);
    }

    // disable interrupts
    self->should_process_callbacks = false;
    DTERR_C(dtgpiopin_enable(self->config.gpiopin_handle, false));

    {
        char* s = NULL;
        DTERR_C(dteventlogger_printf_item1( //
          event_logger,
          "interrupt timing summary",
          "micros",
          "rise/fall",
          &s));

        dtlog_info(TAG, "record press timing:\n%s", s);
        dtstr_dispose(s);
    }

cleanup:
    // stop doing callback logic if some leak in from the ISR happens late
    self->should_process_callbacks = false;

    // now safe to dispose of the event logger since no more callbacks will occur
    dteventlogger_dispose(event_logger);

    return dterr;
}

// --------------------------------------------------------------------------------------
// Stop, unregister and dispose of the demo instance resources
void
dtmc_base_demo_gpiopin_record_dispose(dtmc_base_demo_gpiopin_record_t* self)
{
    if (self == NULL)
        return;

    memset(self, 0, sizeof(dtmc_base_demo_gpiopin_record_t));

    free(self);
}