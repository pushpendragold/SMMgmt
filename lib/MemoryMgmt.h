#ifndef _H_SHAREDMEMORY_MGMT_H_
#define _H_SHAREDMEMORY_MGMT_H_

#define _DFT_KEY  4212312899  /* Default key */
#define _DFT_SIZE 1024*1024   /* Default size */

#define PRINT_DEBUG if(1)

#include <stdio.h>

#define BLOCK_END 0             /* Block End point */

/* To hold memory offset */
typedef unsigned long Offset_t;

/* To hold Memory addresses */
typedef unsigned long Address_t;

/* Header for Allocated/De-Allocated Memory */
typedef struct M_Header {
   
    Offset_t prev_block;
    Offset_t next_block;
    size_t   size_block;
    
} M_Header;

/* Block having fixed information */
typedef struct SM_Header {
  
    Offset_t   sm_endAddress;                 /* End address of available memory */
    Offset_t   sm_startAddress;               /* Start address of available memory */

    Offset_t   sm_alloc_memory_slist;         /* Starting location: list for allocated memory */
    Offset_t   sm_alloc_memory_elist;         /* Last location: list for allocated memory */
    
    Offset_t   sm_free_memory_slist;          /* Starting location: list for freed memory */
    Offset_t   sm_free_memory_elist;          /* Last location: list for freed memory */
    
} SM_Header;

const size_t SM_Header_size = sizeof(SM_Header);
const size_t M_Header_size  = sizeof(M_Header);

class SMManager {

private:
    
    unsigned long m_key;                      /* Key used to create shared memory segment */
    size_t        m_size;                     /* Memory size allocated */

    bool          m_initialized;              /* set to true if initialized without any error */
    
    SM_Header   * sm_Header;                  /* SManager Header info */
    
    Address_t     m_startAddress;             /* starting address of memory pool */
    Address_t     m_endAddress;               /* End address of memory pool */
    
    unsigned long m_shmid;                    /* Shared memory id */
    
    /* Add to Freed list - Add to End */
    void AddToFreeList(Offset_t _aloc_off);
    
    /* Remove from free list */
    void RemoveFromFreeList(Offset_t _aloc_off);
    
    /* Add to Allocated list - Add to End */
    void AddToAllocList(Offset_t _aloc_off);
    
    /* Remove from Allocate list */
    void RemoveFromAlocList(Offset_t _aloc_off);

public:
    
    SMManager() {
      m_initialized = false;
      m_key =  _DFT_KEY;
      m_size = _DFT_SIZE;
    }
    
    ~SMManager() {
	CleanUp();
    }
    
    bool init();
    void * Allocate(size_t size);
    bool Free(void * _adr);
    
    bool CleanUp(bool _removeMemory = false);
    bool Detach();
    
    unsigned long GetKey();
    void SetKey(unsigned long _key);  
    
    size_t GetSize();
    void SetSize(size_t _size);
    
    void * GetAddress(Offset_t _offset);
    Offset_t GetOffset(void * adr);
    void * GetFirstBlockAllocated();
    
    void PrintFreeList();
    void PrintAllocList();
    
    void Dump();
};


#endif
