CC = g++
CFLAGS = -Wextra -Wall -pedantic --std=c++11 -g
MEM_SYSTEM = memory.cpp memory.h
SORT_MEM = sortmem.cpp sortmem.h

all: sortarrays benchmark

benchmark: benchmark.cpp  $(MEM_SYSTEM) $(SORT_MEM)
	$(CC) $(CFLAGS) -o benchmark benchmark.cpp $(MEM_SYSTEM) $(SORT_MEM) -lpthread

sortarrays: sortarrays.cpp  $(MEM_SYSTEM) $(SORT_MEM)
	$(CC) $(CFLAGS) -o sortArrays sortarrays.cpp $(MEM_SYSTEM) $(SORT_MEM) -lpthread

clean:
	rm sortArrays benchmark
