//
// Created by selman.ozleyen2017 on 3.07.2020.
//

#include <stdexcept>
#include "memory.h"
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <ctime>
#include <numeric>

#define NO_EINTR(stmt) while ((stmt) == -1 && errno == EINTR);

using namespace std;

Memory::Memory(int _frameSize, int _numPhysical,
        int _numVirtual, int _pageTablePrintInt,
        ReplacementPolicy _replacementPolicy,
        AllocPolicy _allocPol,
        const char *_diskFileName):
        frameSize(_frameSize),
        numPhysical(_numPhysical),
        numVirtual(_numVirtual),
        pageTablePrintInt(_pageTablePrintInt),
        replacementPol(_replacementPolicy),
        allocPol(_allocPol),
        diskFileName(_diskFileName),
        pMemSize(1 << (_frameSize+_numPhysical)),
        vMemSize(1 << (_frameSize+_numVirtual)),
        pageIntCount(1 << frameSize),
        pageSizeByte((1 << frameSize)*sizeof(int)),
        cont(this)
{
    /*Checking for arguments*/
    if(numPhysical > numVirtual ||
    numVirtual < 4 || numPhysical < 1 ||
    frameSize < 1 || pageTablePrintInt < 1){
        throw std::invalid_argument("Invalid Argument.");
    }

    /* Initialize the physical memory*/
    pMem.resize(pMemSize);
    /* Initialize page table */
    pageTable.resize(1 << numVirtual);

    int fid,cnt = 0;
    /* Opening the file to resize*/
    NO_EINTR(fid = open(diskFileName,O_RDWR | O_CREAT,S_IRWXU | S_IRWXG | S_IRWXO))
    if(fid < 0){
        throw system_error(errno,generic_category());
    }
    /* Resizing the file */
    NO_EINTR(cnt = ftruncate(fid,vMemSize*sizeof(int)))
    if(cnt != 0){
        throw system_error(errno,generic_category());
    }
    /* Closing the file*/
    NO_EINTR(cnt = close(fid))
    if(cnt != 0){
        throw system_error(errno,generic_category());
    }
    /* diskStream Will be closed implicitly when the object is destructed*/
    diskStream.open(diskFileName, ios::binary| ios::in | ios::out);
    diskStream.exceptions(std::ios::failbit | std::ios::badbit);


    /* Initializing thread to reset R bits periodically */
    stopSysThread = false;
    intThread = thread([this](){
       while (!stopSysThread) {
            resetRBits();
            this_thread::sleep_for(chrono::milliseconds (interruptPeriod));
       }
    });
}

/* When calling this memory should be protected */
bool Memory::checkHit(unsigned int index) const {
    return pageTable[(index >> frameSize)].present;
}

void Memory::set(unsigned int index, int value, char *tName) {
    PC++;
    unsigned int phyAddr = 0;
    /* Protect the memory */
    unique_lock<mutex> mtx(memMutex);
    phyAddr = cont.updatePT(index,getSimulationPid(tName),true,false);
    pMem[phyAddr] = value;
}

unsigned int Memory::getPhyAddr(unsigned int va) const {
    // TODO: math explanation on report
    return (pageTable[va >> frameSize].pageFrameNo <<  frameSize) +
    (va & ((1 << frameSize) - 1));
}

Memory::~Memory() {
    stopSysThread = true;
    intThread.join();
}

/* Resets the R bits when called */
void Memory::resetRBits() {
    unique_lock<mutex> mtx(memMutex);
    for(auto &ent: pageTable){
        ent.referenced = false;
    }
}

void Memory::addProcess(const std::function<void(Memory *)> &func, const char *tName) {
    processFuncs.push_back(func);
    int pid = ((int)processFuncs.size())-1;
    /* Add name to process name map*/
    processNames.insert(pair<string,int>(string(tName),pid));
    stats.insert(pair<string,ProcessStats>(string(tName),ProcessStats()));
}

