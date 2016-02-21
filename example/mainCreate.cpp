#include "../lib/MemoryMgmt.h"

#include <cstdio>
#include <iostream>
#include <string.h>

namespace std { }
using namespace std;

typedef struct Data {
  int Data;
  Offset_t next;
}Data;

void List();
void String();
SMManager sm;

int main() {

  sm.init();
  //String();
  List();
  
  sm.Detach();
  //sm.CleanUp(true);
  return 0;
}

void String()
{
  char B[] = "PushpendraTest";
  char * A = (char *)sm.Allocate(strlen(B)+1);
  char * C = (char *)sm.Allocate(strlen(B)+1);
  char * D = (char *)sm.Allocate(strlen(B)+1);
  char * E = (char *)sm.Allocate(strlen(B)+1);
  char * F = (char *)sm.Allocate(strlen(B)+1);
  
  strcpy(A,B);
  strcpy(C,B);
  strcpy(D,B);
  strcpy(E,B);
  strcpy(F,B);
  
  sm.PrintAllocList();
  sm.PrintFreeList();
  
  printf("Written: %s\n",A);
  printf("Written: %s\n",B);
  printf("Written: %s\n",D);
  
  sm.PrintAllocList();
  sm.PrintFreeList();
  
#if 0
    printf("Free C - delete from middle\n");
    sm.Free(C);
    
    sm.PrintAllocList();
    sm.PrintFreeList();
    
    printf("Free A - delete first\n");
    sm.Free(A);
    
    sm.PrintAllocList();
    sm.PrintFreeList();
    
    printf("Free F - delete from end\n");
    sm.Free(F);
    
    sm.PrintAllocList();
    sm.PrintFreeList();
    
    printf("Free E - delete from end\n");
    sm.Free(E);
    
    sm.PrintAllocList();
    sm.PrintFreeList();
    
    printf("Free D - delete from end\n");
    sm.Free(D);
    
    sm.PrintAllocList();
    sm.PrintFreeList();
    
#endif
  
}

void List()
{
   
  Data * root = (Data *) sm.Allocate(sizeof(Data));
  cout << "root Address :" << root << endl;
  cout << "root Offset  :" << sm.GetOffset(root) << endl;
  root->Data = 10;
  root->next = BLOCK_END;
  
  Data * nR = (Data *) sm.Allocate(sizeof(Data));
  cout << "nR Address :" << nR << endl;
  cout << "nR Offset  :" << sm.GetOffset(nR) << endl;
  nR->Data = 12;
  nR->next = BLOCK_END;
  root->next = sm.GetOffset(nR);
  
  Data * n = (Data *) sm.Allocate(sizeof(Data));
  n->next = BLOCK_END;
  n->Data = 14;
  nR->next = sm.GetOffset(n);
  
  sm.PrintAllocList();
  
  Data * r = (Data *) sm.GetFirstBlockAllocated();
  cout << "Read Address :" << r << endl;
  cout << "Read Offset  :" << sm.GetOffset(r) << endl;
  cout << "Data Read: " << r->Data << endl;
  
  r = (Data *)sm.GetAddress((Offset_t)r->next);
  cout << "Data Read: " << r->Data << endl;

}
