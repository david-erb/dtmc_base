#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtpackx.h>

#include <dtcore/dtunittest.h>

#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtnetportal_shm.h>
#include <dtmc_base/dtpackable_object_distributor.h>

#define TAG "test_dtmc_base_dtpackable_object_distributor"

// pick model numbers that are very unlikely to collide with real library models
#define TEST_DTMC_BASE_DTPACKABLE_OBJECT_DISTRIBUTOR_DUMMY_MODEL_NUMBER 80005

// ------------------------------
// test_dtmc_base_dtpackable_object_distributor_dummy (guidable + packable)
// ------------------------------

typedef struct test_dtmc_base_dtpackable_object_distributor_dummy_t
{
    int32_t model_number;
    dtguid_t guid;
} test_dtmc_base_dtpackable_object_distributor_dummy_t;

static bool test_dtmc_base_dtpackable_object_distributor_dummy_vtables_registered = false;

DTOBJECT_DECLARE_API(test_dtmc_base_dtpackable_object_distributor_dummy);
DTGUIDABLE_DECLARE_API(test_dtmc_base_dtpackable_object_distributor_dummy);
DTPACKABLE_DECLARE_API(test_dtmc_base_dtpackable_object_distributor_dummy);

DTOBJECT_INIT_VTABLE(test_dtmc_base_dtpackable_object_distributor_dummy)
DTGUIDABLE_INIT_VTABLE(test_dtmc_base_dtpackable_object_distributor_dummy)
DTPACKABLE_INIT_VTABLE(test_dtmc_base_dtpackable_object_distributor_dummy)

static dterr_t*
test_dtmc_base_dtpackable_object_distributor_dummy_register_vtables(void)
{
    dterr_t* dterr = NULL;

    if (!test_dtmc_base_dtpackable_object_distributor_dummy_vtables_registered)
    {
        DTERR_C(dtobject_set_vtable(TEST_DTMC_BASE_DTPACKABLE_OBJECT_DISTRIBUTOR_DUMMY_MODEL_NUMBER,
          &test_dtmc_base_dtpackable_object_distributor_dummy_object_vt));
        DTERR_C(dtguidable_set_vtable(TEST_DTMC_BASE_DTPACKABLE_OBJECT_DISTRIBUTOR_DUMMY_MODEL_NUMBER,
          &test_dtmc_base_dtpackable_object_distributor_dummy_guidable_vt));
        DTERR_C(dtpackable_set_vtable(TEST_DTMC_BASE_DTPACKABLE_OBJECT_DISTRIBUTOR_DUMMY_MODEL_NUMBER,
          &test_dtmc_base_dtpackable_object_distributor_dummy_packable_vt));
        test_dtmc_base_dtpackable_object_distributor_dummy_vtables_registered = true;
    }

cleanup:
    return dterr;
}

dterr_t*
test_dtmc_base_dtpackable_object_distributor_dummy_create(test_dtmc_base_dtpackable_object_distributor_dummy_t** self_ptr)
{
    dterr_t* dterr = NULL;

    DTERR_C(dtheaper_alloc_and_zero(sizeof(test_dtmc_base_dtpackable_object_distributor_dummy_t),
      "test_dtmc_base_dtpackable_object_distributor_dummy_t",
      (void**)self_ptr));
    test_dtmc_base_dtpackable_object_distributor_dummy_t* self = *self_ptr;
    self->model_number = TEST_DTMC_BASE_DTPACKABLE_OBJECT_DISTRIBUTOR_DUMMY_MODEL_NUMBER;
    dtguid_generate_sequential(&self->guid);

    DTERR_C(test_dtmc_base_dtpackable_object_distributor_dummy_register_vtables());
cleanup:
    return dterr;
}

// dtobject (minimal)
void
test_dtmc_base_dtpackable_object_distributor_dummy_copy(test_dtmc_base_dtpackable_object_distributor_dummy_t* self,
  test_dtmc_base_dtpackable_object_distributor_dummy_t* that)
{
    (void)self;
    (void)that;
}
void
test_dtmc_base_dtpackable_object_distributor_dummy_dispose(test_dtmc_base_dtpackable_object_distributor_dummy_t* self)
{
    if (self)
    {
        dtheaper_free(self);
    }
}
bool
test_dtmc_base_dtpackable_object_distributor_dummy_equals(test_dtmc_base_dtpackable_object_distributor_dummy_t* self,
  test_dtmc_base_dtpackable_object_distributor_dummy_t* that)
{
    return self == that;
}
const char*
test_dtmc_base_dtpackable_object_distributor_dummy_get_class(test_dtmc_base_dtpackable_object_distributor_dummy_t* self)
{
    (void)self;
    return "test_dtmc_base_dtpackable_object_distributor_dummy";
}
bool
test_dtmc_base_dtpackable_object_distributor_dummy_is_iface(test_dtmc_base_dtpackable_object_distributor_dummy_t* self,
  const char* iface_name)
{
    (void)self;
    return iface_name && strcmp(iface_name, "test_dtmc_base_dtpackable_object_distributor_dummy") == 0;
}
void
test_dtmc_base_dtpackable_object_distributor_dummy_to_string(test_dtmc_base_dtpackable_object_distributor_dummy_t* self,
  char* buffer,
  size_t buffer_size)
{
    if (!buffer || buffer_size == 0)
        return;
    (void)self;
    snprintf(buffer, buffer_size, "test_dtmc_base_dtpackable_object_distributor_dummy");
}

// dtguidable
dterr_t*
test_dtmc_base_dtpackable_object_distributor_dummy_get_guid(test_dtmc_base_dtpackable_object_distributor_dummy_t* self,
  dtguid_t* guid)
{
    dterr_t* dterr = NULL;

    if (self == NULL || guid == NULL)
        return dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "null arg");

    dtguid_copy(guid, &self->guid);
    return dterr;
}

