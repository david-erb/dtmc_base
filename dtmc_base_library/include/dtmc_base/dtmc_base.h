/*
 * dtmc_base -- Library flavor and other library identification macros.
 *
 * Provides DTMC_BASE_FLAVOR string constant that
 * identifies the library build at runtime.  Also includes the appropriate version file. Used by dtruntime and diagnostic
 * routines to report the active library variant in environment tables and
 * log output.
 *
 * This file is processed by tooling in the automated build system.
 * It's important to maintain the structure and formatting of the version macros for compatibility with version parsing scripts.
 *
 * cdox v1.0.2.1
 */

#pragma once

#include <dtmc_base/version.h>

#define DTMC_BASE_FLAVOR "dtmc_base"
