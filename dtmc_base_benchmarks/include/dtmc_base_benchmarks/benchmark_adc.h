#pragma once

#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtadc.h>
#include <dtmc_base/dttasker.h>

#define benchmark_name       "dtmc_base_benchmark_adc"
#define benchmark_t          dtmc_base_benchmark_adc_t
#define benchmark_describe   dtmc_base_benchmark_adc_describe
#define benchmark_config_t   dtmc_base_benchmark_adc_config_t
#define benchmark_create     dtmc_base_benchmark_adc_create
#define benchmark_configure  dtmc_base_benchmark_adc_configure
#define benchmark_start      dtmc_base_benchmark_adc_start
#define benchmark_entrypoint dtmc_base_benchmark_adc_entrypoint
#define benchmark_dispose    dtmc_base_benchmark_adc_dispose

typedef struct benchmark_t benchmark_t;

typedef struct benchmark_config_t
{
    // pre-configured (but not yet activated) ADC whose scan delivery to measure
    dtadc_handle adc_handle;

    // nominal scan interval matching what the ADC was configured with
    int32_t nominal_scan_interval_us;

    // core on which to run the busywork load task
    int32_t app_core;

} benchmark_config_t;

extern dterr_t*
benchmark_describe(benchmark_t* self, char** out_description);

extern dterr_t*
benchmark_create(benchmark_t** self);

extern dterr_t*
benchmark_configure(benchmark_t* self, benchmark_config_t* config);

extern dterr_t*
benchmark_start(benchmark_t* self);

extern dterr_t*
benchmark_entrypoint(void* opaque_self, dttasker_handle tasker_handle);

extern void
benchmark_dispose(benchmark_t* self);
