#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtcore/dtunittest.h>

#include <dtmc_base/dtbuffer_distributor.h>
#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtnetportal_shm.h>

#define TAG "test_dtmc_base_dtbuffer_distributor"

// --- helpers ----------------------------------------------------------------

static dterr_t*
fill_sample(dtbuffer_t** buffer)
{
    dterr_t* dterr = NULL;

    DTERR_C(dtbuffer_create(buffer, 1024));

    char* chars = "abcdefghijklmnopqrstuvwxyz"
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                  "0123456789"
                  "!@#$%^&*()-_=+[]{}|;:',.<>/?`~\"\\ ";
    int nchars = (int)strlen(chars);
    char* ptr = (char*)(*buffer)->payload;
    int i;
    for (i = 0; i < (*buffer)->length - 1; i++)
    {
        ptr[i] = chars[i % nchars];
    }
    ptr[i] = '\0';

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Unit test: Ensure the btle client records method calls
static dterr_t*
test_dtmc_base_dtbuffer_distributor_shm(void)
{
    dterr_t* dterr = NULL;
    dtnetportal_handle netportal_handle = NULL;

    dtbufferqueue_handle bufferqueue_handle = NULL;
    dtbufferqueue_handle tx_bufferqueue_handle = NULL;

    dtbuffer_distributor_t _tx_distributor = { 0 }, *tx_distributor = &_tx_distributor;
    dtbuffer_distributor_t _rx_distributor = { 0 }, *rx_distributor = &_rx_distributor;

    dtbuffer_t* tx_buffer = NULL;
    dtbuffer_t* rx_buffer = NULL;

    {
        dtnetportal_shm_t* o = NULL;
        DTERR_C(dtnetportal_shm_create(&o));
        netportal_handle = (dtnetportal_handle)o;
        dtnetportal_shm_config_t c = { 0 };
        DTERR_C(dtnetportal_shm_configure(o, &c));
    }

    // bufferqueue shared by both distributors
    DTERR_C(dtbufferqueue_create(&bufferqueue_handle, 10, false));

    {
        dtbuffer_distributor_init(rx_distributor);
        dtbuffer_distributor_config_t c = { 0 };
        c.netportal_handle = netportal_handle;
        c.bufferqueue_handle = bufferqueue_handle;
        c.topic = "test/topic";
        DTERR_C(dtbuffer_distributor_configure(rx_distributor, &c));
    }

    {
        dtbuffer_distributor_init(tx_distributor);
        dtbuffer_distributor_config_t c = { 0 };
        c.netportal_handle = netportal_handle;
        c.bufferqueue_handle = bufferqueue_handle;
        c.topic = "test/topic";
        DTERR_C(dtbuffer_distributor_configure(tx_distributor, &c));
    }

    DTERR_C(dtbuffer_distributor_subscribe(rx_distributor));

    DTERR_C(fill_sample(&tx_buffer));

    DTERR_C(dtbuffer_distributor_distribute(tx_distributor, tx_buffer));

    DTERR_C(dtbuffer_distributor_collect(rx_distributor, &rx_buffer, 1000));

    DTUNITTEST_ASSERT_EQUAL_STRING(tx_buffer->payload, rx_buffer->payload);

cleanup:
    dtbuffer_dispose(rx_buffer);
    dtbuffer_dispose(tx_buffer);
    dtbuffer_distributor_dispose(tx_distributor);
    dtbuffer_distributor_dispose(rx_distributor);
    dtbufferqueue_dispose(tx_bufferqueue_handle);
    dtbufferqueue_dispose(bufferqueue_handle);
    dtnetportal_dispose(netportal_handle);

    if (dterr != NULL)
    {
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "test_distributing_buffer failed");
    }
    return dterr;
}

// -----------------------------------------------------------------------------
// suite runner

void
test_dtmc_base_dtbuffer_distributor(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtbuffer_distributor_shm);
}
