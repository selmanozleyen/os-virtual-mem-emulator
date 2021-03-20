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


void SortMem::merge(Memory *m, unsigned long start, unsigned long end) {
    auto name = const_cast<char *> ("merge");
    mergeSortHelper(m,start,end-1,name);
    cout << "[Merge]: Process Finished.\n";
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
            cout << "[Bubble]: Process Finished.\n";
            return;
        }
    }
    cout << "[Bubble]: Process Finished.\n";
}

/* TODO: add to report end is not included*/
void SortMem::quick(Memory *m, unsigned long start, unsigned long end) {
    auto name = const_cast<char *>("quick");
    quick_helper(m,start,end-1,name);
    cout << "[Quick]: Process Finished.\n";
}

void SortMem::index(Memory *m, unsigned long start, unsigned long end) {
    /* Getting the name of the thread*/
    auto name = const_cast<char *>("index");
    unsigned int size = end-start;
    int tempi,tempj,tempInd;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> indMap;
    /* Create the index list */
    /* List to navigate through the memory*/
    indMap.resize(size);
    indices.reserve(size);
    for(unsigned int i = 0 ;i < size; i++){
        indices.push_back(i);
    }
    /* Sort the index list*/
    for (unsigned int i = 0; i < size; ++i) {
        for (unsigned int j = 0; j < size; ++j) {
            tempi = m->get(indices[i]+start,name);
            tempj = m->get(indices[j]+start,name);

            // swap the indices
            if(tempi > tempj && i < j){
                tempInd = indices[i];
                indices[i] = indices[j];
                indices[j] = tempInd;
            }
        }
    }
    /* Creating and inverse array for indices*/
    for (unsigned int i = 0; i < size; ++i) {
        indMap[indices[i]] = i ;
    }
    /* Found the indices in order
     * it is time for an efficient write
     * */
    int temp1,temp2;
    unsigned int j = 0,tempInd1,tempInd2;
    for (unsigned int i = 0; i < size; ++i) {
        j = indices[i];
        if(i != j){
            temp1 = m->get(i+start,name);
            tempInd1 = indMap[i];
            while(i != j){
                temp2 = m->get(j+start,name);
                tempInd2 = indMap[j];

                m->set(j+start,temp1,name);
                indMap[j] = tempInd1;

                tempInd1 = tempInd2;
                temp1 = temp2;

                j = tempInd1;
            }
            m->set(i+start,temp1,name);
            indMap[i] = tempInd1;
        }
    }
    cout << "[Index]: Process Finished.\n";
}

void SortMem::check(Memory *m) {
    unsigned long int quarter = m->getIntCount() >> 2;
    auto name = const_cast<char *>("check");
    auto prev = m->get(0,name);
    auto cur = prev;
    /* After setting variables we are ready to compare */
    for (int j = 0; j < 4; ++j) {
        prev = m->get(quarter*j,name);
        for (unsigned int i = quarter*j+1; i < quarter*(j+1); ++i) {
            cur = m->get(i,name);
            if(prev > cur){
                cout << "[Check]: Memory is not sorted increasingly.\n";
                return;
            }
            prev = cur;
        }
    }

    cout << "[Check]: Memory is sorted increasingly.\n";
}

void SortMem::run(int argc, const char **argv, RunStats &rs) {


    /* Init variables*/
    int frameSize,numPhysical,numVirtual,pageTablePrintInt;
    const char * fName;
    AllocPolicy allocPolicy;
    ReplacementPolicy pageReplacement;

    /* Reading and checking the input of the user */
    read_args(argc,argv,&frameSize,&numPhysical,&numVirtual,&pageTablePrintInt,
              &pageReplacement,&allocPolicy,&fName);
    run(frameSize,numPhysical,numVirtual,pageTablePrintInt,pageReplacement,allocPolicy,fName,rs);
}

