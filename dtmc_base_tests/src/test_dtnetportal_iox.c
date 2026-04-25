#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtcore/dtunittest.h>

#include <dtmc_base/dtframer_simple.h>
#include <dtmc_base/dtiox_dummy.h>
#include <dtmc_base/dtnetportal_iox.h>

#include <dtmc_base/dtframer.h>
#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtsemaphore.h>
#include <dtmc_base/dttasker.h>
#include <dtmc_base/dtuart_helpers.h>

#define TAG "test_dtmc_base_dtnetportal_iox"

typedef struct simple_receiver_t
{
    dtbuffer_t* buffer;
} simple_receiver_t;

// --------------------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtnetportal_iox_topic1_receive_callback(void* receiver_self, const char* topic, dtbuffer_t* buffer)
{
    dterr_t* dterr = NULL;
    simple_receiver_t* receiver = (simple_receiver_t*)receiver_self;

    // here we take ownership of the buffer
    receiver->buffer = buffer;

    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtnetportal_iox_subscribe(void)
{
    dterr_t* dterr = NULL;
    dtiox_handle iox_handle = NULL;
    dtframer_handle framer_handle = NULL;
    dtnetportal_handle netportal_handle = NULL;

    simple_receiver_t receiver1 = { 0 };
    simple_receiver_t receiver2 = { 0 };

    dtbuffer_t* send_buffer = NULL;

    // === the uart ===
    {
        dtiox_dummy_t* o = NULL;
        DTERR_C(dtiox_dummy_create(&o));
        iox_handle = (dtiox_handle)o;
        // Set desired behavior BEFORE configure/enable
        dtiox_dummy_behavior_t behavior = { 0 };
        behavior.loopback = true; // loopback writes to rx buffer
        behavior.rx_capacity = 256;
        behavior.tx_capacity = 256;

        DTERR_C(dtiox_dummy_set_behavior(o, &behavior));

        dtiox_dummy_config_t c = { //
            .name = "dummy1",
            .pin_tx = 0,
            .pin_rx = 1,
            .uart_config = dtuart_helper_default_config
        };
        DTERR_C(dtiox_dummy_configure(o, &c));
    }

    // === the framer ===
    {
        dtframer_simple_t* o = NULL;
        DTERR_C(dtframer_simple_create(&o));
        framer_handle = (dtframer_handle)o;
        dtframer_simple_config_t c = { 0 };
        DTERR_C(dtframer_simple_configure(o, &c));
    }

    // === the netportal ===
    {
        dtnetportal_iox_t* o = NULL;
        DTERR_C(dtnetportal_iox_create(&o));
        netportal_handle = (dtnetportal_handle)o;
        dtnetportal_iox_config_t c = { 0 };
        c.iox_handle = iox_handle;
        c.framer_handle = framer_handle;
        DTERR_C(dtnetportal_iox_configure(o, &c));
    }

    // Connect
    DTERR_C(dtnetportal_activate(netportal_handle));

    // Subscribe
    const char* topic1 = "test/topic1";
    const char* data1 = "Hello, Topic 1!";

    DTERR_C(
      dtnetportal_subscribe(netportal_handle, topic1, &receiver1, test_dtmc_base_dtnetportal_iox_topic1_receive_callback));
    DTERR_C(
      dtnetportal_subscribe(netportal_handle, topic1, &receiver2, test_dtmc_base_dtnetportal_iox_topic1_receive_callback));

    // Send
    DTERR_C(dtbuffer_create(&send_buffer, 128));
    strcpy(send_buffer->payload, data1);
    DTERR_C(dtnetportal_publish(netportal_handle, topic1, send_buffer));

    // drain the uart rx fifo like the task would normally do
    DTERR_C(dtnetportal_iox_rx_drain((dtnetportal_iox_t*)netportal_handle));

    // check first recipient got it
    DTUNITTEST_ASSERT_NOT_NULL(receiver1.buffer);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)receiver1.buffer->payload, data1);

    // check second recipient got it
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
test_dtmc_base_dtnetportal_iox(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtnetportal_iox_subscribe);
}
