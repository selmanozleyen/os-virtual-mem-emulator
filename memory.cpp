//
// Created by selman.ozleyen2017 on 3.07.2020.
//

#include <stdexcept>
#include <fstream>
#include "memory.h"
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <functional>

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
        vMemSize(1 << (_frameSize+_numVirtual))
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
    pageTable.resize(vMemSize);

    int fid,cnt = 0;
    /* Opening the file to resize*/
    NO_EINTR(fid = open(diskFileName,O_RDWR | O_CREAT))
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
    diskStream.open(diskFileName, ios::binary | ios::out | ios::in);
    diskStream.exceptions(std::ios::failbit | std::ios::badbit);

    /*TODO decide when interrupts are going to happen*/

    /* Initializing thread to reset R bits periodically */
    intThread = thread([this](){
       while (stopIntThread) {
            resetRBits();
            this_thread::sleep_for(chrono::milliseconds(interruptPeriod));
       }
    });


}

bool Memory::checkHit(unsigned int index) const {
    return pageTable[index].present;
}

void Memory::set(unsigned int index, int value, char *tName) {
    if(checkHit(index)){

    }
    /* If it is not a hit*/
    else{
        /* If it is empty just replace it */
    }
}

unsigned int Memory::getPhyAddr(unsigned int va) const {
    // TODO: math explanation on report
    return (pageTable[va >> frameSize].pageFrameNo <<  frameSize) + (va & ((1 << frameSize) - 1));
}

Memory::~Memory() {
    stopIntThread = true;
    intThread.join();
    waitProcesses();
}

/* Resets the R bits when called */
void Memory::resetRBits() {
    memMutex.lock();
    for(auto &ent: pageTable){
        ent.referenced = false;
    }
    memMutex.unlock();
}

void Memory::addProcess(const function<void(Memory*)>& func) {
    processFuncs.push_back(func);
}

void Memory::startProcesses() {
    for(auto& f :processFuncs){
        processThreads.emplace_back(
                thread(
                    [this,f](){
                            f(this);
                    }
                )
        );
    }
}
/* Must call after calling startProcesses method*/
void Memory::waitProcesses() {
    for(auto& t: processThreads)
        t.join();

    processThreads.clear();
    processFuncs.clear();
}

