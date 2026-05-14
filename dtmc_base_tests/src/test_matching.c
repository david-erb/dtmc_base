#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtledger.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_matching"

// ----------------------------------------------------------------
void
test_dtmc_base_matching(DTUNITTEST_SUITE_ARGS)
{
    // ledgers we will check at end of each test
    dtledger_t* ledgers[10] = { 0 };
    {
        int i = 0;
        ledgers[i++] = dtstr_ledger;
        ledgers[i++] = dterr_ledger;
        ledgers[i++] = dtbuffer_ledger;
    }

    unittest_control->ledgers = ledgers;

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtbufferqueue);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtbuffer_distributor);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtcpu);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtdotstar_dummy);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtinterval_scheduled);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtmanifold);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtnetportal_shm);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtpackable_object_distributor);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtnetprofile);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtnetprofile_distributor);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtdebouncer);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtruntime);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtradioconfig);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtlightsensor_dummy);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtgpiopin_dummy);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtiox_dummy);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtframer_simple);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dttimeseries);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtmcp4728_dummy);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtlock);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtsemaphore);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtnetportal_iox);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dttasker);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dttasker_registry);
    DTUNITTEST_RUN_SUITE(test_dtmc_base_dtbusywork);

    DTUNITTEST_RUN_SUITE(test_dtmc_base_dthttpd_callback_rpc);
}
