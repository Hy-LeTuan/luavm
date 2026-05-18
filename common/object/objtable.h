#ifndef common_object_objtable_h
#define common_object_objtable_h

#include <object.h>
#include <table.h>

#define IS_TABLE(value) (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_TABLE)
#define AS_TABLE(value) ((ObjTable*)(AS_OBJ(value)))

typedef struct
{
    Object obj;
    Table content;
} ObjTable;

ObjTable* newTable();

#endif
