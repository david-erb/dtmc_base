#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dttimeout.h>

#include <dtmc_base/dtmanifold.h>
#include <dtmc_base/dtsemaphore.h>

#define TAG "dtmanifold"

/* If you want to enable debug logs, remove this macro.
   We intentionally keep #ifndef dtlog_debug fences around formatted logs
   to avoid formatting cost when debug is disabled. */
#define dtlog_debug(TAG, ...)

/* Compile-time size of subject_name field */
#define DTMANIFOLD_SUBJECT_NAME_CAP (sizeof(((dtmanifold_t*)0)->subjects[0].subject_name))

/* ------------------------------------------------------------------------ */
/* small helpers */

/* Validate subject name:
   - already presumed not NULL
   - must fit without truncation (rejects over-length to avoid ghost topics) */
static inline dterr_t*
validate_subject_name_cstr(const char* subject_name)
{
    size_t len = strlen(subject_name);
    if (len >= DTMANIFOLD_SUBJECT_NAME_CAP)
    {
        return dterr_new(
          DTERR_RANGE, DTERR_LOC, NULL, "subject_name too long (max %u)", (unsigned)(DTMANIFOLD_SUBJECT_NAME_CAP - 1));
    }
    return NULL;
}

/* ------------------------------------------------------------------------ */

static inline dterr_t*
validate_subject_name_param(const char* subject_name)
{
    dterr_t* e = validate_subject_name_cstr(subject_name);
    if (e)
        return dterr_new(e->error_code, DTERR_LOC, e, "invalid subject name");
    return NULL;
}

/* ------------------------------------------------------------------------ */
/* lookup */

/* find subject, return NULL if not found */
static dterr_t*
search_subject(dtmanifold_t* self, const char* subject_name, dtmanifold_subject_t** subject)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(subject_name);
    DTERR_ASSERT_NOT_NULL(subject);

    /* Enforce length rule for consistent equality semantics. */
    DTERR_C(validate_subject_name_param(subject_name));

    *subject = NULL;

    for (int i = 0; i < DTMANIFOLD_MAX_SUBJECTS; i++)
    {
        if (self->subjects[i].subject_name[0] == '\0')
            continue;
        if (strcmp(self->subjects[i].subject_name, subject_name) == 0)
        {
            *subject = &self->subjects[i];
            break;
        }
    }

cleanup:
    return dterr;
}

/* ------------------------------------------------------------------------ */

static dterr_t*
search_recipient(dtmanifold_subject_t* subject, void* recipient_self, dtmanifold_recipient_t** recipient)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(subject);
    DTERR_ASSERT_NOT_NULL(recipient);

    *recipient = NULL;

    for (int i = 0; i < DTMANIFOLD_MAX_RECIPIENTS; i++)
    {
        if (subject->recipients[i].recipient_self == recipient_self)
        {
            *recipient = &subject->recipients[i];
            break;
        }
    }

cleanup:
    return dterr;
}

/* ------------------------------------------------------------------------ */

/* find subject or create a new one */
static dterr_t*
find_or_add_subject(dtmanifold_t* self, const char* subject_name, dtmanifold_subject_t** subject)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(subject);
    DTERR_ASSERT_NOT_NULL(subject_name);

    DTERR_C(validate_subject_name_param(subject_name));

    *subject = NULL;
    dtmanifold_subject_t* free_subject = NULL;

    for (int i = 0; i < DTMANIFOLD_MAX_SUBJECTS; i++)
    {
        if (self->subjects[i].subject_name[0] == '\0')
        {
            if (free_subject == NULL)
            {
                free_subject = &self->subjects[i];
            }
        }
        else if (strcmp(self->subjects[i].subject_name, subject_name) == 0)
        {
            *subject = &self->subjects[i];
            break;
        }
    }

    if (*subject == NULL)
    {
        if (free_subject == NULL)
        {
            dterr = dterr_new(DTERR_OVERFLOW, DTERR_LOC, NULL, "subjects table is full");
            goto cleanup;
        }

        /* safe bounded copy, we already validated length < CAP */
        strncpy(free_subject->subject_name, subject_name, DTMANIFOLD_SUBJECT_NAME_CAP - 1);
        free_subject->subject_name[DTMANIFOLD_SUBJECT_NAME_CAP - 1] = '\0';

        *subject = free_subject;
    }

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(
          dterr->error_code, DTERR_LOC, dterr, "failed to find or create subject \"%s\"", subject_name ? subject_name : "NULL");
    }

    return dterr;
}