void Memory::startProcesses() {
    int i = 0,pCount = -1;
    /* Get the count of the initial process count*/
    pCount = processFuncs.size();
    /* Get free frames */
    vector<unsigned int> freeFrames(1 << numPhysical);
    iota(freeFrames.begin(),freeFrames.end(),0);
    cont.set(pCount, freeFrames);

    doneThreads.resize(processFuncs.size(),-1);
    for(auto& f :processFuncs){
        processThreads.emplace_back(
                thread(
                    [this,f,i](){
                        f(this);
                        unique_lock<mutex> mtx(processManMut);
                        doneThreads[i] = 0;
                        threadDone.notify_all();
                    }
                )
        );
        i++;
    }
}
/* Must call after calling startProcesses method*/
void Memory::waitProcesses() {
    int count = 0;
    unique_lock<mutex> mtx(processManMut);
    int initProcessCount = processThreads.size();
    /* Checking if a thread is done*/
    for(int i = 0; i < initProcessCount; i++){
        if(doneThreads[i] == 0){
            count++;
            processThreads[i].join();
            /* show it is joined*/
            doneThreads[i] = -2;
            /* TODO add free pages here */
        }
    }
    /* Wait until all threads are done*/
    while(count != initProcessCount){
        threadDone.wait(mtx);
        for(int i = 0; i < initProcessCount; i++){
            if(doneThreads[i] == 0){
                count++;
                /* join */
                processThreads[i].join();
                /* show it is joined*/
                doneThreads[i] = -2;
            }
        }
    }
    mtx.unlock();

    /* Write the remaining dirty blocks*/
    for(unsigned int i = 0 ; i < pageTable.size(); i++){
        auto& ent = pageTable[i];
        if(ent.present && ent.modified){
            /* The owner of the writes to disk cannot be determined due to the nature of the system*/
            if(processNames.size() == 1){
                stats.find(processNames.begin()->first)->second.noDiskWrites++;
            }
            writeDisk(i,ent.pageFrameNo);
        }
        ent.present = false;
        ent.modified = false;
    }
    cont.clear();
    processThreads.clear();
    processFuncs.clear();
    doneThreads.clear();
}

int Memory::getSimulationPid(const char *tName) const {
    return  processNames.at(string(tName));
}

Memory::FrameCont::FrameCont(Memory *parent) :parent(parent){/* Empty */}
// TODO do partition classes read only

void Memory::FrameCont::loadToMem(unsigned int pageNo, int pid, ProcessStats &pStats) {
    /* According to alloc policy get the partition index*/
    int partNo = 0;
    if(parent->allocPol == AllocPolicy::local){
        partNo = pid;
        if(partNo > partitions.size())
            throw invalid_argument("Given Pid is invalid");
    }
    /* Get new pageNo by the partition object*/
    long int retval = -1;
    unsigned int replacedPageNo = 0,newFrame = 0;
    if(parent->pageTable[pageNo].present){
        throw logic_error("Page Table corrupted");
    }
    newFrame = partitions[partNo]->getFrame(pageNo,&retval);

    /* This frame was replaced so update the values*/
    if(retval >= 0){
        /* There is a page replacement Update the stats*/
        pStats.noPageReplacement++;

        replacedPageNo = (unsigned int) retval;
        if(!parent->pageTable[replacedPageNo].present)
            throw logic_error("Page Table structure is corrupted.");
        parent->pageTable[replacedPageNo].present = false;
        /* If the replaced page is modified write before replacing it*/
        if(parent->pageTable[replacedPageNo].modified){
           parent->writeDisk(replacedPageNo,parent->pageTable[replacedPageNo].pageFrameNo);
           /* Update the stats */
           pStats.noDiskWrites++;
        }
        parent->pageTable[replacedPageNo].modified = false;
    }
    /* Make the entry point to the new frameNo */
    parent->pageTable[pageNo].present = true;
    if(parent->pageTable[pageNo].modified){
        throw logic_error("Corrupted Page Table");
    }
    parent->pageTable[pageNo].modified = false;
    parent->pageTable[pageNo].pageFrameNo = newFrame;
    parent->readDisk(pageNo,newFrame);
    pStats.noDiskReads++;
}

Memory::FrameCont::~FrameCont() {
    clear();
}

