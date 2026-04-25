#pragma once

#include <dtcore/dterr.h>
#include <dtcore/dtkvp.h>

#include <dtmc_base/dtnvblob.h>

// some local macros referring to the current demo to make it a bit easier to read the code
#define demo_t dtmc_base_demo_write_kvp_t
#define demo_config_t dtmc_base_demo_write_kvp_config_t
#define demo_create dtmc_base_demo_write_kvp_create
#define demo_configure dtmc_base_demo_write_kvp_configure
#define demo_start dtmc_base_demo_write_kvp_start
#define demo_dispose dtmc_base_demo_write_kvp_dispose

// forward declare the demo's privates
typedef struct dtmc_base_demo_write_kvp_t dtmc_base_demo_write_kvp_t;

// how the demo can be configured
typedef struct dtmc_base_demo_write_kvp_config_t
{
    dtkvp_list_t* kvp_list;
    dtnvblob_handle nvblob_handle;
} dtmc_base_demo_write_kvp_config_t;

// create a new instance, allocating memory as needed
extern dterr_t*
dtmc_base_demo_write_kvp_create(dtmc_base_demo_write_kvp_t** self);

// configure the demo instance with handles to implementations and settings
extern dterr_t*
dtmc_base_demo_write_kvp_configure(dtmc_base_demo_write_kvp_t* self, dtmc_base_demo_write_kvp_config_t* config);

// run the demo logic, typically returning leaving tasks running and callbacks registered
extern dterr_t*
dtmc_base_demo_write_kvp_start(dtmc_base_demo_write_kvp_t* self);

// stop, unregister and dispose of the demo instance resources
extern void
dtmc_base_demo_write_kvp_dispose(dtmc_base_demo_write_kvp_t* self);
