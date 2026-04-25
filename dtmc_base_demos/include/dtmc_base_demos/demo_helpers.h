#pragma once

#include <dtcore/dterr.h>

// functions and types common to all demos in this project

typedef dterr_t* (*demo_describe_fn)(void* self, char** out_description);

// return a string description of the demo (the returned string is heap allocated)
extern dterr_t*
dtmc_base_demo_helpers_decorate_description(void* self, demo_describe_fn describe_fn, char** out_description);
