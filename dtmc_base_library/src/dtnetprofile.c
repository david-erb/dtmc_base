// dtnetprofile.c
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtpackx.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtnetprofile.h>

/* ============================================================
   Internal helpers — newline-separated single string per list.
   Storage example: "a\nb\nc\0"
   ============================================================ */

static size_t
str_bytes_or_one(const char* s)
/* Bytes needed including trailing '\0'. For NULL, return 1. */
{
    return s ? (strlen(s) + 1) : 1;
}

static int
list_is_empty(const char* list)
{
    return (!list || *list == '\0');
}

static void
list_add(char** plist, const char* s)
/* Append s to a newline-separated list. */
{
    // TODO: Handle memory allocation failures in dtnetprofile_add_*() functions.
    *plist = dtstr_concat_format(*plist, "\n", "%s", s);
}

static void
buf_vappend(char* out, int out_size, int* used, const char* fmt, va_list ap)
{
    if (!out || out_size <= 0 || !used || *used >= out_size)
        return;
    int space = out_size - *used;
    int n = vsnprintf(out + *used, (size_t)space, fmt, ap);
    if (n < 0)
        return;
    if (n >= space)
    {
        *used = out_size - 1;
    }
    else
    {
        *used += n;
    }
}

static void
buf_append(char* out, int out_size, int* used, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    buf_vappend(out, out_size, used, fmt, ap);
    va_end(ap);
}

/* Iterate newline-separated tokens. */
static int
next_token_nl(const char** pp, const char** tok, size_t* len)
{
    const char* p = *pp;
    if (!p || *p == '\0')
        return 0;

    const char* start = p;
    while (*p && *p != '\n')
        ++p;

    *tok = start;
    *len = (size_t)(p - start);

    *pp = (*p == '\n') ? p + 1 : p;
    return 1;
}

static void
append_list_line(char* out, int out_size, int* used, const char* label, const char* list)
{
    if (list_is_empty(list))
        return;

    buf_append(out, out_size, used, "    %s: ", label);

    const char* p = list;
    const char* tok;
    size_t len;
    int first = 1;

    while (next_token_nl(&p, &tok, &len))
    {
        if (!first)
            buf_append(out, out_size, used, ", ");
        buf_append(out, out_size, used, "%.*s", (int)len, tok);
        first = 0;
    }
    buf_append(out, out_size, used, "\n");
}

/* ============================================================
   Public API
   ============================================================ */

void
dtnetprofile_add_name(dtnetprofile_t* self, const char* name)
{
    list_add(&self->name_list, name);
}
void
dtnetprofile_add_version(dtnetprofile_t* self, const char* component, const char* version)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "%s=%s", component ? component : "(null)", version ? version : "(null)");
    list_add(&self->version_list, buf);
}
void
dtnetprofile_add_url(dtnetprofile_t* self, const char* url)
{
    list_add(&self->url_list, url);
}
void
dtnetprofile_add_role(dtnetprofile_t* self, const char* role)
{
    list_add(&self->role_list, role);
}
void
dtnetprofile_add_subscription(dtnetprofile_t* self, const char* subscription)
{
    list_add(&self->subscription_list, subscription);
}
void
dtnetprofile_add_publication(dtnetprofile_t* self, const char* publication)
{
    list_add(&self->publication_list, publication);
}
void
dtnetprofile_add_state(dtnetprofile_t* self, const char* state)
{
    list_add(&self->state_list, state);
}

void
dtnetprofile_get(dtnetprofile_t* self, const char* list, int index, const char** value)
{
    (void)self;
    *value = NULL;
    if (!list || index < 0)
        return;

    const char* p = list;
    const char* tok = NULL;
    size_t len = 0;
    int i = 0;

    while (next_token_nl(&p, &tok, &len))
    {
        if (i == index)
        {
            static char* scratch = NULL;
            static size_t cap = 0;
            if (len + 1 > cap)
            {
                size_t newcap = (len + 1) < 64 ? 64 : (len + 1);
                char* nb = (char*)realloc(scratch, newcap);
                if (!nb)
                    return;
                scratch = nb;
                cap = newcap;
            }
            memcpy(scratch, tok, len);
            scratch[len] = '\0';
            *value = scratch;
            return;
        }
        ++i;
    }
}

