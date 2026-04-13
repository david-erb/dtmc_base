#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtringfifo.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtiox_dummy.h>
#include <dtmc_base/dtsemaphore.h>
#include <dtmc_base/dtuart_helpers.h>

#define TAG "dtiox_dummy"

#define dtlog_debug(TAG, ...)

/* ---- vtable -------------------------------------------------------------- */
DTIOX_INIT_VTABLE(dtiox_dummy);

/* ---- object -------------------------------------------------------------- */
struct dtiox_dummy_t
{
    DTIOX_COMMON_MEMBERS
    bool _is_malloced;

    dtiox_dummy_config_t cfg;
    bool enabled;

    dtiox_dummy_behavior_t behavior;

    dtsemaphore_handle rx_semaphore;

    /* Storage owned by object (simple; no locks — single-thread tests) */
    uint8_t* rx_mem;
    uint8_t* tx_mem;
    dtringfifo_t rx_fifo;
    dtringfifo_t tx_fifo;
};

static dterr_t*
_alloc_buffers(dtiox_dummy_t* self, int32_t rx_cap, int32_t tx_cap)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    /* allocate backing storage */
    self->rx_mem = (uint8_t*)malloc(rx_cap);
    if (!self->rx_mem)
        return dterr_new(DTERR_NOMEM, DTERR_LOC, NULL, "rx malloc %" PRId32 "", rx_cap);

    self->tx_mem = (uint8_t*)malloc(tx_cap);
    if (!self->tx_mem)
    {
        free(self->rx_mem);
        self->rx_mem = NULL;
        return dterr_new(DTERR_NOMEM, DTERR_LOC, NULL, "tx malloc %" PRId32 "", tx_cap);
    }

    /* configure FIFOs */
    {
        dtringfifo_config_t rcfg = {
            .buffer = self->rx_mem,
            .capacity = (int32_t)rx_cap,
        };
        dtringfifo_config_t tcfg = {
            .buffer = self->tx_mem,
            .capacity = (int32_t)tx_cap,
        };

        DTERR_C(dtringfifo_configure(&self->rx_fifo, &rcfg));
        DTERR_C(dtringfifo_configure(&self->tx_fifo, &tcfg));
    }

cleanup:
    if (dterr)
    {
        if (self->rx_mem)
        {
            free(self->rx_mem);
            self->rx_mem = NULL;
        }
        if (self->tx_mem)
        {
            free(self->tx_mem);
            self->tx_mem = NULL;
        }
    }
    return dterr;
}

/* ---- lifecycle ----------------------------------------------------------- */
dterr_t*
dtiox_dummy_create(dtiox_dummy_t** self_ptr)
{
    dterr_t* dterr = NULL;
    dtiox_dummy_t* o = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    o = (dtiox_dummy_t*)calloc(1, sizeof(*o));
    if (!o)
        return dterr_new(DTERR_NOMEM, DTERR_LOC, NULL, "calloc %" PRId32 "", sizeof(*o));
    o->_is_malloced = true;

    DTERR_C(dtiox_dummy_init(o));
    *self_ptr = o;

cleanup:
    if (dterr)
    {
        free(o);
        *self_ptr = NULL;
    }
    return dterr;
}

// ----------------------------------------------------------------------------
dterr_t*
dtiox_dummy_init(dtiox_dummy_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    memset(self, 0, sizeof(*self));

    self->model_number = DTMC_BASE_CONSTANTS_IOX_MODEL_DUMMY;

    /* sensible defaults */
    self->cfg.uart_config = dtuart_helper_default_config;
    self->behavior.loopback = true;
    self->behavior.rx_capacity = 1024;
    self->behavior.tx_capacity = 1024;

    /* init FIFOs (no storage yet) */
    DTERR_C(dtringfifo_init(&self->rx_fifo));
    DTERR_C(dtringfifo_init(&self->tx_fifo));

    /* allocate and bind backing buffers */
    DTERR_C(_alloc_buffers(self, self->behavior.rx_capacity, self->behavior.tx_capacity));

    DTERR_C(dtiox_set_vtable(self->model_number, &dtiox_dummy_vt));

cleanup:
    if (dterr)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "dtiox_dummy_init failed");
    return dterr;
}

// ----------------------------------------------------------------------------
dterr_t*
dtiox_dummy_configure(dtiox_dummy_t* self, const dtiox_dummy_config_t* cfg)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(cfg);
    DTERR_ASSERT_NOT_NULL(cfg->name);

    self->cfg = *cfg;

cleanup:
    return dterr;
}

// ----------------------------------------------------------------------------
dterr_t*
dtiox_dummy_set_behavior(dtiox_dummy_t* self, const dtiox_dummy_behavior_t* b)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(b);

    /* if capacities change, reallocate */
    bool need_rx = (self->rx_mem == NULL) || (self->behavior.rx_capacity != b->rx_capacity);
    bool need_tx = (self->tx_mem == NULL) || (self->behavior.tx_capacity != b->tx_capacity);

    self->behavior = *b;

    if (need_rx || need_tx)
    {
        if (self->rx_mem)
            free(self->rx_mem);
        if (self->tx_mem)
            free(self->tx_mem);
        self->rx_mem = self->tx_mem = NULL;

        DTERR_C(_alloc_buffers(self, self->behavior.rx_capacity, self->behavior.tx_capacity));
    }

