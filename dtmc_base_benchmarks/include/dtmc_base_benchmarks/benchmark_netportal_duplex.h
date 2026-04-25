#pragma once

#include <dtcore/dterr.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>

// some local macros referring to the current benchmark to make it a bit easier to read the code
#define benchmark_t dtmc_base_benchmark_netportal_duplex_t
#define benchmark_describe dtmc_base_benchmark_netportal_duplex_describe
#define benchmark_config_t dtmc_base_benchmark_netportal_duplex_config_t
#define benchmark_create dtmc_base_benchmark_netportal_duplex_create
#define benchmark_configure dtmc_base_benchmark_netportal_duplex_configure
#define benchmark_start dtmc_base_benchmark_netportal_duplex_start
#define benchmark_dispose dtmc_base_benchmark_netportal_duplex_dispose

// forward declare the benchmark's privates
typedef struct benchmark_t benchmark_t;

// how the benchmark can be configured
typedef struct benchmark_config_t
{
    dtnetportal_handle netportal_handle;

    bool inhibit_write_until_first_read;

    int32_t eventlogger_max_items;

} benchmark_config_t;

// return a string description of the benchmark (the returned string is heap allocated)
extern dterr_t*
benchmark_describe(benchmark_t* self, char** out_description);

// create a new instance, allocating memory as needed
extern dterr_t*
benchmark_create(benchmark_t** self);

// configure the benchmark instance with handles to implementations and settings
extern dterr_t*
benchmark_configure(benchmark_t* self, benchmark_config_t* config);

// run the benchmark logic, typically returning leaving tasks running and callbacks registered
extern dterr_t*
benchmark_start(benchmark_t* self);

// stop, unregister and dispose of the benchmark instance resources
extern void
benchmark_dispose(benchmark_t* self);
