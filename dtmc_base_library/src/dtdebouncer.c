#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtdebouncer.h>

// --------------------------------------------------------------------------------------------
dterr_t*
dtdebouncer_init(dtdebouncer_t* self,
  bool initial_state,
  dtdebouncer_milliseconds_t debounce_ms,
  dtdebouncer_callback_fn callback_fn,
  void* callback_context,
  dtdebouncer_time_fn time_fn)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(time_fn);

    self->stable_state = initial_state;
    self->candidate_state = initial_state;
    self->candidate_time_ms = time_fn();
    self->debounce_ms = debounce_ms;
    self->callback_fn = callback_fn;
    self->callback_context = callback_context;
    self->time_now_ms = time_fn;

    // diagnostics
    self->raw_edge_count = 0;
    self->debounced_edge_count = 0;
    self->rise_count = 0;
    self->fall_count = 0;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
void
dtdebouncer_update(dtdebouncer_t* self, bool raw_state)
{
    if (!self)
        return;
    if (!self->time_now_ms)
        return;

    dtdebouncer_milliseconds_t now = self->time_now_ms();

    // Raw edge: raw_state changed vs previous raw sample (candidate_state)
    if (raw_state != self->candidate_state)
    {
        self->candidate_state = raw_state;
        self->candidate_time_ms = now;
        self->raw_edge_count++; // count *every* raw transition seen
        return;
    }

    // If candidate differs from accepted state, see if it has been stable long enough
    if (self->candidate_state != self->stable_state)
    {
        if ((now - self->candidate_time_ms) >= self->debounce_ms)
        {
            bool old = self->stable_state;
            bool new = self->candidate_state;

            self->stable_state = new;

            // debounced diagnostics
            self->debounced_edge_count++;
            if (!old && new)
            {
                self->rise_count++;
            }
            else if (old && !new)
            {
                self->fall_count++;
            }

            // notify
            if (self->callback_fn)
            {
                self->callback_fn(self->stable_state, self->callback_context);
            }
        }
    }
}