/* ------------------------------------------------------------------------ */

/* find recipient or create a new one */
static dterr_t*
find_or_add_recipient(dtmanifold_t* self,
  dtmanifold_subject_t* subject,
  void* recipient_self,
  dtmanifold_recipient_t** recipient)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(subject);
    DTERR_ASSERT_NOT_NULL(recipient);

    *recipient = NULL;
    dtmanifold_recipient_t* free_recipient = NULL;

    for (int i = 0; i < DTMANIFOLD_MAX_RECIPIENTS; i++)
    {
        if (subject->recipients[i].recipient_self == NULL)
        {
            if (free_recipient == NULL)
            {
                free_recipient = &subject->recipients[i];
            }
        }
        else if (subject->recipients[i].recipient_self == recipient_self)
        {
            *recipient = &subject->recipients[i];
            break;
        }
    }

    if (*recipient == NULL)
    {
        if (free_recipient == NULL)
        {
            dterr = dterr_new(DTERR_OVERFLOW, DTERR_LOC, NULL, "recipients table is full");
            goto cleanup;
        }
        free_recipient->recipient_self = recipient_self;
        *recipient = free_recipient;
    }

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(
          dterr->error_code, DTERR_LOC, dterr, "failed to find or create recipient for subject \"%s\"", subject->subject_name);
    }

    return dterr;
}

/* ------------------------------------------------------------------------ */
/* public API */

dterr_t*
dtmanifold_init(dtmanifold_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    memset(self, 0, sizeof(*self));

cleanup:
    return dterr;
}

/* ------------------------------------------------------------------------ */

dterr_t*
dtmanifold_set_threadsafe_semaphore(dtmanifold_t* self, dtsemaphore_handle semaphore_handle, dttimeout_millis_t timeout)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    self->_semaphore_handle = semaphore_handle;
    self->_semaphore_timeout = timeout;

cleanup:
    return dterr;
}

/* ------------------------------------------------------------------------ */

dterr_t*
dtmanifold_get_threadsafe_semaphore(dtmanifold_t* self,
  dtsemaphore_handle* semaphore_handle,
  dttimeout_millis_t* semaphore_timeout)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(semaphore_handle);
    DTERR_ASSERT_NOT_NULL(semaphore_timeout);

    *semaphore_handle = self->_semaphore_handle;
    *semaphore_timeout = self->_semaphore_timeout;

cleanup:
    return dterr;
}

/* ------------------------------------------------------------------------ */

dterr_t*
dtmanifold_subscribe(dtmanifold_t* self,
  const char* subject_name,
  void* recipient_self,
  dtmanifold_newbuffer_callback_t newbuffer_callback)
{
    dterr_t* dterr = NULL;
    bool acquired = false;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(subject_name);
    DTERR_ASSERT_NOT_NULL(recipient_self);
    DTERR_ASSERT_NOT_NULL(newbuffer_callback);

    /* Prevent truncation-induced duplicates. */
    DTERR_C(validate_subject_name_param(subject_name));

    if (self->_semaphore_handle != NULL)
    {
        DTERR_C(dtsemaphore_wait(self->_semaphore_handle, self->_semaphore_timeout, NULL));
        acquired = true;
    }

    dtmanifold_subject_t* subject;
    DTERR_C(find_or_add_subject(self, subject_name, &subject));

    dtmanifold_recipient_t* recipient;
    DTERR_C(find_or_add_recipient(self, subject, recipient_self, &recipient));

    recipient->newbuffer_callback = newbuffer_callback;

cleanup:

    if (acquired)
    {
        dtsemaphore_post(self->_semaphore_handle);
        acquired = false;
    }

    if (dterr != NULL)
    {
        dterr = dterr_new(
          dterr->error_code, DTERR_LOC, dterr, "failed to subscribe to subject \"%s\"", subject_name ? subject_name : "NULL");
    }

    return dterr;
}

/* ------------------------------------------------------------------------ */

dterr_t*
dtmanifold_unsubscribe(dtmanifold_t* self, const char* subject_name, void* recipient_self)
{
    dterr_t* dterr = NULL;
    bool acquired = false;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(subject_name);
    DTERR_ASSERT_NOT_NULL(recipient_self);

    DTERR_C(validate_subject_name_param(subject_name));

    if (self->_semaphore_handle != NULL)
    {
        DTERR_C(dtsemaphore_wait(self->_semaphore_handle, self->_semaphore_timeout, NULL));
        acquired = true;
    }

    dtmanifold_subject_t* subject;
    DTERR_C(search_subject(self, subject_name, &subject));
    if (subject != NULL)
    {
        dtmanifold_recipient_t* recipient;
        DTERR_C(search_recipient(subject, recipient_self, &recipient));

        if (recipient != NULL)
        {
            memset(recipient, 0, sizeof(*recipient));
        }
    }

cleanup:
    if (acquired)
    {
        dtsemaphore_post(self->_semaphore_handle);
        acquired = false;
    }

    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code,
          DTERR_LOC,
          dterr,
          "failed to unsubscribe from subject \"%s\"",
          subject_name ? subject_name : "NULL");
    }

    return dterr;
}

