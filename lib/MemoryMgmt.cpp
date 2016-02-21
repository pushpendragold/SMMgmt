#include "MemoryMgmt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef HAVE_IOSTREAM_H
  #include <iostream.h>
  using namespace std {}
#else 
  #include <iostream>
  using namespace std;
#endif

/* Returns true on success */
bool SMManager::init()
{
    /* connect to (and possibly create) the segment: */
    if ((m_shmid = shmget(m_key, m_size, 0666 | IPC_CREAT)) == -1) {
		PRINT_DEBUG printf("shmget Failed\n");
		return false;
    }

    sm_Header = (SM_Header *)shmat(m_shmid,(void *)0,0);
    if( sm_Header == (SM_Header *) 0 || sm_Header == (SM_Header *)-1) {
		PRINT_DEBUG printf("shmat Failed\n");
		return false;
    }
    
    /* Shared memory is attached successfully */
    m_initialized = true;
    
    /* initialize shared memory start and end address */
    m_startAddress = (Address_t) sm_Header; /* Used while detaching memory & during clean-up */
    m_endAddress = (Address_t) sm_Header + (Address_t)m_size;
        
    /* Shared memory reuse case - Check if memory were already used */
    if( sm_Header->sm_alloc_memory_slist || sm_Header->sm_free_memory_slist || sm_Header->sm_endAddress  ) {
      // TODO : Check if something needed to be done here.
      PRINT_DEBUG printf("Memory were already allocated.\n");
    }
    else {
        
		PRINT_DEBUG printf("Memory allocated for first time.\n");
		sm_Header->sm_alloc_memory_slist = sm_Header->sm_alloc_memory_elist = BLOCK_END;
		sm_Header->sm_free_memory_slist = sm_Header->sm_free_memory_elist = BLOCK_END;
		sm_Header->sm_startAddress = (Offset_t)SM_Header_size;
		sm_Header->sm_endAddress = (Offset_t)SM_Header_size;
    }
    
    Dump();
    
    return true;
}

/* Allocating memory from pool */
void* SMManager::Allocate(size_t size) {

  /* Check if we have available memory in pool. */
  if((Offset_t)(m_size - sm_Header->sm_endAddress) >=  (Offset_t) (M_Header_size + size)) {
    
    Offset_t blockStart = sm_Header->sm_endAddress + 1;
    PRINT_DEBUG printf("blockStart Address : %lu\n",blockStart);
    
    M_Header * tBlock = (M_Header *) GetAddress( blockStart );

    tBlock->size_block = size;
    tBlock->prev_block = BLOCK_END;
    tBlock->next_block = BLOCK_END;
    
    /* Add allocated memory block to alloc list */
    AddToAllocList(blockStart);
    
    /* Move end address to appropriate location */
    sm_Header->sm_endAddress = sm_Header->sm_endAddress + M_Header_size + size;
    
    return GetAddress((Address_t)(blockStart + M_Header_size));
  }
  
  /* if free memory is not available. Find first free block of size and allocate. */
  if(sm_Header->sm_free_memory_slist != BLOCK_END) {
    
    Offset_t tNode = sm_Header->sm_free_memory_slist;
    
    while(tNode != BLOCK_END) {
      
      // Use first block which can handle 
      if(((M_Header *)GetAddress(tNode))->size_block >= size ) {
	RemoveFromFreeList(tNode);
	AddToAllocList(tNode);
	return GetAddress((Address_t)(tNode + M_Header_size));
      }
      
      tNode = ((M_Header *)GetAddress(tNode))->next_block;
    }
  }

  PRINT_DEBUG printf("SMManager::Allocate : Memory Allocation failed.\n");
  return NULL;
}

