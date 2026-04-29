#ifndef common_var_reserved_var_h
#define common_var_reserved_var_h

typedef enum
{
    RESERVED_VAR,
    RESERVED_STEP,
    RESERVED_LIMIT
} ReservedVarType;

typedef struct
{
    ReservedVarType type;
    int length;
    const char* name;
} ReservedVar;

extern ReservedVar ReservedVarTable[];

#endif
