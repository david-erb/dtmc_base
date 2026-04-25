#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dttimeout.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtmanifold.h>
#include <dtmc_base/dtsemaphore.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_dtmanifold"

/* Recipient used in tests; callback owns delivered buffers and stashes last one. */
typedef struct
{
    int call_count;
    dtbuffer_t* last_buffer;
} test_recipient_t;

static dterr_t*
test_newbuffer_callback(void* self, const char* subject_name, dtbuffer_t* buffer)
{
    (void)subject_name;
    test_recipient_t* r = (test_recipient_t*)self;
    r->call_count++;
    if (r->last_buffer)
        dtbuffer_dispose(r->last_buffer);
    r->last_buffer = buffer; /* callback takes ownership */
    return NULL;
}

//------------------------------------------------------------------------
// Example: minimal subscribe → publish → receive → unsubscribe
static dterr_t*
test_dtmc_base_dtmanifold_example_basic_pubsub(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    test_recipient_t r = { 0 };
    dtbuffer_t* buf = NULL;

    DTERR_C(dtmanifold_init(&mf));

    DTERR_C(dtbuffer_create(&buf, 32));
    strcpy((char*)buf->payload, "hello");

    DTERR_C(dtmanifold_subscribe(&mf, "topic.A", &r, test_newbuffer_callback));
    DTERR_C(dtmanifold_publish(&mf, "topic.A", buf));

    DTUNITTEST_ASSERT_INT(r.call_count, ==, 1);
    DTUNITTEST_ASSERT_PTR(r.last_buffer, !=, NULL);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)r.last_buffer->payload, "hello");

    DTERR_C(dtmanifold_unsubscribe(&mf, "topic.A", &r));
    DTERR_C(dtmanifold_publish(&mf, "topic.A", buf));
    DTUNITTEST_ASSERT_INT(r.call_count, ==, 1); /* no new delivery */

cleanup:
    dtbuffer_dispose(r.last_buffer);
    dtbuffer_dispose(buf);
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// Example: subject iteration & recipient counts snapshot
static bool
foreach_capture(void* ctx, const char* subject, int rcnt)
{
    (void)subject;
    int* sum = (int*)ctx;
    *sum += rcnt;
    return true;
}

static dterr_t*
test_dtmc_base_dtmanifold_example_foreach_and_counts(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    test_recipient_t a = { 0 }, b = { 0 }, c = { 0 };
    int total = 0;

    DTERR_C(dtmanifold_init(&mf));
    DTERR_C(dtmanifold_subscribe(&mf, "alpha", &a, test_newbuffer_callback));
    DTERR_C(dtmanifold_subscribe(&mf, "beta", &b, test_newbuffer_callback));
    DTERR_C(dtmanifold_subscribe(&mf, "beta", &c, test_newbuffer_callback));

    int rc_alpha = -1, rc_beta = -1;
    DTERR_C(dtmanifold_subject_recipient_count(&mf, "alpha", &rc_alpha));
    DTERR_C(dtmanifold_subject_recipient_count(&mf, "beta", &rc_beta));
    DTUNITTEST_ASSERT_INT(rc_alpha, ==, 1);
    DTUNITTEST_ASSERT_INT(rc_beta, ==, 2);

    DTERR_C(dtmanifold_foreach_topic(&mf, foreach_capture, &total));
    DTUNITTEST_ASSERT_INT(total, ==, 3);

cleanup:
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// Example: subject length guard (no silent truncation)
static dterr_t*
test_dtmc_base_dtmanifold_example_subject_length_guard(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    test_recipient_t r = { 0 };
    char too_long[DTMANIFOLD_MAX_SUBJECT_LENGTH + 8];

    memset(too_long, 'A', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';

    DTERR_C(dtmanifold_init(&mf));

    dterr = dtmanifold_subscribe(&mf, too_long, &r, test_newbuffer_callback);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_RANGE);

    dterr = dtmanifold_has_subject(&mf, too_long, &(bool){ false });
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_RANGE);

cleanup:
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// Example: publish with no subscribers (should succeed, no delivery)
static dterr_t*
test_dtmc_base_dtmanifold_example_publish_no_subscribers(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    dtbuffer_t* buf = NULL;

    DTERR_C(dtmanifold_init(&mf));
    DTERR_C(dtbuffer_create(&buf, 8));
    strcpy((char*)buf->payload, "ping");

    DTERR_C(dtmanifold_publish(&mf, "lonely.topic", buf)); /* no subscribers, not an error */

cleanup:
    dtbuffer_dispose(buf);
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// 01: Error conditions and NULL guards
static dterr_t*
test_dtmc_base_dtmanifold_01_error_handling_(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    test_recipient_t r = { 0 };

    DTERR_C(dtmanifold_init(&mf));

    dterr = dtmanifold_subscribe(&mf, NULL, &r, test_newbuffer_callback);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_ARGUMENT_NULL);

    dterr = dtmanifold_subscribe(&mf, "X", NULL, test_newbuffer_callback);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_ARGUMENT_NULL);

    dterr = dtmanifold_subscribe(&mf, "X", &r, NULL);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_ARGUMENT_NULL);

    dterr = dtmanifold_publish(&mf, "X", NULL);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_ARGUMENT_NULL);

    dterr = dtmanifold_unsubscribe(&mf, NULL, NULL);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_ARGUMENT_NULL);

