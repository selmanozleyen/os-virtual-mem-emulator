#include <iostream>
#include <functional>
#include "memory.h"
#include "sortmem.h"
#include <chrono>

using namespace std;

int main(int argc, const char ** argv){
    try{
        auto start =chrono::steady_clock::now();
        RunStats rs;
        SortMem::run(argc, argv, rs);
        auto end =  chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        printf("Time elapsed: %.5ld microseconds\n", elapsed.count());
    }
    catch (exception& e) {
        cerr << "Exception Caught: " << e.what() << endl;
    }
    return 0;
}