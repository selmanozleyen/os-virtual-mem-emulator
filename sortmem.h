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
    static void testRun(int argc, const char ** argv);
private:
    static void fill(Memory * m);
    static void printMem(Memory * m);
    static void bubble(Memory * m,unsigned long int start,unsigned long int end);
    static void merge(Memory * m,unsigned long int start,unsigned long int end);
    static unsigned int qsPartition(Memory *m, unsigned int low, unsigned int high, char *tName);
    static void quick(Memory * m,unsigned long int start,unsigned long int end);
    static void index(Memory * m,unsigned long int start,unsigned long int end);
    static void check(Memory * m);

    static const int argCount = 8;
    static std::stringstream stream;
};


#endif //OSFINAL_SORTMEM_H
