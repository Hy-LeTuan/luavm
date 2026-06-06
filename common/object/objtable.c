#include <object.h>

ObjTable* newTable()
{
    ObjTable* table = ALLOCATE_OBJ(OBJ_TABLE, ObjTable);
    initTable(&table->content);
    return table;
}