/* ------------------------------------------------------------------------ */

/* NOTE ON LIFETIME:
   Callbacks are invoked UNLOCKED. Implementations MUST guarantee that
   each recipient_self remains valid (not freed) for the entire duration
   of any in-flight publish that could reach it. Unsubscribe should
   synchronize with publish (or use refcount/RCU). */
dterr_t*
dtmanifold_publish(dtmanifold_t* self, const char* subject_name, dtbuffer_t* buffer)
{
    dterr_t* dterr = NULL;
    bool acquired = false;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(subject_name);
    DTERR_ASSERT_NOT_NULL(buffer);

    DTERR_C(validate_subject_name_param(subject_name));

    /* Snapshot recipients under lock, then unlock before callbacks. */
    typedef struct
    {
        void* recipient_self;
        dtmanifold_newbuffer_callback_t cb;
    } recipient_snapshot_t;

    recipient_snapshot_t snaps[DTMANIFOLD_MAX_RECIPIENTS];
    int snap_count = 0;

    if (self->_semaphore_handle != NULL)
    {
        DTERR_C(dtsemaphore_wait(self->_semaphore_handle, self->_semaphore_timeout, NULL));
        acquired = true;
    }

    dtmanifold_subject_t* subject;
    DTERR_C(find_or_add_subject(self, subject_name, &subject));

    for (int i = 0; i < DTMANIFOLD_MAX_RECIPIENTS; i++)
    {
        dtmanifold_recipient_t* r = &subject->recipients[i];
        if (r->recipient_self != NULL && r->newbuffer_callback != NULL)
        {
            if (snap_count < DTMANIFOLD_MAX_RECIPIENTS)
            {
                snaps[snap_count].recipient_self = r->recipient_self;
                snaps[snap_count].cb = r->newbuffer_callback;
                snap_count++;
            }
        }
    }

    /* Release lock before running user callbacks */
    if (acquired)
    {
        dtsemaphore_post(self->_semaphore_handle);
        acquired = false;
    }

#ifndef dtlog_debug
    if (snap_count == 0)
    {
        dtlog_debug(
          TAG, "no recipients on topic \"%s\" (data length %" PRId32 ")", subject_name ? subject_name : "NULL", buffer->length);
    }
#endif

    /* Deliver to each recipient outside the lock */
    for (int i = 0; i < snap_count; i++)
    {
        dtbuffer_t* buffer_copy = NULL;
        DTERR_C(dtbuffer_create(&buffer_copy, buffer->length));
        memcpy(buffer_copy->payload, buffer->payload, buffer->length);

#ifndef dtlog_debug
        dtlog_debug(TAG,
          "publishing on topic \"%s\" to cb %p recipient %p buffer %p length %" PRId32,
          subject_name ? subject_name : "NULL",
          (void*)snaps[i].cb,
          (void*)snaps[i].recipient_self,
          buffer_copy,
          buffer_copy->length);
#endif

        /* Contract: callback takes ownership of buffer_copy, even on error. */
        dterr_t* derr = snaps[i].cb(snaps[i].recipient_self, subject_name, buffer_copy);
        if (derr)
        {
            dterr = dterr_new(derr->error_code,
              DTERR_LOC,
              derr,
              "publish callback failed for subject \"%s\"",
              subject_name ? subject_name : "NULL");
            goto cleanup;
        }
    }

cleanup:
    if (acquired)
    {
        dtsemaphore_post(self->_semaphore_handle);
        acquired = false;
    }

    if (dterr != NULL)
    {
        dterr = dterr_new(
          dterr->error_code, DTERR_LOC, dterr, "failed to publish to subject \"%s\"", subject_name ? subject_name : "NULL");
    }

    return dterr;
}

/* ------------------------------------------------------------------------ */

void
dtmanifold_dispose(dtmanifold_t* self)
{
    if (self == NULL)
        return;

    /* NOTE: Caller must ensure no concurrent use when disposing. */
    memset(self, 0, sizeof(*self));
}

