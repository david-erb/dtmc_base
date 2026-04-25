#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtadc.h>

// some local macros referring to the current demo to make it a bit easier to read the code
#define demo_name "dtmc_base_demo_adc_print"
#define demo_t dtmc_base_demo_adc_print_t
#define demo_config_t dtmc_base_demo_adc_print_config_t
#define demo_describe dtmc_base_demo_adc_print_describe
#define demo_create dtmc_base_demo_adc_print_create
#define demo_configure dtmc_base_demo_adc_print_configure
#define demo_start dtmc_base_demo_adc_print_start
#define demo_entrypoint dtmc_base_demo_adc_print_entrypoint
#define demo_dispose dtmc_base_demo_adc_print_dispose

// forward declare the demo's privates
typedef struct demo_t demo_t;

// how the demo can be configured
typedef struct demo_config_t
{
    // caller provides a the platform-specific adc handle
    dtadc_handle adc_handle;

    // number of scans to print before stopping the demo
    int32_t max_scan_count;

    // timeout in milliseconds to wait for all scans before stopping the demo, or 0 to wait indefinitely
    uint32_t scan_timeout_ms;

} demo_config_t;

// create a new instance, allocating memory as needed
extern dterr_t*
demo_create(demo_t** self);

// configure the demo instance with handles to implementations and settings
extern dterr_t*
demo_configure(demo_t* self, demo_config_t* config);

// run the demo logic, typically returning leaving tasks running and callbacks registered
extern dterr_t*
demo_start(demo_t* self);

// stop, unregister and dispose of the demo instance resources
extern void
demo_dispose(demo_t* self);
