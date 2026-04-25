#pragma once

#include <dtcore/dterr.h>

// some local macros referring to the current demo to make it a bit easier to read the code
#define demo_name "dtmc_base_demo_runtime_info"
#define demo_t dtmc_base_demo_runtime_info_t
#define demo_config_t dtmc_base_demo_runtime_info_config_t
#define demo_describe dtmc_base_demo_runtime_info_describe
#define demo_create dtmc_base_demo_runtime_info_create
#define demo_configure dtmc_base_demo_runtime_info_configure
#define demo_start dtmc_base_demo_runtime_info_start
#define demo_dispose dtmc_base_demo_runtime_info_dispose

// forward declare the demo's privates
typedef struct dtmc_base_demo_runtime_info_t dtmc_base_demo_runtime_info_t;

// how the demo can be configured
typedef struct dtmc_base_demo_runtime_info_config_t
{
    int32_t placeholder;
} dtmc_base_demo_runtime_info_config_t;

// create a new instance, allocating memory as needed
extern dterr_t*
dtmc_base_demo_runtime_info_create(dtmc_base_demo_runtime_info_t** self);

// configure the demo instance with handles to implementations and settings
extern dterr_t*
dtmc_base_demo_runtime_info_configure(dtmc_base_demo_runtime_info_t* self, dtmc_base_demo_runtime_info_config_t* config);

// run the demo logic, typically returning leaving tasks running and callbacks registered
extern dterr_t*
dtmc_base_demo_runtime_info_start(dtmc_base_demo_runtime_info_t* self);

// stop, unregister and dispose of the demo instance resources
extern void
dtmc_base_demo_runtime_info_dispose(dtmc_base_demo_runtime_info_t* self);