/* ------------------------------------------------------------------------ */

static int
subject_recipient_count(const dtmanifold_subject_t* subject)
{
    int n = 0;
    for (int i = 0; i < DTMANIFOLD_MAX_RECIPIENTS; i++)
        if (subject->recipients[i].recipient_self != NULL)
            n++;
    return n;
}

/* ------------------------------------------------------------------------ */

dterr_t*
dtmanifold_foreach_topic(dtmanifold_t* self, dtmanifold_topic_iter_t it, void* ctx)
{
    dterr_t* dterr = NULL;
    bool acquired = false;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(it);

    /* Snapshot subjects (name + recipient count) under lock, then release and invoke user iterator. */
    char names[DTMANIFOLD_MAX_SUBJECTS][DTMANIFOLD_SUBJECT_NAME_CAP];
    int rcnts[DTMANIFOLD_MAX_SUBJECTS];
    int count = 0;

    if (self->_semaphore_handle != NULL)
    {
        DTERR_C(dtsemaphore_wait(self->_semaphore_handle, self->_semaphore_timeout, NULL));
        acquired = true;
    }

    for (int i = 0; i < DTMANIFOLD_MAX_SUBJECTS; i++)
    {
        dtmanifold_subject_t* s = &self->subjects[i];
        if (s->subject_name[0] == '\0')
            continue;

        if (count < DTMANIFOLD_MAX_SUBJECTS)
        {
            strncpy(names[count], s->subject_name, DTMANIFOLD_SUBJECT_NAME_CAP - 1);
            names[count][DTMANIFOLD_SUBJECT_NAME_CAP - 1] = '\0';

            rcnts[count] = subject_recipient_count(s);
            count++;
        }
    }

    if (acquired)
    {
        dtsemaphore_post(self->_semaphore_handle);
        acquired = false;
    }

    for (int i = 0; i < count; i++)
    {
        bool keep_going = it(ctx, names[i], rcnts[i]);
        if (!keep_going)
            break;
    }

cleanup:
    if (acquired)
    {
        dtsemaphore_post(self->_semaphore_handle);
        acquired = false;
    }

    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "failed to iterate manifold topics");
    }

    return dterr;
}

/* ------------------------------------------------------------------------ */

dterr_t*
dtmanifold_subject_recipient_count(dtmanifold_t* self, const char* subject_name, int* count_out)
{
    dterr_t* dterr = NULL;
    bool acquired = false;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(subject_name);
    DTERR_ASSERT_NOT_NULL(count_out);

    DTERR_C(validate_subject_name_param(subject_name));

    if (self->_semaphore_handle != NULL)
    {
        DTERR_C(dtsemaphore_wait(self->_semaphore_handle, self->_semaphore_timeout, NULL));
        acquired = true;
    }

    dtmanifold_subject_t* subject = NULL;
    dterr_t* derr = search_subject(self, subject_name, &subject);
    if (derr)
    {
        dterr = derr;
        goto cleanup;
    }

    if (!subject || subject->subject_name[0] == '\0')
    {
        *count_out = 0;
        goto cleanup;
    }

    *count_out = subject_recipient_count(subject);

cleanup:
    if (acquired)
    {
        dtsemaphore_post(self->_semaphore_handle);
        acquired = false;
    }

    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "failed to get manifold recipient count");
    }

    return dterr;
}

/* ------------------------------------------------------------------------ */

dterr_t*
dtmanifold_has_subject(dtmanifold_t* self, const char* subject_name, bool* has_out)
{
    dterr_t* dterr = NULL;
    bool acquired = false;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(subject_name);
    DTERR_ASSERT_NOT_NULL(has_out);

    DTERR_C(validate_subject_name_param(subject_name));

    if (self->_semaphore_handle != NULL)
    {
        DTERR_C(dtsemaphore_wait(self->_semaphore_handle, self->_semaphore_timeout, NULL));
        acquired = true;
    }

    dtmanifold_subject_t* subject = NULL;
    DTERR_C(search_subject(self, subject_name, &subject));

    *has_out = (subject != NULL && subject->subject_name[0] != '\0');

cleanup:
    if (acquired)
    {
        dtsemaphore_post(self->_semaphore_handle);
        acquired = false;
    }

    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code,
          DTERR_LOC,
          dterr,
          "failed to interrogate manifold for subject \"%s\"",
          subject_name ? subject_name : "NULL");
    }
    return dterr;
}

/* ------------------------------------------------------------------------ */
