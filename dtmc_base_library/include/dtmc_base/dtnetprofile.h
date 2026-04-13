/*
 * dtnetprofile -- Network node metadata record with incremental assembly and serialization.
 *
 * Accumulates descriptive attributes for a networked node into newline-
 * separated string lists covering names, component versions, URLs, roles,
 * subscriptions, publications, and states. Provides pack/unpack for binary
 * transport and a to_string formatter for diagnostics. The incremental add
 * API lets each subsystem append its own entries without pre-allocating a
 * fixed-size structure.
 *
 * cdox v1.0.2
 */
#pragma once

typedef struct dtnetprofile_t
{
    char* name_list;
    char* version_list;
    char* url_list;
    char* role_list;
    char* subscription_list;
    char* publication_list;
    char* state_list;
} dtnetprofile_t;

extern void
dtnetprofile_add_name(dtnetprofile_t* self, const char* name);
extern void
dtnetprofile_add_version(dtnetprofile_t* self, const char* component, const char* version);
extern void
dtnetprofile_add_url(dtnetprofile_t* self, const char* url);
extern void
dtnetprofile_add_role(dtnetprofile_t* self, const char* role);
extern void
dtnetprofile_add_subscription(dtnetprofile_t* self, const char* subscription);
extern void
dtnetprofile_add_publication(dtnetprofile_t* self, const char* publication);
extern void
dtnetprofile_add_state(dtnetprofile_t* self, const char* state);

extern void
dtnetprofile_get(dtnetprofile_t* self, const char* list, int index, const char** value);

// String Conversion
extern void
dtnetprofile_to_string(const dtnetprofile_t* self, char* out_string, int out_size);

// Length
extern int32_t
dtnetprofile_pack_length(const dtnetprofile_t* self);

// Packing/Unpacking
extern int32_t
dtnetprofile_pack(const dtnetprofile_t* self, uint8_t* output, int32_t offset, int32_t length);

extern int32_t
dtnetprofile_unpack(dtnetprofile_t* self, const uint8_t* input, int32_t offset, int32_t length);

extern void
dtnetprofile_dispose(dtnetprofile_t* self);

#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]

# dtnetprofile

Aggregates network-related metadata into structured profiles.

This group of functions provides aggregation and serialization for network profile metadata.
It is intended for assembling descriptive attributes for transport or diagnostic exchange.
It is designed to keep representation simple and explicit.
The implementation favors newline-separated string lists and explicit lifecycle control.

## Mini-guide

- Accumulate profile attributes incrementally by calling the add functions for each category.
- Retrieve individual entries from a specific list by index using the getter.
- Convert the aggregated content to a readable string by providing a caller-owned buffer.
- Determine packed size before serialization by calling the length function with the packer.
- Serialize and deserialize the profile in a fixed field order by using the pack and unpack functions.
- Release all internally allocated storage by calling the dispose function when finished.

## Example

```c
dtnetprofile_t profile = {0};

dtnetprofile_add_name(&profile, "node-a");
dtnetprofile_add_version(&profile, "core", "1.2.3");
dtnetprofile_add_url(&profile, "https://example.net");

char buf[256];
dtnetprofile_to_string(&profile, buf, sizeof(buf));

dtnetprofile_dispose(&profile);
```

## Data structures

### dtnetprofile_t

Holds newline-separated string lists describing a network profile.

Members:

> `char* name_list`  Stores profile names.  
> `char* version_list`  Stores component version pairs.  
> `char* url_list`  Stores related URLs.  
> `char* role_list`  Stores assigned roles.  
> `char* subscription_list`  Stores subscription identifiers.  
> `char* publication_list`  Stores publication identifiers.  
> `char* state_list`  Stores state descriptors.  

## Functions

### dtnetprofile_add_name

Appends a name entry to the profile.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const char* name`  Name string to append.  

Return: `void`  No return value.

### dtnetprofile_add_publication

Appends a publication entry to the profile.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const char* publication`  Publication string to append.  

Return: `void`  No return value.

### dtnetprofile_add_role

Appends a role entry to the profile.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const char* role`  Role string to append.  

Return: `void`  No return value.

### dtnetprofile_add_state

Appends a state entry to the profile.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const char* state`  State string to append.  

Return: `void`  No return value.

### dtnetprofile_add_subscription

Appends a subscription entry to the profile.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const char* subscription`  Subscription string to append.  

Return: `void`  No return value.

### dtnetprofile_add_url

Appends a URL entry to the profile.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const char* url`  URL string to append.  

Return: `void`  No return value.

### dtnetprofile_add_version

Appends a component version entry to the profile.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const char* component`  Component identifier.  
> `const char* version`  Version string.  

Return: `void`  No return value.

### dtnetprofile_dispose

Releases all storage owned by the profile.

Params:

> `dtnetprofile_t* self`  Target profile instance.  

Return: `void`  No return value.

### dtnetprofile_get

Retrieves a value from a specified list by index.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const char* list`  Newline-separated list string.  
> `int index`  Zero-based index of the requested entry.  
> `const char** value`  Receives the extracted value pointer.  

Return: `void`  No return value.

### dtnetprofile_pack

Serializes the profile into a buffer using the packer.

Params:

> `const dtnetprofile_t* self`  Source profile instance.  
> `const dtpacker_t* packer`  String packer interface.  
> `uint8_t* output`  Destination buffer.  
> `int32_t offset`  Start offset in the buffer.  
> `int32_t length`  Available buffer length.  

Return: `int32_t`  Number of bytes written.

### dtnetprofile_pack_length

Calculates the required buffer size for serialization.

Params:

> `const dtnetprofile_t* self`  Source profile instance.  
> `const dtpacker_t* packer`  String packer interface.  

Return: `int32_t`  Required packed length.

### dtnetprofile_to_string

Formats the profile content into a human-readable string.

Params:

> `const dtnetprofile_t* self`  Source profile instance.  
> `char* out_string`  Destination string buffer.  
> `int out_size`  Size of the destination buffer.  

Return: `void`  No return value.

### dtnetprofile_unpack

Deserializes the profile from a buffer using the packer.

Params:

> `dtnetprofile_t* self`  Target profile instance.  
> `const dtpacker_t* packer`  String packer interface.  
> `const uint8_t* input`  Source buffer.  
> `int32_t offset`  Start offset in the buffer.  
> `int32_t length`  Available buffer length.  

Return: `int32_t`  Number of bytes consumed.

<!-- FG_IDC: 5ef1b0a9-af3f-475f-b8f8-b099ba4a3c0a | FG_UTC: 2026-01-17T13:11:59Z FG_MAN=yes -->

// --8<-- [end:markdown-documentation]
// clang-format on
#endif
