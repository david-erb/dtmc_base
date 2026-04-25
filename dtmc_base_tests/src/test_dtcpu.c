#include <sys/types.h>

#include <inttypes.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtcore/dtunittest.h>

#include <dtmc_base/dtcpu.h>

#define TAG "test_dtcpu"

// ----------------------------------------------------------------------------------
// Unit test: Microsecond time should measure elapsed time accurately
static dterr_t*
test_dtmc_base_dtcpu_elapsed_microseconds()
{
    dterr_t* dterr = NULL;

    DTERR_C(dtcpu_sysinit());

    uint64_t micros = 100000;

    dtcpu_t cpu = { 0 };
    dtcpu_mark(&cpu);
    // need busy wait here because on some microcontrollers the timer stops during sleep waits
    dtcpu_busywait_microseconds(micros);
    dtcpu_mark(&cpu);
    uint64_t elapsed = dtcpu_elapsed_microseconds(&cpu);

    dtlog_debug(TAG, "busy waited %" PRIu64 ", measured elapsed microseconds: %" PRIu64, micros, elapsed);

    DTUNITTEST_ASSERT_UINT64(elapsed, >=, 99000);
    DTUNITTEST_ASSERT_UINT64(elapsed, <=, 120000);

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
void
test_dtmc_base_dtcpu(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtcpu_elapsed_microseconds);
}
