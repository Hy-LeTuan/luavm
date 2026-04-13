#ifndef common_object_objstring_h
#define common_object_objstring_h

#include <object.h>
#include <table.h>

#define IS_STRING(value) (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_STRING)
#define AS_STRING(value) ((ObjString*)(AS_OBJ(value)))

typedef struct
{
    Object obj;
    size_t length;
    char* chars;
} ObjString;

ObjString* copyString(const char* const_chars, int length, Table* strings);
ObjString* takeString(char* chars, int length, Table* strings);

ObjString* concatenateString(ObjString* a, ObjString* b, Table* strings);

#endif
