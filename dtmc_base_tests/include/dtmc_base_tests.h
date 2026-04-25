#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtlock.h>
#include <dtmc_base/dtnvblob.h>
#include <dtmc_base/dttasker.h>

extern void test_dtmc_base_matching(DTUNITTEST_SUITE_ARGS);

// helper functions for facade tests
extern dterr_t*
test_dtmc_base_dtnvblob_example_write_read(dtnvblob_handle nvblob_handle);

extern dterr_t*
test_dtmc_base_dtlock_simple(dttasker_handle tasker1, dttasker_handle tasker2);
extern dterr_t*
test_dtmc_base_dtlock_stress(dttasker_handle t1, dttasker_handle t2, dttasker_handle t3);