// dtpackable (minimal but real: packs model_number + guid)
dterr_t*
test_dtmc_base_dtpackable_object_distributor_dummy_packx_length(test_dtmc_base_dtpackable_object_distributor_dummy_t* self,
  int32_t* length)
{
    (void)self;
    if (length == NULL)
        return dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "null arg");

    *length = dtpackx_pack_int32_length() + dtguid_pack_length();
    return NULL;
}

dterr_t*
test_dtmc_base_dtpackable_object_distributor_dummy_packx(test_dtmc_base_dtpackable_object_distributor_dummy_t* self,
  uint8_t* output,
  int32_t* offset,
  int32_t length)
{
    dterr_t* dterr = NULL;
    int32_t p = offset ? *offset : 0;

    if (self == NULL || output == NULL || offset == NULL)
        return dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "null arg");

    int32_t n = dtpackx_pack_int32(self->model_number, output, p, length);
    if (n < 0)
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "pack int32 failed");
    p += n;

    n = dtguid_pack(&self->guid, output, p, length);
    if (n < 0)
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "pack guid failed");
    p += n;

    *offset = p;
    return dterr;
}

// not needed by these tests, but provide a stub to satisfy the vtable
dterr_t*
test_dtmc_base_dtpackable_object_distributor_dummy_unpackx(test_dtmc_base_dtpackable_object_distributor_dummy_t* self,
  const uint8_t* input,
  int32_t* offset,
  int32_t length)
{
    dterr_t* dterr = NULL;
    int32_t p = offset ? *offset : 0;

    if (self == NULL || input == NULL || offset == NULL)
        return dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "null arg");

    int32_t model = 0;

    // Mirror packx(): model_number then guid.
    int32_t n = dtpackx_unpack_int32(input, p, length, &model);
    if (n < 0)
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "unpack int32 failed");
    p += n;

    dtguid_t guid = { 0 };
    n = dtguid_unpack(&guid, input, p, length);
    if (n < 0)
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "unpack guid failed");
    p += n;

    self->model_number = model;
    dtguid_copy(&self->guid, &guid);

    *offset = p;
    return dterr;
}

// not needed by these tests, but provide a stub to satisfy the vtable
dterr_t*
test_dtmc_base_dtpackable_object_distributor_dummy_validate_unpacked(test_dtmc_base_dtpackable_object_distributor_dummy_t* self)
{
    (void)self;
    return dterr_new(DTERR_NOTIMPL, DTERR_LOC, NULL, "validated_unpacked not implemented in test model");
}

// --------------------------------------------------------------------------------------------
// Unit test: Ensure the btle client records method calls
static dterr_t*
test_dtmc_base_dtpackable_object_distributor_shm(void)
{
    dterr_t* dterr = NULL;
    dtnetportal_handle netportal_handle = NULL;

    dtbufferqueue_handle bufferqueue_handle = NULL;
    dtbufferqueue_handle tx_bufferqueue_handle = NULL;

    dtpackable_object_distributor_t _tx_distributor = { 0 }, *tx_distributor = &_tx_distributor;
    dtpackable_object_distributor_t _rx_distributor = { 0 }, *rx_distributor = &_rx_distributor;

    dtobject_handle dummy1_handle = NULL;

    {
        test_dtmc_base_dtpackable_object_distributor_dummy_t* o = NULL;
        DTERR_C(test_dtmc_base_dtpackable_object_distributor_dummy_create(&o));
        dummy1_handle = (dtobject_handle)o;
    }
    {
        dtnetportal_shm_t* o = NULL;
        DTERR_C(dtnetportal_shm_create(&o));
        netportal_handle = (dtnetportal_handle)o;
        dtnetportal_shm_config_t c = { 0 };
        DTERR_C(dtnetportal_shm_configure(o, &c));
    }

    // bufferqueue shared by both distributors

    // due to chunking, we may need several buffers to reassemble one large message
    DTERR_C(dtbufferqueue_create(&bufferqueue_handle, 10, false));

    {
        dtpackable_object_distributor_init(rx_distributor);
        dtpackable_object_distributor_config_t c = { 0 };
        c.netportal_handle = netportal_handle;
        c.bufferqueue_handle = bufferqueue_handle;
        c.topic = "test/topic";
        DTERR_C(dtpackable_object_distributor_configure(rx_distributor, &c));
    }

    {
        dtpackable_object_distributor_init(tx_distributor);
        dtpackable_object_distributor_config_t c = { 0 };
        c.netportal_handle = netportal_handle;
        c.bufferqueue_handle = bufferqueue_handle;
        c.topic = "test/topic";
        DTERR_C(dtpackable_object_distributor_configure(tx_distributor, &c));
    }

    DTERR_C(dtpackable_object_distributor_activate(rx_distributor));

    DTERR_C(dtpackable_object_distributor_distribute(tx_distributor, dummy1_handle));

    DTERR_C(dtpackable_object_distributor_collect(rx_distributor, &dummy1_handle, 1000));

cleanup:

    dtobject_dispose(dummy1_handle);

    dtpackable_object_distributor_dispose(tx_distributor);

    dtpackable_object_distributor_dispose(rx_distributor);

    dtbufferqueue_dispose(tx_bufferqueue_handle);

    dtbufferqueue_dispose(bufferqueue_handle);

    dtnetportal_dispose(netportal_handle);

    if (dterr != NULL)
    {
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "test_distributing_buffer failed");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------
void
test_dtmc_base_dtpackable_object_distributor(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtpackable_object_distributor_shm);
}