cleanup:
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// 02: Max subjects (overflow on creation of another distinct subject)
static dterr_t*
test_dtmc_base_dtmanifold_02_max_subjects_(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    test_recipient_t r = { 0 };
    char subject[32];

    DTERR_C(dtmanifold_init(&mf));

    for (int i = 0; i < DTMANIFOLD_MAX_SUBJECTS; i++)
    {
        snprintf(subject, sizeof(subject), "topic.%d", i);
        DTERR_C(dtmanifold_subscribe(&mf, subject, &r, test_newbuffer_callback));
    }

    snprintf(subject, sizeof(subject), "overflow.topic");
    dterr = dtmanifold_subscribe(&mf, subject, &r, test_newbuffer_callback);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_OVERFLOW);

cleanup:
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// 03: Max recipients per subject
static dterr_t*
test_dtmc_base_dtmanifold_03_max_recipients_(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    test_recipient_t rec[DTMANIFOLD_MAX_RECIPIENTS + 1];

    memset(rec, 0, sizeof(rec));
    DTERR_C(dtmanifold_init(&mf));

    for (int i = 0; i < DTMANIFOLD_MAX_RECIPIENTS; i++)
        DTERR_C(dtmanifold_subscribe(&mf, "shared.topic", &rec[i], test_newbuffer_callback));

    dterr = dtmanifold_subscribe(&mf, "shared.topic", &rec[DTMANIFOLD_MAX_RECIPIENTS], test_newbuffer_callback);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_OVERFLOW);