/* Returns the updated Frame No of the memory */
unsigned int Memory::FrameCont::updatePT(unsigned int va, int pid, bool modified, bool referenced) {
    auto& pStats = parent->stats.at(parent->getThreadNameFromPid(pid));
    /* An access to memory happened printing PT if needed*/
    parent->refCount++;
    if(parent->refCount == parent->pageTablePrintInt){
        parent->refCount = 0;
        /* Printing the Page Table */
        parent->printPageTable();
    }
    /* Check if it is not a hit*/
    if(referenced) pStats.noReads++;
    if(modified) pStats.noWrites++;

    if(!parent->checkHit(va)){
        /* It is a miss so update stats */
        pStats.noPageMisses++;
        /*If it is not load the page*/
        loadToMem((va >> parent->frameSize), pid, pStats);
    }

    /* Update the other fields for the replacement algorithms*/
    auto* ent = &parent->pageTable[(va >> parent->frameSize)];
    ent->modified |= modified ;
    ent->referenced |= referenced;
    ent->counter = parent->PC;
    clockid_t tCid;
    pthread_getcpuclockid(pthread_self(),&tCid);
    clock_gettime(tCid,&ent->timer);

    return parent->getPhyAddr(va);
}

Memory::FrameCont::FrameCont():parent(nullptr){/* Empty */}

void Memory::FrameCont::clear() {
    for(auto part: partitions)
        delete part;
    partitions.clear();
}

void Memory::FrameCont::set(int processCount, const std::vector<unsigned int> &freeFrames) {
    /* Depending on the alloc policy partition count will be decided*/
    int partitionNum = 1;
    if(parent->allocPol == AllocPolicy::local){
        partitionNum = processCount;
    }
    /* If there are more partitions than free frames*/
    if(freeFrames.size() < partitionNum)
        throw logic_error("Given free frames for memory is below the minimum required"
                          "(Min. increases if local allocation is active).");

    int partFrameCount = ((int)freeFrames.size())/partitionNum;
    auto startItr = freeFrames.begin();
    auto endItr = freeFrames.begin() + partFrameCount;

    for (int i = 0; i < partitionNum-1; ++i) {
        //if(parent->replacementPol == ReplacementPolicy::nru)
            partitions.emplace_back(
                    new NRUPart(parent,vector<unsigned int>(startItr,endItr))
           );
        startItr = endItr;
        endItr += partFrameCount;
    }
    endItr = freeFrames.end();
    //if(parent->replacementPol == ReplacementPolicy::nru)
        partitions.emplace_back(
                new NRUPart(parent,vector<unsigned int>(startItr,endItr))
        );
}


Memory::NRUPart::NRUPart(Memory *mem, const vector<unsigned int> &freeFrames):
Memory::ContPartition(mem,freeFrames)
{/* Empty */}

unsigned int Memory::NRUPart::getFrame(unsigned int pageNo, long *replacedFramePageNo) {
    unsigned int frameNoToFill =0;

    /* If there are free pages use them*/
    if(!freeFrames.empty()){
        /* return free frame of memory*/
        frameNoToFill =  freeFrames.back();
        freeFrames.pop_back();
        /* Enter an invalid entry*/
        *replacedFramePageNo = -1;
        /* Add to the loaded pageNo's list */
        phyLoadedPages.push_back(pageNo);
        return frameNoToFill;
    }
    /* If there is no free frame replace one*/
    else{
        /* Find the classes from 0 to 3*/
        vector<unsigned int> class1(0);
        vector<unsigned int> class2(0);
        for(unsigned int i = 0; i < phyLoadedPages.size() ;i++){
            /* Class 0 found no need to proceed*/
            if(!parent->pageTable[phyLoadedPages[i]].referenced &&
                !parent->pageTable[phyLoadedPages[i]].modified){
                frameNoToFill = parent->pageTable[phyLoadedPages[i]].pageFrameNo;
                *replacedFramePageNo = (long) phyLoadedPages[i];
                phyLoadedPages[i] = pageNo;
                return frameNoToFill;
            }
            /* Class 1*/
            else if(!parent->pageTable[phyLoadedPages[i]].referenced &&
                parent->pageTable[phyLoadedPages[i]].modified){
                class1.push_back(i);
            }
            /* Class 2*/
            else if(parent->pageTable[phyLoadedPages[i]].referenced &&
            !parent->pageTable[phyLoadedPages[i]].modified){
                class2.push_back(i);
            }
        }
        /* If this part executes there are no class 0 so try 1 and 2 */
        if(!class1.empty()){
            frameNoToFill = parent->pageTable[phyLoadedPages[class1.back()]].pageFrameNo;
            *replacedFramePageNo = (long) phyLoadedPages[class1.back()];
            phyLoadedPages[class1.back()] = pageNo;
            return frameNoToFill;
        }if(!class2.empty()){
            frameNoToFill = parent->pageTable[phyLoadedPages[class2.back()]].pageFrameNo;
            *replacedFramePageNo = (long) phyLoadedPages[class2.back()];
            phyLoadedPages[class2.back()] = pageNo;
            return frameNoToFill;
        }
        /* Return an arbitrary page*/
        frameNoToFill = parent->pageTable[phyLoadedPages.back()].pageFrameNo;
        *replacedFramePageNo = (long) phyLoadedPages.back();
        *phyLoadedPages.rbegin() = pageNo;
        return frameNoToFill;
    }
}

