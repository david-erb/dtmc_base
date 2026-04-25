/*
 * dtmanifold -- Fixed-capacity publish/subscribe message router.
 *
 * Routes dtbuffer_t payloads to multiple named subscribers using a
 * statically allocated subject table (up to 10 subjects, 10 recipients
 * each). An optional external semaphore serializes subscribe/unsubscribe
 * mutations while leaving delivery callbacks unlocked for maximum
 * throughput. Each subscriber receives its own heap copy of the published
 * buffer and is responsible for disposing it.
 *
 * cdox v1.0.2
 */
#pragma once
// See markdown documentation at the end of this file.

#include <stdbool.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dttimeout.h>

#include <dtmc_base/dtsemaphore.h>

// Maximum number of distinct subjects in a ::dtmanifold_t.
#define DTMANIFOLD_MAX_SUBJECTS (10)
// Maximum allowed characters in a subject name (excluding the terminating NUL).
#define DTMANIFOLD_MAX_SUBJECT_LENGTH (64)
// Maximum number of recipients per subject.
#define DTMANIFOLD_MAX_RECIPIENTS (10)

// **Ownership:** The callback takes ownership and must dispose it (e.g., ::dtbuffer_dispose()).
//
// Warning: Callbacks are invoked **without** the manifold’s semaphore held. Coordinate your own synchronization.
// Warning: Contract: The recipient (`recipient_self`) must remain valid during any in-flight ::dtmanifold_publish().
typedef dterr_t* (*dtmanifold_newbuffer_callback_t)(void* recipient_self, const char* subject_name, dtbuffer_t* buffer);

typedef struct
{
    // User context pointer returned to the callback; defines identity for (un)subscribe.
    void* recipient_self;
    // Function invoked on publish; takes ownership of the delivered buffer.
    dtmanifold_newbuffer_callback_t newbuffer_callback;
} dtmanifold_recipient_t;

typedef struct
{
    // Subject name; NUL-terminated. Valid length is `<= ::DTMANIFOLD_MAX_SUBJECT_LENGTH`.
    char subject_name[DTMANIFOLD_MAX_SUBJECT_LENGTH + 1];
    // Fixed array of recipient slots for this subject.
    dtmanifold_recipient_t recipients[DTMANIFOLD_MAX_RECIPIENTS];
} dtmanifold_subject_t;

typedef struct dtmanifold_t
{

    // Fixed array of subject records. Empty entries have `subject_name[0] == '\0'`.
    dtmanifold_subject_t subjects[DTMANIFOLD_MAX_SUBJECTS];
    // Optional: external semaphore to serialize internal mutation; callbacks run unlocked.
    dtsemaphore_handle _semaphore_handle;

    dttimeout_millis_t _semaphore_timeout;

} dtmanifold_t;

// Note: Safe to call on stack/heap instances prior to any other API.
extern dterr_t*
dtmanifold_init(dtmanifold_t* self);

// Note: The semaphore is **not** held during subscriber callbacks; see ::dtmanifold_publish().
extern dterr_t*
dtmanifold_set_threadsafe_semaphore(dtmanifold_t* self,
  dtsemaphore_handle semaphore_handle,
  dttimeout_millis_t semaphore_timeout);

extern dterr_t*
dtmanifold_get_threadsafe_semaphore(dtmanifold_t* self,
  dtsemaphore_handle* semaphore_handle,
  dttimeout_millis_t* semaphore_timeout);

// Warning: Contract: `recipient_self` must remain valid across any in-flight ::dtmanifold_publish().
extern dterr_t*
dtmanifold_subscribe(dtmanifold_t* self,
  const char* subject_name,
  void* recipient_self,
  dtmanifold_newbuffer_callback_t newbuffer_callback);

// Note: No error if the subject or recipient slot is already absent.
// Warning: If other threads can publish concurrently, ensure unsubscription is synchronized with publish.
extern dterr_t*
dtmanifold_unsubscribe(dtmanifold_t* self, const char* subject_name, void* recipient_self);

// Callbacks execute **unlocked** (no semaphore held).
// Warning: **Ownership:** Each callback owns its ::dtbuffer_t and must dispose it.
// Warning: **Lifetime:** `recipient_self` objects must outlive this call’s delivery window.
extern dterr_t*
dtmanifold_publish(dtmanifold_t* self, const char* subject_name, dtbuffer_t* buffer);

// Note: Does not free memory (no dynamic allocation in ::dtmanifold_t); simply clears all fields.
// Call only when no concurrent access is possible.
extern void
dtmanifold_dispose(dtmanifold_t* self);

// -------------------------------------------------------------------------------
// Topic inspection helpers
// -------------------------------------------------------------------------------

typedef bool (*dtmanifold_topic_iter_t)(void* ctx, const char* subject_name, int recipient_count);

dterr_t*
dtmanifold_foreach_topic(dtmanifold_t* self, dtmanifold_topic_iter_t it, void* ctx);

dterr_t*
dtmanifold_subject_recipient_count(dtmanifold_t* self, const char* subject_name, int* count_out);

dterr_t*
dtmanifold_has_subject(dtmanifold_t* self, const char* subject_name, bool* has_out);

#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]
# dtmanifold 

Fixed-capacity, lock-optional pub/sub "manifold" for fan-out delivery of `dtbuffer_t` payloads.  
Provides simple message routing across multiple named subjects with per-subject subscriber lists.

## Mini-guide

