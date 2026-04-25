#pragma once

#include <dtcore/dterr.h>

#include <dtmc_base/dtgpiopin.h>

// some local macros referring to the current demo to make it a bit easier to read the code
#define demo_name "dtmc_base_demo_gpiopin_record"
#define demo_title "record GPIO pin edge changes"
#define demo_t dtmc_base_demo_gpiopin_record_t
#define demo_config_t dtmc_base_demo_gpiopin_record_config_t
#define demo_create dtmc_base_demo_gpiopin_record_create
#define demo_configure dtmc_base_demo_gpiopin_record_configure
#define demo_start dtmc_base_demo_gpiopin_record_start
#define demo_dispose dtmc_base_demo_gpiopin_record_dispose

// forward declare the demo's privates
typedef struct dtmc_base_demo_gpiopin_record_t dtmc_base_demo_gpiopin_record_t;

// how the demo can be configured
typedef struct dtmc_base_demo_gpiopin_record_config_t
{
    dtgpiopin_handle gpiopin_handle; // pin to monitor
    const char* pin_name;            // friendly name for logging
} dtmc_base_demo_gpiopin_record_config_t;

// create a new instance, allocating memory as needed
extern dterr_t*
dtmc_base_demo_gpiopin_record_create(dtmc_base_demo_gpiopin_record_t** self);

// configure the demo instance with handles to implementations and settings
extern dterr_t*
dtmc_base_demo_gpiopin_record_configure(dtmc_base_demo_gpiopin_record_t* self, dtmc_base_demo_gpiopin_record_config_t* config);

// run the demo logic, typically returning leaving tasks running and callbacks registered
extern dterr_t*
dtmc_base_demo_gpiopin_record_start(dtmc_base_demo_gpiopin_record_t* self);

// stop, unregister and dispose of the demo instance resources
extern void
dtmc_base_demo_gpiopin_record_dispose(dtmc_base_demo_gpiopin_record_t* self);
