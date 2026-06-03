#include <reserved_var.h>

ReservedVar ReservedVarTable[] = {
    [RESERVED_VAR] = { .type = RESERVED_VAR, .length = 9, .name = "@temp_var" },
    [RESERVED_STEP] = { .type = RESERVED_STEP, .length = 10, .name = "@temp_step" },
    [RESERVED_LIMIT] = { .type = RESERVED_LIMIT, .length = 11, .name = "@temp_limit" },
    [RESERVED_ARG] = { .type = RESERVED_ARG, .length = 3, .name = "arg" },
    [RESERVED_FUNCTION] = { .type = RESERVED_FUNCTION, .length = 14, .name = "@temp_function" },
    [RESERVED_STATE] = { .type = RESERVED_STATE, .length = 11, .name = "@temp_state" }
};
