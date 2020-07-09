//
// Created by selman.ozleyen2017 on 8.07.2020.
//

#ifndef OSFINAL_SORTMEM_H
#define OSFINAL_SORTMEM_H
#include "memory.h"
#include <sstream>

class SortMem {
public:
    static void run(int argc, const char ** argv);
private:
    static void fill(Memory * m);
    static void printMem(Memory * m);
    static void bubble(Memory * m,unsigned long int start,unsigned long int end);
    /* Merge Algorithm mostly inspired by www.geeksforgeeks.org */
    static void merge(Memory * m,unsigned long int start,unsigned long int end);
    static void mergeSortHelper(Memory * m, unsigned long int l, unsigned long int r, char * tName);
    static void mergeArr(Memory * m,unsigned long int l,unsigned long int mid,unsigned long r,char * tName);



    static unsigned int qsPartition(Memory *m, unsigned int low, unsigned int high, char *tName);
    static void quick_helper(Memory * m,unsigned long int start,unsigned long int end,char * tName);
    static void quick(Memory * m,unsigned long int start,unsigned long int end);
    static void index(Memory * m,unsigned long int start,unsigned long int end);
    static void check(Memory * m);

    static const int argCount = 8;
    static std::stringstream stream;

    static void
    read_args(int argc, const char **argv, int *frameSize, int *numPhysical, int *numVirtual, int *pageTablePrintInt,
              ReplacementPolicy *pageReplacement, AllocPolicy *allocPolicy, const char **diskFileName);
};


#endif //OSFINAL_SORTMEM_H