/* Lomuto Partition For QuickSort*/
/* Returns the new pivot*/
unsigned int SortMem::qsPartition(Memory *m, unsigned int low, unsigned int high, char *tName) {
    /* mj stands for m[j]*/
    int pivot = m->get(high,tName),mj = 0;
    /* temps for swap operation */
    int swap1 = 0,swap2 = 0;
    unsigned int i = (low - 1); // will become the pivot - 1 after func
    for (unsigned int j = low; j <= high; ++j) {
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

    return (i+1);
}

void SortMem::quick_helper(Memory *m, unsigned long start, unsigned long end, char *tName) {
    if(end > start){
        auto pIndex = qsPartition(m,start,end,tName);

        quick_helper(m,start,pIndex-1,tName);
        quick_helper(m,pIndex+1,end,tName);
    }
}


void SortMem::read_args(int argc, const char **argv, int *frameSize, int *numPhysical, int *numVirtual,
                        int *pageTablePrintInt, ReplacementPolicy *pageReplacement, AllocPolicy *allocPolicy,
                        const char **diskFileName) {
    /* Getting numeric values*/
    if(SortMem::argCount != argc)
        throw invalid_argument("Argument count is invalid");
    *frameSize = stoi(argv[1]),*numPhysical = stoi(argv[2]),
    *numVirtual = stoi(argv[3]),*pageTablePrintInt = stoi(argv[6]);

    if(*frameSize <= 0 || *numPhysical <= 0 || *numVirtual <= 0)
        throw invalid_argument("Please check your arguments.");

    /* Get Policies */
    const char * repPol = argv[4];
    if(!strcmp(repPol,"NRU")){
        *pageReplacement = ReplacementPolicy::nru;
    }else if(!strcmp(repPol,"FIFO")){
        *pageReplacement = ReplacementPolicy::fifo;
    }else if(!strcmp(repPol,"SC")){
        *pageReplacement = ReplacementPolicy::sc;
    }else if(!strcmp(repPol,"LRU")){
        *pageReplacement = ReplacementPolicy::lru;
    }else if(!strcmp(repPol,"WSClock")){
        *pageReplacement = ReplacementPolicy::wsclock;
    }else{
        throw invalid_argument("Invalid pageReplacement arg");
    }

    /* Getting alloc policy */
    if(!strcmp(argv[5],"local")){
        *allocPolicy = AllocPolicy::local;
    }else if(!strcmp(argv[5],"global")){
        *allocPolicy = AllocPolicy::global;
    }else{
        throw invalid_argument("Invalid allocPolicy arg");
    }
    /* Get file Name */
    *diskFileName = argv[7];
}

void SortMem::mergeArr(Memory *m, unsigned long l, unsigned long mid, unsigned long r, char *tName) {
    auto start = mid + 1,index = start;

    int arrMid = m->get(mid,tName),arrStart = m->get(start,tName),
            arrL,value,arrIndex1;
    if(arrStart >= arrMid){ // already sorted
        return;
    }

    while(l <= mid && start <= r){
        /* get m[l] and m[start] */
        arrL = m->get(l,tName);
        arrStart = m->get(start,tName);
        if(arrL <= arrStart){
            l++;
        }else{
            index = start;
            value = m->get(start,tName); // m[start]

            while(index != l){
                arrIndex1 = m->get(index-1,tName);
                m->set(index,arrIndex1,tName);
                index--;
            }
            m->set(l,value,tName);

            l++;mid++;start++;
        }
    }
}

void SortMem::mergeSortHelper(Memory *m, unsigned long l, unsigned long r, char *tName) {
    if(l < r){
        unsigned mid = l + (r-l)/2;
        mergeSortHelper(m,l,mid,tName);
        mergeSortHelper(m,mid+1,r,tName);

        mergeArr(m,l,mid,r,tName);
    }
}

void SortMem::run(int frameSize, int numPhysical, int numVirtual, int pageTablePrintInt,
                  ReplacementPolicy pageReplacement, AllocPolicy allocPolicy, const char *fName,RunStats & rs) {
    /* Seed the rand() function */
    srand(1000);
    /* Setting functions as types to give it to the Memory Class */
    /* This is important for the sake of encapsulation. */
    unsigned long int totSize = (1 << (numVirtual + frameSize));
    auto quarterLen = (totSize >> 2);
    function <void(Memory*)> bubbleFunc = bind(bubble,placeholders::_1,0,quarterLen),
            quickFunc = bind(quick,placeholders::_1,1*quarterLen,2*quarterLen),
            mergeFunc = bind(merge,placeholders::_1,2*quarterLen,3*quarterLen),
            indexFunc = bind(index,placeholders::_1,3*quarterLen,4*quarterLen),
            fillFunc = bind(fill,placeholders::_1),
            checkFunc = bind(check,placeholders::_1),
            printFunc = bind(printMem,placeholders::_1);
    /* Function initializing done */

    /* Creating the memory object */
    Memory m(frameSize,numPhysical,numVirtual,pageTablePrintInt,pageReplacement,allocPolicy,fName);

    /* Created The Memory */
    cout << "[Main]: Created the memory, now running process fill (process in simulation)\n";
    m.addProcess(fillFunc,"fill");
    /* Start and wait the process*/
    m.startProcesses();
    /* Join the processes*/
    m.waitProcesses();

    cout << "[Main]: Fill process finished, now starting sort processes\n";
    m.addProcess(bubbleFunc,"bubble");
    m.addProcess(quickFunc,"quick");
    m.addProcess(mergeFunc,"merge");
    m.addProcess(indexFunc,"index");
    /* Start and wait the process*/
    m.startProcesses();
    cout << "[Main]: Started the sort processes\n";
    m.waitProcesses();
    cout << "[Main]: Sort processes are finished, now starting check process\n";


    /* RUNNING THE CHECK FUNCTION */
    m.addProcess(checkFunc,"check");
    /* Start and wait the process*/
    m.startProcesses();
    m.waitProcesses();
    /* CHECK FUNC END*/

    cout << "[Main]: Check process is done.\n"; // report


    /* All processes are done now print the stats*/
    m.printStats();
    /* Returning the run stats */
    rs.stats = m.stats;
    rs.total = m.getTotalStats();
    rs.processWs = m.processWs;
}