cleanup:
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// 04: Multiple recipients receive one copy each
static dterr_t*
test_dtmc_base_dtmanifold_04_multiple_recipients_(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    dtbuffer_t* buf = NULL;
    test_recipient_t r[3] = { 0 };

    DTERR_C(dtmanifold_init(&mf));
    DTERR_C(dtbuffer_create(&buf, 32));

    for (int i = 0; i < 3; i++)
        DTERR_C(dtmanifold_subscribe(&mf, "broadcast", &r[i], test_newbuffer_callback));

    strcpy((char*)buf->payload, "Hello, World!");
    DTERR_C(dtmanifold_publish(&mf, "broadcast", buf));

    for (int i = 0; i < 3; i++)
    {
        DTUNITTEST_ASSERT_INT(r[i].call_count, ==, 1);
        DTUNITTEST_ASSERT_PTR(r[i].last_buffer, !=, NULL);
        DTUNITTEST_ASSERT_EQUAL_STRING((char*)r[i].last_buffer->payload, "Hello, World!");
        dtbuffer_dispose(r[i].last_buffer);
        r[i].last_buffer = NULL;
    }

cleanup:
    dtbuffer_dispose(buf);
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// 05: Double subscribe does not duplicate delivery
static dterr_t*
test_dtmc_base_dtmanifold_05_double_subscribe_(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    dtbuffer_t* buf = NULL;
    test_recipient_t r = { 0 };

    DTERR_C(dtmanifold_init(&mf));
    DTERR_C(dtbuffer_create(&buf, 16));

    DTERR_C(dtmanifold_subscribe(&mf, "topic", &r, test_newbuffer_callback));
    DTERR_C(dtmanifold_subscribe(&mf, "topic", &r, test_newbuffer_callback));

    DTERR_C(dtmanifold_publish(&mf, "topic", buf));
    DTUNITTEST_ASSERT_INT(r.call_count, ==, 1);

cleanup:
    dtbuffer_dispose(r.last_buffer);
    dtbuffer_dispose(buf);
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// 06: Resubscribe after unsubscribe
static dterr_t*
test_dtmc_base_dtmanifold_06_resubscribe_(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    dtbuffer_t* buf = NULL;
    test_recipient_t r = { 0 };

    DTERR_C(dtmanifold_init(&mf));
    DTERR_C(dtbuffer_create(&buf, 16));

    DTERR_C(dtmanifold_subscribe(&mf, "topic", &r, test_newbuffer_callback));
    DTERR_C(dtmanifold_unsubscribe(&mf, "topic", &r));

    DTERR_C(dtmanifold_publish(&mf, "topic", buf));
    DTUNITTEST_ASSERT_INT(r.call_count, ==, 0);

    DTERR_C(dtmanifold_subscribe(&mf, "topic", &r, test_newbuffer_callback));
    DTERR_C(dtmanifold_publish(&mf, "topic", buf));
    DTUNITTEST_ASSERT_INT(r.call_count, ==, 1);

cleanup:
    dtbuffer_dispose(r.last_buffer);
    dtbuffer_dispose(buf);
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// 07: Use semaphore
static dterr_t*
test_dtmc_base_dtmanifold_07_use_semaphore_(void)
{
    dterr_t* dterr = NULL;
    dtmanifold_t mf;
    dtbuffer_t* buf = NULL;
    test_recipient_t r = { 0 };
    dtsemaphore_handle semaphore_handle = NULL;

    DTERR_C(dtsemaphore_create(&semaphore_handle, 0, 0));

    DTERR_C(dtmanifold_set_threadsafe_semaphore(&mf, semaphore_handle, DTTIMEOUT_FOREVER));

    DTERR_C(dtmanifold_init(&mf));
    DTERR_C(dtbuffer_create(&buf, 16));

    DTERR_C(dtmanifold_subscribe(&mf, "topic", &r, test_newbuffer_callback));
    DTERR_C(dtmanifold_unsubscribe(&mf, "topic", &r));

    DTERR_C(dtmanifold_publish(&mf, "topic", buf));
    DTUNITTEST_ASSERT_INT(r.call_count, ==, 0);

    DTERR_C(dtmanifold_subscribe(&mf, "topic", &r, test_newbuffer_callback));
    DTERR_C(dtmanifold_publish(&mf, "topic", buf));
    DTUNITTEST_ASSERT_INT(r.call_count, ==, 1);

cleanup:
    dtsemaphore_dispose(semaphore_handle);
    dtbuffer_dispose(r.last_buffer);
    dtbuffer_dispose(buf);
    dtmanifold_dispose(&mf);
    return dterr;
}

//------------------------------------------------------------------------
// Aggregator
void
test_dtmc_base_dtmanifold(DTUNITTEST_SUITE_ARGS)
{

    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_example_basic_pubsub);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_example_foreach_and_counts);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_example_subject_length_guard);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_example_publish_no_subscribers);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_01_error_handling_);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_02_max_subjects_);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_03_max_recipients_);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_04_multiple_recipients_);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_05_double_subscribe_);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_06_resubscribe_);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtmanifold_07_use_semaphore_);
}
