#pragma once

#include <dtcore/dterr.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>

// some local macros referring to the current demo to make it a bit easier to read the code
#define demo_t dtmc_base_demo_netportal_t
#define demo_config_t dtmc_base_demo_netportal_config_t
#define demo_describe dtmc_base_demo_netportal_describe
#define demo_create dtmc_base_demo_netportal_create
#define demo_configure dtmc_base_demo_netportal_configure
#define demo_start dtmc_base_demo_netportal_start
#define demo_dispose dtmc_base_demo_netportal_dispose

// forward declare the demo's privates
typedef struct dtmc_base_demo_netportal_t dtmc_base_demo_netportal_t;

// how the demo can be configured
typedef struct dtmc_base_demo_netportal_config_t
{
    dtnetportal_handle netportal_handle;

    bool is_server;

} dtmc_base_demo_netportal_config_t;

// return a string description of the demo (the returned string is heap allocated)
extern dterr_t*
dtmc_base_demo_netportal_describe(dtmc_base_demo_netportal_t* self, char** out_description);

// create a new instance, allocating memory as needed
extern dterr_t*
dtmc_base_demo_netportal_create(dtmc_base_demo_netportal_t** self);

// configure the demo instance with handles to implementations and settings
extern dterr_t*
dtmc_base_demo_netportal_configure(dtmc_base_demo_netportal_t* self, dtmc_base_demo_netportal_config_t* config);

// run the demo logic, typically returning leaving tasks running and callbacks registered
extern dterr_t*
dtmc_base_demo_netportal_start(dtmc_base_demo_netportal_t* self);

// stop, unregister and dispose of the demo instance resources
extern void
dtmc_base_demo_netportal_dispose(dtmc_base_demo_netportal_t* self);
