#include <dtcore/dterr.h>
#include <dtcore/dtstr.h>

#include <dtmc_base_demos/demo_helpers.h>

// --------------------------------------------------------------------------------------
// return a string description of the demo (the returned string is heap allocated)
extern dterr_t*
dtmc_base_demo_helpers_decorate_description(void* self, demo_describe_fn describe_fn, char** out_description)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(describe_fn);
    DTERR_ASSERT_NOT_NULL(out_description);

    char* d = *out_description;
    char* s = "\n";

    d = dtstr_concat_format(d, s, "");
    d = dtstr_concat_format(d, s, "----------------------------------------------");
    d = dtstr_concat_format(d, s, "Description of the demo:");
    DTERR_C(describe_fn(self, &d));
    d = dtstr_concat_format(d, s, "----------------------------------------------");

    *out_description = d;

cleanup:
    return dterr;
}
