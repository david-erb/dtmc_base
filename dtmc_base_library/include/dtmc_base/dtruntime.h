/*
 * dtruntime -- Runtime environment inspection, timing, and device reporting.
 *
 * Provides the library flavor and version strings, millisecond wall-clock
 * access and sleep, QEMU detection, and formatted table output for both the
 * software environment and the registered hardware devices. Task registration
 * with a dttasker_registry allows the runtime to enumerate and report all
 * active tasks in a single diagnostic call.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtmc_base/dttasker_registry.h>

const char*
dtruntime_flavor(void);

const char*
dtruntime_version(void);

extern bool
dtruntime_is_qemu();

extern dterr_t*
dtruntime_format_environment_as_table(char** out_string);
extern void
dtruntime_log_environment(const char* tag, dtlog_level_t log_level, const char* prefix);

extern dterr_t*
dtruntime_format_devices_as_table(char** out_string);
extern void
dtruntime_log_devices(const char* tag, dtlog_level_t log_level, const char* prefix);

extern dterr_t*
dtruntime_register_tasks(dttasker_registry_t* registry);

typedef uint32_t dtruntime_milliseconds_t;
#define DTRUNTIME_MILLISECONDS_PRI PRIu32

extern dtruntime_milliseconds_t
dtruntime_now_milliseconds();

extern void
dtruntime_sleep_milliseconds(dtruntime_milliseconds_t milliseconds);