/* Free Allocated memory */
bool SMManager::Free(void * _adr) {
  
  /* Check if passed address is in shared memory segment */
  if( (Address_t)_adr < m_startAddress || (Address_t)_adr > m_endAddress ) {
    PRINT_DEBUG printf("Error: Outside memory allocation\n");
    return false;
  }
  
  Offset_t to_search = GetOffset(_adr) - M_Header_size;
  PRINT_DEBUG printf("SMManager::Free - offset to search %ld\n",to_search);
  
  if(sm_Header->sm_alloc_memory_slist == BLOCK_END) {
    PRINT_DEBUG printf("SMManager::Free - nothing found in alloc list\n");
    return false;
  }
  
  // Check if this is first node
  if(sm_Header->sm_alloc_memory_slist == to_search) {
    Offset_t next = ((M_Header *)GetAddress(to_search))->next_block;
    sm_Header->sm_alloc_memory_slist = next;
    
    if(next != BLOCK_END){
	  ((M_Header *)GetAddress(to_search))->prev_block = BLOCK_END;
    }
    else {
      // move last node as well
      sm_Header->sm_alloc_memory_elist = BLOCK_END;
    }
    
    // Move memory block to Free list
    AddToFreeList(to_search);
    return true;
  }
  
  // Check if this is last node
  if(sm_Header->sm_alloc_memory_elist == to_search) {
     Offset_t prev = ((M_Header *)GetAddress(to_search))->prev_block;
     
     if(prev == BLOCK_END) {
       sm_Header->sm_alloc_memory_slist = BLOCK_END;
     }
     
     sm_Header->sm_alloc_memory_elist = prev;
     ((M_Header*)GetAddress(prev))->next_block = BLOCK_END;
     
     AddToFreeList(to_search); 
     return true;
  }
  
  Offset_t tmpOffset = sm_Header->sm_alloc_memory_slist;
  
  while(tmpOffset != BLOCK_END)  {
     
    if(tmpOffset == to_search) {
        
        /* Delete from alloc list */
        Offset_t next = ((M_Header *)GetAddress(tmpOffset))->next_block;
        Offset_t prev = ((M_Header *)GetAddress(tmpOffset))->prev_block;
        
	((M_Header *)GetAddress(next))->prev_block = prev;
	((M_Header *)GetAddress(prev))->next_block = next;
	
        /* Add block to free list */
        AddToFreeList(to_search);
        
        return true;
     }
     tmpOffset = ((M_Header *) GetAddress(tmpOffset))->next_block;
  }
  
  /* Block Not found - Multiple free cases or invalid memory address supplied */
  return false;
}


/* Adding at last in Alloc list */
void SMManager::AddToAllocList(Offset_t _aloc_off) {

    if(sm_Header->sm_alloc_memory_slist == BLOCK_END) {
      
      sm_Header->sm_alloc_memory_elist = 
      sm_Header->sm_alloc_memory_slist = _aloc_off;
      
      M_Header * tmp = (M_Header *)GetAddress(_aloc_off);
      
      tmp->next_block = BLOCK_END;
      tmp->prev_block = BLOCK_END;

    } else {
      
      M_Header * tmp = (M_Header *)GetAddress(_aloc_off);
      
      tmp->prev_block = sm_Header->sm_alloc_memory_elist;
      tmp->next_block = BLOCK_END;
      
      M_Header * prev = (M_Header *)GetAddress(sm_Header->sm_alloc_memory_elist);
      prev->next_block = _aloc_off;
      
      sm_Header->sm_alloc_memory_elist = _aloc_off;
          
    }
}

/* Remove from free list */
void SMManager::RemoveFromFreeList(Offset_t _aloc_off)
{
    /* First block removed */
    if(sm_Header->sm_free_memory_slist == _aloc_off) {
      
       sm_Header->sm_free_memory_slist = ((M_Header *)GetAddress(_aloc_off))->next_block;
       if(sm_Header->sm_free_memory_elist == _aloc_off)
	 sm_Header->sm_free_memory_elist = sm_Header->sm_free_memory_slist;
     
    } else if(sm_Header->sm_free_memory_elist == _aloc_off) {
      /* Last block removed */
      sm_Header->sm_free_memory_elist = ((M_Header *)GetAddress(_aloc_off))->prev_block;
      if(sm_Header->sm_free_memory_slist == _aloc_off)
	sm_Header->sm_free_memory_slist = sm_Header->sm_free_memory_elist;
      
    } else {
      /* Middle block removed */
      Offset_t next = ((M_Header *)GetAddress(_aloc_off))->next_block;
      Offset_t prev = ((M_Header *)GetAddress(_aloc_off))->prev_block;
      
      ((M_Header *)GetAddress(next))->prev_block = prev;
      ((M_Header *)GetAddress(prev))->next_block = next;
    }
}

/* Remove from alloc list */
void SMManager::RemoveFromAlocList(Offset_t _aloc_off)
{
    /* First block removed */
    if(sm_Header->sm_alloc_memory_slist == _aloc_off) {
      
       sm_Header->sm_alloc_memory_slist = ((M_Header *)GetAddress(_aloc_off))->next_block;
       if(sm_Header->sm_alloc_memory_elist == _aloc_off)
	 sm_Header->sm_alloc_memory_elist = sm_Header->sm_alloc_memory_slist;
     
    } else if(sm_Header->sm_alloc_memory_elist == _aloc_off) {
      /* Last block removed */
      sm_Header->sm_alloc_memory_elist = ((M_Header *)GetAddress(_aloc_off))->prev_block;
      if(sm_Header->sm_alloc_memory_slist == _aloc_off)
	sm_Header->sm_alloc_memory_slist = sm_Header->sm_alloc_memory_elist;
      
    } else {
      /* Middle block removed */
      Offset_t next = ((M_Header *)GetAddress(_aloc_off))->next_block;
      Offset_t prev = ((M_Header *)GetAddress(_aloc_off))->prev_block;
      
      ((M_Header *)GetAddress(next))->prev_block = prev;
      ((M_Header *)GetAddress(prev))->next_block = next;
    }
}


