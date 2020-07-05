//
// Created by selman.ozleyen2017 on 3.07.2020.
//

#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>


#ifndef OSFINAL_MEMORY_H
#define OSFINAL_MEMORY_H

#define INT_PERIOD 20

enum ReplacementPolicy{
    nru,fifo,sc,lru,wsclock
};

enum AllocPolicy{
    local,global
};


/*
 * Is the abstraction for memory
 * when there is a get/set request
 * this class will abstract away the
 * cache and will act as if there is only
 * one memory (in the layout of vm) we
 * need to access.
 */
class Memory {
public:
    Memory(int frameSize, int numPhysical, int numVirtual, int pageTablePrintInt,
            ReplacementPolicy replacementPolicy, AllocPolicy allocPol, const char * diskFileName);

    class PageTableEntry{
    public:
        uint64_t counter = 0;
        bool present = false;
        bool modified = false;
        bool referenced = false;
        /* First numPhysical bits will be used in pageFrameNo */
        unsigned int pageFrameNo = 0;
    };
    ~Memory();
    Memory(const Memory& mem) = delete ;
    Memory& operator=(const Memory& other) = delete;

    void set(unsigned int index, int value, char * tName);

    void addProcess(const std::function<void(Memory*)>& func);

    void startProcesses();

    void waitProcesses();

private:
    /* Function that will return if it is a hit or not*/
    bool checkHit(unsigned int index) const;
    unsigned int getPhyAddr(unsigned int va) const;
    void resetRBits();





    const int frameSize;
    const int numPhysical;
    const int numVirtual;
    const int pageTablePrintInt;
    const ReplacementPolicy replacementPol;
    const AllocPolicy allocPol;
    const char * diskFileName;

    /* Physical Memory*/
    std::vector<int> pMem;
    const long int pMemSize;
    const long int vMemSize;


    std::vector<PageTableEntry> pageTable;

    std::ofstream diskStream;

    /* Thread Handling */
    // for the interrupt thread
    const int interruptPeriod = INT_PERIOD;
    std::mutex memMutex;
    /* Thread to reset R bits periodically*/
    std::thread intThread;
    std::atomic_bool stopIntThread{false};

    std::vector<std::function<void(Memory*)>> processFuncs;
    std::vector<std::thread> processThreads;
    /* Flag if a thread is finished or not*/
    std::atomic_bool threadDone{false};
};


#endif //OSFINAL_MEMORY_H
