#include <iostream>
#include <functional>
#include "memory.h"
#include "sortmem.h"

using namespace std;

int main(int argc, const char ** argv){
    try{
        SortMem::run(argc, argv, <#initializer#>);
    }
    catch (exception& e) {
        cerr << "Exception Caught: " << e.what() << endl;
    }
    return 0;
}