cleanup:
    return dterr;
}

/* ---- facade ops ---------------------------------------------------------- */
dterr_t*
dtiox_dummy_attach(dtiox_dummy_t* self DTIOX_ATTACH_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
cleanup:
    return dterr;
}

// ----------------------------------------------------------------------------
dterr_t*
dtiox_dummy_detach(dtiox_dummy_t* self DTIOX_DETACH_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
cleanup:
    return dterr;
}

// ----------------------------------------------------------------------------
dterr_t*
dtiox_dummy_enable(dtiox_dummy_t* self DTIOX_ENABLE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    self->enabled = enabled;

cleanup:
    return dterr;
}

// ----------------------------------------------------------------------------
dterr_t*
dtiox_dummy_read(dtiox_dummy_t* self DTIOX_READ_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(buf);
    DTERR_ASSERT_NOT_NULL(out_read);

    if (buf_len == 0)
    {
        *out_read = 0;
        goto cleanup;
    }

    /* dtringfifo uses int32_t lengths */
    int32_t max_len = (buf_len > (int32_t)INT32_MAX) ? (int32_t)INT32_MAX : (int32_t)buf_len;
    int32_t got = dtringfifo_pop(&self->rx_fifo, buf, max_len);
    if (got < 0)
        got = 0;

    *out_read = (int32_t)got;

cleanup:
    return dterr;
}

// ----------------------------------------------------------------------------
dterr_t*
dtiox_dummy_write(dtiox_dummy_t* self DTIOX_WRITE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(buf);
    DTERR_ASSERT_NOT_NULL(out_written);

    if (len == 0)
    {
        *out_written = 0;
        goto cleanup;
    }

    int32_t max_len = (len > (int32_t)INT32_MAX) ? (int32_t)INT32_MAX : (int32_t)len;
    int32_t pushed = dtringfifo_push(&self->tx_fifo, buf, max_len);
    if (pushed < 0)
        pushed = 0;

    int32_t wrote = (int32_t)pushed;

    /* optional loopback: echo to RX FIFO, then notify */
    if (self->behavior.loopback && wrote > 0)
    {
        dtlog_debug(TAG, "loopback %" PRId32 " bytes", wrote);
        int32_t echoed = dtringfifo_push(&self->rx_fifo, buf, (int32_t)wrote);
        (void)echoed; /* dummy ignores RX overflow detail here */
    }

    *out_written = wrote;

    if (wrote < len)
    {
        dterr = dterr_new(DTERR_FAIL,
          DTERR_LOC,
          NULL,
          "tx overflow (cap=%" PRId32 ", asked=%" PRId32 ", wrote=%" PRId32 ")",
          (int32_t)self->behavior.tx_capacity,
          (int32_t)len,
          (int32_t)wrote);
    }

cleanup:
    return dterr;
}

dterr_t*
dtiox_dummy_set_rx_semaphore(dtiox_dummy_t* self DTIOX_SET_RX_SEMAPHORE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    self->rx_semaphore = rx_semaphore;

cleanup:
    return dterr;
}

dterr_t*
dtiox_dummy_concat_format(dtiox_dummy_t* self DTIOX_CONCAT_FORMAT_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_str);

    *out_str = dtstr_concat_format(in_str, separator, "dummy (%" PRId32 ") \"%s\"", self->model_number, self->cfg.name);

cleanup:
    return dterr;
}

void
dtiox_dummy_dispose(dtiox_dummy_t* self)
{
    if (!self)
        return;

    dtringfifo_reset(&self->rx_fifo);
    dtringfifo_reset(&self->tx_fifo);

    if (self->rx_mem)
        free(self->rx_mem);
    if (self->tx_mem)
        free(self->tx_mem);

    if (self->_is_malloced)
    {
        free(self);
    }
    else
    {
        memset(self, 0, sizeof(*self));
    }
}

/* ---- test helper: inject bytes into RX ----------------------------------- */
dterr_t*
dtiox_dummy_push_rx_bytes(dtiox_dummy_t* self, const uint8_t* data, int32_t len)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(data);

    if (len == 0)
        goto cleanup;

    int32_t max_len = (len > (int32_t)INT32_MAX) ? (int32_t)INT32_MAX : (int32_t)len;
    int32_t pushed = dtringfifo_push(&self->rx_fifo, data, max_len);
    if (pushed < 0)
        pushed = 0;

    int32_t wrote = (int32_t)pushed;
    if (wrote < len)
    {
        dterr = dterr_new(DTERR_FAIL,
          DTERR_LOC,
          NULL,
          "rx overflow (cap=%" PRId32 ", asked=%" PRId32 ", wrote=%" PRId32 ")",
          (int32_t)self->behavior.rx_capacity,
          (int32_t)len,
          (int32_t)wrote);
    }

cleanup:
    return dterr;
}
