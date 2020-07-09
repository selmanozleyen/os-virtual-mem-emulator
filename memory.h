//
// Created by selman.ozleyen2017 on 3.07.2020.
//
#ifndef OSFINAL_MEMORY_H
#define OSFINAL_MEMORY_H

#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <map>
#include <sstream>
#include <queue>
#include <list>

#define INT_PERIOD 20
#define TAU 40
#define RESET   "\033[0m"
#define GREEN   "\033[32m"

enum ReplacementPolicy{
    nru,fifo,sc,lru,wsclock
};

enum AllocPolicy{
    local,global
};

class ProcessStats{
public:
    uint64_t noReads = 0;
    uint64_t noWrites = 0;
    uint64_t noPageMisses = 0;
    uint64_t noPageReplacement = 0;
    uint64_t noDiskWrites = 0;
    uint64_t noDiskReads = 0;
};


/* Class For An Entry Of Page Table*/
struct PageTableEntry{
public:
    unsigned long int counter = 0;
    struct timespec timer{};
    bool present = false;
    bool modified = false;
    bool referenced = false;
    /* First numPhysical bits will be used in pageFrameNo */
    unsigned int pageFrameNo = 0;
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
    ~Memory();
    Memory(const Memory& mem) = delete ;
    Memory& operator=(const Memory& other) = delete;

    void set(unsigned int index, int value, char * tName);
    int get(unsigned int index, char * tName);

    void addProcess(const std::function<void(Memory *)> &func, const char *tName);
    void startProcesses();
    void waitProcesses();

    void writeDisk(unsigned int destPageNo,unsigned int srcFrameNo);
    void readDisk(unsigned int srcPageNo,unsigned int destFrameNo);

    void printPageTable();
    int getSimulationPid(const char * tName) const;
    std::string getThreadNameFromPid(int pid) const;
    unsigned long getIntCount() const;

    void printStats() const;
    class ContPartition;
    class NRUPart;
    class LRUPart;
    class FIFOPart;
    class SCPart;
    class WSClockPart;
private:

    /* Class to Contain Frames In Memory */
    class FrameCont{
    public:
        explicit FrameCont(Memory *parent);
        ~FrameCont();
        FrameCont();
        FrameCont(const FrameCont& fc) = delete;
        FrameCont& operator=(const FrameCont& fc) = delete;
        unsigned int updatePT(unsigned int va, int pid, bool modified, bool referenced);

        void clear();
        void set(int processCount, const std::vector<unsigned int> &freeFrames);
    private:
        void loadToMem(unsigned int pageNo, int pid, ProcessStats &pStats);
        Memory* parent;
        std::vector<Memory::ContPartition*> partitions;
    };

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
    uint64_t PC = 0;

    /* Physical Memory*/
    std::vector<int> pMem;
    const long unsigned int pMemSize;
    const long unsigned int vMemSize;
    const long unsigned int pageIntCount; // how many int a page hold
    const long unsigned int pageSizeByte; // page size in bytes

    std::vector<PageTableEntry> pageTable; // Has same elements as the pageCount of VM
    long unsigned int refCount = 0;

    std::fstream diskStream; // file representing the disk

    // for the interrupt thread
    const int interruptPeriod = INT_PERIOD;
    std::mutex memMutex; // for saving the memory from the race conditions
    /* Thread to reset R bits periodically*/
    std::thread intThread;
    std::atomic_bool stopSysThread{false};

    static std::chrono::nanoseconds convertTs(timespec t);

    /* PROCESS MANAGEMENT START */
    /* The size of the process management variables won't change between;
     * startProcesses()
     * ...
     * endProcesses()
     *
     * calls
     * */
    FrameCont cont;
    std::vector<std::function<void(Memory*)>> processFuncs;
    std::map<std::string,int> processNames;
    std::map<std::string,ProcessStats> stats;
    std::vector<std::thread> processThreads;
    /* Index of the currently finished thread */
    std::vector<int> doneThreads;
    std::condition_variable threadDone;
    std::mutex processManMut; // Process management mutex
    /* PROCESS MANAGEMENT END */
};




class Memory::ContPartition{
public:
    ContPartition(Memory *mem, std::vector<unsigned int> freeFrames);
    virtual ~ContPartition() = default;
    /*
     * Returns the replaced/free Frame and the replaced frame pageNo if the frame was free
     * replacedFramePageNo will be -1
     */
    virtual unsigned int getFrame(unsigned int pageNo, long *replacedFramePageNo) = 0;

    /* Free frame no's in the physical memory*/
    std::vector<unsigned int> freeFrames;
    Memory * parent;
};


class Memory::NRUPart: public Memory::ContPartition{
public:

    NRUPart(Memory *mem, const std::vector<unsigned int> &freeFrames);
    unsigned int getFrame(unsigned int pageNo, long *replacedFramePageNo) override;
private:
    std::vector<unsigned int> phyLoadedPages;
};

class Memory::LRUPart: public Memory::ContPartition{
public:

    LRUPart(Memory *mem, const std::vector<unsigned int> &freeFrames);
    unsigned int getFrame(unsigned int pageNo, long *replacedFramePageNo) override;
private:
    std::vector<unsigned int> phyLoadedPages;
};


class Memory::FIFOPart: public Memory::ContPartition{
public:

    FIFOPart(Memory *mem, const std::vector<unsigned int> &freeFrames);
    unsigned int getFrame(unsigned int pageNo, long *replacedFramePageNo) override;
private:
    std::queue<unsigned int> phyLoadedPages;
};

class Memory::SCPart: public Memory::ContPartition{
public:

    SCPart(Memory *mem, const std::vector<unsigned int> &freeFrames);
    unsigned int getFrame(unsigned int pageNo, long *replacedFramePageNo) override;
private:
    std::queue<unsigned int> phyLoadedPages;
};


class Memory::WSClockPart: public Memory::ContPartition{
public:

    WSClockPart(Memory *mem, const std::vector<unsigned int> &freeFrames);
    unsigned int getFrame(unsigned int pageNo, long *replacedFramePageNo) override;

private:
    void incHand();
    std::list<unsigned int> phyLoadedPages;
    std::list<unsigned int>::iterator hand;
    std::chrono::nanoseconds tau{};
};

#endif //OSFINAL_MEMORY_H
