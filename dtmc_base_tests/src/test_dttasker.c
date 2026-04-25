// test_dtmc_base_dttasker.c
#include <stdbool.h>

#include <dtcore/dterr.h>
#include <dtcore/dtguidable.h>
#include <dtcore/dtguidable_pool.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dttasker.h>

#define TAG "app_main"

// comment out the logging here
#define dtlog_info(TAG, ...)

// -------------------------------------------------------------------------------
// tasks' operating variables
typedef struct subscriber_t
{
    bool should_die_before_ready;
    bool should_die_after_ready;
} subscriber_t;

typedef struct stoppable_subscriber_t
{
    bool saw_stop;
    int32_t loop_count;
} stoppable_subscriber_t;

typedef struct info_cb_ctx_t
{
    int call_count;
    dttasker_status_t statuses[8];
} info_cb_ctx_t;

static dterr_t*
info_cb_fn(void* ctx_arg, dttasker_info_t* info)
{
    info_cb_ctx_t* ctx = (info_cb_ctx_t*)ctx_arg;
    if (ctx->call_count < 8)
        ctx->statuses[ctx->call_count] = info->status;
    ctx->call_count++;
    return NULL;
}

// -------------------------------------------------------------------------------
// this executes the main logic of the task
static dterr_t*
subscriber_tasker_entrypoint(void* self_arg, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;

    dtlog_info(TAG, "%s(): business logic started", __func__);

    subscriber_t* self = (subscriber_t*)self_arg;

    if (self->should_die_before_ready)
    {
        dtlog_info(TAG, "%s(): dying before ready", __func__);
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "subscriber task honoring self->should_die_before_ready");
        goto cleanup;
    }

    DTERR_C(dttasker_ready(tasker_handle));

    dtruntime_sleep_milliseconds(100);

    if (self->should_die_after_ready)
    {
        dtlog_info(TAG, "%s(): dying after ready", __func__);
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "subscriber task honoring self->should_die_after_ready");
        goto cleanup;
    }

cleanup:
    return dterr;
}

// -------------------------------------------------------------------------------
// this executes task logic that waits until stop() is requested
static dterr_t*
stoppable_subscriber_tasker_entrypoint(void* self_arg, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    stoppable_subscriber_t* self = (stoppable_subscriber_t*)self_arg;
    bool should_stop = false;

    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dttasker_ready(tasker_handle));

    for (;;)
    {
        DTERR_C(dttasker_poll(tasker_handle, &should_stop));

        if (should_stop)
        {
            self->saw_stop = true;
            break;
        }

        self->loop_count++;
        dtruntime_sleep_milliseconds(10);
    }

