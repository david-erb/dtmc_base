#pragma once

#include <dtcore/dterr.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>

#include <dtmc_base/dttasker.h>

// some local macros referring to the current benchmark to make it a bit easier to read the code
#define benchmark_name "dtmc_base_benchmark_netportal_simplex"
#define benchmark_t dtmc_base_benchmark_netportal_simplex_t
#define benchmark_describe dtmc_base_benchmark_netportal_simplex_describe
#define benchmark_config_t dtmc_base_benchmark_netportal_simplex_config_t
#define benchmark_create dtmc_base_benchmark_netportal_simplex_create
#define benchmark_configure dtmc_base_benchmark_netportal_simplex_configure
#define benchmark_start dtmc_base_benchmark_netportal_simplex_start
#define benchmark_entrypoint dtmc_base_benchmark_netportal_simplex_entrypoint
#define benchmark_dispose dtmc_base_benchmark_netportal_simplex_dispose

// forward declare the benchmark's privates
typedef struct benchmark_t benchmark_t;

// how the benchmark can be configured
typedef struct benchmark_config_t
{
    dtnetportal_handle netportal_handle;

    bool is_server;
    int32_t app_core;

    int32_t message_count;
    int32_t payload_size;

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

// entrypoint for the main task that runs the benchmark logic
extern dterr_t*
benchmark_entrypoint(void* opaque_self, dttasker_handle tasker_handle);

// stop, unregister and dispose of the benchmark instance resources
extern void
benchmark_dispose(benchmark_t* self);
