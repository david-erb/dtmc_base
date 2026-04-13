#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>

#include <dtmc_base/dtmanifold.h>
#include <dtmc_base/dtsemaphore.h>

#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtnetportal_shm.h>

DTNETPORTAL_INIT_VTABLE(dtnetportal_shm);
DTOBJECT_INIT_VTABLE(dtnetportal_shm);

#define dtlog_debug(TAG, ...)

#define TAG "dtnetportal_shm"

// --------------------------------------------------------------------------------------
typedef struct dtnetportal_shm_t
{
    DTNETPORTAL_COMMON_MEMBERS;

    dtnetportal_shm_config_t config;

    dtmanifold_t _manifold, *manifold; // manifold for managing topics

    dtbuffer_t* pending;

    // used when we self-receive
    dtsemaphore_handle receive_semaphore_handle;

    bool _is_malloced;

} dtnetportal_shm_t;

extern dtnetportal_shm_t* static_self;

// --------------------------------------------------------------------------------------
extern dterr_t*
dtnetportal_shm_create(dtnetportal_shm_t** self_ptr)
{
    dterr_t* dterr = NULL;

    *self_ptr = (dtnetportal_shm_t*)malloc(sizeof(dtnetportal_shm_t));
    if (*self_ptr == NULL)
    {
        dterr = dterr_new(
          DTERR_NOMEM, DTERR_LOC, NULL, "failed to allocate %zu bytes for dtnetportal_shm_t", sizeof(dtnetportal_shm_t));
        goto cleanup;
    }

    DTERR_C(dtnetportal_shm_init(*self_ptr));

    (*self_ptr)->_is_malloced = true;

cleanup:

    if (dterr != NULL)
    {
        if (*self_ptr != NULL)
        {
            free(*self_ptr);
        }

        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "%s(): failed", __func__);
    }
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_shm_init(dtnetportal_shm_t* self)
{
    dterr_t* dterr = NULL;

    memset(self, 0, sizeof(*self));
    self->model_number = DTMC_BASE_CONSTANTS_NETPORTAL_MODEL_SHM;

    // set the vtable for this model number
    DTERR_C(dtnetportal_set_vtable(self->model_number, &dtnetportal_shm_vt));
    DTERR_C(dtobject_set_vtable(self->model_number, &dtnetportal_shm_object_vt));

    self->manifold = &self->_manifold;
    DTERR_C(dtmanifold_init(self->manifold));

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "%s(): failed", __func__);
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_shm_configure(dtnetportal_shm_t* self, dtnetportal_shm_config_t* config)
{
    dterr_t* dterr = NULL;
    if (config == NULL)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "%s(): called with NULL config", __func__);
        goto cleanup;
    }

    self->config = *config;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_shm_activate(dtnetportal_shm_t* self DTNETPORTAL_ACTIVATE_ARGS)
{
    dterr_t* dterr = NULL;

    goto cleanup;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_shm_subscribe(dtnetportal_shm_t* self DTNETPORTAL_SUBSCRIBE_ARGS)
{
    dterr_t* dterr = NULL;

    // TODO: Consider checking dtnetportal_receive_callback_t is same as dtmanifold_newbuffer_callback_t.
    DTERR_C(dtmanifold_subscribe(self->manifold, topic, recipient_self, receive_callback));

    dtlog_debug(TAG, "subscribed to topic \"%s\"", topic);

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "%s(): failed", __func__);
    }
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_shm_publish(dtnetportal_shm_t* self DTNETPORTAL_PUBLISH_ARGS)
{
    dterr_t* dterr = NULL;

    DTERR_C(dtmanifold_publish(self->manifold, topic, buffer));

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "%s(): failed for topic \"%s\"", __func__, topic);
    }

    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_shm_get_info(dtnetportal_shm_t* self, dtnetportal_info_t* info)
{
    dterr_t* dterr = NULL;
    if (info == NULL)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "called with NULL info");
        goto cleanup;
    }

    memset(info, 0, sizeof(*info));

    snprintf(info->listening_origin, sizeof(info->listening_origin), "shm");
    info->listening_origin[sizeof(info->listening_origin) - 1] = '\0';

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to obtain netportal info");

    return dterr;
}

// --------------------------------------------------------------------------------------
void
dtnetportal_shm_dispose(dtnetportal_shm_t* self)
{
    if (self == NULL)
        return;

    if (self->_is_malloced)
    {
        free(self);
    }
    else
    {
        memset(self, 0, sizeof(*self));
    }
}

// --------------------------------------------------------------------------------------------
// dtobject implementation
// --------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------
// Copy constructor
void
dtnetportal_shm_copy(dtnetportal_shm_t* this, dtnetportal_shm_t* that)
{
    // this object does not support copying
    (void)this;
    (void)that;
}

// --------------------------------------------------------------------------------------------
// Equality check
bool
dtnetportal_shm_equals(dtnetportal_shm_t* a, dtnetportal_shm_t* b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }

    // TODO: Reconside equality semantics for dtnetportal_shm_equals backend.
    return (a->model_number == b->model_number);
}

// --------------------------------------------------------------------------------------------
const char*
dtnetportal_shm_get_class(dtnetportal_shm_t* self)
{
    return "dtnetportal_shm_t";
}

// --------------------------------------------------------------------------------------------

bool
dtnetportal_shm_is_iface(dtnetportal_shm_t* self, const char* iface_name)
{
    return strcmp(iface_name, DTNETPORTAL_IFACE_NAME) == 0 || //
           strcmp(iface_name, "dtobject_iface") == 0;
}

// --------------------------------------------------------------------------------------------
// Convert to string
void
dtnetportal_shm_to_string(dtnetportal_shm_t* self, char* buffer, size_t buffer_size)
{
    if (self == NULL || buffer == NULL || buffer_size == 0)
        return;

    strncpy(buffer, "dtnetportal_shm_t", buffer_size);
    buffer[buffer_size - 1] = '\0';
}