- Call `dtmanifold_init()` to zero and prepare a new manifold instance.
- Use `dtmanifold_set_threadsafe_semaphore()` to attach an external semaphore for thread-safe mutation.
- Add subscribers with `dtmanifold_subscribe()` and remove them with `dtmanifold_unsubscribe()`.
- Publish messages using `dtmanifold_publish()`. Each subscriber receives its own `dtbuffer_t` copy.
- Callbacks run **unlocked** — if using threads, ensure your subscribers are safe to call concurrently.
- Subject names are fixed-length and validated; names longer than 64 chars cause a `DTERR_RANGE` error.
- The structure is entirely static — no dynamic allocations beyond those for buffer copies.

## Example

```c
#include <dtmc_base/dtmanifold.h>
#include <stdio.h>

static dterr_t* on_buffer(void* self, const char* subject, dtbuffer_t* buf)
{
    printf("Received on %s: %d bytes\n", subject, (int)buf->length);
    dtbuffer_dispose(buf);
    return NULL;
}

void demo(void)
{
    dtmanifold_t mf;
    dtmanifold_init(&mf);

    // Subscribe
    dtmanifold_subscribe(&mf, "telemetry", (void*)1, on_buffer);

    // Publish
    dtbuffer_t* msg = NULL;
    dtbuffer_create(&msg, 4);
    memcpy(msg->payload, "ping", 4);
    dtmanifold_publish(&mf, "telemetry", msg);

    // Cleanup
    dtbuffer_dispose(msg);
    dtmanifold_dispose(&mf);
}
```

## Functions

### dtmanifold_init

```c
dterr_t* dtmanifold_init(dtmanifold_t* self);
```
Initializes the manifold structure, clearing all subjects and recipients.

**Contracts**
- `self` must not be `NULL`.
- Returns `NULL` on success; otherwise a `dterr_t*` on failure.

### dtmanifold_set_threadsafe_semaphore

```c
dterr_t* dtmanifold_set_threadsafe_semaphore(dtmanifold_t* self,
                                             dtsemaphore_handle semaphore,
                                             dttimeout_millis_t timeout);
```
Assigns a semaphore to serialize internal updates (subscribe/unsubscribe). Callbacks still run unlocked.

### dtmanifold_get_threadsafe_semaphore

```c
dterr_t* dtmanifold_get_threadsafe_semaphore(dtmanifold_t* self,
                                             dtsemaphore_handle* handle_out,
                                             dttimeout_millis_t* timeout_out);
```
Retrieves the currently configured semaphore and timeout.

### dtmanifold_subscribe

```c
dterr_t* dtmanifold_subscribe(dtmanifold_t* self,
                              const char* subject_name,
                              void* recipient_self,
                              dtmanifold_newbuffer_callback_t callback);
```
Registers a recipient under the given subject name.  
If the subject doesn’t exist, it is created automatically.

**Contracts**
- `subject_name` length ≤ 64.
- `recipient_self` and `callback` must be non-null.
- Caller ensures `recipient_self` remains valid during publication.

### dtmanifold_unsubscribe

```c
dterr_t* dtmanifold_unsubscribe(dtmanifold_t* self,
                                const char* subject_name,
                                void* recipient_self);
```
Removes the recipient from a subject. Silent if already absent.

### dtmanifold_publish

```c
dterr_t* dtmanifold_publish(dtmanifold_t* self,
                            const char* subject_name,
                            dtbuffer_t* buffer);
```
Publishes a buffer copy to all subscribers under the subject.

**Behavior**
- Each recipient gets its own freshly allocated `dtbuffer_t` copy.
- Callbacks are invoked **without** holding the semaphore.
- Each callback assumes ownership of its copy and must dispose it.

### dtmanifold_dispose

```c
void dtmanifold_dispose(dtmanifold_t* self);
```
Resets the manifold to zero state. Does not free heap memory — safe to reuse or deallocate the struct itself.

### dtmanifold_foreach_topic

```c
dterr_t* dtmanifold_foreach_topic(dtmanifold_t* self,
                                  dtmanifold_topic_iter_t iterator,
                                  void* ctx);
```
Iterates all current subjects. For each, invokes `iterator(ctx, subject_name, recipient_count)` until it returns false.

### dtmanifold_subject_recipient_count

```c
dterr_t* dtmanifold_subject_recipient_count(dtmanifold_t* self,
                                            const char* subject_name,
                                            int* count_out);
```
Returns the number of current recipients for a subject.

### dtmanifold_has_subject

```c
dterr_t* dtmanifold_has_subject(dtmanifold_t* self,
                                const char* subject_name,
                                bool* has_out);
```
Checks whether a subject exists, regardless of recipient count.

## Data structures

### dtmanifold_t

| Field | Description |
|--------|-------------|
| `subjects` | Fixed array of subjects (10 entries by default). |
| `_semaphore_handle` | Optional external semaphore for serialization. |
| `_semaphore_timeout` | Timeout used with the semaphore. |

### dtmanifold_subject_t

| Field | Description |
|--------|-------------|
| `subject_name` | NUL-terminated name string. |
| `recipients` | Array of `dtmanifold_recipient_t` slots. |

### dtmanifold_recipient_t

| Field | Description |
|--------|-------------|
| `recipient_self` | User-provided context pointer. |
| `newbuffer_callback` | Function invoked with a new buffer copy. |

### dtmanifold_newbuffer_callback_t

```c
typedef dterr_t* (*dtmanifold_newbuffer_callback_t)(void* recipient_self,
                                                    const char* subject_name,
                                                    dtbuffer_t* buffer);
```
Called for every publication to which the subscriber is registered.

## Contracts and behavior

- Table sizes are fixed at compile-time (`10 subjects × 10 recipients each`).
- `dtbuffer_create()` is used for delivery copies; each must be freed by the recipient.
- Overlength subject names yield `DTERR_RANGE`.
- Thread safety is optional and governed by the caller’s semaphore.
- Zero-copy handoff is never used — each subscriber owns its copy fully.


// --8<-- [end:markdown-documentation]
// clang-format on
#endif