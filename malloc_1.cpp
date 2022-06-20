#include <iostream>
#include <unistd.h>

void* smalloc(size_t size)
{
    if((size == 0) || size > 100000000 )
    {
        return NULL;
    }
    void* pointer = sbrk(size);
    if(pointer == (void *)(-1))
    {
        return NULL;
    }
    return pointer;
}