#include <vm.h>
#include <compiler.h>

void interpret(const char* source)
{
    compile(source);
}
