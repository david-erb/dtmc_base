#pragma once

#include <dtcore/dterr.h>

// functions and types common to all benchmarks in this project

typedef dterr_t* (*benchmark_describe_fn)(void* self, char** out_description);

// return a string description of the benchmark (the returned string is heap allocated)
extern dterr_t*
dtmc_base_benchmark_helpers_decorate_description(void* self, benchmark_describe_fn describe_fn, char** out_description);
