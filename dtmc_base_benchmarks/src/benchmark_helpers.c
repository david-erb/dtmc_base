#include <dtcore/dtbuffer.h>
#include <dtcore/dtbytes.h>
#include <dtcore/dterr.h>
#include <dtcore/dteventlogger.h>
#include <dtcore/dtguid.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtpackable.h>
#include <dtcore/dtpackx.h>
#include <dtcore/dtpicosdk_helper.h>
#include <dtcore/dtstr.h>

#include <dtcore/dteventlogger.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtsemaphore.h>
#define dtlog_debug(TAG, ...)

#include <dtmc_base_benchmarks/benchmark_helpers.h>

#define TAG "dtmc_base_benchmark_helpers"

// --------------------------------------------------------------------------------------
// return a string description of the benchmark (the returned string is heap allocated)
extern dterr_t*
dtmc_base_benchmark_helpers_decorate_description(void* self, benchmark_describe_fn describe_fn, char** out_description)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(describe_fn);
    DTERR_ASSERT_NOT_NULL(out_description);

    char* d = *out_description;
    char* s = "\n";

    d = dtstr_concat_format(d, s, "");
    d = dtstr_concat_format(d, s, "----------------------------------------------");
    d = dtstr_concat_format(d, s, "Description of the benchmark:");
    DTERR_C(describe_fn(self, &d));
    d = dtstr_concat_format(d, s, "----------------------------------------------");

    *out_description = d;

cleanup:
    return dterr;
}
