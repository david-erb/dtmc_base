#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtiox.h>

// some local macros referring to the current demo to make it a bit easier to read the code
#define demo_name "dtmc_base_demo_iox"
#define demo_t dtmc_base_demo_iox_t
#define demo_config_t dtmc_base_demo_iox_config_t
#define demo_describe dtmc_base_demo_iox_describe
#define demo_create dtmc_base_demo_iox_create
#define demo_configure dtmc_base_demo_iox_configure
#define demo_start dtmc_base_demo_iox_start
#define demo_entrypoint dtmc_base_demo_iox_entrypoint
#define demo_dispose dtmc_base_demo_iox_dispose

// forward declare the demo's privates
typedef struct demo_t demo_t;

// how the demo can be configured
typedef struct demo_config_t
{
    dtiox_handle iox_handle;
    const char* node_name;
    bool inhibit_write;
    bool inhibit_read;
    bool should_read_before_first_write;
    int32_t max_attach_retries;
    int32_t max_cycles_to_run;
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
