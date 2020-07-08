//
// Created by selman.ozleyen2017 on 8.07.2020.
//

#include "sortmem.h"
#include <iostream>
#include <string>
#include <cstring>


using namespace std;

stringstream SortMem::stream;

void SortMem::fill(Memory *m) {
    unsigned long int intCount = m->getIntCount();
    auto name = const_cast<char *>("fill");
    for (unsigned int i = 0; i < intCount; ++i) {
        m->set(i, rand(),name);
    }
}

void SortMem::printMem(Memory *m) {
    unsigned long int intCount = m->getIntCount();
    for (unsigned int i = 0; i < intCount; ++i) {
        SortMem::stream << "M[" << i <<"]:" << m->get(i,const_cast<char *>("print")) << "\n";
    }
    //stream.flush();
    cout << SortMem::stream.str();
}

void SortMem::run(int argc, const char **argv) {

}

void SortMem::merge(Memory *m, unsigned long start, unsigned long end) {

}
/* Bubble sort for memory*/
void SortMem::bubble(Memory *m, unsigned long start, unsigned long end) {
    /* Initializing necessary variables */
    bool swapped = true;
    auto name = const_cast<char *> ("bubble");
    int comp1Temp = m->get(0,name),comp2Temp = comp1Temp;
    auto i = start,j = start;

    for (i = start; i < end - 1; ++i) {
        swapped = false;
        for (j = start; j < end - 1 - i; ++j) {
            comp1Temp = m->get(j,name);
            comp2Temp = m->get(j+1,name);
            /* Comparing them */
            if(comp1Temp > comp2Temp){
                swapped = true;
                /* Swapping the values*/
                m->set(j,comp2Temp,name);
                m->set(j+1,comp1Temp,name);
            }
        }
        /* Exit if already Sorted*/
        if(!swapped){
            cout << "Bubble Thread: Quarter Sorted.\n";
            return;
        }
    }
    cout << "Bubble Thread: Quarter Sorted.\n";
}




void SortMem::quick(Memory *m, unsigned long start, unsigned long end) {
    auto name = const_cast<char *>("quick");

}

void SortMem::index(Memory *m, unsigned long start, unsigned long end) {

}

void SortMem::check(Memory *m) {
    unsigned long int intCount = m->getIntCount();
    auto name = const_cast<char *>("check");
    auto prev = m->get(0,name);
    auto cur = prev;
    /* After setting variables we are ready to compare */
    for (unsigned int i = 1; i < intCount; ++i) {
        cur = m->get(i,name);
        if(prev > cur){
            cout << "Check Thread: Memory is not sorted increasingly.\n";
            return;
        }
        prev = cur;
    }
    cout << "Check Thread: Memory is sorted increasingly.\n";
}

void SortMem::testRun(int argc, const char **argv) {
    srand(1000);
    if(SortMem::argCount != argc)
        throw invalid_argument("Argument count is invalid");
    int frameSize = stoi(argv[1]),numPhysical = stoi(argv[2]),
        numVirtual = stoi(argv[3]),pageTablePrintInt = stoi(argv[6]);

    if(frameSize <= 0 || numPhysical <= 0 || numVirtual <= 0)
        throw invalid_argument("Please check your arguments.");

    /* Get Policies */
    ReplacementPolicy pageReplacement;
    const char * repPol = argv[4];
    if(!strcmp(repPol,"NRU")){
        pageReplacement = ReplacementPolicy::nru;
    }else if(!strcmp(repPol,"FIFO")){
        pageReplacement = ReplacementPolicy::fifo;
    }else if(!strcmp(repPol,"SC")){
        pageReplacement = ReplacementPolicy::sc;
    }else if(!strcmp(repPol,"LRU")){
        pageReplacement = ReplacementPolicy::lru;
    }else if(!strcmp(repPol,"WSClock")){
        pageReplacement = ReplacementPolicy::wsclock;
    }else{
        throw invalid_argument("Invalid pageReplacement arg");
    }

    AllocPolicy allocPolicy;
    if(!strcmp(argv[5],"local")){
        allocPolicy = AllocPolicy::local;
    }else if(!strcmp(argv[5],"global")){
        allocPolicy = AllocPolicy::global;
    }else{
        throw invalid_argument("Invalid allocPolicy arg");
    }

    /* Setting Functions */
    unsigned long int totSize = (1 << (numVirtual + frameSize));
    auto quarterLen = (totSize >> 2);
    function <void(Memory*)> bubbleFunc = bind(bubble,placeholders::_1,0,totSize),
        quickFunc = bind(quick,placeholders::_1,1*quarterLen,2*quarterLen),
        mergeFunc = bind(merge,placeholders::_1,2*quarterLen,3*quarterLen),
        indexFunc = bind(index,placeholders::_1,3*quarterLen,4*quarterLen),
        fillFunc = bind(fill,placeholders::_1),
        checkFunc = bind(check,placeholders::_1),
        printFunc = bind(printMem,placeholders::_1);


    Memory m(frameSize,numPhysical,numVirtual,pageTablePrintInt,pageReplacement,allocPolicy,argv[7]);
    /* Created The Memory */
    cout << "Main: Created the memory, now running process fill (process in simulation)\n";
    m.addProcess(fillFunc,"fill");
    /* Start and wait the process*/
    m.startProcesses();
    m.waitProcesses();

    cout << "Main: Fill process finished, now starting sort processes\n";
    m.addProcess(bubbleFunc,"bubble");
    m.addProcess(quickFunc,"quick");
    m.addProcess(mergeFunc,"merge");
    m.addProcess(indexFunc,"index");
    /* Start and wait the process*/
    m.startProcesses();
    cout << "Main: Started the sort processes\n";
    m.waitProcesses();
    cout << "Main: Sort processes are finished, now starting check process\n";
    m.addProcess(checkFunc,"check");
    /* Start and wait the process*/
    m.startProcesses();
    m.waitProcesses();
    cout << "Main: Check process is done.\n";
    m.printStats();

}

/* Lomuto Partition For QuickSort*/
/* Returns the new pivot*/
unsigned int SortMem::qsPartition(Memory *m, unsigned int low, unsigned int high, char *tName) {
    /* mj stands for m[j]*/
    int pivot = m->get(high,tName),mj = 0;
    /* temps for swap operation */
    int swap1 = 0,swap2 = 0;
    unsigned int i = (low - 1); // will become the pivot - 1 after func
    for (unsigned int j = 0; j <= high; ++j) {
        mj = m->get(j,tName);
        if(pivot > mj){
            i++;
            swap1 = m->get(i,tName);
            swap2 = m->get(j,tName);
            m->set(i,swap2,tName);
            m->set(j,swap1,tName);
        }
    }
    swap1 = m->get(i+1,tName);
    swap2 = m->get(high,tName);
    m->set(i+1,swap2,tName);
    m->set(high,swap1,tName);

    return i+1;
}
