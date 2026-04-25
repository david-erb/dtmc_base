#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>

#include <dtcore/dtstr.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtradioconfig.h>

// The unit test function for packing and unpacking a radio configuration.
static dterr_t*
test_dtmc_base_dtradioconfig_pack_unpack(void)
{
    dterr_t* dterr = NULL;

    // start with cleared disposable objects
    dtradioconfig_t radioconfig = (dtradioconfig_t){ 0 };
    dtradioconfig_t unpacked_config = (dtradioconfig_t){ 0 };
    uint8_t* buffer = NULL;

    DTERR_C(dtradioconfig_init(&radioconfig));
    DTERR_C(dtradioconfig_init(&unpacked_config));

    // Set known values.
    radioconfig.signature = (DTRADIOCONFIG_SIGNATURE);
    radioconfig.self_node_name = ("Node1");
    radioconfig.wifi_ssid = ("MyWiFi");
    radioconfig.wifi_password = ("secret");
    radioconfig.mqtt_host = ("mqtt://broker");
    radioconfig.mqtt_user = ("user1");
    radioconfig.mqtt_password = ("password1");
    radioconfig.mqtt_port = 1883;
    radioconfig.mqtt_wsport = 9001;

    // Compute required buffer length for packing.
    int32_t pack_len = 0;
    DTERR_C(dtradioconfig_packx_length(&radioconfig, &pack_len));

    buffer = (uint8_t*)malloc(pack_len);
    assert(buffer != NULL);
    memset(buffer, 0, pack_len);

    // Pack the configuration into the buffer.
    int32_t offset = 0;
    DTERR_C(dtradioconfig_packx(&radioconfig, buffer, &offset, pack_len));

    // Now unpack into a new structure.
    int32_t unpack_offset = 0;
    DTERR_C(dtradioconfig_unpackx(&unpacked_config, buffer, &unpack_offset, offset));
    // Verify that all values match.
    DTUNITTEST_ASSERT_EQUAL_STRING(unpacked_config.signature, DTRADIOCONFIG_SIGNATURE);
    DTUNITTEST_ASSERT_EQUAL_STRING(unpacked_config.self_node_name, "Node1");
    DTUNITTEST_ASSERT_EQUAL_STRING(unpacked_config.wifi_ssid, "MyWiFi");
    DTUNITTEST_ASSERT_EQUAL_STRING(unpacked_config.wifi_password, "secret");
    DTUNITTEST_ASSERT_EQUAL_STRING(unpacked_config.mqtt_host, "mqtt://broker");
    DTUNITTEST_ASSERT_EQUAL_STRING(unpacked_config.mqtt_user, "user1");
    DTUNITTEST_ASSERT_EQUAL_STRING(unpacked_config.mqtt_password, "password1");
    DTUNITTEST_ASSERT_INT(unpacked_config.mqtt_port, ==, 1883);
    DTUNITTEST_ASSERT_INT(unpacked_config.mqtt_wsport, ==, 9001);
    DTUNITTEST_ASSERT_INT(unpack_offset, ==, offset);

cleanup:
    if (buffer != NULL)
        free(buffer);

    dtradioconfig_dispose(&unpacked_config);

    return dterr;
}

// --------------------------------------------------------------------------------------------
void
test_dtmc_base_dtradioconfig(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtradioconfig_pack_unpack);
}
