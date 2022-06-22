#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define MMAPTHRESHOLD 131072
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))


typedef struct MallocMetadata_t MallocMetadata;

typedef struct freeNode_t{
    MallocMetadata* meta_data;
    freeNode_t* next;
    freeNode_t* prev;
}FreeNode;

struct MallocMetadata_t{
    size_t size;
    bool is_free;
    MallocMetadata_t* next;
    MallocMetadata_t* prev;
    FreeNode free_node;
};

size_t global_num_free_blocks = 0;
size_t global_num_free_bytes = 0;
size_t global_num_allocated_blocks = 0;
size_t global_num_allocated_bytes = 0;
size_t global_num_meta_data_bytes = 0;
size_t global_size_meta_data = sizeof(MallocMetadata);





FreeNode* free_list = NULL;
MallocMetadata* allocated_list = NULL;

MallocMetadata* mmap_allocated_list = NULL;

//aligning beginning and size
static MallocMetadata* addBlock(MallocMetadata* last_alloced, size_t size)
{
    size = ALIGN(size);
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

static FreeNode * findFreeBlockBySize(size_t size, FreeNode** prev)
{
    FreeNode * iterator = free_list;
    while (iterator!=NULL)
    {
        if(iterator->meta_data->size >= size) {
                return iterator;
        }
        *prev = iterator;
        iterator=iterator->next;
    }
    return iterator;
}

static void removeFromFreeList(FreeNode* node_to_remove) {
    if (node_to_remove == free_list)
    {
        free_list = free_list->next;
        if(free_list != NULL)
          free_list->prev = NULL;
        return;
    }
    if (node_to_remove->prev!=NULL)
    {
        node_to_remove->prev->next = node_to_remove->next;
    }
    if(node_to_remove->next!=NULL)
    {
        node_to_remove->next->prev = node_to_remove->prev;
    }
    return;
}


//merge blocks, update free_list(remove merged blocks from it), and update stats, returns meta_data after merging
static MallocMetadata* mergeBlocks(MallocMetadata* freed_block)
{
    int count = 0 , orginal_size = freed_block->size;
    if(freed_block->prev!=NULL && freed_block->prev->is_free)
    {
        freed_block->prev->next = freed_block->next;
        if (freed_block->next!= NULL)
            freed_block->next->prev = freed_block->prev;
        global_num_free_blocks--;
        freed_block->prev->size += (freed_block->size + global_size_meta_data);
        //remove prev from free list (it is there)
        removeFromFreeList(&freed_block->prev->free_node);
        freed_block = freed_block->prev;
        count++;
    }
    if(freed_block->next!=NULL && freed_block->next->is_free)
    {
        freed_block->size += (freed_block->next->size + global_size_meta_data);
        removeFromFreeList(&freed_block->next->free_node);
        freed_block->next = freed_block->next->next;
        if (freed_block->next != NULL)
            freed_block->next->prev = freed_block;
        global_num_free_blocks--;
        count ++;
    }
    global_num_free_bytes += orginal_size + count*global_size_meta_data;
    global_num_meta_data_bytes-= count*global_size_meta_data;
    global_num_allocated_bytes+= count*global_size_meta_data;
    return freed_block;
}

static void insertFreeNode(MallocMetadata* freed_block)
{
    FreeNode* freenode = &freed_block->free_node ;

    freenode->meta_data = freed_block;

    FreeNode* node_to_insert_after = NULL;
    FreeNode* node_to_insert_before = findFreeBlockBySize(freed_block->size, &node_to_insert_after);
    if (node_to_insert_after == NULL)
    {
       // freenode->next = free_list;
        free_list = freenode ;
        return;
    }
    node_to_insert_after->next = freenode;
    freenode->prev = node_to_insert_after;
    if(node_to_insert_before!=NULL) {
        node_to_insert_before->prev = freenode;
        freenode->next = node_to_insert_before;
    }
}


static MallocMetadata* splitBlocks(FreeNode* node_to_split , size_t size)
{
    size_t orginal_size= node_to_split->meta_data->size;
    MallocMetadata* reuse_block = node_to_split->meta_data;
    char* unuse_block1 = (char*)reuse_block + size + global_size_meta_data;
    MallocMetadata* unuse_block = (MallocMetadata*)unuse_block1;
    reuse_block->size = size;
    reuse_block->is_free = false;
    unuse_block->next = reuse_block->next;
    reuse_block->next->prev = unuse_block;
    reuse_block->next = unuse_block;

    unuse_block->size = orginal_size - size - global_size_meta_data;
    unuse_block->prev = reuse_block;
    unuse_block->is_free = true;

    removeFromFreeList(node_to_split);
    insertFreeNode(unuse_block);

    global_num_free_bytes-= (reuse_block->size + global_size_meta_data);
    global_num_allocated_bytes -= global_size_meta_data;
    global_num_allocated_blocks++;
    global_num_meta_data_bytes+=global_size_meta_data;
    return reuse_block;
}

static void* mallocMmap(size_t size)
{
    MallocMetadata* new_block;
    void* p_ret = mmap(NULL, size+global_size_meta_data, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(p_ret == (void*)(-1))
        return NULL;
    new_block = (MallocMetadata*)p_ret;
    //addidng to the head of the list
    new_block->next = mmap_allocated_list;
    if (mmap_allocated_list!=NULL)
    {
        mmap_allocated_list->prev = new_block;
    }
    mmap_allocated_list = new_block;
    new_block->size = size;
    new_block->is_free = false;
    global_num_allocated_blocks++;
    global_num_allocated_bytes+=size;
    global_num_meta_data_bytes+=global_size_meta_data;
    return (void*)(new_block+1);
}
static void freeMunmap(void* p, MallocMetadata* meta_data)
{
    size_t size = meta_data->size;
    if(meta_data==mmap_allocated_list)
        mmap_allocated_list = meta_data->next;
    if(meta_data->prev != NULL)
        meta_data->prev->next = meta_data->next;
    if (meta_data->next != NULL)
        meta_data->next->prev = meta_data->prev;
    if(munmap(meta_data, size + global_size_meta_data)!=0)
        return;
    global_num_allocated_blocks--;
    global_num_allocated_bytes-=size;
    global_num_meta_data_bytes-=global_size_meta_data;

}
void* smalloc(size_t size)
{

    if((size <= 0) || size > 100000000 )
        return NULL;
    if (size > MMAPTHRESHOLD)
        return mallocMmap(size);
    MallocMetadata* last_alloced_block = NULL;
    FreeNode* last_free_block = NULL;
    FreeNode* reuse_block = findFreeBlockBySize(size, &last_free_block);
    if(reuse_block == NULL){
           MallocMetadata* block_to_use = findFreeBlock(size, &last_alloced_block);
           if(last_alloced_block != NULL && last_alloced_block->is_free)
           {
               //wilderness
               void* p_ret = sbrk(size - last_alloced_block->size);
               if(p_ret == (void*)(-1))
                   return NULL;
               global_num_free_bytes-= last_alloced_block->size;
               global_num_allocated_bytes+= size - last_alloced_block->size;
               global_num_free_blocks--;

               last_alloced_block->size = size;
               last_alloced_block->is_free = false;
               removeFromFreeList(&last_alloced_block->free_node);


               return (void*)(last_alloced_block+1);
           }
           else
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
    MallocMetadata* meta_data =(MallocMetadata*)(p);
    meta_data = meta_data -1;
    if(meta_data->is_free)
    {
        return;
    }
    //if size is bigger then must have come from mmap
    if(meta_data->size > MMAPTHRESHOLD)
    {
        freeMunmap(p, meta_data);
        return;
    }
    meta_data->is_free = true;
    meta_data = mergeBlocks(meta_data);
    insertFreeNode(meta_data);
    global_num_free_blocks++;
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
    if(size > MMAPTHRESHOLD) // mmap block
    {
        if(pointer->size == size)
        {
            return oldp;
        }
    }
    else { // sbrk block

    }

    //basic situation for both cases
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