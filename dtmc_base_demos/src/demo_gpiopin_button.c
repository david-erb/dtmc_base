#include <dtcore/dterr.h>

#include <dtcore/dterr.h>
#include <dtcore/dteventlogger.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtpicosdk_helper.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtdebouncer.h>
#include <dtmc_base/dtgpiopin.h>

#include <dtmc_base_demos/demo_gpiopin_button.h>

#define TAG "dtmc_base_demo_gpiopin_button"

// the demo's privates
typedef struct dtmc_base_demo_gpiopin_button_t
{
    // demo configuration
    dtmc_base_demo_gpiopin_button_config_t config;
    dtcpu_t cpu;                   // maintains irq-to-irq timing delta
    volatile bool raw_state;       // last irq raw state
    dteventlogger_t* event_logger; // event logger for interrupt events
    volatile int32_t event_count;  // separate total event count which can be more than debounced count
    bool should_process_callbacks; // set false during shutdown to ignore further interrupts
} dtmc_base_demo_gpiopin_button_t;

// ----------------------------------------------------------------------------
static void
raw_callback(dtgpiopin_edge_t edge, void* caller_context)
{
    dtmc_base_demo_gpiopin_button_t* self = (dtmc_base_demo_gpiopin_button_t*)caller_context;
    if (!self)
        return;
    if (!self->should_process_callbacks)
        return;

    // "rising" is switch released due to pull-up; "falling" is switch pressed
    self->raw_state = (edge & DTGPIOPIN_IRQ_FALLING) ? true : false;

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
dtmc_base_demo_gpiopin_button_create(dtmc_base_demo_gpiopin_button_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    *self = (dtmc_base_demo_gpiopin_button_t*)malloc(sizeof(dtmc_base_demo_gpiopin_button_t));
    if (*self == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM,
          DTERR_LOC,
          NULL,
          "failed to allocate %zu bytes for dtmc_base_demo_gpiopin_button_t",
          sizeof(dtmc_base_demo_gpiopin_button_t));
        goto cleanup;
    }

    memset(*self, 0, sizeof(dtmc_base_demo_gpiopin_button_t));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the demo instance with handles to implementations and settings
dterr_t*
dtmc_base_demo_gpiopin_button_configure( //
  dtmc_base_demo_gpiopin_button_t* self,
  dtmc_base_demo_gpiopin_button_config_t* config)
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
dtmc_base_demo_gpiopin_button_start(dtmc_base_demo_gpiopin_button_t* self)
{
    dterr_t* dterr = NULL;
    dtdebouncer_t _debouncer = { 0 }, *debouncer = &_debouncer;
    dteventlogger_t _event_logger = { 0 }, *event_logger = &_event_logger;

    DTERR_ASSERT_NOT_NULL(self);

    // set up the debouncer
    DTERR_C(dtdebouncer_init(debouncer,
      false,                     // initial_state
      20,                        // debounce_ms
      NULL,                      // callback function when new deboucnced state (not needed here)
      NULL,                      // callback context (not needed here)
      dtruntime_now_milliseconds // time function
      ));

    // create event logger for interrupt timing
    DTERR_C(dteventlogger_init(event_logger, 21, sizeof(dteventlogger_item1_t)));
    self->event_logger = event_logger;

    // attach interrupt callback to GPIO pin and enable interrupts
    DTERR_C(dtgpiopin_attach(self->config.gpiopin_handle, raw_callback, self))
    DTERR_C(dtgpiopin_enable(self->config.gpiopin_handle, true));
    self->should_process_callbacks = true;

    int want_release_count = 5;
    dtlog_info(TAG, "please press the button %d times...", want_release_count);

    dtcpu_mark(&self->cpu);
    bool last_debounced_state = false;
    while (self->event_count < event_logger->item_count + 1)
    {
        // feed the latest raw state into the debouncer
        dtdebouncer_update(debouncer, self->raw_state);
        bool this_debounced_state = debouncer->stable_state;
        if (last_debounced_state != this_debounced_state)
        {
            dtlog_info(TAG, "%d debounced state: %s", want_release_count, this_debounced_state ? "PRESSED" : "RELEASED");
            // we got a button release?
            if (this_debounced_state == false)
            {
                want_release_count--;
                if (want_release_count <= 0)
                {
                    dtlog_info(TAG, "button press demo complete");
                    break;
                }
            }
            last_debounced_state = this_debounced_state;
        }

        // sleep before polling again
        dtruntime_sleep_milliseconds(2);
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

        dtlog_info(TAG, "button press timing:\n%s", s);
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
dtmc_base_demo_gpiopin_button_dispose(dtmc_base_demo_gpiopin_button_t* self)
{
    if (self == NULL)
        return;

    memset(self, 0, sizeof(dtmc_base_demo_gpiopin_button_t));

    free(self);
}