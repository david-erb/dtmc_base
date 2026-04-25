#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtnvblob.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_dtnvblob"

//------------------------------------------------------------------------
// Example: minimal write and read back
dterr_t*
test_dtmc_base_dtnvblob_example_write_read(dtnvblob_handle nvblob_handle)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* write_buffer = NULL;
    dtbuffer_t* read_buffer = NULL;

    int32_t buffer_write_size = 32;
    DTERR_C(dtbuffer_create(&write_buffer, buffer_write_size));
    strcpy((char*)write_buffer->payload, "nvblob test data");

    DTERR_C(dtnvblob_write(nvblob_handle, write_buffer->payload, &buffer_write_size));
    DTUNITTEST_ASSERT_INT(buffer_write_size, ==, write_buffer->length);

    // check the size to read back
    int32_t buffer_read_size = 0;
    DTERR_C(dtnvblob_read(nvblob_handle, NULL, &buffer_read_size));
    DTUNITTEST_ASSERT_INT(buffer_read_size, ==, buffer_write_size);

    // read back into a new buffer
    DTERR_C(dtbuffer_create(&read_buffer, buffer_read_size));
    DTERR_C(dtnvblob_read(nvblob_handle, read_buffer->payload, &buffer_read_size));
    DTUNITTEST_ASSERT_INT(buffer_read_size, ==, read_buffer->length);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)read_buffer->payload, "nvblob test data");

cleanup:
    dtbuffer_dispose(read_buffer);
    dtbuffer_dispose(write_buffer);
    return dterr;
}
