#include "../lib/MemoryMgmt.h"

#include <cstdio>
#include <iostream>

namespace std { }
using namespace std;

int main() {
   SMManager sm;
   sm.init();
   
   sm.CleanUp(true);
   return 0;
}