/* Adding to free list */
void SMManager::AddToFreeList(Offset_t _aloc_off) {
  
    if(sm_Header->sm_free_memory_slist == BLOCK_END) {
      
      sm_Header->sm_free_memory_slist = 
      sm_Header->sm_free_memory_elist = _aloc_off;
      
      M_Header * tmp = (M_Header *)GetAddress(_aloc_off);
      
      tmp->next_block = BLOCK_END;
      tmp->prev_block = BLOCK_END;
      
    } else {
      
      M_Header * tmp = (M_Header *)GetAddress(_aloc_off);
      
      tmp->prev_block = sm_Header->sm_free_memory_elist;
      tmp->next_block = BLOCK_END;
      
      M_Header * prev = (M_Header *)GetAddress(sm_Header->sm_free_memory_elist);
      prev->next_block = _aloc_off;
      
      sm_Header->sm_free_memory_elist = _aloc_off;
      
    }
}


void* SMManager::GetAddress(Offset_t _offset) {
  return (void *)((Address_t)m_startAddress + (Address_t)_offset);
}

Offset_t SMManager::GetOffset(void* adr) {
  return (Address_t)adr - (Address_t)m_startAddress;
}

void* SMManager::GetFirstBlockAllocated() {
  return GetAddress(sm_Header->sm_startAddress + M_Header_size + 1);
}


/* Clean up activity - Release & delete memory */
bool SMManager::CleanUp(bool _removeMemory) {
    
    /* detach from the segment: */
    if (shmdt((void *)m_startAddress) == -1) {
        PRINT_DEBUG printf("shmdt Failed\n");
        return false;
    }
    
    /* Remove shared memory segment */
    if( _removeMemory ) {
      if( shmctl(m_shmid, IPC_RMID, NULL) == -1) {
	PRINT_DEBUG printf("shmctl Failed\n");
        return false;
      }
    }
    
    return true;
}

bool SMManager::Detach() {
    /* detach from the segment: */
    if (shmdt((void *)m_startAddress) == -1) {
        PRINT_DEBUG printf("shmdt Failed\n");
        return false;
    }
    return true;
}


void SMManager::PrintAllocList() {
  PRINT_DEBUG {
     printf("Allocation List\n");
     Offset_t tOff = sm_Header->sm_alloc_memory_slist;
     while(tOff != BLOCK_END) {
       M_Header * t = (M_Header *)GetAddress(tOff);
       printf("Offset: %ld, size: %ld, next_block: %ld, prev_block: %ld ", 
	      tOff,
	      t->size_block ,
	      (Offset_t)t->next_block, 
	      (Offset_t)t->prev_block);
       tOff = t->next_block;
       printf("\n");
    }
  }
}

void SMManager::PrintFreeList() {
  PRINT_DEBUG {
     printf("Free Block List\n");
     Offset_t tOff = sm_Header->sm_free_memory_slist;
     while(tOff != BLOCK_END) {
	printf("Offset: %ld, size: %ld, next_block: %ld, prev_block: %ld \n",
	       tOff,
	       ((M_Header *)GetAddress(tOff))->size_block,
	       ((M_Header *)GetAddress(tOff))->next_block,
	       ((M_Header *)GetAddress(tOff))->prev_block);
	
	tOff = ((M_Header *)GetAddress(tOff))->next_block;
     }
     
  }
}

void SMManager::Dump() {
  PRINT_DEBUG {
    printf("m_key : %lu\n",m_key);
	printf("m_size : %lu\n",m_size);
	printf("m_initialized : %d\n",m_initialized);
	printf("sm_Header : %lu\n",(Offset_t)sm_Header);
	printf("m_startAddress : %lu\n",m_startAddress);
	printf("m_endAddress : %lu\n",m_endAddress);
	printf("m_shmid : %lu\n",m_shmid);
	printf("sm_startAddress : %lu\n",m_startAddress);
  }
}


long unsigned int SMManager::GetKey() {
   return m_key;
}

void SMManager::SetKey(long unsigned int _key) {
   m_key = _key;
}

size_t SMManager::GetSize() {
   return m_size;
}

void SMManager::SetSize(size_t _size) {
   m_size = _size;
}