cleanup:
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_single(void)
{
    dterr_t* dterr = NULL;

    subscriber_t _subscriber = { 0 }, *subscriber = &_subscriber;
    dttasker_config_t subscriber_tasker_config = (dttasker_config_t){ 0 };
    dttasker_handle subscriber_tasker = NULL;

    subscriber_tasker_config.name = "single";                                      // name of the task
    subscriber_tasker_config.tasker_entry_point_fn = subscriber_tasker_entrypoint; // main function for the task
    subscriber_tasker_config.tasker_entry_point_arg = subscriber;                  // self pointer
    subscriber_tasker_config.stack_size = 4096;                                    // pthread may raise this to min
    subscriber_tasker_config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;           // best-effort / ignored on SCHED_OTHER

    DTERR_C(dttasker_create(&subscriber_tasker, &subscriber_tasker_config));

    DTERR_C(dttasker_start(subscriber_tasker));
    dtlog_info(TAG, "%s(): continues since task \"%s\" has indicated ready", __func__, subscriber_tasker_config.name);

    dttasker_info_t state;
    DTERR_C(dttasker_get_info(subscriber_tasker, &state));
    DTUNITTEST_ASSERT_INT(state.status, ==, RUNNING);

    dtruntime_sleep_milliseconds(200);
    DTERR_C(dttasker_get_info(subscriber_tasker, &state));
    DTUNITTEST_ASSERT_INT(state.status, ==, STOPPED);
    DTUNITTEST_ASSERT_NULL(state.dterr);

cleanup:
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(subscriber_tasker);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_dies_before_ready(void)
{
    dterr_t* dterr = NULL;

    subscriber_t _subscriber = { .should_die_before_ready = true }, *subscriber = &_subscriber;
    dttasker_config_t subscriber_tasker_config = (dttasker_config_t){ 0 };
    dttasker_handle subscriber_tasker = NULL;

    subscriber_tasker_config.name = "dies_before";
    subscriber_tasker_config.tasker_entry_point_fn = subscriber_tasker_entrypoint;
    subscriber_tasker_config.tasker_entry_point_arg = subscriber;
    subscriber_tasker_config.stack_size = 4096;
    subscriber_tasker_config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;

    DTERR_C(dttasker_create(&subscriber_tasker, &subscriber_tasker_config));

    // start() will return even though thread fails before ready()
    DTERR_C(dttasker_start(subscriber_tasker));

    dttasker_info_t state;
    DTERR_C(dttasker_get_info(subscriber_tasker, &state));
    DTUNITTEST_ASSERT_INT(state.status, ==, STOPPED);
    DTUNITTEST_ASSERT_NOT_NULL(state.dterr);
    dterr_dispose(state.dterr);

cleanup:
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(subscriber_tasker);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_dies_after_ready(void)
{
    dterr_t* dterr = NULL;

    subscriber_t _subscriber = { .should_die_after_ready = true }, *subscriber = &_subscriber;
    dttasker_config_t subscriber_tasker_config = (dttasker_config_t){ 0 };
    dttasker_handle subscriber_tasker = NULL;

    subscriber_tasker_config.name = "dies_after";
    subscriber_tasker_config.tasker_entry_point_fn = subscriber_tasker_entrypoint;
    subscriber_tasker_config.tasker_entry_point_arg = subscriber;
    subscriber_tasker_config.stack_size = 4096;
    subscriber_tasker_config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;

    DTERR_C(dttasker_create(&subscriber_tasker, &subscriber_tasker_config));

    DTERR_C(dttasker_start(subscriber_tasker));
    dtlog_info(TAG, "%s(): continues since task \"%s\" has indicated ready", __func__, subscriber_tasker_config.name);

    dtruntime_sleep_milliseconds(200);

    dttasker_info_t state;
    DTERR_C(dttasker_get_info(subscriber_tasker, &state));
    DTUNITTEST_ASSERT_INT(state.status, ==, STOPPED);
    DTUNITTEST_ASSERT_NOT_NULL(state.dterr);
    dterr_dispose(state.dterr);

cleanup:
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(subscriber_tasker);
    return dterr;
}

// -----------------------------------------------------------------------------

static dterr_t*
test_dtmc_base_dttasker_guidable_pool_insert_and_search(void)
{
    dterr_t* dterr = NULL;
    dtguidable_pool_t pool = { 0 };

    DTERR_C(dtguidable_pool_init(&pool, 4));

    dttasker_handle task_handle_one;
    dttasker_handle task_handle_two;

    {
        // Configure
        dttasker_config_t c = { 0 };
        c.name = "task_one";
        c.tasker_entry_point_fn = subscriber_tasker_entrypoint;
        c.stack_size = 4096;
        c.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;
        DTERR_C(dttasker_create(&task_handle_one, &c));
    }
    {
        // Configure
        dttasker_config_t c = { 0 };
        c.name = "task_two";
        c.tasker_entry_point_fn = subscriber_tasker_entrypoint;
        c.stack_size = 4096;
        c.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;
        DTERR_C(dttasker_create(&task_handle_two, &c));
    }

    DTERR_C(dtguidable_pool_insert(&pool, (dtguidable_handle)task_handle_one));
    DTERR_C(dtguidable_pool_insert(&pool, (dtguidable_handle)task_handle_two));

    dtguid_t guid;
    dtguidable_handle found;

    found = NULL;
    dtguidable_get_guid((dtguidable_handle)task_handle_one, &guid);
    DTERR_C(dtguidable_pool_search(&pool, &guid, &found));
    DTUNITTEST_ASSERT_PTR(found, ==, (dtguidable_handle)task_handle_one);

    found = NULL;
    dtguidable_get_guid((dtguidable_handle)task_handle_two, &guid);
    DTERR_C(dtguidable_pool_search(&pool, &guid, &found));
    DTUNITTEST_ASSERT_PTR(found, ==, (dtguidable_handle)task_handle_two);

cleanup:
    dtguidable_pool_dispose(&pool);
    dttasker_dispose(task_handle_one);
    dttasker_dispose(task_handle_two);

    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, __LINE__, __FILE__, __func__, dterr, "test failed: %s", dterr->message);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_stop_poll_join(void)
{
    dterr_t* dterr = NULL;

    stoppable_subscriber_t _subscriber = { 0 }, *subscriber = &_subscriber;
    dttasker_config_t subscriber_tasker_config = (dttasker_config_t){ 0 };
    dttasker_handle subscriber_tasker = NULL;
    bool was_timeout = false;
    dttasker_info_t state;

    subscriber_tasker_config.name = "stop_poll_join";
    subscriber_tasker_config.tasker_entry_point_fn = stoppable_subscriber_tasker_entrypoint;
    subscriber_tasker_config.tasker_entry_point_arg = subscriber;
    subscriber_tasker_config.stack_size = 4096;
    subscriber_tasker_config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;

    DTERR_C(dttasker_create(&subscriber_tasker, &subscriber_tasker_config));

    DTERR_C(dttasker_start(subscriber_tasker));
    dtlog_info(TAG, "%s(): continues since task \"%s\" has indicated ready", __func__, subscriber_tasker_config.name);

    DTERR_C(dttasker_get_info(subscriber_tasker, &state));
    DTUNITTEST_ASSERT_INT(state.status, ==, RUNNING);

    DTERR_C(dttasker_stop(subscriber_tasker));
    DTERR_C(dttasker_join(subscriber_tasker, 1000, &was_timeout));

    DTUNITTEST_ASSERT_INT(was_timeout, ==, false);
    DTUNITTEST_ASSERT_INT(subscriber->saw_stop, ==, true);
    DTUNITTEST_ASSERT_INT(subscriber->loop_count, >, 0);

    DTERR_C(dttasker_get_info(subscriber_tasker, &state));
    DTUNITTEST_ASSERT_INT(state.status, ==, STOPPED);
    DTUNITTEST_ASSERT_NULL(state.dterr);

cleanup:
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(subscriber_tasker);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_join_never_started(void)
{
    dterr_t* dterr = NULL;
    dttasker_config_t config = (dttasker_config_t){ 0 };
    dttasker_handle tasker = NULL;
    bool was_timeout = false;

    config.name = "join_never_started";
    config.tasker_entry_point_fn = subscriber_tasker_entrypoint;
    config.stack_size = 4096;
    config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;

    DTERR_C(dttasker_create(&tasker, &config));

    // join without ever starting should be a silent no-op
    DTERR_C(dttasker_join(tasker, DTTIMEOUT_FOREVER, &was_timeout));
    DTUNITTEST_ASSERT_INT(was_timeout, ==, false);

cleanup:
    dttasker_dispose(tasker);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_join_times_out(void)
{
    dterr_t* dterr = NULL;
    stoppable_subscriber_t _subscriber = { 0 }, *subscriber = &_subscriber;
    dttasker_config_t config = (dttasker_config_t){ 0 };
    dttasker_handle tasker = NULL;
    bool was_timeout = false;

    config.name = "join_times_out";
    config.tasker_entry_point_fn = stoppable_subscriber_tasker_entrypoint;
    config.tasker_entry_point_arg = subscriber;
    config.stack_size = 4096;
    config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;

    DTERR_C(dttasker_create(&tasker, &config));
    DTERR_C(dttasker_start(tasker));

    // task is still running; short timeout must set was_timeout without returning an error
    DTERR_C(dttasker_join(tasker, 50, &was_timeout));
    DTUNITTEST_ASSERT_INT(was_timeout, ==, true);

cleanup:
    dttasker_stop(tasker);
    { bool _t = false; dttasker_join(tasker, 1000, &_t); }
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(tasker);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_join_null_timeout_returns_error(void)
{
    dterr_t* dterr = NULL;
    dterr_t* join_err = NULL;
    stoppable_subscriber_t _subscriber = { 0 }, *subscriber = &_subscriber;
    dttasker_config_t config = (dttasker_config_t){ 0 };
    dttasker_handle tasker = NULL;

    config.name = "join_null_timeout";
    config.tasker_entry_point_fn = stoppable_subscriber_tasker_entrypoint;
    config.tasker_entry_point_arg = subscriber;
    config.stack_size = 4096;
    config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;

    DTERR_C(dttasker_create(&tasker, &config));
    DTERR_C(dttasker_start(tasker));

    // NULL was_timeout with a real timeout must return DTERR_TIMEOUT as an error
    join_err = dttasker_join(tasker, 50, NULL);
    DTUNITTEST_ASSERT_NOT_NULL(join_err);
    dterr_dispose(join_err);
    join_err = NULL;

cleanup:
    dttasker_stop(tasker);
    { bool _t = false; dttasker_join(tasker, 1000, &_t); }
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(tasker);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_join_idempotent(void)
{
    dterr_t* dterr = NULL;
    stoppable_subscriber_t _subscriber = { 0 }, *subscriber = &_subscriber;
    dttasker_config_t config = (dttasker_config_t){ 0 };
    dttasker_handle tasker = NULL;
    bool was_timeout = false;

    config.name = "join_idempotent";
    config.tasker_entry_point_fn = stoppable_subscriber_tasker_entrypoint;
    config.tasker_entry_point_arg = subscriber;
    config.stack_size = 4096;
    config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;

    DTERR_C(dttasker_create(&tasker, &config));
    DTERR_C(dttasker_start(tasker));
    DTERR_C(dttasker_stop(tasker));
    DTERR_C(dttasker_join(tasker, 1000, &was_timeout));
    DTUNITTEST_ASSERT_INT(was_timeout, ==, false);

    // second join must be a silent no-op regardless of timeout value
    was_timeout = true;
    DTERR_C(dttasker_join(tasker, DTTIMEOUT_FOREVER, &was_timeout));
    DTUNITTEST_ASSERT_INT(was_timeout, ==, false);

cleanup:
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(tasker);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_double_start_rejected(void)
{
    dterr_t* dterr = NULL;
    dterr_t* second_start_err = NULL;
    stoppable_subscriber_t _subscriber = { 0 }, *subscriber = &_subscriber;
    dttasker_config_t config = (dttasker_config_t){ 0 };
    dttasker_handle tasker = NULL;

    config.name = "double_start";
    config.tasker_entry_point_fn = stoppable_subscriber_tasker_entrypoint;
    config.tasker_entry_point_arg = subscriber;
    config.stack_size = 4096;
    config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;

    DTERR_C(dttasker_create(&tasker, &config));
    DTERR_C(dttasker_start(tasker));

    // second start while task is running must be rejected with an error
    second_start_err = dttasker_start(tasker);
    DTUNITTEST_ASSERT_NOT_NULL(second_start_err);
    dterr_dispose(second_start_err);
    second_start_err = NULL;

cleanup:
    dttasker_stop(tasker);
    { bool _t = false; dttasker_join(tasker, 1000, &_t); }
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(tasker);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_info_callback(void)
{
    dterr_t* dterr = NULL;
    subscriber_t _subscriber = { 0 }, *subscriber = &_subscriber;
    info_cb_ctx_t _ctx = { 0 }, *ctx = &_ctx;
    dttasker_config_t config = (dttasker_config_t){ 0 };
    dttasker_handle tasker = NULL;

    config.name = "info_callback";
    config.tasker_entry_point_fn = subscriber_tasker_entrypoint;
    config.tasker_entry_point_arg = subscriber;
    config.stack_size = 4096;
    config.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;
    config.tasker_info_callback = info_cb_fn;
    config.tasker_info_callback_context = ctx;

    DTERR_C(dttasker_create(&tasker, &config));
    DTERR_C(dttasker_start(tasker));
    dtruntime_sleep_milliseconds(200); // let the task complete its 100 ms sleep and exit

    DTUNITTEST_ASSERT_INT(ctx->call_count, ==, 3);
    DTUNITTEST_ASSERT_INT(ctx->statuses[0], ==, INITIALIZED);
    DTUNITTEST_ASSERT_INT(ctx->statuses[1], ==, RUNNING);
    DTUNITTEST_ASSERT_INT(ctx->statuses[2], ==, STOPPED);

cleanup:
    dtruntime_sleep_milliseconds(1);
    dttasker_dispose(tasker);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Entry point for all dttasker unit tests
void
test_dtmc_base_dttasker(DTUNITTEST_SUITE_ARGS)
{

    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_single);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_dies_before_ready);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_dies_after_ready);

    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_guidable_pool_insert_and_search);

    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_stop_poll_join);

    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_join_never_started);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_join_times_out);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_join_null_timeout_returns_error);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_join_idempotent);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_double_start_rejected);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_info_callback);
}
