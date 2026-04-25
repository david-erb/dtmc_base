#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtcore/dtunittest.h>

#include <dtmc_base/dtnetportal_shm.h>

#define TAG "test_dtmc_base_dtnetportal_shm"

typedef struct simple_receiver_t
{
    dtbuffer_t* buffer;
} simple_receiver_t;

// --------------------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtnetportal_shm_topic1_receive_callback(void* receiver_self, const char* topic, dtbuffer_t* buffer)
{
    dterr_t* dterr = NULL;
    simple_receiver_t* receiver = (simple_receiver_t*)receiver_self;

    // here we take ownership of the buffer
    receiver->buffer = buffer;

    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtnetportal_shm_subscribe(void)
{
    dterr_t* dterr = NULL;
    dtnetportal_handle netportal_handle = NULL;

    simple_receiver_t receiver1 = { 0 };
    simple_receiver_t receiver2 = { 0 };

    dtbuffer_t* send_buffer = NULL;

    {
        dtnetportal_shm_t* o = NULL;
        DTERR_C(dtnetportal_shm_create(&o));
        netportal_handle = (dtnetportal_handle)o;

        dtnetportal_shm_config_t c = { .placeholder = 42 };
        DTERR_C(dtnetportal_shm_configure(o, &c));
    }

    // Connect
    DTERR_C(dtnetportal_activate(netportal_handle));

    // Subscribe
    const char* topic1 = "test/topic1";
    const char* data1 = "Hello, Topic 1!";

    DTERR_C(
      dtnetportal_subscribe(netportal_handle, topic1, &receiver1, test_dtmc_base_dtnetportal_shm_topic1_receive_callback));
    DTERR_C(
      dtnetportal_subscribe(netportal_handle, topic1, &receiver2, test_dtmc_base_dtnetportal_shm_topic1_receive_callback));

    // Send
    DTERR_C(dtbuffer_create(&send_buffer, 128));
    strcpy(send_buffer->payload, data1);
    DTERR_C(dtnetportal_publish(netportal_handle, topic1, send_buffer));

    DTUNITTEST_ASSERT_NOT_NULL(receiver1.buffer);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)receiver1.buffer->payload, data1);

    DTUNITTEST_ASSERT_NOT_NULL(receiver2.buffer);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)receiver2.buffer->payload, data1);

cleanup:

    dtbuffer_dispose(send_buffer);

    dtbuffer_dispose(receiver2.buffer);

    dtbuffer_dispose(receiver1.buffer);

    dtnetportal_dispose(netportal_handle);

    return dterr;
}

// --------------------------------------------------------------------------------------------
void
test_dtmc_base_dtnetportal_shm(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtnetportal_shm_subscribe);
}
