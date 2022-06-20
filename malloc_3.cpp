#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

typedef struct MallocMetadata_t{
    size_t size;
    bool is_free;
    MallocMetadata_t* next;
    MallocMetadata_t* prev;
}MallocMetadata;


size_t global_num_free_blocks = 0;
size_t global_num_free_bytes = 0;
size_t global_num_allocated_blocks = 0;
size_t global_num_allocated_bytes = 0;
size_t global_num_meta_data_bytes = 0;
size_t global_size_meta_data = sizeof(MallocMetadata);


MallocMetadata* allocated_list = NULL;

static MallocMetadata* addBlock(MallocMetadata* last_alloced, size_t size)
{
    MallocMetadata* new_block;
    void* p_ret = sbrk(size+global_size_meta_data);
    if(p_ret == (void*)(-1))
        return NULL;
    new_block = (MallocMetadata*)p_ret;
    if (last_alloced!=NULL)
        last_alloced->next = new_block;
    else
        allocated_list=new_block;
    new_block->size = size;
    new_block->is_free = false;
    new_block->prev = last_alloced;
    global_num_allocated_blocks++;
    global_num_allocated_bytes+=size;
    global_num_meta_data_bytes+=global_size_meta_data;
    return new_block;
}
static MallocMetadata* findFreeBlock(size_t size, MallocMetadata** last_alloced)
{
    MallocMetadata* iterator = allocated_list;
    while (iterator!=NULL)
    {
        if(iterator->is_free == true && iterator->size >= size)
            return iterator;
        *last_alloced = iterator;
        iterator=iterator->next;
    }
    return iterator;
}
void* smalloc(size_t size)
{

    if((size <= 0) || size > 100000000 )
        return NULL;

    MallocMetadata* last_alloced_block = NULL;
    FreeNode* last_free_block = NULL;
    FreeNode* reuse_block = findFreeBlockBySize(size, &last_free_block);
    if(reuse_block == NULL){
           MallocMetadata* block_to_use = findFreeBlock(size, &last_alloced_block);
           block_to_use = addBlock(last_alloced_block, size);
        return (void*)(block_to_use+1); // not found empty space = add block and return
    }
    else {// found empty space
        if(reuse_block->meta_data->size - 128 < size) // cant make split
        {
            global_num_free_blocks--;
            global_num_free_bytes-= reuse_block->meta_data->size;
            MallocMetadata* block_to_use = reuse_block->meta_data;
            block_to_use->is_free = false;
            removeFromFreeList(reuse_block);
            return (void*)(block_to_use +1);
        }
        else // split
        {
            MallocMetadata* block_to_use = splitBlocks(reuse_block , size);
            return (void*)(block_to_use +1);
        }
    }

}

void* scalloc(size_t num, size_t size)
{
    if(size <= 0 || num <= 0)
        return NULL;
    size_t total_size= size*num;
    if(total_size > 10000000)
        return NULL;
    void* ret_pointer = smalloc(total_size);
    memset(ret_pointer, 0, total_size);
    return ret_pointer;

}

void sfree(void* p)
{
    if(p == NULL)
    {
        return;
    }

    MallocMetadata* pointer =(MallocMetadata*)(p);
    pointer = pointer -1;
    if(pointer->is_free == true)
    {
        return;
    }
    pointer->is_free = true;
    global_num_free_blocks++;
    global_num_free_bytes+= pointer->size;
}

void* srealloc(void* oldp , size_t size)
{
    if(size <= 0 || size > 100000000)
        return NULL;
    if(oldp == NULL)
    {
        return smalloc(size);
    }
    MallocMetadata* pointer =(MallocMetadata*)(oldp);
    pointer = pointer -1;
    if(pointer->size >= size)
    {
        return oldp;
    }

    void* newp = smalloc(size);
    if(newp == NULL) {
        return NULL;
    }
    memmove(newp , oldp , pointer->size);
    sfree(oldp);
    return newp;
}


size_t _num_free_blocks(){
    return global_num_free_blocks;
}

size_t _num_free_bytes(){
    return global_num_free_bytes;
}

size_t _num_allocated_blocks()
{
    return global_num_allocated_blocks;
}

size_t _num_allocated_bytes(){
    return global_num_allocated_bytes;
}

size_t _num_meta_data_bytes()
{
    return global_num_meta_data_bytes;
}

size_t _size_meta_data(){
    return global_size_meta_data;
}