#include <inttypes.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dttasker.h>

#include <dtcore/dtledger.h>
DTLEDGER_REGISTER(dttasker)

// --------------------------------------------------------------------------------------------
const char*
dttasker_state_to_string(dttasker_status_t status)
{
    switch (status)
    {
        case UNINITIALIZED:
            return "UNINITIALIZED";
        case INITIALIZED:
            return "INITIALIZED";
        case STARTING:
            return "STARTING";
        case RUNNING:
            return "RUNNING";
        case STOPPED:
            return "STOPPED";
        default:
            return "UNKNOWN";
    }
}

// -----------------------------------------------------------------------------
bool
dttasker_priority_enum_is_urgent(dttasker_priority_t p)
{
    return p >= DTTASKER_PRIORITY_URGENT_LOWEST && p < DTTASKER_PRIORITY__COUNT;
}

// -----------------------------------------------------------------------------
bool
dttasker_priority_enum_is_normal(dttasker_priority_t p)
{
    return p >= DTTASKER_PRIORITY_NORMAL_LOWEST && p < DTTASKER_PRIORITY_URGENT_LOWEST;
}

// -----------------------------------------------------------------------------
bool
dttasker_priority_enum_is_background(dttasker_priority_t p)
{
    return p >= DTTASKER_PRIORITY_BACKGROUND_LOWEST && p < DTTASKER_PRIORITY_NORMAL_LOWEST;
}

// -----------------------------------------------------------------------------
bool
dttasker_priority_enum_is_valid(dttasker_priority_t p)
{
    return p >= DTTASKER_PRIORITY_DEFAULT_FOR_SITUATION && p < DTTASKER_PRIORITY__COUNT;
}

// -----------------------------------------------------------------------------
dterr_t*
dttasker_validate_priority_enum(dttasker_priority_t p)
{
    dterr_t* dterr = NULL;
    if (p < 0 || p >= DTTASKER_PRIORITY__COUNT)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "invalid dttasker_priority_t enum value %d", (int32_t)p);
        goto cleanup;
    }
cleanup:
    return dterr;
}

// -----------------------------------------------------------------------------
// Shared string conversion (same across all platforms)
const char*
dttasker_priority_enum_to_string(dttasker_priority_t p)
{
    switch (p)
    {
        case DTTASKER_PRIORITY_DEFAULT_FOR_SITUATION:
            return "DEFAULT_FOR_SITUATION";

        case DTTASKER_PRIORITY_BACKGROUND_LOWEST:
            return "BACKGROUND/LOWEST";
        case DTTASKER_PRIORITY_BACKGROUND_LOW:
            return "BACKGROUND/LOW";
        case DTTASKER_PRIORITY_BACKGROUND_MEDIUM:
            return "BACKGROUND/MEDIUM";
        case DTTASKER_PRIORITY_BACKGROUND_HIGH:
            return "BACKGROUND/HIGH";
        case DTTASKER_PRIORITY_BACKGROUND_HIGHEST:
            return "BACKGROUND/HIGHEST";

        case DTTASKER_PRIORITY_NORMAL_LOWEST:
            return "NORMAL/LOWEST";
        case DTTASKER_PRIORITY_NORMAL_LOW:
            return "NORMAL/LOW";
        case DTTASKER_PRIORITY_NORMAL_MEDIUM:
            return "NORMAL/MEDIUM";
        case DTTASKER_PRIORITY_NORMAL_HIGH:
            return "NORMAL/HIGH";
        case DTTASKER_PRIORITY_NORMAL_HIGHEST:
            return "NORMAL/HIGHEST";

        case DTTASKER_PRIORITY_URGENT_LOWEST:
            return "URGENT/LOWEST";
        case DTTASKER_PRIORITY_URGENT_LOW:
            return "URGENT/LOW";
        case DTTASKER_PRIORITY_URGENT_MEDIUM:
            return "URGENT/MEDIUM";
        case DTTASKER_PRIORITY_URGENT_HIGH:
            return "URGENT/HIGH";
        case DTTASKER_PRIORITY_URGENT_HIGHEST:
            return "URGENT/HIGHEST";

        default:
            return "UNKNOWN";
    }
}
