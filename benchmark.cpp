#include <iostream>
#include <functional>
#include "memory.h"
#include "sortmem.h"
#include <chrono>

#define PRINT_INT 1000000000

using namespace std;

struct mem_args{
    mem_args(int frameSize,int numPhysical,int numVirtual, int allocNo, int replaceNo):
    frameSize(frameSize),numPhysical(numPhysical),numVirtual(numVirtual),allocNo(allocNo),replaceNo(replaceNo)
    { /* Empty */ }
    int frameSize = 0;
    int numPhysical = 0;
    int numVirtual = 0;
    int allocNo = 0;
    int replaceNo = 0;
};

void printStats(ProcessStats& ps, const char * alg, const char * param, const basic_string<char>& res);
void writeWsData(const string& alName, const RunStats& rs, ofstream &fs);

int main(int argc, const char ** argv){
    try{
        if(argc != 2 && argc != 3)
            throw invalid_argument("Invalid Arg Usage : ./benchmark intCount[recommended = 5] "
                                   "wsStatsFileName[optional]");
        const char * fName = nullptr;
        const char * tempName = "temp.data";
        if(argc == 3)
            fName = argv[2];
        //if(fName){printf("Not implemented\n");};

        /* Setting the args */
        int aPolCount = 2,rPolCount = 5;
        const char * aPolNames[] = {"global","local"};
        const char * rPolNames[] = {"NRU","LRU","SC","WSClock","FIFO"};
        AllocPolicy aPolicies[] = {global,local};
        ReplacementPolicy rPolicies[] = {nru,lru,sc,wsclock,fifo};
        int intCount = stoi(argv[1]);
        if(intCount < 4){
            throw invalid_argument("Invalid Arg Usage : ./benchmark intCount too small.");
        }
        int printInt = PRINT_INT; // we dont want it to be printed often
        int numVirtual,numPhysical;
        int frameCount = intCount -2;
        vector<mem_args> arg_list{};
        vector<RunStats> stat_list{};
        RunStats rs;
        auto start = chrono::steady_clock::now(),end = start;
        for (int repInd = 0; repInd < rPolCount ; ++repInd) {
            for (int curFrameSize = 1; curFrameSize <= frameCount; ++curFrameSize) {
                numPhysical = intCount - curFrameSize;
                numVirtual = intCount - curFrameSize + 4;
                printf("[BENCHMARK]: Running with arguments frameSize = %d,"
                       " numPhysical = %d, numVirtual = %d, allocPol = %6s, replacementPol = %-7s\n",
                       curFrameSize,numPhysical,numVirtual,aPolNames[0],rPolNames[repInd]
                       );
                arg_list.emplace_back(mem_args(curFrameSize,numPhysical,numVirtual,0,repInd));
                start =chrono::steady_clock::now();
                SortMem::run(curFrameSize,numPhysical,numVirtual,printInt,
                             rPolicies[repInd],aPolicies[0],tempName,rs);
                stat_list.emplace_back(rs);
                end =  chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
                printf("[BENCHMARK]: Finished in %.5ld microseconds\n", elapsed.count());
            }
        }
        printf("[BENCHMARK]: Done calculating results.\n");
        /* Compare the results */
        /* Variables to keep track of the minimum*/
        vector<vector<map<string,ProcessStats>>> perFrameSize(frameCount);
        vector<vector<map<string,ProcessStats>>> perReplacement(rPolCount);

        /* Calculate best frame size for each algorithm */
        for (int i = 0; i < (int) stat_list.size(); ++i) {
            perReplacement[arg_list[i].replaceNo].emplace_back(stat_list[i].stats);
            perFrameSize[arg_list[i].frameSize-1].emplace_back(stat_list[i].stats);
        }
        /* Getting the sums for every replacement and each Processes */
        vector<ProcessStats> perReplacementSumIndex(rPolCount,ProcessStats());
        vector<ProcessStats> perReplacementSumMerge(rPolCount,ProcessStats());
        vector<ProcessStats> perReplacementSumBubble(rPolCount,ProcessStats());
        vector<ProcessStats> perReplacementSumQuick(rPolCount,ProcessStats());
        vector<ProcessStats> perFrameSizeSumIndex(frameCount,ProcessStats());
        vector<ProcessStats> perFrameSizeSumMerge(frameCount,ProcessStats());
        vector<ProcessStats> perFrameSizeSumBubble(frameCount,ProcessStats());
        vector<ProcessStats> perFrameSizeSumQuick(frameCount,ProcessStats());

        for (int i = 0; i < (int)perReplacement.size(); ++i) {
            for (auto j : perReplacement[i]){
                perReplacementSumQuick[i] = perReplacementSumQuick[i] + j.at("quick");
                perReplacementSumIndex[i] = perReplacementSumIndex[i] + j.at("index");
                perReplacementSumMerge[i] = perReplacementSumMerge[i] + j.at("merge");
                perReplacementSumBubble[i] = perReplacementSumBubble[i] + j.at("bubble");
            }
        }
        for (int i = 0; i < (int) perFrameSize.size(); ++i) {
            for (auto j : perFrameSize[i]){
                perFrameSizeSumQuick[i] = perFrameSizeSumQuick[i] + j.at("quick");
                perFrameSizeSumIndex[i] = perFrameSizeSumIndex[i] + j.at("index");
                perFrameSizeSumMerge[i] = perFrameSizeSumMerge[i] + j.at("merge");
                perFrameSizeSumBubble[i] = perFrameSizeSumBubble[i] + j.at("bubble");
            }
        }
        /* Find the optimal Frame Size*/
        int minIndexesFrame[] = {0,0,0,0};
        uint64_t minPageRepsFrame[] = {
                perFrameSizeSumBubble[0].noPageReplacement,
                perFrameSizeSumQuick[0].noPageReplacement,
                perFrameSizeSumMerge[0].noPageReplacement,
                perFrameSizeSumIndex[0].noPageReplacement
        };
        for (int i = 0; i < frameCount; ++i) {
            if(perFrameSizeSumBubble[i].noPageReplacement < minPageRepsFrame[0]){
                minPageRepsFrame[0] = perFrameSizeSumBubble[i].noPageReplacement;
                minIndexesFrame[0] = i;
            }if(perFrameSizeSumQuick[i].noPageReplacement < minPageRepsFrame[1]){
                minPageRepsFrame[1] = perFrameSizeSumQuick[i].noPageReplacement;
                minIndexesFrame[1] = i;
            }if(perFrameSizeSumMerge[i].noPageReplacement < minPageRepsFrame[2]){
                minPageRepsFrame[2] = perFrameSizeSumMerge[i].noPageReplacement;
                minIndexesFrame[2] = i;
            }if(perFrameSizeSumIndex[i].noPageReplacement < minPageRepsFrame[3]){
                minPageRepsFrame[3] = perFrameSizeSumIndex[i].noPageReplacement;
                minIndexesFrame[3] = i;
            }
        }
        /* Find The Optimal Replacement Algorithm */
        int minIndexesPol[] = {0,0,0,0};
        uint64_t minPageRepsPol[] = {
                perReplacementSumBubble[0].noPageReplacement,
                perReplacementSumQuick[0].noPageReplacement,
                perReplacementSumMerge[0].noPageReplacement,
                perReplacementSumIndex[0].noPageReplacement
        };
        for (int i = 0; i < aPolCount; ++i) {
            if(perReplacementSumBubble[i].noPageReplacement < minPageRepsPol[0]){
                minPageRepsPol[0] = perReplacementSumBubble[i].noPageReplacement;
                minIndexesPol[0] = i;
            }if(perReplacementSumQuick[i].noPageReplacement < minPageRepsPol[1]){
                minPageRepsPol[1] = perReplacementSumQuick[i].noPageReplacement;
                minIndexesPol[1] = i;
            }if(perReplacementSumMerge[i].noPageReplacement < minPageRepsPol[2]){
                minPageRepsPol[2] = perReplacementSumMerge[i].noPageReplacement;
                minIndexesPol[2] = i;
            }if(perReplacementSumIndex[i].noPageReplacement < minPageRepsPol[3]){
                minPageRepsPol[3] = perReplacementSumIndex[i].noPageReplacement;
                minIndexesPol[3] = i;
            }
        }
        /* PRINTING STATS FOR BUBBLE SORT -------------------------------------------------------*/
        int bubbleFrame = minIndexesFrame[0];
        auto bubbleStats = perFrameSizeSumBubble[bubbleFrame]/(rPolCount);
        bubbleFrame+=1;
        printStats(bubbleStats,"BUBBLE","FRAMESIZE","Optimal Frame Size is "+ to_string(bubbleFrame));
        int bubbleRep = minIndexesPol[0];
        bubbleStats = perReplacementSumBubble[bubbleRep]/(frameCount);
        printStats(bubbleStats,"BUBBLE","REPLACEMENT-POLICY","Optimal Replacement Algorithm is "+ string(rPolNames[bubbleRep]));
        printf("\n");
        /* PRINTING STATS FOR QUICK SORT -------------------------------------------------------*/
        int quickFrame = minIndexesFrame[1];
        auto quickStats = perFrameSizeSumQuick[quickFrame]/(rPolCount);
        quickFrame+=1;
        printStats(quickStats,"QUICK","FRAMESIZE","Optimal Frame Size is "+ to_string(quickFrame));
        int quickRep = minIndexesPol[1];
        quickStats = perReplacementSumQuick[quickRep]/(frameCount);
        printStats(quickStats,"QUICK","REPLACEMENT-POLICY","Optimal Replacement Algorithm is "+ string(rPolNames[quickRep]));
        printf("\n");
        /* PRINTING STATS FOR MERGE SORT -------------------------------------------------------*/
        int mergeFrame = minIndexesFrame[2];
        auto mergeStats = perFrameSizeSumMerge[mergeFrame]/(rPolCount);
        mergeFrame+=1;
        printStats(mergeStats,"MERGE","FRAMESIZE","Optimal Frame Size is "+ to_string(mergeFrame));
        int mergeRep = minIndexesPol[2];
        mergeStats = perReplacementSumMerge[mergeRep]/(frameCount);
        printStats(mergeStats,"MERGE","REPLACEMENT-POLICY","Optimal Replacement Algorithm is "+ string(rPolNames[mergeRep]));
        printf("\n");
        /* PRINTING STATS FOR  INDEX -------------------------------------------------------*/
        int indexFrame = minIndexesFrame[3];
        auto indexStats = perFrameSizeSumIndex[indexFrame]/(rPolCount);
        indexFrame+=1;
        printStats(indexStats,"INDEX","FRAMESIZE","Optimal Frame Size is "+ to_string(indexFrame));
        int indexRep = minIndexesPol[3];
        indexStats = perReplacementSumIndex[indexRep]/(frameCount);
        printStats(indexStats,"INDEX","REPLACEMENT-POLICY","Optimal Replacement Algorithm is "+ string(rPolNames[indexRep]));

        /* Writing the file the working set stats */
        /* Get the LRU run with the best parameters */
        auto sampleRun = stat_list[0];
        if(fName){
            ofstream file("wsfile.txt",ios::out);
            printf("Running a process to sample\n");
            SortMem::run(5,6,7,printInt,lru,local,tempName,rs);
            /*Do it for Bubble */
            writeWsData("bubble",rs,file);
            writeWsData("quick",rs,file);
            writeWsData("merge",rs,file);
            writeWsData("index",rs,file);
        }
    }
    catch (exception& e) {
        cerr << "Exception Caught: " << e.what() << endl;
    }
    return 0;
}

void printStats(ProcessStats& ps, const char * alg, const char * param, const basic_string<char>& res){
    printf("[BENCHMARK-%s-%s]:%s\n"
           "Averages for this parameter: | Read Count : %10lu | Write Count %10lu | "
           "Page Misses: %8lu | Page Replacement %8lu | Disk Reads: %8lu | "
           "Disk Writes %8lu\n",alg,param,res.c_str(),
           ps.noReads,ps.noWrites,ps.noPageMisses,
           ps.noPageReplacement,ps.noDiskReads,ps.noDiskWrites);
}

void writeWsData(const string& alName, const RunStats& rs, ofstream &fs){
    auto ws = rs.processWs.at(alName);
    vector<bool> curWs(1 << 7,false);
    vector<int> wsSize;
    int count = 0;
    for (int i = (int)ws.size()-1; i >= 0; --i) {
        // or the working set
        count = 0;
        for (int j = 0; j < (int)ws[i].size(); ++j) {
            curWs[j] = ws[i][j] || curWs[j];
            if(curWs[j])
                count++;
        }
        // count the set
        wsSize.push_back(count);
    }
    fs << alName << endl;
    for(auto i: wsSize)
        fs << i << endl;
}