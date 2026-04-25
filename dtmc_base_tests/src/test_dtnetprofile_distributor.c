#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtcore/dtunittest.h>

#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtnetportal_shm.h>
#include <dtmc_base/dtnetprofile.h>
#include <dtmc_base/dtnetprofile_distributor.h>

#define TAG "test_dtmc_base_dtnetprofile_distributor"

#define dtlog_debug(...)

// --- helpers ----------------------------------------------------------------

static void
fill_sample(dtnetprofile_t* p)
{
    memset(p, 0, sizeof(*p));
    dtnetprofile_add_name(p, "alpha");
    dtnetprofile_add_name(p, "beta");
    dtnetprofile_add_url(p, "coap://fd00::1:5683");
    dtnetprofile_add_role(p, "sensor");
    dtnetprofile_add_role(p, "edge");
    dtnetprofile_add_subscription(p, "foo/in");
    dtnetprofile_add_publication(p, "bar/out");
    dtnetprofile_add_state(p, "ready");
    dtnetprofile_add_state(p, "ok");
}

// --------------------------------------------------------------------------------------------
// Unit test: Ensure the btle client records method calls
static dterr_t*
test_dtmc_base_dtnetprofile_distributor_shm(void)
{
    dterr_t* dterr = NULL;
    dtnetprofile_t tx_netprofile = { 0 };
    dtnetprofile_t rx_netprofile = { 0 };

    dtnetportal_handle netportal_handle = NULL;

    dtbufferqueue_handle bufferqueue_handle = NULL;
    dtbufferqueue_handle tx_bufferqueue_handle = NULL;

    dtnetprofile_distributor_t _rx_distributor = { 0 }, *rx_distributor = &_rx_distributor;
    dtnetprofile_distributor_t _tx_distributor = { 0 }, *tx_distributor = &_tx_distributor;

    dtlog_debug(TAG, "starting");

    {
        dtnetportal_shm_t* o = NULL;
        DTERR_C(dtnetportal_shm_create(&o));
        netportal_handle = (dtnetportal_handle)o;
        dtnetportal_shm_config_t c = { 0 };
        DTERR_C(dtnetportal_shm_configure(o, &c));
    }

    DTERR_C(dtbufferqueue_create(&bufferqueue_handle, 10, false));

    {
        dtnetprofile_distributor_init(rx_distributor);
        dtnetprofile_distributor_config_t c = { 0 };
        c.netportal_handle = netportal_handle;
        c.bufferqueue_handle = bufferqueue_handle;
        DTERR_C(dtnetprofile_distributor_configure(rx_distributor, &c));
    }

    {
        dtnetprofile_distributor_init(tx_distributor);
        dtnetprofile_distributor_config_t c = { 0 };
        c.netportal_handle = netportal_handle;
        c.bufferqueue_handle = bufferqueue_handle;
        DTERR_C(dtnetprofile_distributor_configure(tx_distributor, &c));
    }

    DTERR_C(dtnetprofile_distributor_activate(rx_distributor));

    fill_sample(&tx_netprofile);
    dtlog_debug(TAG, "sending netprofile");
    DTERR_C(dtnetprofile_distributor_distribute(tx_distributor, &tx_netprofile));

    DTERR_C(dtnetprofile_distributor_collect(rx_distributor, &rx_netprofile));

    // Compare human-readable forms (simple and robust)
    char sa[256], sb[256];
    dtnetprofile_to_string(&tx_netprofile, sa, (int)sizeof(sa));
    dtnetprofile_to_string(&rx_netprofile, sb, (int)sizeof(sb));
    DTUNITTEST_ASSERT_EQUAL_STRING(sa, sb);

    dtlog_debug(TAG, "received netprofile\n%s", sb);

cleanup:
    dtnetprofile_distributor_dispose(tx_distributor);
    dtnetprofile_distributor_dispose(rx_distributor);
    dtbufferqueue_dispose(tx_bufferqueue_handle);
    dtbufferqueue_dispose(bufferqueue_handle);
    dtnetportal_dispose(netportal_handle);
    dtnetprofile_dispose(&tx_netprofile);
    dtnetprofile_dispose(&rx_netprofile);

    if (dterr != NULL)
    {
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "test_distributing_netprofile failed");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------
void
test_dtmc_base_dtnetprofile_distributor(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtnetprofile_distributor_shm);
}