void
dtnetprofile_to_string(const dtnetprofile_t* self, char* out_string, int out_size)
{
    if (!out_string || out_size <= 0)
        return;
    out_string[0] = '\0';
    int used = 0;

    append_list_line(out_string, out_size, &used, "names", self->name_list);
    append_list_line(out_string, out_size, &used, "versions", self->version_list);
    append_list_line(out_string, out_size, &used, "urls", self->url_list);
    append_list_line(out_string, out_size, &used, "roles", self->role_list);
    append_list_line(out_string, out_size, &used, "subscriptions", self->subscription_list);
    append_list_line(out_string, out_size, &used, "publications", self->publication_list);
    append_list_line(out_string, out_size, &used, "states", self->state_list);

    if (used == 0)
        buf_append(out_string, out_size, &used, "(empty)\n");

    if (used > 0 && out_string[used - 1] == '\n')
        out_string[used - 1] = '\0'; // trim final newline
}

void
dtnetprofile_dispose(dtnetprofile_t* self)
{
    dtstr_dispose(self->name_list);
    self->name_list = NULL;
    dtstr_dispose(self->version_list);
    self->version_list = NULL;
    dtstr_dispose(self->url_list);
    self->url_list = NULL;
    dtstr_dispose(self->role_list);
    self->role_list = NULL;
    dtstr_dispose(self->subscription_list);
    self->subscription_list = NULL;
    dtstr_dispose(self->publication_list);
    self->publication_list = NULL;
    dtstr_dispose(self->state_list);
    self->state_list = NULL;
}

int32_t
dtnetprofile_pack_length(const dtnetprofile_t* self)
{
    return                                        //
      str_bytes_or_one(self->name_list) +         //
      str_bytes_or_one(self->version_list) +      //
      str_bytes_or_one(self->url_list) +          //
      str_bytes_or_one(self->role_list) +         //
      str_bytes_or_one(self->subscription_list) + //
      str_bytes_or_one(self->publication_list) +  //
      str_bytes_or_one(self->state_list);
}
int32_t
dtnetprofile_pack(const dtnetprofile_t* self, uint8_t* output, int32_t offset, int32_t length)
{
    int32_t p = offset;
    const char* empty = "";
    int32_t n = 0;

    n = dtpackx_pack_string(self->name_list ? self->name_list : empty, output, p, length);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_pack_string(self->version_list ? self->version_list : empty, output, p, length);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_pack_string(self->url_list ? self->url_list : empty, output, p, length);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_pack_string(self->role_list ? self->role_list : empty, output, p, length);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_pack_string(self->subscription_list ? self->subscription_list : empty, output, p, length);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_pack_string(self->publication_list ? self->publication_list : empty, output, p, length);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_pack_string(self->state_list ? self->state_list : empty, output, p, length);
    if (n < 0)
        return -1;
    p += n;

    return p - offset;
}

int32_t
dtnetprofile_unpack(dtnetprofile_t* self, const uint8_t* input, int32_t offset, int32_t length)
{
    int32_t p = offset;
    int32_t n = 0;

    n = dtpackx_unpack_string(input, p, length, &self->name_list);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_unpack_string(input, p, length, &self->version_list);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_unpack_string(input, p, length, &self->url_list);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_unpack_string(input, p, length, &self->role_list);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_unpack_string(input, p, length, &self->subscription_list);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_unpack_string(input, p, length, &self->publication_list);
    if (n < 0)
        return -1;
    p += n;

    n = dtpackx_unpack_string(input, p, length, &self->state_list);
    if (n < 0)
        return -1;
    p += n;

    return p - offset;
}