Memory::ContPartition::ContPartition(Memory *mem, vector<unsigned int> freeFrames):
parent(mem),
freeFrames(move(freeFrames))
{ /* Empty */}


/* Writes the given frameNo in the phyMemory ro the disk file */
void Memory::writeDisk(unsigned int destPageNo, unsigned int srcFrameNo) {
    /* Calculating the exact position in disk */
    unsigned long int diskPos = pageSizeByte*destPageNo;
    diskStream.seekp((streamoff)diskPos);
    diskStream.write((char *)(pMem.data() + pageIntCount*srcFrameNo),pageSizeByte);
}

/* Reads the file and loads it to pMem variable in other words our memory*/
void Memory::readDisk(unsigned int srcPageNo, unsigned int destFrameNo) {
    /* Calculating the exact position in disk */
    unsigned long int diskPos = pageSizeByte*srcPageNo;
    diskStream.seekg((streamoff)diskPos);
    diskStream.read((char *)(pMem.data() + pageIntCount*destFrameNo),pageSizeByte);
}

int Memory::get(unsigned int index, char *tName) {
    PC++;
    unsigned int phyAddr = 0;
    /* Protect the memory */
    unique_lock<mutex> mtx(memMutex);
    phyAddr = cont.updatePT(index,getSimulationPid(tName),false,true);
    /* Return the value */
    return pMem[phyAddr];
}

/* This for runs at most 4 times*/
// TODO make this const
std::string Memory::getThreadNameFromPid(int pid) const {
    auto it = processNames.begin();
    while(it != processNames.end())
        if(it->second == pid)
            return it->first;
    throw logic_error("Invalid Pid");
}

unsigned long Memory::getIntCount() const {
    return (1 << (numVirtual + frameSize));
}

void Memory::printPageTable() {
    /* No need to save cout with a mutex because this function is called with a mutex */
    printf("Printing the Page Table (If this happens often change the printIntPageTable arg):\n");
    for (unsigned int i = 0; i < pageTable.size(); ++i) {
        auto& ent = pageTable[i];
        auto virtualTime =chrono::duration_cast<chrono::nanoseconds>
                (chrono::seconds{ent.timer.tv_sec} +
                chrono::nanoseconds{ent.timer.tv_nsec});
        printf( "Present:%d | %10uth Entry | FrameNo:%7u | "
                "M:%d | R:%d | Counter:%10lu | VirtualTime:%20ld|\n",
                int(ent.present),i,ent.pageFrameNo,
                int(ent.modified),
                int(ent.referenced),
                ent.counter,
                virtualTime.count()
                );
        fflush(stdout);
    }
}

void Memory::printStats() const {
    printf("Printing Stats For Each Simulated Process:\n");
    for (auto& p: stats){
        auto &s = p.second;
        printf("Process Name: %8s | Read Count : %10lu | Write Count %10lu | "
               "Page Misses: %8lu | Page Replacement %8lu | Disk Reads: %8lu | "
               "Disk Writes %8lu\n",
                p.first.c_str(),s.noReads,s.noWrites,s.noPageMisses,
                s.noPageReplacement,s.noDiskReads,s.noDiskWrites);
    }
    fflush(stdout);
}

