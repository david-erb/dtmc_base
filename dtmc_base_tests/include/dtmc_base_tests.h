#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dthttpd.h>
#include <dtmc_base/dtinterval.h>
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

extern dterr_t*
test_dtmc_base_dtinterval_ticks(dtinterval_handle h, int32_t interval_ms);
extern dterr_t*
test_dtmc_base_dtinterval_null_guards(dtinterval_handle h);
extern dterr_t*
test_dtmc_base_dtinterval_callback_error_propagates(dtinterval_handle h);
extern dterr_t*
test_dtmc_base_dtinterval_accuracy_under_load(dtinterval_handle h, int32_t interval_ms);
extern dterr_t*
test_dtmc_base_dtinterval_pause_is_idempotent(dtinterval_handle h);

extern dterr_t*
test_dtmc_base_dthttpd_simple(dthttpd_handle httpd_handle);
extern dterr_t*
test_dtmc_base_dthttpd_post_json(dthttpd_handle httpd_handle);
extern dterr_t*
test_dtmc_base_dthttpd_post_empty_body(dthttpd_handle httpd_handle);
extern dterr_t*
test_dtmc_base_dthttpd_get_not_found(dthttpd_handle httpd_handle);
extern dterr_t*
test_dtmc_base_dthttpd_post_no_callback(dthttpd_handle httpd_handle);
extern dterr_t*
test_dtmc_base_dthttpd_post_sequential(dthttpd_handle httpd_handle);
