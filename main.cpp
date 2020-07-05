#include <iostream>
#include <functional>
#include "memory.h"



using namespace std;

void qSortMem(Memory * m, int start, int end){
    cout << start << "lol" << end << endl;
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    Memory m(12,5,10,10000,lru,local,"mem.dat");
    function <void(Memory*)> qFunc = bind(qSortMem,placeholders::_1,0,100);
    m.addProcess(qFunc);

    return 0;
}
