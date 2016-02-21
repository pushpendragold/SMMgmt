#include "../lib/MemoryMgmt.h"
#include <cstdio>
#include <iostream>

namespace std { }
using namespace std;

typedef struct Data {
  int Data;
  Offset_t next;
}Data;


int main() {
  
  SMManager sm;
  sm.init();
    
  Data * r = (Data *) sm.GetFirstBlockAllocated();
  while(true) {
     cout << r->Data << endl;
     if(r->next == BLOCK_END) break;
     r = (Data *)sm.GetAddress((Offset_t)r->next);
  }
  
  return 0;
}
